/*
 * taskRoutingLookup.c
 *
 *  Created on: Mar 3, 2016
 *      Author: tony
 */

#include "common.h"



static RoutingLookupPercolate *allocPercolateBlock(MemDispenser *disp)
{

	RoutingLookupPercolate *block=dAllocCacheAligned(disp, sizeof(RoutingLookupPercolate));

	block->entryCount=0;
	block->entries=dAllocCacheAligned(disp, TR_LOOKUPS_PER_PERCOLATE_BLOCK*sizeof(u32));

	return block;
}


static void expandLookupPercolateBlock(RoutingLookupPercolate *block, MemDispenser *disp)
{
	int oldSize=block->entryCount;
	int size=oldSize*2;

	int oldEntrySize=oldSize*sizeof(u32);

	SmerEntry *entries=dAllocCacheAligned(disp, size*sizeof(u32));
	memcpy(entries,block->entries,oldEntrySize);
	block->entries=entries;
	//block->presenceAbsence=NULL;//dAlloc(disp, PAD_BYTELENGTH_8BYTE(PAD_1BITLENGTH_BYTE(size)));
}



static void assignSmersToLookupPercolates(SmerId *smers, int smerCount, RoutingLookupPercolate *smerLookupPercolates[], MemDispenser *disp)
{
	//smerCount*=2;

	for(int i=0;i<smerCount;i++)
		{
		//SmerId fsmer=smers[i];
		//SmerId rsmer=smers[i+1];

		SmerId smer=smers[i];

		SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
		u64 hash=hashForSmer(smerEntry);
		u32 slice=sliceForSmer(smer,hash);
		u32 sliceGroup=slice>>SMER_LOOKUP_PERCOLATE_SHIFT;

		//char buffer[SMER_BASES+1];
		//unpackSmer(smer, buffer);
		//LOG(LOG_INFO,"Assigned %s to %i as %x", buffer, slice, smerEntry);

		RoutingLookupPercolate *percolate=smerLookupPercolates[sliceGroup];
		u64 entryCount=percolate->entryCount;

		if(entryCount>=TR_LOOKUPS_PER_PERCOLATE_BLOCK && !((entryCount & (entryCount - 1))))
			expandLookupPercolateBlock(percolate, disp);

		percolate->entries[entryCount++]=slice;
		percolate->entries[entryCount]=smerEntry;
		percolate->entryCount+=2;
		}

}


static RoutingSmerEntryLookup *allocEntryLookupBlock(MemDispenser *disp)
{

	RoutingSmerEntryLookup *block=dAllocCacheAligned(disp, sizeof(RoutingSmerEntryLookup));

	block->nextPtr=NULL;

	block->entryCount=0;
	block->entries=dAllocCacheAligned(disp, TR_LOOKUPS_PER_SLICE_BLOCK*sizeof(SmerEntry));
	//block->presenceAbsence=NULL;//dAlloc(disp, PAD_BYTELENGTH_8BYTE(PAD_1BITLENGTH_BYTE(TR_LOOKUPS_PER_SLICE_BLOCK)));

	return block;
}


static void expandEntryLookupBlock(RoutingSmerEntryLookup *block, MemDispenser *disp)
{
	int oldSize=block->entryCount;
	int size=oldSize*2;

	int oldEntrySize=oldSize*sizeof(SmerEntry);

	SmerEntry *entries=dAllocCacheAligned(disp, size*sizeof(SmerEntry));
	memcpy(entries,block->entries,oldEntrySize);
	block->entries=entries;
	//block->presenceAbsence=NULL;//dAlloc(disp, PAD_BYTELENGTH_8BYTE(PAD_1BITLENGTH_BYTE(size)));
}



static void assignPercolatesToEntryLookups(RoutingLookupPercolate *smerLookupPercolates[], RoutingSmerEntryLookup *smerEntryLookups[], MemDispenser *disp)
{
	for(int i=0;i<SMER_LOOKUP_PERCOLATES;i++)
		{
		RoutingLookupPercolate *percolate=smerLookupPercolates[i];
		int intCount=0;

		while(intCount<percolate->entryCount)
			{
			u32 slice=percolate->entries[intCount++];
			SmerEntry smerEntry=percolate->entries[intCount++];

			RoutingSmerEntryLookup *lookupForSlice=smerEntryLookups[slice];
			u64 entryCount=lookupForSlice->entryCount;

			if(entryCount>=TR_LOOKUPS_PER_SLICE_BLOCK && !((entryCount & (entryCount - 1))))
				expandEntryLookupBlock(lookupForSlice, disp);

			lookupForSlice->entries[entryCount]=smerEntry;
			lookupForSlice->entryCount++;
			}

		}
}

static void extractLookupPercolatesFromEntryLookups(RoutingSmerEntryLookup *smerEntryLookups[], RoutingLookupPercolate *smerLookupPercolates[])
{
	int entryCount[SMER_SLICES];
	memset(entryCount,0,sizeof(entryCount));

	for(int i=0;i<SMER_LOOKUP_PERCOLATES;i++)
		{
		RoutingLookupPercolate *percolate=smerLookupPercolates[i];
		int intCount=0;

		while(intCount<percolate->entryCount)
			{
			u32 slice=percolate->entries[intCount++];

			RoutingSmerEntryLookup *lookupForSlice=smerEntryLookups[slice];
			percolate->entries[intCount++]=lookupForSlice->entries[entryCount[slice]++];
			}

		percolate->entryCount=intCount;
		percolate->entryPosition=0;
		}
}

/*
static int extractIndexedSmerDataFromLookupPercolates(SmerId *allSmers, int allSmerCount, SmerId *fsmers, SmerId *rsmers,
			u32 *slices, u32 *sliceIndexes, u32 *readIndexes, RoutingLookupPercolate *smerLookupPercolates[])
{
	int foundCount=0;

	for(int i=0;i<allSmerCount;i++)
		{
		SmerId fsmer=allSmers[i*2];
		SmerId rsmer=allSmers[i*2+1];

		SmerId smer=CANONICAL_SMER(fsmer,rsmer);

		SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
		u64 hash=hashForSmer(smerEntry);
		u32 slice=sliceForSmer(smer,hash);
		u32 sliceGroup=slice>>SMER_LOOKUP_PERCOLATE_SHIFT;

		RoutingLookupPercolate *percolate=smerLookupPercolates[sliceGroup];

		//u32 slice==percolate->entries[entryCount]; ??
		u64 entryCount=percolate->entryCount+1;
		int sliceIndex=percolate->entries[entryCount++];

		if(i==0)
			{
			*(fsmers-foundCount)=fsmer;
			*(rsmers-foundCount)=rsmer;
			*(slices-foundCount)=slice;
			*(sliceIndexes-foundCount)=sliceIndex;
			*(readIndexes-foundCount)=i;
			foundCount++;
			}

		if(sliceIndex!=SMER_DUMMY)
			{
			*(fsmers-foundCount)=fsmer;
			*(rsmers-foundCount)=rsmer;
			*(slices-foundCount)=slice;
			*(sliceIndexes-foundCount)=sliceIndex;
			*(readIndexes-foundCount)=i;
			foundCount++;
			}

		if(i==allSmerCount-1)
			{
			*(fsmers-foundCount)=fsmer;
			*(rsmers-foundCount)=rsmer;
			*(slices-foundCount)=slice;
			*(sliceIndexes-foundCount)=sliceIndex;
			*(readIndexes-foundCount)=i;
			foundCount++;
			}

		percolate->entryCount=entryCount;
		}

	return foundCount;
}


*/

#define EXTRACT_FIRST_SMER 1
#define EXTRACT_LAST_SMER 2

//static
int extractIndexedSmerDataFromLookupPercolates(SmerId *allSmers, int allSmerCount, u16 allRevComp,
		DispatchLinkSmer *indexedData, u8 *revCompFlag,
		s32 extractFlag, RoutingLookupPercolate *smerLookupPercolates[])
{
	//int foundCount=0;

	DispatchLinkSmer *indexedDataOrig=indexedData;

	int lastSmer=allSmerCount-1;

	for(int i=0;i<=lastSmer;i++)
		{
		SmerId smer=allSmers[i];

		SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
		u64 hash=hashForSmer(smerEntry);
		u32 slice=sliceForSmer(smer,hash);
		u32 sliceGroup=slice>>SMER_LOOKUP_PERCOLATE_SHIFT;

		RoutingLookupPercolate *percolate=smerLookupPercolates[sliceGroup];

		u32 sliceTest=percolate->entries[percolate->entryPosition];
		if(sliceTest!=slice)
			{
			LOG(LOG_CRITICAL,"Mismatch between expected %i and found %i",sliceTest, slice);
			}

		u64 entryCount=percolate->entryPosition+1;
		int sliceIndex=percolate->entries[entryCount++];

		if((extractFlag & EXTRACT_FIRST_SMER) && (i==0))
			{
			indexedData->smer=smer;
			indexedData->slice=slice;
			indexedData->sliceIndex=sliceIndex;
			indexedData->seqIndexOffset=i;
			indexedData++;
			(*revCompFlag++)=allRevComp&0x1;
			}

		if(sliceIndex!=SMER_SLICE_DUMMY)
			{
			indexedData->smer=smer;
			indexedData->slice=slice;
			indexedData->sliceIndex=sliceIndex;
			indexedData->seqIndexOffset=i;
			indexedData++;
			(*revCompFlag++)=allRevComp&0x1;
			}

		if((extractFlag & EXTRACT_LAST_SMER) && (i==lastSmer))
			{
			indexedData->smer=smer;
			indexedData->slice=slice;
			indexedData->sliceIndex=sliceIndex;
			indexedData->seqIndexOffset=i;
			indexedData++;
			(*revCompFlag++)=allRevComp&0x1;
			}

		percolate->entryPosition=entryCount;

		if(entryCount>percolate->entryCount)
			{
			LOG(LOG_INFO,"Beyond end of percolate %i %i",entryCount, percolate->entryCount);
			}

		allRevComp>>=1;
		}

	int entries=indexedData-indexedDataOrig;

	// Calculate differential offsets

	int prevIndex=indexedDataOrig->seqIndexOffset;
	for(int i=1; i<entries; i++)
		{
		int thisIndex=indexedDataOrig[i].seqIndexOffset;
		indexedDataOrig[i].seqIndexOffset=thisIndex-prevIndex;
		prevIndex=thisIndex;
		}

	return entries;
}

//static
void dumpExtractedIndexedSmers(DispatchLinkSmer *indexedData, u8 *revCompFlag, s32 smerCount)
{
	LOG(LOG_INFO,"DumpExtractedIndexedSmers");

	for(int i=0;i<smerCount;i++)
		{
		u8 smerBuffer[SMER_BASES+1];
		unpackSmer(indexedData[i].smer, smerBuffer);

		if(revCompFlag[i])
			LOG(LOG_INFO,"Smer %s (rc) [%i] at %i:%i", smerBuffer, indexedData[i].seqIndexOffset, indexedData[i].slice, indexedData[i].sliceIndex);
		else
			LOG(LOG_INFO,"Smer %s [%i] at %i:%i", smerBuffer, indexedData[i].seqIndexOffset, indexedData[i].slice, indexedData[i].sliceIndex);
		}
}


static void queueLookupForSlice(RoutingBuilder *rb, RoutingSmerEntryLookup *lookupForSlice, int sliceNum)
{
	RoutingSmerEntryLookup *current=NULL;
	int loopCount=0;

	__atomic_thread_fence(__ATOMIC_SEQ_CST); // Queue is release, fence first

	do
		{
		if(loopCount++>1000000)
			{
			LOG(LOG_CRITICAL,"Loop Stuck");
			}

		current=__atomic_load_n(rb->smerEntryLookupPtr+sliceNum, __ATOMIC_SEQ_CST);
		__atomic_store_n(&lookupForSlice->nextPtr, current, __ATOMIC_SEQ_CST);
		}
	while(!__atomic_compare_exchange_n(rb->smerEntryLookupPtr+sliceNum, &current, lookupForSlice, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));


}

static RoutingSmerEntryLookup *dequeueLookupForSliceList(RoutingBuilder *rb, int sliceNum)
{
	RoutingSmerEntryLookup *current=NULL;
	int loopCount=0;

	do
		{
		if(loopCount++>1000000)
			{
			LOG(LOG_CRITICAL,"Loop Stuck");
			}

		current=__atomic_load_n(rb->smerEntryLookupPtr+sliceNum, __ATOMIC_SEQ_CST);

		if(current==NULL)
			return NULL;
		}
	while(!__atomic_compare_exchange_n(rb->smerEntryLookupPtr+sliceNum, &current, NULL, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

	__atomic_thread_fence(__ATOMIC_SEQ_CST);

	return current;
}


static RoutingReadLookupBlock *allocateReadLookupBlock(RoutingBuilder *rb)
{
	//LOG(LOG_INFO,"Queue readblock");

	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		int i;

		for(i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
			{
			u64 current=__atomic_load_n(&rb->readLookupBlocks[i].compStat, __ATOMIC_SEQ_CST);

			if(current==LOOKUP_BLOCK_STATUS_IDLE)
				{
				if(__atomic_compare_exchange_n(&(rb->readLookupBlocks[i].compStat), &current, LOOKUP_BLOCK_STATUS_ALLOCATED, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
					{
					//LOG(LOG_INFO,"allocateReadLookupBlock %p",(rb->readLookupBlocks+i));
					return rb->readLookupBlocks+i;
					}
				}
			}
		}

	LOG(LOG_CRITICAL,"Failed to queue lookup readblock, should never happen"); // Can actually happen, just stupidly unlikely
	return NULL;
}



static void queueReadLookupBlock(RoutingReadLookupBlock *readBlock, u64 usedSlices)
{
	if(usedSlices==0)
		LOG(LOG_CRITICAL,"queueReadLookupBlock empty block");

	//LOG(LOG_INFO,"ReadBlock type %i contains %i",readBlock->blockType, readBlock->readCount);

	__atomic_thread_fence(__ATOMIC_SEQ_CST); // Queue is release, fence first

	int loopCount=0;
	u64 current, compStat;

	do
		{
		if(loopCount++>1000000)
			{
			LOG(LOG_CRITICAL,"Loop Stuck");
			}

		current=__atomic_load_n(&(readBlock->compStat), __ATOMIC_SEQ_CST);

		if(current!=LOOKUP_BLOCK_STATUS_ALLOCATED)
			LOG(LOG_CRITICAL,"Tried to queue lookup readblock in wrong state %08x, should never happen", current);

		compStat=LOOKUP_BLOCK_STATUS_ACTIVE | usedSlices;
		}
	while(!__atomic_compare_exchange_n(&(readBlock->compStat), &current, compStat, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

//	LOG(LOG_INFO,"queueReadLookupBlock %p",readBlock);
}

static int scanForCompleteReadLookupBlock(RoutingBuilder *rb)
{
	RoutingReadLookupBlock *readBlock;
	int i;

	for(i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
		{
		readBlock=rb->readLookupBlocks+i;
		u64 current=__atomic_load_n(&readBlock->compStat, __ATOMIC_SEQ_CST); // Combined access checks both count and status

		if(current == LOOKUP_BLOCK_STATUS_ACTIVE) // Also ensures zero completion count
			{
			return i;
			}

		if((current & LOOKUP_BLOCK_STATUS_MASK) > LOOKUP_BLOCK_STATUS_COMPLETE)
			LOG(LOG_CRITICAL,"LookupBlock %i has invalid status %8x", i, current);
		}
	return -1;
}

static RoutingReadLookupBlock *dequeueCompleteReadLookupBlock(RoutingBuilder *rb, int index)
{
	//LOG(LOG_INFO,"Try dequeue readblock");

	RoutingReadLookupBlock *readBlock=rb->readLookupBlocks+index;

	u64 current=__atomic_load_n(&readBlock->compStat, __ATOMIC_SEQ_CST); // Combined access checks both count and status

	if(current == LOOKUP_BLOCK_STATUS_ACTIVE) // Also ensures zero completion count
		{
		if(__atomic_compare_exchange_n(&(readBlock->compStat), &current, LOOKUP_BLOCK_STATUS_COMPLETE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			{
			//LOG(LOG_INFO,"dequeueCompleteReadLookupBlock %p",readBlock);
			return readBlock;
			}
		}

	return NULL;
}


static void unallocateReadLookupBlock(RoutingReadLookupBlock *readBlock)
{
	int loopCount=0;
	u64 current;

	do
		{
		if(loopCount++>1000000)
			{
			LOG(LOG_CRITICAL,"Loop Stuck");
			}

		current=__atomic_load_n(&(readBlock->compStat), __ATOMIC_SEQ_CST);

		if(current!=LOOKUP_BLOCK_STATUS_COMPLETE)
			LOG(LOG_CRITICAL,"Tried to unreserve lookup readblock in wrong state %08x, should never happen", current);
		}
	while(!__atomic_compare_exchange_n(&(readBlock->compStat), &current, LOOKUP_BLOCK_STATUS_IDLE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

	//LOG(LOG_INFO,"unallocateReadLookupBlock %p",readBlock);
}


int countLookupReadsRemaining(RoutingBuilder *rb)
{
	int readsRemaining=0;

	for(int i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
	{
		u64 compStat=__atomic_load_n(&(rb->readLookupBlocks[i].compStat), __ATOMIC_RELAXED);

		if((compStat & LOOKUP_BLOCK_STATUS_MASK) == LOOKUP_BLOCK_STATUS_ACTIVE)
			readsRemaining+=compStat & LOOKUP_BLOCK_COUNT_MASK;
	}

	return readsRemaining;
}



static int reserveReadLookupBlock(RoutingBuilder *rb, int reservationMargin)
{
	u64 current, newVal;
	int loopCount=0;

	do
		{
		if(loopCount++>10000)
			LOG(LOG_CRITICAL,"Loop Stuck");

		current=__atomic_load_n(&(rb->allocatedReadLookupBlocks), __ATOMIC_SEQ_CST);

		if((reservationMargin + current)>=TR_READBLOCK_LOOKUPS_INFLIGHT)
			return 0;

		newVal=current+1;
		}
	while(!__atomic_compare_exchange_n(&(rb->allocatedReadLookupBlocks), &current, newVal, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

	return 1;
}

static void unreserveReadLookupBlock(RoutingBuilder *rb)
{
	__atomic_fetch_sub(&(rb->allocatedReadLookupBlocks),1, __ATOMIC_SEQ_CST);
}



static void queueLookupRecycleBlock(RoutingBuilder *rb, RoutingReadLookupRecycleBlock *recycleBlock)
{
	s32 blockType=recycleBlock->blockType;

	RoutingReadLookupRecycleBlock **recycleBlockPtr=(blockType==LOOKUP_RECYCLE_BLOCK_PREDISPATCH?&(rb->lookupPredispatchRecyclePtr):&(rb->lookupPostdispatchRecyclePtr));
	u64 *recycleCountPtr=(blockType==LOOKUP_RECYCLE_BLOCK_PREDISPATCH?&(rb->lookupPredispatchRecycleCount):&(rb->lookupPostdispatchRecycleCount));

	RoutingReadLookupRecycleBlock *current=NULL;
	int loopCount=0;

	do
		{
		if(loopCount++>10000)
			LOG(LOG_CRITICAL,"Loop Stuck");

		current= __atomic_load_n(recycleBlockPtr, __ATOMIC_SEQ_CST);
		__atomic_store_n(&(recycleBlock->nextPtr), current, __ATOMIC_SEQ_CST);
		}
	while(!__atomic_compare_exchange_n(recycleBlockPtr, &current, recycleBlock, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

	__atomic_fetch_add(recycleCountPtr ,1, __ATOMIC_SEQ_CST);

}

RoutingReadLookupRecycleBlock *recycleLookupLink(RoutingBuilder *rb, RoutingReadLookupRecycleBlock *recycleBlock, s32 blockType, u32 lookupLinkIndex)
{
	if(recycleBlock==NULL)
		{
		MemDispenser *disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_LOOKUP_RECYCLE, DISPENSER_BLOCKSIZE_SMALL, DISPENSER_BLOCKSIZE_SMALL);

		recycleBlock=dAlloc(disp, sizeof(RoutingReadLookupRecycleBlock));
		recycleBlock->disp=disp;
		recycleBlock->blockType=blockType;
		recycleBlock->readCount=0;
		recycleBlock->readPosition=0;
		}

	recycleBlock->lookupLinkIndex[recycleBlock->readCount++]=lookupLinkIndex;

	if(recycleBlock->readCount==TR_LOOKUP_RECYCLE_BLOCKSIZE)
		{
		queueLookupRecycleBlock(rb, recycleBlock);
		recycleBlock=NULL;
		}

	return recycleBlock;
}

//static
void flushRecycleBlock(RoutingBuilder *rb, RoutingReadLookupRecycleBlock *recycleBlock)
{
	if(recycleBlock != NULL && recycleBlock->readCount>0)
		queueLookupRecycleBlock(rb, recycleBlock);

}


static RoutingReadLookupRecycleBlock *scanLookupRecyclePtr(RoutingBuilder *rb, int blockType)
{
	RoutingReadLookupRecycleBlock **recycleBlockPtr=(blockType==LOOKUP_RECYCLE_BLOCK_PREDISPATCH?&(rb->lookupPredispatchRecyclePtr):&(rb->lookupPostdispatchRecyclePtr));

	return __atomic_load_n(recycleBlockPtr, __ATOMIC_SEQ_CST);
}

static int getLookupRecycleCount(RoutingBuilder *rb, int blockType)
{
	u64 *recycleCountPtr=(blockType==LOOKUP_RECYCLE_BLOCK_PREDISPATCH?&(rb->lookupPredispatchRecycleCount):&(rb->lookupPostdispatchRecycleCount));

	return __atomic_load_n(recycleCountPtr, __ATOMIC_SEQ_CST);
}

static RoutingReadLookupRecycleBlock *dequeueLookupRecycleList(RoutingBuilder *rb, int blockType)
{
	RoutingReadLookupRecycleBlock **recycleBlockPtr=(blockType==LOOKUP_RECYCLE_BLOCK_PREDISPATCH?&(rb->lookupPredispatchRecyclePtr):&(rb->lookupPostdispatchRecyclePtr));
	u64 *recycleCountPtr=(blockType==LOOKUP_RECYCLE_BLOCK_PREDISPATCH?&(rb->lookupPredispatchRecycleCount):&(rb->lookupPostdispatchRecycleCount));

	RoutingReadLookupRecycleBlock *current=NULL;
	int loopCount=0;

	do
		{
		if(loopCount++>1000000)
			{
			LOG(LOG_CRITICAL,"Loop Stuck");
			}

		current= __atomic_load_n(recycleBlockPtr, __ATOMIC_SEQ_CST);

		if(current==NULL)
			return NULL;
		}
	while(!__atomic_compare_exchange_n(recycleBlockPtr, &current, NULL, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

	__atomic_store_n(recycleCountPtr, 0, __ATOMIC_SEQ_CST);

	return current;
}


/*

void queueReadsForSmerLookup(SwqBuffer *rec, int ingressPosition, int ingressSize, int nodeSize, RoutingBuilder *rb)
{

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

static SequenceLink *skipConsumedSequenceLinks(SequenceLink *sequenceLink, MemSingleBrickPile *sequencePile)
{
	int sequencePos=sequenceLink->position;
	int sequenceLen=sequenceLink->length;
	u32 nextIndex=sequenceLink->nextIndex;

	while(sequencePos==sequenceLen && nextIndex!=LINK_INDEX_DUMMY)
		{
		//LOG(LOG_INFO,"skipConsumedSequenceLinks: Skipping to %i", nextIndex);
		sequenceLink=mbSingleBrickFindByIndex(sequencePile, nextIndex);

		sequencePos=sequenceLink->position;
		sequenceLen=sequenceLink->length;
		nextIndex=sequenceLink->nextIndex;
		}

	return sequenceLink;
}


static void populateLookupLinkSmers(LookupLink *lookupLink, SequenceLink *sequenceLink, MemSingleBrickPile *sequencePile)
{
	sequenceLink=skipConsumedSequenceLinks(sequenceLink, sequencePile);
	int sequencePos=sequenceLink->position;
	int sequenceLen=sequenceLink->length;

	//LOG(LOG_INFO,"Populate from position %i of %i", sequencePos, sequenceLen);

	SequenceLink *nextSequenceLink=NULL;
	u8 *nextPackedSequence=NULL;
	int nextSequencePackAvailable=0;

	int sequenceLenNext=0;
	int eosFlag=0;

	int smerCount=0;
	int smersAvailable=sequenceLen-sequencePos-SMER_BASES+1;

	if(smersAvailable>LOOKUP_LINK_SMERS)
		{
		smerCount=LOOKUP_LINK_SMERS;
		}
	else
		{
		smerCount=smersAvailable;

		u32 nextIndex=sequenceLink->nextIndex;

		if(nextIndex==LINK_INDEX_DUMMY)
			eosFlag=1;

		else if(smerCount < LOOKUP_LINK_SMERS)
			{
			//LOG(LOG_INFO,"Moving to next seqlink - %u", nextIndex);
			nextSequenceLink=mbSingleBrickFindByIndex(sequencePile, nextIndex);

			//trDumpSequenceLink(nextSequenceLink, nextIndex);

			if(nextSequenceLink->position!=0)
				LOG(LOG_CRITICAL,"Next sequence link at position %i",nextSequenceLink->position);

			sequenceLenNext=nextSequenceLink->length;
			smerCount+=sequenceLenNext;

			if((smerCount<=LOOKUP_LINK_SMERS)&&(nextSequenceLink->nextIndex==LINK_INDEX_DUMMY))
				eosFlag=1;

			if(smerCount>LOOKUP_LINK_SMERS)
				smerCount=LOOKUP_LINK_SMERS;



			//LOG(LOG_INFO,"Adjusted smer count %i",smerCount);

			nextPackedSequence=nextSequenceLink->packedSequence;
			nextSequencePackAvailable=(sequenceLenNext+3)>>2;
			}
		}

	int sequencePackedPos=sequencePos>>2;
	int sequenceSkipSmers=sequencePos&0x3;

	int sequencePackAvailable=((sequenceLen+3)>>2)-sequencePackedPos;

	SmerId smerBuf[LOOKUP_LINK_SMERS+3];
	u32 revCompBuf=0;

	//LOG(LOG_INFO,"Pack available %i %i",sequencePackAvailable, nextSequencePackAvailable);

	calculatePossibleSmersAndOrientation(sequenceLink->packedSequence+sequencePackedPos, nextPackedSequence, sequencePackAvailable, nextSequencePackAvailable,
			smerCount+sequenceSkipSmers, smerBuf, &revCompBuf);

	revCompBuf>>=sequenceSkipSmers;
	lookupLink->revComp=revCompBuf | (eosFlag?LOOKUPLINK_EOS_FLAG:0);
	lookupLink->smerCount=-smerCount;

	memcpy(lookupLink->smers, smerBuf+sequenceSkipSmers, smerCount*sizeof(SmerId));
}

static void populateLookupLinkFromIngress(LookupLink *lookupLink, u32 sequenceLinkIndex, MemSingleBrickPile *sequencePile)
{
	SequenceLink *sequenceLink=mbSingleBrickFindByIndex(sequencePile, sequenceLinkIndex);

	if(sequenceLink==NULL)
		LOG(LOG_CRITICAL,"Failed to find sequence link with ID %i",sequenceLinkIndex);

	//LOG(LOG_INFO,"populateLookupLinkFromIngress");
	populateLookupLinkSmers(lookupLink, sequenceLink, sequencePile);

	lookupLink->sourceIndex=sequenceLinkIndex;
	lookupLink->indexType=LINK_INDEXTYPE_SEQ;

//	trDumpSequenceLink(sequenceLink, sequenceLinkIndex);
//	trDumpLookupLink(lookupLink, lookupLinkIndex);
}


void populateLookupLinkForNonDispatchRecycle(LookupLink *lookupLink, u32 dispatchLinkIndex, SequenceLink *sequenceLink, MemSingleBrickPile *sequencePile)
{
	//LOG(LOG_INFO,"populateLookupLinkForNonDispatchRecycle");
	populateLookupLinkSmers(lookupLink, sequenceLink, sequencePile);

	// Ensures: Lookup -> Dispatch -> Seq* -> null

	lookupLink->sourceIndex=dispatchLinkIndex;
	lookupLink->indexType=LINK_INDEXTYPE_DISPATCH;

//	trDumpSequenceLink(sequenceLink, sequenceLinkIndex);
//	trDumpLookupLink(lookupLink, lookupLinkIndex);
}

void populateLookupLinkForDispatch(LookupLink *lookupLink, u32 lookupLinkIndex, DispatchLink *dispatchLink,
		SequenceLink *sequenceLink, u32 sequenceLinkIndex, MemSingleBrickPile *sequencePile)
{
	//LOG(LOG_INFO,"populateLookupLinkForDispatch");
	populateLookupLinkSmers(lookupLink, sequenceLink, sequencePile);

	// Ensures: Dispatch -> Lookup -> Seq* -> null

	dispatchLink->nextOrSourceIndex=lookupLinkIndex;
	dispatchLink->indexType=LINK_INDEXTYPE_LOOKUP;

	lookupLink->sourceIndex=sequenceLinkIndex;
	lookupLink->indexType=LINK_INDEXTYPE_SEQ;
}





int queueSmerLookupsForIngress(RoutingBuilder *rb, RoutingReadIngressBlock *ingressBlock)
{
	u32 sequences=ingressBlock->sequenceCount-ingressBlock->sequencePosition;
	if(sequences>TR_ROUTING_BLOCKSIZE)
		sequences=TR_ROUTING_BLOCKSIZE;

	MemSingleBrickPile *sequencePile=&(rb->sequenceLinkPile);
	MemDoubleBrickPile *lookupPile=&(rb->lookupLinkPile);
	MemDoubleBrickPile *dispatchPile=&(rb->dispatchLinkPile);

	if(!mbCheckDoubleBrickAvailability(lookupPile, sequences+TR_INGRESS_LOOKUP_PILEMARGIN))
		{
		//LOG(LOG_INFO,"ingress No lookup bricks");
		return 0;
		}

	if(!mbCheckDoubleBrickAvailability(dispatchPile, sequences+TR_INGRESS_DISPATCH_PILEMARGIN))
		{
		//LOG(LOG_INFO,"ingress No dispatch bricks");
		return 0;
		}

	if(!reserveReadLookupBlock(rb, TR_INGRESS_LOOKUP_BLOCKMARGIN))
		{
		//LOG(LOG_INFO,"ingress: No lookup blocks");
		return 0;
		}

//	LOG(LOG_INFO,"queueSmerLookupsForIngress");

	RoutingReadLookupBlock *lookupBlock=allocateReadLookupBlock(rb);

	if(lookupBlock==NULL)
		LOG(LOG_CRITICAL,"Failed to allocate reserved lookup block");

	MemDispenser *disp=lookupBlock->disp;

	for(int i=0;i<SMER_LOOKUP_PERCOLATES;i++)
		lookupBlock->smerEntryLookupsPercolates[i]=allocPercolateBlock(disp);

	for(int i=0;i<SMER_SLICES;i++)
		lookupBlock->smerEntryLookups[i]=allocEntryLookupBlock(disp);

	lookupBlock->blockType=LOOKUP_RECYCLE_BLOCK_PREDISPATCH;

	MemDoubleBrickAllocator alloc;
	mbInitDoubleBrickAllocator(&alloc, lookupPile, sequences);

	u32 blockIndex=0;
	u32 sequencePosition=ingressBlock->sequencePosition;

	while(sequences>0)
		{
		LookupLink *lookupLink=mbDoubleBrickAllocate(&alloc, lookupBlock->lookupLinkIndex+blockIndex);

		if(lookupLink==NULL)
			break;

		u32 sequenceLinkIndex=ingressBlock->sequenceLinkIndex[sequencePosition];
		populateLookupLinkFromIngress(lookupLink, sequenceLinkIndex, sequencePile);

		assignSmersToLookupPercolates(lookupLink->smers, -(lookupLink->smerCount), lookupBlock->smerEntryLookupsPercolates, disp);

		sequencePosition++;
		blockIndex++;
		sequences--;
		}

	mbDoubleBrickAllocatorCleanup(&alloc);

	ingressBlock->sequencePosition=sequencePosition;
	lookupBlock->readCount=blockIndex;

	assignPercolatesToEntryLookups(lookupBlock->smerEntryLookupsPercolates, lookupBlock->smerEntryLookups, disp);

	u64 usedSlices=0;
	int highestSlice=-1;

	for(int i=0;i<SMER_SLICES;i++)
		{
		RoutingSmerEntryLookup *lookupForSlice=lookupBlock->smerEntryLookups[i];

		if(lookupForSlice->entryCount>0)
			{
			lookupForSlice->completionCountPtr=&(lookupBlock->compStat);
			usedSlices++;
			highestSlice=i;
			}
		else
			lookupForSlice->completionCountPtr=NULL;
		}

	queueReadLookupBlock(lookupBlock, usedSlices);

	u64 usedSlices2=0;
	for(int i=0;i<=highestSlice;i++) // Careful now
		{
		RoutingSmerEntryLookup *lookupForSlice=lookupBlock->smerEntryLookups[i];
		if(lookupForSlice->entryCount>0)
			{
			queueLookupForSlice(rb, lookupForSlice, i);
			usedSlices2++;
			}
		}

	if(usedSlices!=usedSlices2)
		LOG(LOG_CRITICAL,"Count 1v2 %p mismatch %lu vs %lu",lookupBlock, usedSlices,usedSlices2);


	return blockIndex;
}


//static
u32 findSequenceLinkIndexForDispatchLink(RoutingBuilder *rb, DispatchLink *dispatchLink)
{
	u8 indexType=dispatchLink->indexType;

	while(indexType!=LINK_INDEXTYPE_SEQ)
		{
		if(indexType!=LINK_INDEXTYPE_DISPATCH)
			LOG(LOG_CRITICAL,"Unable to handle dispatch link with lookup sourceIndexType");

		u32 linkIndex=dispatchLink->nextOrSourceIndex;
		dispatchLink=mbDoubleBrickFindByIndex(&(rb->dispatchLinkPile), linkIndex);

		indexType=dispatchLink->indexType;
		}

	return dispatchLink->nextOrSourceIndex;
}

/*
static SequenceLink *findUnfinishedSequenceLinkInChain(MemSingleBrickPile *sequencePile, u32 sequenceLinkIndex)
{
	SequenceLink *sequenceLink=mbSingleBrickFindByIndex(sequencePile, sequenceLinkIndex);

	int pos=sequenceLink->position;
	int len=sequenceLink->length;

	while(pos==len)
		{
		u32 sequenceLinkNextIndex=sequenceLink->nextIndex;
		if(sequenceLinkNextIndex!=LINK_INDEX_DUMMY)
			{
			sequenceLink=mbSingleBrickFindByIndex(sequencePile, sequenceLinkNextIndex);
			pos=sequenceLink->position;
			len=sequenceLink->length;
			}
		else
			return NULL;
		}
	return sequenceLink;
}
*/

static int transferFoundSmersToDispatch(DispatchLink *dispatchLink, MemDoubleBrickAllocator *dispatchLinkAlloc,
		int foundCount, DispatchLinkSmer *indexedSmers, u8 *indexedRevComp, DispatchLink **lastDispatchLink)
{
	int smerIndex=dispatchLink->length;

	if(smerIndex>DISPATCH_LINK_SMERS)
		LOG(LOG_CRITICAL,"Invalid dispatch link index");

	int totalCount=smerIndex;

	for(int i=0;i<foundCount;i++)
	{
		if(smerIndex==DISPATCH_LINK_SMERS)
			{
//			LOG(LOG_INFO,"Need additional dispatch link");

			u32 newDispatchLinkIndex=LINK_INDEX_DUMMY;
			DispatchLink *newDispatchLink=mbDoubleBrickAllocate(dispatchLinkAlloc, &newDispatchLinkIndex);

			newDispatchLink->nextOrSourceIndex=dispatchLink->nextOrSourceIndex;
			newDispatchLink->indexType=dispatchLink->indexType;

			newDispatchLink->length=2; // Overlap of two from previous
			newDispatchLink->position=0;
			newDispatchLink->minEdgePosition=TR_INIT_EDGEPOSITION_INVALID;
			newDispatchLink->maxEdgePosition=TR_INIT_EDGEPOSITION_INVALID; // Invalid to force later transfer from earlier dispatch link

			int smerOffset=(DISPATCH_LINK_SMERS-2);

			memcpy(newDispatchLink->smers, dispatchLink->smers+smerOffset, sizeof(DispatchLinkSmer)*2);
			newDispatchLink->revComp=dispatchLink->revComp>>smerOffset;

			dispatchLink->nextOrSourceIndex=newDispatchLinkIndex;
			dispatchLink->indexType=LINK_INDEXTYPE_DISPATCH;

			dispatchLink->length=smerIndex;
			smerIndex=2;

			dispatchLink=newDispatchLink;

			}

		dispatchLink->smers[smerIndex]=indexedSmers[i];

		if(indexedRevComp[i])
			dispatchLink->revComp|=1<<smerIndex;

		smerIndex++;
		totalCount++;
	}

	dispatchLink->length=smerIndex;
	*lastDispatchLink=dispatchLink;

	return totalCount;
}


static s32 getAndUpdateSequenceLinkPosition(SequenceLink *sequenceLink, MemSingleBrickPile *sequencePile, int smerCount, SequenceLink **seqLinkUnfinished)
{
//	LOG(LOG_INFO,"Update SeqLinkPos: %i",smerCount);

	s32 lookupPosition=0;

	int sequenceLen=sequenceLink->length;
	int sequencePos=sequenceLink->position;

	while(sequencePos==sequenceLen)
		{
		lookupPosition+=sequencePos;

		u32 seqLinkIndex=sequenceLink->nextIndex;

		if(seqLinkIndex==LINK_INDEX_DUMMY)
			{
			if(sequencePos>0)
				LOG(LOG_CRITICAL,"No next sequence link with position overflow");
			}

		sequenceLink=mbSingleBrickFindByIndex(sequencePile, seqLinkIndex);
		sequenceLen=sequenceLink->length;
		sequencePos=sequenceLink->position;
		}

	lookupPosition+=sequencePos; // Add partial
	sequencePos+=smerCount; // Possible onto next seqLink

//	LOG(LOG_INFO,"UpdatingMulti %i of %i",sequencePos, sequenceLen);

	while(sequencePos>=sequenceLen)
		{
		sequenceLink->position=sequenceLen;
//		LOG(LOG_INFO,"UpdateA to %i of %i",sequenceLink->position, sequenceLink->length);

		sequencePos-=sequenceLen;

		u32 seqLinkIndex=sequenceLink->nextIndex;

		if(seqLinkIndex==LINK_INDEX_DUMMY)
			{
			if(sequencePos>0)
				LOG(LOG_CRITICAL,"No next sequence link with position overflow");

			*seqLinkUnfinished=NULL;
			return lookupPosition;
			}

		sequenceLink=mbSingleBrickFindByIndex(sequencePile, seqLinkIndex);
		sequenceLen=sequenceLink->length;
//		if(sequenceLink->position>0)
//			LOG(LOG_CRITICAL,"Consuming into pre-consumed seqLink");
		}

	sequenceLink->position=sequencePos;

//	LOG(LOG_INFO,"UpdateB to %i of %i",sequenceLink->position, sequenceLink->length);

	*seqLinkUnfinished=sequenceLink;

	return lookupPosition;
}

static s32 getLastDispatchPosition(DispatchLink *dispatchLink)
{
	s32 total=0;
	for(int i=0;i<dispatchLink->length;i++)
		total+=dispatchLink->smers[i].seqIndexOffset;

	return total;
}


static void processPredispatchLookupCompleteReadLookupBlocks(RoutingBuilder *rb, MemDoubleBrickAllocator *dispatchLinkAlloc, RoutingReadLookupBlock *lookupReadBlock)
{
	MemSingleBrickPile *sequencePile=&(rb->sequenceLinkPile);
	MemDoubleBrickPile *lookupPile=&(rb->lookupLinkPile);
	MemDoubleBrickPile *dispatchPile=&(rb->dispatchLinkPile);

	extractLookupPercolatesFromEntryLookups(lookupReadBlock->smerEntryLookups, lookupReadBlock->smerEntryLookupsPercolates);

	RoutingReadReferenceBlockDispatchArray *outboundDispatches=allocDispatchArray(lookupReadBlock->outboundDispatches);
	lookupReadBlock->outboundDispatches=outboundDispatches;

	RoutingReadLookupRecycleBlock *predispatchRecycleBlock=NULL;
	RoutingReadLookupRecycleBlock *postdispatchRecycleBlock=NULL;

	int reads=lookupReadBlock->readCount;

	for(int i=0;i<reads;i++)
		{
		u32 lookupLinkIndex=lookupReadBlock->lookupLinkIndex[i];

		LookupLink *lookupLink=mbDoubleBrickFindByIndex(lookupPile, lookupLinkIndex);

		lookupLink->smerCount=-(lookupLink->smerCount); // Swap sign to indicate completion

		DispatchLink *dispatchLink=NULL;
		u32 dispatchLinkIndex=LINK_INDEX_DUMMY;
		u32 sequenceLinkIndex=LINK_INDEX_DUMMY;

		s32 dispatchPosition=0;

		int firstFlag=0;

		if(lookupLink->indexType==LINK_INDEXTYPE_SEQ) // First lookup scenario: Lookup -> Seq* -> null
			{
			firstFlag=1;

			dispatchLink=
					mbDoubleBrickAllocate(dispatchLinkAlloc, &dispatchLinkIndex);

			dispatchLink->nextOrSourceIndex=lookupLink->sourceIndex;
			dispatchLink->indexType=LINK_INDEXTYPE_SEQ;
			dispatchLink->length=0;
			dispatchLink->position=0;
			dispatchLink->revComp=0;
			dispatchLink->minEdgePosition=TR_INIT_MINEDGEPOSITION;
			dispatchLink->maxEdgePosition=TR_INIT_MAXEDGEPOSITION;

			// dispatchLink->smers populated later

			dispatchPosition=0;

			sequenceLinkIndex=lookupLink->sourceIndex;

			lookupLink->sourceIndex=dispatchLinkIndex;
			lookupLink->indexType=LINK_INDEXTYPE_DISPATCH;
			}
		else if(lookupLink->indexType==LINK_INDEXTYPE_DISPATCH) // Recycled lookup scenario: Lookup -> Dispatch -> Seq* -> null
			{
			dispatchLinkIndex=lookupLink->sourceIndex;
			dispatchLink=mbDoubleBrickFindByIndex(dispatchPile, dispatchLinkIndex);

			dispatchPosition=getLastDispatchPosition(dispatchLink);

			sequenceLinkIndex=findSequenceLinkIndexForDispatchLink(rb, dispatchLink);
			}
		else
			LOG(LOG_CRITICAL,"Unable to handle lookup link with lookup sourceIndexType");

		SequenceLink *sequenceLink=mbSingleBrickFindByIndex(sequencePile, sequenceLinkIndex);

		// Get current position, update for lookup and get first link with unused smers in chain (could be NULL if at end)
		SequenceLink *seqLinkUnfinished=NULL;
		s32 lookupPosition=getAndUpdateSequenceLinkPosition(sequenceLink, sequencePile, lookupLink->smerCount, &seqLinkUnfinished);

		int lastFlag=lookupLink->revComp & LOOKUPLINK_EOS_FLAG;

		DispatchLinkSmer indexedSmers[LOOKUP_LINK_SMERS+2]; // Allow for possible seq start/end smers
		u8 indexedRevComp[LOOKUP_LINK_SMERS+2];

		s32 extractFlag=(firstFlag?EXTRACT_FIRST_SMER:0) | (lastFlag?EXTRACT_LAST_SMER:0);

		int foundCount=
				extractIndexedSmerDataFromLookupPercolates(lookupLink->smers, lookupLink->smerCount, lookupLink->revComp,
				indexedSmers, indexedRevComp,
				extractFlag, lookupReadBlock->smerEntryLookupsPercolates);
//		LOG(LOG_INFO,"Found %i - flags %i",foundCount, extractFlag);

		// Correct offset of first found smer, allowing for lookup position and existing dispatch smers

		int positionAdjust=lookupPosition-dispatchPosition;

		//LOG(LOG_INFO,"Adjust position by %i (%i %i)",positionAdjust, lookupPosition, dispatchPosition);

		indexedSmers[0].seqIndexOffset+=positionAdjust;

		// Copy the needful
		DispatchLink *lastDispatchLink=NULL;
		int dispatchCount=transferFoundSmersToDispatch(dispatchLink, dispatchLinkAlloc, foundCount, indexedSmers, indexedRevComp, &lastDispatchLink);

		//LOG(LOG_INFO,"Ignoring found smers");
		if(dispatchCount < DISPATCH_LINK_SMER_THRESHOLD) // If dispatch link less than threshold
			{
			if(!lastFlag)  // more to come - recycle to lookup
				{
//				LOG(LOG_INFO,"Recycle scenario");
//				trDumpSequenceLink(sequenceLink, sequenceLinkIndex);
//				trDumpDispatchLink(dispatchLink, dispatchLinkIndex);
//				trDumpLookupLink(lookupLink, lookupLinkIndex);

				// Populate lookup with first unfinished seqLink entries and recycle
				populateLookupLinkForNonDispatchRecycle(lookupLink, dispatchLinkIndex, seqLinkUnfinished, sequencePile);
				predispatchRecycleBlock=recycleLookupLink(rb, predispatchRecycleBlock, LOOKUP_RECYCLE_BLOCK_PREDISPATCH, lookupLinkIndex);

				//LOG(LOG_INFO,"Recycle scenario done");
				}
			else // Reached end - copy and dispatch anyway
				{
//				LOG(LOG_INFO,"Small dispatch scenario");

				mbDoubleBrickFreeByIndex(lookupPile, lookupLinkIndex);
				assignToDispatchArrayEntry(outboundDispatches, dispatchLinkIndex, dispatchLink);
				}
			}
		else if(dispatchCount <= DISPATCH_LINK_SMERS) // Can fit in one dispatch link, copy and dispatch
			{
			if(!lastFlag)
				{
//				LOG(LOG_INFO,"Single dispatch scenario with lookup");

				populateLookupLinkForDispatch(lookupLink,lookupLinkIndex, dispatchLink, sequenceLink, sequenceLinkIndex, sequencePile); // Need to use original seqLink
				postdispatchRecycleBlock=recycleLookupLink(rb, postdispatchRecycleBlock, LOOKUP_RECYCLE_BLOCK_POSTDISPATCH, lookupLinkIndex);

//				trDumpSequenceLink(sequenceLink, sequenceLinkIndex);
//				trDumpLookupLink(lookupLink, lookupLinkIndex);
//				trDumpDispatchLink(dispatchLink, dispatchLinkIndex);
				}
			else
				{
//				LOG(LOG_INFO,"Single dispatch scenario without lookup");

				mbDoubleBrickFreeByIndex(lookupPile, lookupLinkIndex);
				}

			assignToDispatchArrayEntry(outboundDispatches, dispatchLinkIndex, dispatchLink);
			}
		else // Using multiple dispatch links
			{
			if(!lastFlag)
				{
//				LOG(LOG_INFO,"Multiple dispatch scenario with lookup - %p %p", dispatchLink, lastDispatchLink);

//				trDumpSequenceLink(sequenceLink, sequenceLinkIndex);
//				trDumpLookupLink(lookupLink, lookupLinkIndex);
//				trDumpDispatchLink(dispatchLink, dispatchLinkIndex);

				populateLookupLinkForDispatch(lookupLink,lookupLinkIndex, lastDispatchLink, sequenceLink, sequenceLinkIndex, sequencePile); // Need to use original seqLink
				postdispatchRecycleBlock=recycleLookupLink(rb, postdispatchRecycleBlock, LOOKUP_RECYCLE_BLOCK_POSTDISPATCH, lookupLinkIndex);

//				trDumpSequenceLink(sequenceLink, sequenceLinkIndex);
//				trDumpLookupLink(lookupLink, lookupLinkIndex);
//				trDumpDispatchLink(dispatchLink, dispatchLinkIndex);

//				LOG(LOG_INFO,"Multiple dispatch scenario with lookup");
				}
			else
				{
//				LOG(LOG_INFO,"Multiple dispatch scenario without lookup - %p %p", dispatchLink, lastDispatchLink);

				mbDoubleBrickFreeByIndex(lookupPile, lookupLinkIndex);
				}

			assignToDispatchArrayEntry(outboundDispatches, dispatchLinkIndex, dispatchLink);
			}
		}

//	LOG(LOG_INFO,"Flushing");

	flushRecycleBlock(rb, predispatchRecycleBlock);
	flushRecycleBlock(rb, postdispatchRecycleBlock);

	queueDispatchArray(rb, outboundDispatches);

//	LOG(LOG_INFO,"processPredispatchLookupCompleteReadLookupBlocks done");
}


static void processPostdispatchLookupCompleteReadLookupBlocks(RoutingBuilder *rb,
		MemDoubleBrickAllocator *dispatchLinkAlloc, MemDoubleBrickAllocator *lookupLinkAlloc, RoutingReadLookupBlock *lookupReadBlock)
{
	MemSingleBrickPile *sequencePile=&(rb->sequenceLinkPile);
	MemDoubleBrickPile *lookupPile=&(rb->lookupLinkPile);
//	MemDoubleBrickPile *dispatchPile=&(rb->dispatchLinkPile);

	extractLookupPercolatesFromEntryLookups(lookupReadBlock->smerEntryLookups, lookupReadBlock->smerEntryLookupsPercolates);

	RoutingReadLookupRecycleBlock *postdispatchRecycleBlock=NULL;

	int reads=lookupReadBlock->readCount;

	for(int i=0;i<reads;i++)
		{
		u32 oldLookupLinkIndex=lookupReadBlock->lookupLinkIndex[i];

		LookupLink *oldLookupLink=mbDoubleBrickFindByIndex(lookupPile, oldLookupLinkIndex);

		int smerCount=-(oldLookupLink->smerCount); // Swap sign as now completed

		if(oldLookupLink->indexType==LINK_INDEXTYPE_DISPATCH)
			LOG(LOG_CRITICAL,"Unable to handle lookup link with dispatch sourceIndexType");
		else if(oldLookupLink->indexType==LINK_INDEXTYPE_LOOKUP)
			LOG(LOG_CRITICAL,"Unable to handle lookup link with lookup sourceIndexType");

		//lookupLink->sourceIndex=dispatchLinkIndex;
		//lookupLink->indexType=LINK_INDEXTYPE_DISPATCH;

		u32 sequenceLinkIndex=oldLookupLink->sourceIndex;
		SequenceLink *sequenceLink=mbSingleBrickFindByIndex(sequencePile, sequenceLinkIndex);

		// Get current position, update for lookup and get first link with unused smers in chain (could be NULL if at end)
		SequenceLink *seqLinkUnfinished=NULL;
		s32 lookupPosition=getAndUpdateSequenceLinkPosition(sequenceLink, sequencePile, smerCount, &seqLinkUnfinished);

		int lastFlag=oldLookupLink->revComp & LOOKUPLINK_EOS_FLAG;

		DispatchLinkSmer indexedSmers[LOOKUP_LINK_SMERS+2]; // Allow for possible seq start/end smers
		u8 indexedRevComp[LOOKUP_LINK_SMERS+2];

		s32 extractFlag=(lastFlag?EXTRACT_LAST_SMER:0); // Should never be 'first' lookup after dispatch

		int foundCount=
				extractIndexedSmerDataFromLookupPercolates(oldLookupLink->smers, smerCount, oldLookupLink->revComp,
				indexedSmers, indexedRevComp,
				extractFlag, lookupReadBlock->smerEntryLookupsPercolates);

		//LOG(LOG_INFO,"Found %i of %i (offset %i)", foundCount, smerCount, lookupPosition);

		//LOG(LOG_INFO,"Adjust position by %i",lookupPosition);

		indexedSmers[0].seqIndexOffset+=lookupPosition;

		if(foundCount>0)
			{
			u32 dispatchLinkIndex=LINK_INDEX_DUMMY;
			DispatchLink *dispatchLink=mbDoubleBrickAllocate(dispatchLinkAlloc, &dispatchLinkIndex);

			dispatchLink->nextOrSourceIndex=oldLookupLink->sourceIndex;
			dispatchLink->indexType=LINK_INDEXTYPE_SEQ;
			dispatchLink->length=2;
			dispatchLink->position=0;
			dispatchLink->revComp=0;
			dispatchLink->minEdgePosition=TR_INIT_MINEDGEPOSITION;
			dispatchLink->maxEdgePosition=TR_INIT_MINEDGEPOSITION;

			// First two dispatchLink -> smers populated during transferFromCompletedLookup. Remaining dispatchLink->smers populated below.
			dispatchLink->smers[0].smer=SMER_DUMMY;
			dispatchLink->smers[1].smer=SMER_DUMMY;

			DispatchLink *lastDispatchLink=NULL;
			transferFoundSmersToDispatch(dispatchLink, dispatchLinkAlloc, foundCount, indexedSmers, indexedRevComp, &lastDispatchLink);

			if(!lastFlag) // (Old Dispatch ->) Old Lookup -> Dispatch* -> ___Lookup___ -> Seq* -> null
				{
				u32 newLookupLinkIndex=LINK_INDEX_DUMMY;
				LookupLink *newLookupLink=mbDoubleBrickAllocate(lookupLinkAlloc, &newLookupLinkIndex);

				//LOG(LOG_INFO,"processPostdispatchLookupCompleteReadLookupBlocks - A");
				populateLookupLinkForDispatch(newLookupLink, newLookupLinkIndex, lastDispatchLink, 	sequenceLink, sequenceLinkIndex, sequencePile);
				}

//		trDumpSequenceLink(sequenceLink, sequenceLinkIndex);

//		LOG(LOG_INFO,"Linking");

			oldLookupLink->sourceIndex=dispatchLinkIndex;
			oldLookupLink->indexType=LINK_INDEXTYPE_DISPATCH;

			__atomic_thread_fence(__ATOMIC_SEQ_CST);
			__atomic_store_n(&(oldLookupLink->smerCount), smerCount, __ATOMIC_SEQ_CST);

			//LOG(LOG_INFO,"Kraken released");
			}
		else
			{
			//LOG(LOG_INFO,"processPostdispatchLookupCompleteReadLookupBlocks - B");

			populateLookupLinkSmers(oldLookupLink, sequenceLink, sequencePile);
			postdispatchRecycleBlock=recycleLookupLink(rb, postdispatchRecycleBlock, LOOKUP_RECYCLE_BLOCK_POSTDISPATCH, oldLookupLinkIndex);
			}

		}

	flushRecycleBlock(rb, postdispatchRecycleBlock);
	//LOG(LOG_CRITICAL,"TODO - processPostdispatchLookupCompleteReadLookupBlocks");
}



int scanForAndDispatchLookupCompleteReadLookupBlocks(RoutingBuilder *rb)
{
	//LOG(LOG_INFO,"Begin scanForAndDispatchLookupCompleteReadLookupBlocks");

	//Graph *graph=rb->graph;
	//s32 nodeSize=graph->config.nodeSize;

	int lookupReadBlockIndex=scanForCompleteReadLookupBlock(rb);

	if(lookupReadBlockIndex<0)
		{
//		LOG(LOG_INFO,"none complete");
		return 0;
		}

	MemDoubleBrickPile *dispatchPile=&(rb->dispatchLinkPile);

	if(!mbCheckDoubleBrickAvailability(dispatchPile, TR_ROUTING_DISPATCHES_PER_LOOKUP*TR_ROUTING_BLOCKSIZE))
		{
		LOG(LOG_INFO,"dispatch bricks");
		return 0;
		}

	MemDoubleBrickPile *lookupPile=&(rb->lookupLinkPile);

	if(!mbCheckDoubleBrickAvailability(lookupPile, TR_ROUTING_BLOCKSIZE))
		{
		LOG(LOG_INFO,"lookup bricks");
		return 0;
		}

	MemDoubleBrickAllocator dispatchLinkAlloc;
	if(!mbInitDoubleBrickAllocator(&dispatchLinkAlloc, dispatchPile, TR_ROUTING_DISPATCHES_PER_LOOKUP*TR_ROUTING_BLOCKSIZE))
		{
		LOG(LOG_INFO,"dispatch bricks");
		return 0;
		}

	MemDoubleBrickAllocator lookupLinkAlloc;
	if(!mbInitDoubleBrickAllocator(&lookupLinkAlloc, lookupPile, TR_ROUTING_BLOCKSIZE))
		{
		LOG(LOG_INFO,"lookup bricks");
		mbDoubleBrickAllocatorCleanup(&dispatchLinkAlloc);
		return 0;
		}

	RoutingReadLookupBlock *lookupReadBlock=dequeueCompleteReadLookupBlock(rb, lookupReadBlockIndex);

	if(lookupReadBlock==NULL)
		{
//		LOG(LOG_INFO,"none complete");
		mbDoubleBrickAllocatorCleanup(&lookupLinkAlloc);
		mbDoubleBrickAllocatorCleanup(&dispatchLinkAlloc);
		return 0;
		}

	//LOG(LOG_INFO,"scanForAndDispatchLookupCompleteReadLookupBlocks");

	if(lookupReadBlock->blockType==LOOKUP_RECYCLE_BLOCK_PREDISPATCH)
		processPredispatchLookupCompleteReadLookupBlocks(rb, &dispatchLinkAlloc, lookupReadBlock);
	else if(lookupReadBlock->blockType==LOOKUP_RECYCLE_BLOCK_POSTDISPATCH)
		processPostdispatchLookupCompleteReadLookupBlocks(rb, &dispatchLinkAlloc, &lookupLinkAlloc, lookupReadBlock);
	else
		LOG(LOG_CRITICAL,"Unknown lookup type %i", lookupReadBlock->blockType);


	mbDoubleBrickAllocatorCleanup(&lookupLinkAlloc);
	mbDoubleBrickAllocatorCleanup(&dispatchLinkAlloc);

	lookupReadBlock->outboundDispatches=cleanupRoutingDispatchArrays(lookupReadBlock->outboundDispatches);


	for(int i=0;i<SMER_LOOKUP_PERCOLATES;i++)
		lookupReadBlock->smerEntryLookupsPercolates[i]=NULL;

	for(int i=0;i<SMER_SLICES;i++)
		{
		lookupReadBlock->smerEntryLookups[i]->completionCountPtr=NULL;
		lookupReadBlock->smerEntryLookups[i]=NULL;
		}

	//dumpBigDispenser(lookupReadBlock->disp);
	dispenserReset(lookupReadBlock->disp);

	__atomic_thread_fence(__ATOMIC_SEQ_CST);

	unallocateReadLookupBlock(lookupReadBlock);
	unreserveReadLookupBlock(rb);

	//LOG(LOG_INFO,"scanForAndDispatchLookupCompleteReadLookupBlocks DONE");

	return 1;
}



static RoutingReadLookupRecycleBlock *queueLookupBlockFromRecycleList(RoutingBuilder *rb, RoutingReadLookupRecycleBlock *recycleList)
{
	MemDoubleBrickPile *lookupPile=&(rb->lookupLinkPile);

	RoutingReadLookupBlock *lookupBlock=allocateReadLookupBlock(rb);

	if(lookupBlock==NULL)
		LOG(LOG_CRITICAL,"Failed to allocate reserved lookup block");

	s32 lookupReadCount=0;

	MemDispenser *disp=lookupBlock->disp;

	for(int i=0;i<SMER_LOOKUP_PERCOLATES;i++)
		lookupBlock->smerEntryLookupsPercolates[i]=allocPercolateBlock(disp);

	for(int i=0;i<SMER_SLICES;i++)
		lookupBlock->smerEntryLookups[i]=allocEntryLookupBlock(disp);

	lookupBlock->blockType=recycleList->blockType;

	while(recycleList!=NULL && lookupReadCount<TR_ROUTING_BLOCKSIZE)
		{
		s32 recycleCount=recycleList->readCount;
		s32 recyclePosition=recycleList->readPosition;

		s32 linksToTransfer=MIN(recycleCount-recyclePosition, TR_ROUTING_BLOCKSIZE-lookupReadCount);

		for(int i=0;i<linksToTransfer;i++)
			{
			u32 lookupLinkIndex=recycleList->lookupLinkIndex[recyclePosition++];

			LookupLink *lookupLink=mbDoubleBrickFindByIndex(lookupPile, lookupLinkIndex);
			assignSmersToLookupPercolates(lookupLink->smers, -(lookupLink->smerCount), lookupBlock->smerEntryLookupsPercolates, disp);

			lookupBlock->lookupLinkIndex[lookupReadCount++]=lookupLinkIndex;
			}

		if(recyclePosition==recycleCount)
			{
			MemDispenser *recycleDisp=recycleList->disp;
			recycleList=recycleList->nextPtr;

			//dumpBigDispenser(recycleDisp);
			dispenserFree(recycleDisp);
			}
		else
			recycleList->readPosition=recyclePosition;

		}

	lookupBlock->readCount=lookupReadCount;


	assignPercolatesToEntryLookups(lookupBlock->smerEntryLookupsPercolates, lookupBlock->smerEntryLookups, disp);

	u64 usedSlices=0;
	int highestSlice=-1;

	for(int i=0;i<SMER_SLICES;i++)
		{
		RoutingSmerEntryLookup *lookupForSlice=lookupBlock->smerEntryLookups[i];

		if(lookupForSlice->entryCount>0)
			{
			lookupForSlice->completionCountPtr=&(lookupBlock->compStat);
			usedSlices++;
			highestSlice=i;
			}
		else
			lookupForSlice->completionCountPtr=NULL;
		}

	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	queueReadLookupBlock(lookupBlock, usedSlices);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);

	u64 usedSlices2=0;
	for(int i=0;i<=highestSlice;i++) // Careful now
		{
		RoutingSmerEntryLookup *lookupForSlice=lookupBlock->smerEntryLookups[i];
		if(lookupForSlice->entryCount>0)
			{
			queueLookupForSlice(rb, lookupForSlice, i);
			usedSlices2++;
			}
		}

	if(usedSlices!=usedSlices2)
		LOG(LOG_CRITICAL,"Count 1v2 %p mismatch %lu vs %lu",lookupBlock, usedSlices,usedSlices2);

	return recycleList;
}


static int scanForAndProcessLookupRecyclesByType(RoutingBuilder *rb, int force, int blockType)
{
	if(scanLookupRecyclePtr(rb, blockType)==NULL)
		return 0;

	int recycleCount=getLookupRecycleCount(rb, blockType);

	if(!force && recycleCount < TR_LOOKUP_RECYCLE_BLOCK_THRESHOLD_FORCE)
		return 0;

	u64 allocedRLB=__atomic_load_n(&(rb->allocatedReadLookupBlocks), __ATOMIC_SEQ_CST);

	if(allocedRLB>(TR_READBLOCK_LOOKUPS_INFLIGHT-TR_LOOKUP_RECYCLE_TIGHT_BLOCKMARGIN) && recycleCount<TR_LOOKUP_RECYCLE_BLOCK_THRESHOLD_TIGHT)
		return 0;

	if(!reserveReadLookupBlock(rb,TR_LOOKUP_RECYCLE_BLOCKMARGIN))
		return 0;

	RoutingReadLookupRecycleBlock *recycleList=dequeueLookupRecycleList(rb, blockType);

	if(recycleList==NULL)
		{
		unreserveReadLookupBlock(rb);
		return 0;
		}

	recycleList=queueLookupBlockFromRecycleList(rb, recycleList);

	while(recycleList!=NULL)
		{
		if(!reserveReadLookupBlock(rb,0))
			break;

		recycleList=queueLookupBlockFromRecycleList(rb, recycleList);
		}

	u64 postRLB=__atomic_load_n(&(rb->allocatedReadLookupBlocks), __ATOMIC_SEQ_CST);

	while(recycleList!=NULL)
		{
		LOG(LOG_INFO, "Additional lookup blocks needed (recyc blocks %i) - RLB %i -> %i",recycleCount, allocedRLB, postRLB);

		RoutingReadLookupRecycleBlock *recycleListNext=recycleList->nextPtr;
		queueLookupRecycleBlock(rb, recycleList);
		recycleList=recycleListNext;
		}

	return 1;
}

int scanForAndProcessLookupRecycles(RoutingBuilder *rb, int force)
{
	return scanForAndProcessLookupRecyclesByType(rb, force, LOOKUP_RECYCLE_BLOCK_PREDISPATCH)+
		scanForAndProcessLookupRecyclesByType(rb, force, LOOKUP_RECYCLE_BLOCK_POSTDISPATCH);
}


static int scanForSmerLookupsForSlices(RoutingBuilder *rb, int startSlice, int endSlice)
{
	//LOG(LOG_INFO,"Begin scanForSmerLookupsForSlices");

	int work=0;
	int i;
	//int completionCount=0;

	for(i=startSlice;i<endSlice;i++)
		{
		__atomic_thread_fence(__ATOMIC_SEQ_CST);
		RoutingSmerEntryLookup *lookupEntry=dequeueLookupForSliceList(rb, i);
		__atomic_thread_fence(__ATOMIC_SEQ_CST);

		if(lookupEntry!=NULL)
			{
			work++;
			SmerArraySlice *slice=rb->graph->smerArray.slice+i;

			RoutingSmerEntryLookup *lookupEntryScan=lookupEntry;

			while(lookupEntryScan!=NULL)
				{

				for(int j=0;j<lookupEntryScan->entryCount;j++)
					{
					int index=saFindSmerEntry(slice,lookupEntryScan->entries[j]);

//					if(index==-1)
//						LOG(LOG_INFO,"Failed to find smer entry: %lx in %i",lookupEntryScan->entries[j],i);
//					else
//						LOG(LOG_INFO,"Found smer entry: %lx in %i",lookupEntryScan->entries[j],i);

					lookupEntryScan->entries[j]=index;
					}


				u64 *completionCountPtr=__atomic_load_n(&(lookupEntryScan->completionCountPtr), __ATOMIC_SEQ_CST);
				lookupEntryScan=__atomic_load_n(&(lookupEntryScan->nextPtr), __ATOMIC_SEQ_CST);

//				u64 *completionCountPtr=lookupEntryScan->completionCountPtr;
				//lookupEntryScan=lookupEntryScan->nextPtr;

				__atomic_thread_fence(__ATOMIC_SEQ_CST);
				u64 compStat=__atomic_sub_fetch(completionCountPtr,1,__ATOMIC_SEQ_CST);

				if(compStat < INGRESS_BLOCK_STATUS_ACTIVE || compStat >= INGRESS_BLOCK_STATUS_LOCKED)
					LOG(LOG_CRITICAL,"Unexpected status %8x", compStat);

				__atomic_thread_fence(__ATOMIC_SEQ_CST);

//				if(__atomic_sub_fetch(lookupEntryScan->completionCountPtr,1,__ATOMIC_SEQ_CST)==0)
//					completionCount++;
				}
			}
		}

//	*completionCountPtr=completionCount;

	return work;
}




int scanForSmerLookups(RoutingBuilder *rb, u64 workToken, int workerNo, RoutingWorkerState *wState, int force)
{
	//int startPos=(workerNo*SMER_SLICE_PRIME)&SMER_SLICE_MASK;

//	int position=wState->lookupSliceCurrent;
	int work=0;
	//int completionCount=0;

	// SMER_SLICES 16384
	workToken<<=10; // Multiply by 1024
	int startSlice=workToken&SMER_SLICE_MASK;
	int endSlice=startSlice+1024;

	//LOG(LOG_INFO,"Worker %i SmerLookups %i %i",workerNo, startSlice, endSlice);
	work=scanForSmerLookupsForSlices(rb,startSlice,endSlice);

	//if(force && (work==0))
	//	work=scanForSmerLookupsForSlices(rb,0,SMER_SLICES);

	//if(work<TR_LOOKUP_MAX_WORK)
		//work+=scanForSmerLookupsForSlices(rb, 0, position,  &lastSlice);

	//wState->lookupSliceCurrent=lastSlice;

	return work;

}



