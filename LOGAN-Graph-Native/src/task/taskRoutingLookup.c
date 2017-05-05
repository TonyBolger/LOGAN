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
	smerCount*=2;

	for(int i=0;i<smerCount;i+=2)
		{
		SmerId fsmer=smers[i];
		SmerId rsmer=smers[i+1];

		SmerId smer=CANONICAL_SMER(smer,rsmer);

		SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
		u64 hash=hashForSmer(smerEntry);
		u32 slice=sliceForSmer(smer,hash);
		u32 sliceGroup=slice>>SMER_LOOKUP_PERCOLATE_SHIFT;

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


static int extractIndexedSmerDataFromLookupPercolates(SmerId *allSmers, int allSmerCount, RoutingReadIndexedDataEntry *indexedData, RoutingLookupPercolate *smerLookupPercolates[])
{
	//int foundCount=0;

	RoutingReadIndexedDataEntry *indexedDataOrig=indexedData;

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

		u32 sliceTest=percolate->entries[percolate->entryPosition];
		if(sliceTest!=slice)
			{
			LOG(LOG_CRITICAL,"Mismatch between expected %i and found %i",sliceTest, slice);
			}

		u64 entryCount=percolate->entryPosition+1;
		int sliceIndex=percolate->entries[entryCount++];

		if(i==0)
			{
			indexedData->fsmer=fsmer;
			indexedData->rsmer=rsmer;
			indexedData->slice=slice;
			indexedData->sliceIndex=sliceIndex;
			indexedData->readIndex=i;
			indexedData--;
			}

		if(sliceIndex!=SMER_DUMMY)
			{
			indexedData->fsmer=fsmer;
			indexedData->rsmer=rsmer;
			indexedData->slice=slice;
			indexedData->sliceIndex=sliceIndex;
			indexedData->readIndex=i;
			indexedData--;
			}

		if(i==allSmerCount-1)
			{
			indexedData->fsmer=fsmer;
			indexedData->rsmer=rsmer;
			indexedData->slice=slice;
			indexedData->sliceIndex=sliceIndex;
			indexedData->readIndex=i;
			indexedData--;
			}

		percolate->entryPosition=entryCount;

		if(entryCount>percolate->entryCount)
			{
			LOG(LOG_INFO,"Beyond end of percolate %i %i",entryCount, percolate->entryCount);
			}
		}

	return indexedDataOrig-indexedData;
}



static void queueLookupForSlice(RoutingBuilder *rb, RoutingSmerEntryLookup *lookupForSlice, int sliceNum)
{
	RoutingSmerEntryLookup *current=NULL;
	int loopCount=0;

	do
		{
		if(loopCount++>1000000)
			{
			LOG(LOG_INFO,"Loop Stuck");
			exit(0);
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
			LOG(LOG_INFO,"Loop Stuck");
			exit(0);
			}

		current=__atomic_load_n(rb->smerEntryLookupPtr+sliceNum, __ATOMIC_SEQ_CST);

		if(current==NULL)
			return NULL;
		}
	while(!__atomic_compare_exchange_n(rb->smerEntryLookupPtr+sliceNum, &current, NULL, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

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
			u32 current=__atomic_load_n(&rb->readLookupBlocks[i].status, __ATOMIC_SEQ_CST);

			if(current==0)
				{
				if(__atomic_compare_exchange_n(&(rb->readLookupBlocks[i].status), &current, 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
					return rb->readLookupBlocks+i;
				}
			}
		}

	LOG(LOG_INFO,"Failed to queue lookup readblock, should never happen"); // Can actually happen, just stupidly unlikely
	exit(1);
}

static void queueReadLookupBlock(RoutingReadLookupBlock *readBlock)
{
	u32 current=1;
	if(!__atomic_compare_exchange_n(&(readBlock->status), &current, 2, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		{
		LOG(LOG_INFO,"Tried to queue lookup readblock in wrong state, should never happen");
		exit(1);
		}
}

static int scanForCompleteReadLookupBlock(RoutingBuilder *rb)
{
	RoutingReadLookupBlock *readBlock;
	int i;

	for(i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
		{
		readBlock=rb->readLookupBlocks+i;
		if(readBlock->status == 2 && readBlock->completionCount==0)
			return i;
		}
	return -1;
}

static RoutingReadLookupBlock *dequeueCompleteReadLookupBlock(RoutingBuilder *rb, int index)
{
	//LOG(LOG_INFO,"Try dequeue readblock");

	RoutingReadLookupBlock *readBlock=rb->readLookupBlocks+index;

	u32 current=__atomic_load_n(&readBlock->status, __ATOMIC_SEQ_CST);

	if(current == 2 && __atomic_load_n(&readBlock->completionCount, __ATOMIC_SEQ_CST) == 0)
		{
		if(__atomic_compare_exchange_n(&(readBlock->status), &current, 3, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			return readBlock;
		}

	return NULL;
}

static void unallocateReadLookupBlock(RoutingReadLookupBlock *readBlock)
{
	u32 current=3;
	if(!__atomic_compare_exchange_n(&(readBlock->status), &current, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		{
		LOG(LOG_INFO,"Tried to unreserve lookup readblock in wrong state, should never happen");
		exit(1);
		}
}


int reserveReadLookupBlock(RoutingBuilder *rb)
{
	u64 current=__atomic_load_n(&(rb->allocatedReadLookupBlocks), __ATOMIC_SEQ_CST);

	if(current>=TR_READBLOCK_LOOKUPS_INFLIGHT)
		return 0;

	u64 newVal=current+1;

	if(!__atomic_compare_exchange_n(&(rb->allocatedReadLookupBlocks), &current, newVal, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return 0;

	return 1;

}

static void unreserveReadLookupBlock(RoutingBuilder *rb)
{
	__atomic_fetch_sub(&(rb->allocatedReadLookupBlocks),1, __ATOMIC_SEQ_CST);
}


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
		readData->quality=dAlloc(disp,length+1);
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





int scanForAndDispatchLookupCompleteReadLookupBlocks(RoutingBuilder *rb)
{
	Graph *graph=rb->graph;
	s32 nodeSize=graph->config.nodeSize;

	int lookupReadBlockIndex=scanForCompleteReadLookupBlock(rb);

	if(lookupReadBlockIndex<0)
		return 0;

	if(!reserveReadDispatchBlock(rb))
		return 0;

	RoutingReadLookupBlock *lookupReadBlock=dequeueCompleteReadLookupBlock(rb, lookupReadBlockIndex);

	if(lookupReadBlock==NULL)
		{
		unreserveReadDispatchBlock(rb);
		return 0;
		}

	RoutingReadDispatchBlock *dispatchReadBlock=allocateReadDispatchBlock(rb);

	//MemDispenser *disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_DISPATCH, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_LARGE);
	//dispatchReadBlock->disp=disp;

	MemDispenser *disp=dispatchReadBlock->disp;

	s32 lastHit=lookupReadBlock->maxReadLength-nodeSize+2; // Allow for F & L
	s32 maxHits=lastHit+1;

	RoutingReadIndexedDataEntry *indexedDataEntries=((RoutingReadIndexedDataEntry *)alloca(maxHits*sizeof(RoutingReadIndexedDataEntry)))+lastHit;

	extractLookupPercolatesFromEntryLookups(lookupReadBlock->smerEntryLookups, lookupReadBlock->smerEntryLookupsPercolates);

	RoutingReadReferenceBlockDispatchArray *dispArray=allocDispatchArray();
	dispatchReadBlock->dispatchArray=dispArray;

	RoutingReadLookupData *readLookup=lookupReadBlock->readData;
	RoutingReadData **readDispatchPtrArray=dispatchReadBlock->readData;

	int reads=lookupReadBlock->readCount;

	dispatchReadBlock->readCount=reads;
	dispatchReadBlock->completionCount=reads;

	for(int i=0;i<reads;i++)
		{
		int smerCount=readLookup->seqLength-nodeSize+1;

		int foundCount=extractIndexedSmerDataFromLookupPercolates(readLookup->smers, smerCount, indexedDataEntries, lookupReadBlock->smerEntryLookupsPercolates);

		if(foundCount==2)
			{
			LOG(LOG_INFO,"Failed to find any valid smers for read [%i] with length %i and %i possible smers", i, readLookup->seqLength, smerCount);

			LOG(LOG_INFO,"Dispenser %p",lookupReadBlock->disp);

			LOG(LOG_INFO,"Comp count: %i", lookupReadBlock->completionCount);

			for(int j=0;j<smerCount;j++)
				LOG(LOG_INFO,"Smer: %012lx %12lx",readLookup->smers[j*2],readLookup->smers[j*2+1]);

			LOG(LOG_CRITICAL,"Bailing out");
			}

		RoutingReadData *readDispatch=dAlloc(disp, sizeof(RoutingReadData)+foundCount*sizeof(RoutingReadIndexedDataEntry));
		*readDispatchPtrArray=readDispatch;

//		int length=readLookup->seqLength;
//		int packLength=PAD_2BITLENGTH_BYTE(length);

//		readDispatch->packedSeq=dAllocQuadAligned(disp,packLength); // Consider extra padding on these allocs
//		memcpy(readDispatch->packedSeq, readLookup->packedSeq, packLength);

//		readDispatch->quality=dAlloc(disp,length+1);
//		memcpy(readDispatch->quality, readLookup->quality, length+1);

//		readDispatch->seqLength=length;

		int offset=foundCount-1;
		readDispatch->indexCount=foundCount-2;

		memcpy(readDispatch->indexedData, indexedDataEntries-offset, foundCount*sizeof(RoutingReadIndexedDataEntry));

		readDispatch->minEdgePosition=0;
		readDispatch->maxEdgePosition=(INT_MAX>>1);

		readDispatch->completionCountPtr=&(dispatchReadBlock->completionCount);

		assignToDispatchArrayEntry(dispArray, readDispatch);

		readLookup++;
		readDispatch++;
		}

	queueDispatchArray(rb, dispArray);

	queueReadDispatchBlock(dispatchReadBlock);

	//LOG(LOG_INFO,"Disp free %p",lookupReadBlock->disp);
	dispenserReset(lookupReadBlock->disp);
	unallocateReadLookupBlock(lookupReadBlock);
	unreserveReadLookupBlock(rb);

	return 1;
}

static int scanForSmerLookupsForSlices(RoutingBuilder *rb, int startSlice, int endSlice)
{
	int work=0;
	int i;
	//int completionCount=0;

	for(i=startSlice;i<endSlice;i++)
		{
		RoutingSmerEntryLookup *lookupEntry=dequeueLookupForSliceList(rb, i);

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

					//if(index==-1)
						//LOG(LOG_INFO,"Failed to find smer entry: %i",lookupEntryScan->entries[j]);

					lookupEntryScan->entries[j]=index;
					}

				__atomic_fetch_sub(lookupEntryScan->completionCountPtr,1,__ATOMIC_SEQ_CST);

//				if(__atomic_sub_fetch(lookupEntryScan->completionCountPtr,1,__ATOMIC_SEQ_CST)==0)
//					completionCount++;

				lookupEntryScan=lookupEntryScan->nextPtr;
				}
			}
		}

//	*completionCountPtr=completionCount;

	return work;
}



int scanForSmerLookups(RoutingBuilder *rb, int workerNo, RoutingWorkerState *wState)
{
	//int startPos=(workerNo*SMER_SLICE_PRIME)&SMER_SLICE_MASK;
/*
	int work=0;

	work=scanForSmerLookupsForSlices(rb,startPos,SMER_SLICES);
	work+=scanForSmerLookupsForSlices(rb, 0, startPos);
*/

//	int position=wState->lookupSliceCurrent;
	int work=0;
	//int completionCount=0;

	work=scanForSmerLookupsForSlices(rb,wState->lookupSliceStart,wState->lookupSliceEnd);
	//if(work<TR_LOOKUP_MAX_WORK)
		//work+=scanForSmerLookupsForSlices(rb, 0, position,  &lastSlice);

	//wState->lookupSliceCurrent=lastSlice;

	return work;
}



