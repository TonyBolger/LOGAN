#include "common.h"


s32 reserveReadIngressBlock(RoutingBuilder *rb)
{
	u64 current=__atomic_load_n(&(rb->allocatedIngressBlocks), __ATOMIC_SEQ_CST);

	if(current>=TR_READBLOCK_INGRESS_INFLIGHT)
		return 0;

	// FIXME: Should do this a better way
	// Also need to ensure space for sequences 10M/224 per link, plus 10K partially filled, plus 10K margin: 44,643 + 20,000 -> ~65K free bricks
	// In practice, less will be needed. This hack only works if there's a single ingressBlock which can consume the bricks.

	int maximumBricksForSequence=((TASK_INGRESS_BASESTOTAL+SEQUENCE_LINK_BASES)/SEQUENCE_LINK_BASES)+TR_INGRESS_BLOCKSIZE;

	if(!mbCheckSingleBrickAvailability(&(rb->sequenceLinkPile), maximumBricksForSequence))
		{
		//LOG(LOG_INFO,"reserveIngressBlock insufficient bricks: Want %i Have %i",maximumBricksForSequence, mbGetFreeSingleBrickPile(&(rb->sequenceLinkPile)));
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

			if(current==INGRESS_BLOCK_STATUS_IDLE)
				{
				if(__atomic_compare_exchange_n(&(rb->ingressBlocks[i].status), &current, INGRESS_BLOCK_STATUS_ALLOCATED, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
					return rb->ingressBlocks+i;
				}
			}
		}

	LOG(LOG_CRITICAL,"Failed to queue lookup readblock, should never happen"); // Can actually happen, just stupidly unlikely
	return 0;
}


static void queueReadIngressBlock(RoutingReadIngressBlock *ingressBlock)
{
	u32 current=INGRESS_BLOCK_STATUS_ALLOCATED;
	if(!__atomic_compare_exchange_n(&(ingressBlock->status), &current, INGRESS_BLOCK_STATUS_ACTIVE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		LOG(LOG_CRITICAL,"Tried to queue lookup readblock in wrong state, should never happen");
}


static s32 lockIngressBlockForConsumption(RoutingReadIngressBlock *ingressBlock)
{
	s32 current=__atomic_load_n(&(ingressBlock->status), __ATOMIC_SEQ_CST);

	if(current!=INGRESS_BLOCK_STATUS_ACTIVE)
		return 0;

	if(!__atomic_compare_exchange_n(&(ingressBlock->status), &current, INGRESS_BLOCK_STATUS_LOCKED, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return 0;

	return 1;
}


static void unlockIngressBlockIncomplete(RoutingReadIngressBlock *ingressBlock)
{
	u32 current=INGRESS_BLOCK_STATUS_LOCKED;

	if(!__atomic_compare_exchange_n(&(ingressBlock->status), &current, INGRESS_BLOCK_STATUS_ACTIVE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		LOG(LOG_CRITICAL,"Failed to unlock ingress block for consumption, should never happen");
}


static void unlockIngressBlockComplete(RoutingReadIngressBlock *ingressBlock)
{
	u32 current=INGRESS_BLOCK_STATUS_LOCKED;

	if(!__atomic_compare_exchange_n(&(ingressBlock->status), &current, INGRESS_BLOCK_STATUS_COMPLETE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		LOG(LOG_CRITICAL,"Failed to unlock ingress block for consumption, should never happen");
}



/*
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
*/




static void unallocateReadIngressBlock(RoutingReadIngressBlock *readBlock)
{
	u32 current=INGRESS_BLOCK_STATUS_COMPLETE;
	if(!__atomic_compare_exchange_n(&(readBlock->status), &current, INGRESS_BLOCK_STATUS_IDLE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		{
		LOG(LOG_CRITICAL,"Tried to unreserve lookup readblock in wrong state, should never happen");
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

		if(__atomic_load_n(&(readBlock->status), __ATOMIC_RELAXED) == INGRESS_BLOCK_STATUS_ACTIVE)
			{
			u32 count=__atomic_load_n(&(readBlock->sequenceCount), __ATOMIC_RELAXED);
			u32 position=__atomic_load_n(&(readBlock->sequencePosition), __ATOMIC_RELAXED);

			total+=(count-position);
			}
		/*
		else
			{
			LOG(LOG_INFO,"ingress: Block not active - %i",__atomic_load_n(&(readBlock->status), __ATOMIC_SEQ_CST));
			}
			*/
		}

	return total;
}




void populateReadIngressBlock(SwqBuffer *rec, int ingressPosition, int ingressSize, int nodeSize, RoutingBuilder *rb)
{
	RoutingReadIngressBlock *ingressBlock=allocateReadIngressBlock(rb);

	SequenceWithQuality *currentRec=rec->rec+ingressPosition;

	int maximumBricksForSequence=((TASK_INGRESS_BASESTOTAL+SEQUENCE_LINK_BASES)/SEQUENCE_LINK_BASES)+TR_INGRESS_BLOCKSIZE;

	MemSingleBrickAllocator alloc;
	mbInitSingleBrickAllocator(&alloc, &(rb->sequenceLinkPile), maximumBricksForSequence);

	//LOG(LOG_INFO,"Ingress is %i",ingressSize);

	for(int i=0;i<ingressSize;i++)
		{
		int length=currentRec->length;
		int offset=0;

		u32 *brickIndexPtr=ingressBlock->sequenceLinkIndex+i;

		if(length>1000000)
			LOG(LOG_CRITICAL,"Length is %i",length);

		while(offset<length)
			{
			SequenceLink *sequenceLink=mbSingleBrickAllocate(&alloc, brickIndexPtr);

			s32 lengthToPack=MIN(length-offset, SEQUENCE_LINK_BASES);
			packSequence(currentRec->seq+offset, sequenceLink->packedSequence, lengthToPack);

			sequenceLink->position=0;
			sequenceLink->length=lengthToPack;
			brickIndexPtr=&(sequenceLink->nextIndex);
			offset+=lengthToPack;
			}

		*brickIndexPtr=LINK_INDEX_DUMMY;
		currentRec++;
		}

	mbSingleBrickAllocatorCleanup(&alloc);

	ingressBlock->sequenceCount=ingressSize;
	ingressBlock->sequencePosition=0;

	queueReadIngressBlock(ingressBlock);

	//int pileFree=mbGetFreeSingleBrickPile(&(rb->sequenceLinkPile));

	//LOG(LOG_INFO,"populateReadIngressBlock: %i",pileFree);
}





/*
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
*/


s32 processIngressedReads(RoutingBuilder *rb)
{
	s32 availableReads=getAvailableReadIngress(rb);

//	LOG(LOG_INFO, "processIngressedReads %i",availableReads);

	if(!availableReads)
		{
	//	LOG(LOG_INFO, "processIngressedReads: no reads available");
		return 0;
		}

	int work=0;

	for(int i=0;i<TR_READBLOCK_INGRESS_INFLIGHT;i++)
		{
		RoutingReadIngressBlock *readBlock=rb->ingressBlocks+i;

//		LOG(LOG_INFO, "processIngressedReads");

		if(lockIngressBlockForConsumption(readBlock))
			{
			//LOG(LOG_INFO, "processIngressedReads");
			work=queueSmerLookupsForIngress(rb, readBlock);

			if(readBlock->sequencePosition==readBlock->sequenceCount)
				{
				unlockIngressBlockComplete(readBlock); // Status: LOCKED -> COMPLETE
				unallocateReadIngressBlock(readBlock); // Status: COMPLETE -> IDLE
				unreserveReadIngressBlock(rb);
				}
			else
				unlockIngressBlockIncomplete(readBlock);
			}
		//else
//			LOG(LOG_INFO, "processIngressedReads: lock fail");
		}

	return work;

}






