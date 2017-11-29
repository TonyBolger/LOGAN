#include "common.h"


s32 reserveReadIngressBlock(RoutingBuilder *rb)
{
	u64 current=__atomic_load_n(&(rb->allocatedIngressBlocks), __ATOMIC_SEQ_CST);

	if(current>=TR_READBLOCK_INGRESS_INFLIGHT)
		return 0;

	// FIXME: Should do this a better way
	// Also need to ensure space for sequences 10M/224 per link, plus 10K partially filled, plus 10K margin: 44,643 + 20,000 -> ~65K free bricks
	// In practice, less will be needed. This hack only works if there's a single ingressBlock which can consume the bricks.

	int maximumBricksForSequence=((TR_INGRESS_BASESTOTAL+SEQUENCE_LINK_BASES)/SEQUENCE_LINK_BASES)+TR_INGRESS_BLOCKSIZE;

	if(!mbCheckSingleBrickAvailability(&(rb->sequenceLinkPile), maximumBricksForSequence))
		{
		LOG(LOG_INFO,"reserveIngressBlock insufficient bricks: Want %i Have %i",maximumBricksForSequence, mbGetFreeSingleBrickPile(&(rb->sequenceLinkPile)));
		return 0;
		}

	u64 newVal=current+1;

	if(!__atomic_compare_exchange_n(&(rb->allocatedIngressBlocks), &current, newVal, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return 0;

	return 1;

}


static void unreserveReadIngressBlock(RoutingBuilder *rb)
{
	__atomic_fetch_sub(&(rb->allocatedIngressBlocks),1, __ATOMIC_SEQ_CST);
}



static RoutingReadIngressBlock *allocateReadIngressBlock(RoutingBuilder *rb)
{
	//LOG(LOG_INFO,"Queue readblock");

	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		int i;

		for(i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
			{
			u32 current=__atomic_load_n(&rb->ingressBlocks[i].status, __ATOMIC_SEQ_CST);

			if(current==BLOCK_STATUS_IDLE)
				{
				if(__atomic_compare_exchange_n(&(rb->ingressBlocks[i].status), &current, BLOCK_STATUS_ALLOCATED, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
					return rb->ingressBlocks+i;
				}
			}
		}

	LOG(LOG_CRITICAL,"Failed to queue lookup readblock, should never happen"); // Can actually happen, just stupidly unlikely
	return 0;
}


static void queueReadIngressBlock(RoutingReadIngressBlock *ingressBlock)
{
	u32 current=BLOCK_STATUS_ALLOCATED;
	if(!__atomic_compare_exchange_n(&(ingressBlock->status), &current, BLOCK_STATUS_ACTIVE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		LOG(LOG_CRITICAL,"Tried to queue lookup readblock in wrong state, should never happen");
}

static int scanForCompleteReadIngressBlock(RoutingBuilder *rb)
{
	RoutingReadIngressBlock *readBlock;
	int i;

	for(i=0;i<TR_READBLOCK_INGRESS_INFLIGHT;i++)
		{
		readBlock=rb->ingressBlocks+i;
		if(__atomic_load_n(&(readBlock->status), __ATOMIC_RELAXED) == BLOCK_STATUS_ACTIVE && readBlock->completionCount==0)
			return i;
		}
	return -1;
}


static RoutingReadIngressBlock *dequeueCompleteReadIngressBlock(RoutingBuilder *rb, int index)
{
	//LOG(LOG_INFO,"Try dequeue readblock");

	RoutingReadIngressBlock *ingressBlock=rb->ingressBlocks+index;

	u32 current=__atomic_load_n(&ingressBlock->status, __ATOMIC_SEQ_CST);

	if(current == BLOCK_STATUS_ACTIVE && __atomic_load_n(&ingressBlock->completionCount, __ATOMIC_SEQ_CST) == 0)
		{
		if(__atomic_compare_exchange_n(&(ingressBlock->status), &current, BLOCK_STATUS_COMPLETE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			return ingressBlock;
		}

	return NULL;
}




static void unallocateReadIngressBlock(RoutingReadIngressBlock *readBlock)
{
	u32 current=BLOCK_STATUS_COMPLETE;
	if(!__atomic_compare_exchange_n(&(readBlock->status), &current, BLOCK_STATUS_IDLE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		{
		LOG(LOG_INFO,"Tried to unreserve lookup readblock in wrong state, should never happen");
		exit(1);
		}
}


s32 getAvailableReadIngress(RoutingBuilder *rb)
{
	RoutingReadIngressBlock *readBlock;
	int i;
	int total=0;

	for(i=0;i<TR_READBLOCK_INGRESS_INFLIGHT;i++)
		{
		readBlock=rb->ingressBlocks+i;

		if(__atomic_load_n(&(readBlock->status), __ATOMIC_RELAXED) == BLOCK_STATUS_ACTIVE)
			total+=__atomic_load_n(&(readBlock->completionCount), __ATOMIC_RELAXED);
		}

	return total;
}


// Racey due to interaction of status and completion count. Redo with lock
s32 consumeReadIngress(RoutingBuilder *rb, s32 readsToConsume, u32 **sequenceLinkIndexPtr, RoutingReadIngressBlock **blockPtr)
{
	RoutingReadIngressBlock *readBlock;
	int i;

	for(i=0;i<TR_READBLOCK_INGRESS_INFLIGHT;i++)
		{
		readBlock=rb->ingressBlocks+i;

		if(__atomic_load_n(&(readBlock->status), __ATOMIC_SEQ_CST) == BLOCK_STATUS_ACTIVE)
			{
			s32 available=__atomic_load_n(&(readBlock->completionCount), __ATOMIC_SEQ_CST);
			int loopCount=10000;
			while(available>0 && --loopCount>0)
				{
				readsToConsume=MIN(readsToConsume, available);

				s32 sequencePosition=__atomic_load_n(&(readBlock->sequencePosition), __ATOMIC_SEQ_CST);
				s32 nextSequencePosition=MIN(readBlock->sequenceCount, sequencePosition+readsToConsume);

				if(sequencePosition<nextSequencePosition)
					{
					if(__atomic_compare_exchange_n(&(readBlock->sequencePosition), &sequencePosition, nextSequencePosition, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
						{
						int consumed=nextSequencePosition-sequencePosition;

						*sequenceLinkIndexPtr=readBlock->sequenceLinkIndex+sequencePosition;
						*blockPtr=readBlock;

						return consumed;
						}
					}
				available=__atomic_load_n(&(readBlock->completionCount), __ATOMIC_SEQ_CST);
				}
			}
		}

	*sequenceLinkIndexPtr=NULL;
	*blockPtr=NULL;
	return 0;
}





void populateReadIngressBlock(SwqBuffer *rec, int ingressPosition, int ingressSize, int nodeSize, RoutingBuilder *rb)
{
	RoutingReadIngressBlock *ingressBlock=allocateReadIngressBlock(rb);

	SequenceWithQuality *currentRec=rec->rec+ingressPosition;

	MemSingleBrickAllocator alloc;
	mbInitSingleBrickAllocator(&alloc, &(rb->sequenceLinkPile));

	for(int i=0;i<ingressSize;i++)
		{
		int length=currentRec->length;
		int offset=0;

		u32 *brickIndexPtr=ingressBlock->sequenceLinkIndex+i;

		while(offset<length)
			{
			SequenceLink *sequenceLink=mbSingleBrickAllocate(&alloc, brickIndexPtr);

			s32 lengthToPack=MIN(PAD_2BITLENGTH_BYTE(length), SEQUENCE_LINK_BYTES);

			packSequence(currentRec->seq+offset, sequenceLink->packedSequence, length);

			s32 basesPacked=(lengthToPack<<2);

			sequenceLink->length=basesPacked;
			brickIndexPtr=&(sequenceLink->nextIndex);
			offset+=basesPacked;
			}

		*brickIndexPtr=LINK_INDEX_DUMMY;
		currentRec++;
		}

	mbSingleBrickAllocatorCleanup(&alloc);

	ingressBlock->sequenceCount=ingressSize;
	ingressBlock->sequencePosition=0;

	ingressBlock->completionCount=ingressSize;

	queueReadIngressBlock(ingressBlock);

	//int pileFree=mbGetFreeSingleBrickPile(&(rb->sequenceLinkPile));

	//LOG(LOG_INFO,"populateReadIngressBlock: %i",pileFree);
}




s32 scanForAndFreeCompleteReadIngressBlocks(RoutingBuilder *rb)
{
	int index=scanForCompleteReadIngressBlock(rb);
	if(index<0)
		return 0;

	RoutingReadIngressBlock *ingressBlock=dequeueCompleteReadIngressBlock(rb, index);

	if(ingressBlock!=NULL)
		{
		unallocateReadIngressBlock(ingressBlock);
		unreserveReadIngressBlock(rb);

//		LOG(LOG_INFO,"Freed");
		return 1;
		}
	return 0;
}



