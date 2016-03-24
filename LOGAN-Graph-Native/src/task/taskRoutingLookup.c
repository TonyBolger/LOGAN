/*
 * taskRoutingLookup.c
 *
 *  Created on: Mar 3, 2016
 *      Author: tony
 */

#include "../common.h"


static RoutingLookupPercolate *allocPercolateBlock(MemDispenser *disp)
{

	RoutingLookupPercolate *block=dAllocCacheAligned(disp, sizeof(RoutingLookupPercolate));

	block->entryCount=0;
	block->entries=dAllocCacheAligned(disp, TR_LOOKUPS_PER_PERCOLATE_BLOCK*sizeof(SmerEntry));

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
	for(int i=0;i<smerCount;i++)
		{
		SmerId smer=smers[i];
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

		percolate->entryCount=0;
		}
}

static int extractIndexedSmerDataFromLookupPercolates(SmerId *allSmers, u8 *allCompFlags, int allSmerCount, SmerId *smers, u8 *compFlags, u32 *slices, u32 *indexes, RoutingLookupPercolate *smerLookupPercolates[])
{
	int indexCount=0;

	for(int i=0;i<allSmerCount;i++)
		{
		SmerId smer=allSmers[i];
		SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
		u64 hash=hashForSmer(smerEntry);
		u32 slice=sliceForSmer(smer,hash);
		u32 sliceGroup=slice>>SMER_LOOKUP_PERCOLATE_SHIFT;

		RoutingLookupPercolate *percolate=smerLookupPercolates[sliceGroup];
		u64 entryCount=percolate->entryCount;

		//Check percolate->entries[entryCount++]==slice ?
		entryCount++;

		if(percolate->entries[entryCount++]!=SMER_DUMMY)
			{
			*(smers-indexCount)=smer;
			*(compFlags-indexCount)=allCompFlags[i];
			*(slices-indexCount)=slice;
			*(indexes-indexCount)=i;
			indexCount++;
			}

		percolate->entryCount=entryCount;
		}

	return indexCount;
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

		current=rb->smerEntryLookupPtr[sliceNum];
		lookupForSlice->nextPtr=current;
		}
	while(!__sync_bool_compare_and_swap(rb->smerEntryLookupPtr+sliceNum,current,lookupForSlice));

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

		current=rb->smerEntryLookupPtr[sliceNum];

		if(current==NULL)
			return NULL;
		}
	while(!__sync_bool_compare_and_swap(rb->smerEntryLookupPtr+sliceNum,current,NULL));

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
			if(rb->readLookupBlocks[i].status==0)
				{
				if(__sync_bool_compare_and_swap(&(rb->readLookupBlocks[i].status),0,1))
					return rb->readLookupBlocks+i;
				}
			}
		}

	LOG(LOG_INFO,"Failed to queue lookup readblock, should never happen"); // Can actually happen, just stupidly unlikely
	exit(1);
}

static void queueReadLookupBlock(RoutingReadLookupBlock *readBlock)
{
	if(!__sync_bool_compare_and_swap(&(readBlock->status),1,2))
		{
		LOG(LOG_INFO,"Tried to queue lookup readblock in wrong state, should never happen");
		exit(1);
		}
}

static int scanForCompleteReadLookupBlock(RoutingBuilder *rb)
{
	RoutingReadLookupBlock *current;
	int i;

	for(i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
		{
		current=rb->readLookupBlocks+i;
		if(current->status == 2 && current->completionCount==0)
			return i;
		}
	return -1;
}

static RoutingReadLookupBlock *dequeueCompleteReadLookupBlock(RoutingBuilder *rb, int index)
{
	//LOG(LOG_INFO,"Try dequeue readblock");

	RoutingReadLookupBlock *current=rb->readLookupBlocks+index;

	if(current->status == 2 && current->completionCount==0)
		{
		if(__sync_bool_compare_and_swap(&(current->status),2,3))
			return current;
		}

	return NULL;
}

static void unallocateReadLookupBlock(RoutingReadLookupBlock *readBlock)
{
	if(!__sync_bool_compare_and_swap(&(readBlock->status),3,0))
		{
		LOG(LOG_INFO,"Tried to unreserve lookup readblock in wrong state, should never happen");
		exit(1);
		}
}


int reserveReadLookupBlock(RoutingBuilder *rb)
{
	if(rb->allocatedReadLookupBlocks<TR_READBLOCK_LOOKUPS_INFLIGHT)
		{
		rb->allocatedReadLookupBlocks++;
		return 1;
		}

	return 0;
}

static void unreserveReadLookupBlock(RoutingBuilder *rb)
{
	__sync_fetch_and_add(&(rb->allocatedReadLookupBlocks),-1);
}


void queueReadsForSmerLookup(SwqBuffer *rec, int ingressPosition, int ingressSize, int nodeSize, RoutingBuilder *rb)
{

	MemDispenser *disp=dispenserAlloc("RoutingLookup");

	RoutingReadLookupBlock *readBlock=allocateReadLookupBlock(rb);
	readBlock->disp=disp;

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
		readData->smers=dAllocQuadAligned(disp,length*sizeof(SmerId));
		readData->compFlags=dAlloc(disp,length*sizeof(u8));

		packSequence(currentRec->seq, readData->packedSeq, length);

		s32 maxValidIndex=(length-nodeSize);
		calculatePossibleSmersComp(readData->packedSeq, maxValidIndex, readData->smers, readData->compFlags);

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

	readBlock->completionCount=usedSlices;

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

	MemDispenser *disp=dispenserAlloc("RoutingDispatch");
	dispatchReadBlock->disp=disp;

	s32 lastIndex=lookupReadBlock->maxReadLength-nodeSize;
	s32 maxIndexes=lastIndex+1;
	u32 *indexes=(u32 *)alloca(maxIndexes*sizeof(u32))+lastIndex;
	SmerId *smers=(SmerId *)alloca(maxIndexes*sizeof(SmerId))+lastIndex;
	u8 *compFlags=(u8 *)alloca(maxIndexes*sizeof(u8))+lastIndex;
	u32 *slices=(u32 *)alloca(maxIndexes*sizeof(u32))+lastIndex;

	extractLookupPercolatesFromEntryLookups(lookupReadBlock->smerEntryLookups, lookupReadBlock->smerEntryLookupsPercolates);

	RoutingDispatchArray *dispArray=allocDispatchArray();

	RoutingReadLookupData *readLookup=lookupReadBlock->readData;
	RoutingReadDispatchData *readDispatch=dispatchReadBlock->readData;

	int reads=lookupReadBlock->readCount;

	for(int i=0;i<reads;i++)
		{
		int smerCount=readLookup->seqLength-nodeSize+1;

		int foundCount=extractIndexedSmerDataFromLookupPercolates(readLookup->smers, readLookup->compFlags, smerCount, smers, compFlags, slices, indexes, lookupReadBlock->smerEntryLookupsPercolates);

		if(foundCount==0)
			{
			LOG(LOG_INFO,"Failed to find any valid smers for read");
			exit(1);
			}

		int length=readLookup->seqLength;
		int packLength=PAD_2BITLENGTH_BYTE(length);

		readDispatch->packedSeq=dAllocQuadAligned(disp,packLength); // Consider extra padding on these allocs
		memcpy(readDispatch->packedSeq, readLookup->packedSeq, packLength);

		readDispatch->quality=dAlloc(disp,length+1);
		memcpy(readDispatch->quality, readLookup->quality, length+1);

		readDispatch->seqLength=length;
		int offset=foundCount-1;
		readDispatch->indexCount=offset;

		readDispatch->indexes=dAlloc(disp, foundCount*sizeof(u32));
		memcpy(readDispatch->indexes, indexes-offset, foundCount*sizeof(u32));

		readDispatch->smers=dAlloc(disp, foundCount*sizeof(SmerId));
		memcpy(readDispatch->smers, smers-offset, foundCount*sizeof(SmerId));

		readDispatch->compFlags=dAlloc(disp, foundCount*sizeof(u8));
		memcpy(readDispatch->compFlags, compFlags-offset, foundCount*sizeof(u8));

		readDispatch->completionCountPtr=&(dispatchReadBlock->completionCount);

		assignToDispatchArrayEntry(dispArray, readDispatch);

		readLookup++;
		readDispatch++;
		}

	dispatchReadBlock->completionCount=reads;
	dispatchReadBlock->dispatchArray=dispArray;
	//dispatchReadBlock->completionCount=0; // To allow immediate cleanup

	queueDispatchArray(rb, dispArray);

	queueReadDispatchBlock(dispatchReadBlock);

	dispenserFree(lookupReadBlock->disp);
	unallocateReadLookupBlock(lookupReadBlock);
	unreserveReadLookupBlock(rb);

	return 1;
}

static int scanForSmerLookupsForSlices(RoutingBuilder *rb, int startSlice, int endSlice)
{
	int work=0;

	for(int i=startSlice;i<endSlice;i++)
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
					lookupEntryScan->entries[j]=saFindSmerEntry(slice,lookupEntryScan->entries[j]);

				__sync_fetch_and_add(lookupEntryScan->completionCountPtr,-1);
				lookupEntryScan=lookupEntryScan->nextPtr;
				}
			}

		}

	return work>0;
}


int scanForSmerLookups(RoutingBuilder *rb, int workerNo)
{
	int startPos=(workerNo*SMER_SLICE_PRIME)&SMER_SLICE_MASK;

	int work=0;

	work=scanForSmerLookupsForSlices(rb,startPos,SMER_SLICES);
	work+=scanForSmerLookupsForSlices(rb, 0, startPos);

	return work;
}



