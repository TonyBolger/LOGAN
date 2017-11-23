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
		LOG(LOG_INFO,"reserveIngressBlock insufficient bricks: Want %i Have %i",maximumBricksForSequence);
		return 0;
		}

	u64 newVal=current+1;

	if(!__atomic_compare_exchange_n(&(rb->allocatedIngressBlocks), &current, newVal, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return 0;

	return 1;

}

/*
static void unreserveReadLookupBlock(RoutingBuilder *rb)
{
	__atomic_fetch_sub(&(rb->allocatedIngressBlocks),1, __ATOMIC_SEQ_CST);
}
*/


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

/*
static void queueReadIngressBlock(RoutingReadIngressBlock *readBlock)
{
	u32 current=BLOCK_STATUS_ALLOCATED;
	if(!__atomic_compare_exchange_n(&(readBlock->status), &current, BLOCK_STATUS_ACTIVE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		LOG(LOG_CRITICAL,"Tried to queue lookup readblock in wrong state, should never happen");
}

static int scanForCompleteReadIngressBlock(RoutingBuilder *rb)
{
	RoutingReadIngressBlock *readBlock;
	int i;

	for(i=0;i<TR_READBLOCK_INGRESS_INFLIGHT;i++)
		{
		readBlock=rb->ingressBlocks+i;
		if(__atomic_load_n(readBlock->status, __ATOMIC_RELAXED) == BLOCK_STATUS_ACTIVE && readBlock->completionCount==0)
			return i;
		}
	return -1;
}

static RoutingReadIngressBlock *dequeueCompleteReadIngressBlock(RoutingBuilder *rb, int index)
{
	//LOG(LOG_INFO,"Try dequeue readblock");

	RoutingReadIngressBlock *ingressBlock=rb->ingressBlocks+index;

	u32 current=__atomic_load_n(&ingressBlock->status, __ATOMIC_SEQ_CST);

	if(current == 2 && __atomic_load_n(&ingressBlock->completionCount, __ATOMIC_SEQ_CST) == 0)
		{
		if(__atomic_compare_exchange_n(&(ingressBlock->status), &current, 3, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
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

*/



void populateReadIngressBlock(SwqBuffer *rec, int ingressPosition, int ingressSize, int nodeSize, RoutingBuilder *rb)
{
	RoutingReadIngressBlock *ingressBlock=allocateReadIngressBlock(rb);

	SequenceWithQuality *currentRec=rec->rec+ingressPosition;
	int ingressLast=ingressPosition+ingressSize;

//	int maxReadLength=0;

	MemSingleBrickAllocator alloc;
	mbInitSingleBrickAllocator(&alloc, &(rb->sequenceLinkPile));

	for(int i=ingressPosition;i<ingressLast;i++)
		{
		//int length=currentRec->length;

		/*
		readData->seqLength=length;

		if(length>maxReadLength)
			maxReadLength=length;

		readData->packedSeq=dAllocQuadAligned(disp,PAD_2BITLENGTH_BYTE(length)); // Consider extra padding on these allocs
		//readData->quality=dAlloc(disp,length+1);
		readData->smers=dAllocQuadAligned(disp,length*sizeof(SmerId)*2);

		//LOG(LOG_INFO,"Packing %p into %p with length %i",currentRec->seq, readData->packedSeq, length);
		packSequence(currentRec->seq, readData->packedSeq, length);

		s32 maxValidIndex=(length-nodeSize);
		calculatePossibleSmersAndCompSmers(readData->packedSeq, maxValidIndex, readData->smers);

		assignSmersToLookupPercolates(readData->smers, maxValidIndex+1, readBlock->smerEntryLookupsPercolates, disp);

*/
		ingressBlock->sequenceBrickIndex[i]=LINK_INDEX_DUMMY;

		currentRec++;

		}

	ingressBlock->sequenceCount=ingressSize;
	ingressBlock->sequencePosition=0;

	LOG(LOG_INFO,"populateReadIngressBlock");
}







/*
	//MemDispenser *disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_LOOKUP, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);
	//	readBlock->disp=disp;

//	LOG(LOG_INFO,"DispAlloc %p",disp);
	RoutingReadLookupBlock *readBlock=allocateReadLookupBlock(rb);

	MemDispenser *disp=readBlock->disp;

	//int smerCount=0;
	int i=0;
	int ingressLast=ingressPosition+ingressSize;

	for(i=0;i<SMER_LOOKUP_PERCOLATES;i++)
		readBlock->smerEntryLookupsPercolates[i]=allocPercolateBlock(disp);

	for(i=0;i<SMER_SLICES;i++)
		readBlock->smerEntryLookups[i]=allocEntryLookupBlock(disp);

	SequenceWithQuality *currentRec=rec->rec+ingressPosition;
	RoutingReadLookupData *readData=readBlock->readData;

	int maxReadLength=0;

	for(i=ingressPosition;i<ingressLast;i++)
		{
		int length=currentRec->length;
		readData->seqLength=length;

		if(length>maxReadLength)
			maxReadLength=length;

		readData->packedSeq=dAllocQuadAligned(disp,PAD_2BITLENGTH_BYTE(length)); // Consider extra padding on these allocs
		//readData->quality=dAlloc(disp,length+1);
		readData->smers=dAllocQuadAligned(disp,length*sizeof(SmerId)*2);

		//LOG(LOG_INFO,"Packing %p into %p with length %i",currentRec->seq, readData->packedSeq, length);
		packSequence(currentRec->seq, readData->packedSeq, length);

		s32 maxValidIndex=(length-nodeSize);
		calculatePossibleSmersAndCompSmers(readData->packedSeq, maxValidIndex, readData->smers);

		assignSmersToLookupPercolates(readData->smers, maxValidIndex+1, readBlock->smerEntryLookupsPercolates, disp);

		currentRec++;
		readData++;
		}

	readBlock->readCount=ingressLast-ingressPosition;
	readBlock->maxReadLength=maxReadLength;

	assignPercolatesToEntryLookups(readBlock->smerEntryLookupsPercolates, readBlock->smerEntryLookups, disp);

	int usedSlices=0;
	for(i=0;i<SMER_SLICES;i++)
		{
		RoutingSmerEntryLookup *lookupForSlice=readBlock->smerEntryLookups[i];

		if(lookupForSlice->entryCount>0)
			{
			lookupForSlice->completionCountPtr=&(readBlock->completionCount);
			usedSlices++;
			}
		else
			lookupForSlice->completionCountPtr=NULL;
		}

	__atomic_store_n(&(readBlock->completionCount), usedSlices, __ATOMIC_SEQ_CST);

	for(i=0;i<SMER_SLICES;i++)
		{
		RoutingSmerEntryLookup *lookupForSlice=readBlock->smerEntryLookups[i];
		if(lookupForSlice->entryCount>0)
			queueLookupForSlice(rb, lookupForSlice, i);
		}

	queueReadLookupBlock(readBlock);

}
*/

