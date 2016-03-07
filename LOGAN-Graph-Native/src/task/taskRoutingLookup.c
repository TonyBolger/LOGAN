/*
 * taskRoutingLookup.c
 *
 *  Created on: Mar 3, 2016
 *      Author: tony
 */

#include "../common.h"


static RoutingLookupIntermediate *allocIntermediateBlock(MemDispenser *disp)
{

	RoutingLookupIntermediate *block=dAllocCacheAligned(disp, sizeof(RoutingLookupIntermediate));

	block->entryCount=0;
	block->entries=dAllocCacheAligned(disp, TR_LOOKUPS_PER_INTERMEDIATE_BLOCK*sizeof(SmerEntry));

	return block;
}


static void expandIntermediateBlock(RoutingLookupIntermediate *block, MemDispenser *disp)
{
	int oldSize=block->entryCount;
	int size=oldSize*2;

	int oldEntrySize=oldSize*sizeof(u32);

	SmerEntry *entries=dAllocCacheAligned(disp, size*sizeof(u32));
	memcpy(entries,block->entries,oldEntrySize);
	block->entries=entries;
	//block->presenceAbsence=NULL;//dAlloc(disp, PAD_BYTELENGTH_8BYTE(PAD_1BITLENGTH_BYTE(size)));
}



static void assignSmersToIntermediates(SmerId *smers, int smerCount, RoutingLookupIntermediate *smerIntermediates[], MemDispenser *disp)
{
	for(int i=0;i<smerCount;i++)
		{
		SmerId smer=smers[i];
		SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
		u64 hash=hashForSmer(smerEntry);
		u32 slice=sliceForSmer(smer,hash);
		u32 sliceGroup=slice>>SMER_LOOKUPSLICEGROUP_SHIFT;

		RoutingLookupIntermediate *intermediate=smerIntermediates[sliceGroup];
		u64 entryCount=intermediate->entryCount;

		if(entryCount>=TR_LOOKUPS_PER_INTERMEDIATE_BLOCK && !((entryCount & (entryCount - 1))))
			expandIntermediateBlock(intermediate, disp);

		intermediate->entries[entryCount++]=slice;
		intermediate->entries[entryCount]=smerEntry;
		intermediate->entryCount+=2;
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


static void assignIntermediatesToEntryLookups(RoutingLookupIntermediate *smerIntermediates[], RoutingSmerEntryLookup *smerEntryLookups[], MemDispenser *disp)
{
	for(int i=0;i<SMER_LOOKUPSLICEGROUPS;i++)
		{
		RoutingLookupIntermediate *intermediate=smerIntermediates[i];
		int intCount=0;

		//LOG(LOG_INFO,"Intermediate %i has %i",i, intermediate->entryCount);

		while(intCount<intermediate->entryCount)
			{
			u32 slice=intermediate->entries[intCount++];
			SmerEntry smerEntry=intermediate->entries[intCount++];

			RoutingSmerEntryLookup *lookupForSlice=smerEntryLookups[slice];
			u64 entryCount=lookupForSlice->entryCount;

			if(entryCount>=TR_LOOKUPS_PER_SLICE_BLOCK && !((entryCount & (entryCount - 1))))
				expandEntryLookupBlock(lookupForSlice, disp);

			lookupForSlice->entries[entryCount]=smerEntry;
			lookupForSlice->entryCount++;
			}

		}
}

static void extractIntermediatesFromEntryLookups(RoutingSmerEntryLookup *smerEntryLookups[], RoutingLookupIntermediate *smerIntermediates[])
{
	int entryCount[SMER_SLICES];

	memset(entryCount,0,sizeof(entryCount));

	for(int i=0;i<SMER_LOOKUPSLICEGROUPS;i++)
		{
		RoutingLookupIntermediate *intermediate=smerIntermediates[i];
		int intCount=0;

		//LOG(LOG_INFO,"Intermediate %i has %i",i, intermediate->entryCount);

		while(intCount<intermediate->entryCount)
			{
			u32 slice=intermediate->entries[intCount++];

			RoutingSmerEntryLookup *lookupForSlice=smerEntryLookups[slice];
			intermediate->entries[intCount++]=lookupForSlice->entries[entryCount[slice]++];
			}

		intermediate->entryCount=0;
		}
}

static int extractIndexedSmerDataFromIntermediates(SmerId *allSmers, u8 *allCompFlags, int allSmerCount, SmerId *smers, u8 *compFlags, u32 *slices, int *indexes, RoutingLookupIntermediate *smerIntermediates[])
{
	int indexCount=0;

	for(int i=0;i<allSmerCount;i++)
		{
		SmerId smer=allSmers[i];
		SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
		u64 hash=hashForSmer(smerEntry);
		u32 slice=sliceForSmer(smer,hash);
		u32 sliceGroup=slice>>SMER_LOOKUPSLICEGROUP_SHIFT;

		RoutingLookupIntermediate *intermediate=smerIntermediates[sliceGroup];
		u64 entryCount=intermediate->entryCount;

		//Check intermediate->entries[entryCount++]==slice ?
		entryCount++;

		if(intermediate->entries[entryCount++]!=SMER_DUMMY)
			{
			*(smers-indexCount)=smer;
			*(compFlags-indexCount)=allCompFlags[i];
			*(slices-indexCount)=slice;
			*(indexes-indexCount)=i;
			indexCount++;
			}

		intermediate->entryCount=entryCount;
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


static RoutingReadLookupBlock *reserveReadBlock(RoutingBuilder *rb)
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

	LOG(LOG_INFO,"Failed to queue readblock, should never happen"); // Can actually happen, just stupidly unlikely
	exit(1);
}

static void queueReadBlock(RoutingReadLookupBlock *readBlock)
{
	if(!__sync_bool_compare_and_swap(&(readBlock->status),1,2))
		{
		LOG(LOG_INFO,"Tried to queue readblock in wrong state, should never happen");
		exit(1);
		}
}


static RoutingReadLookupBlock *dequeueCompleteReadBlock(RoutingBuilder *rb)
{
	//LOG(LOG_INFO,"Try dequeue readblock");
	RoutingReadLookupBlock *current;
	int i;

	for(i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
		{
		current=rb->readLookupBlocks+i;
		if(current->status == 2 && current->completionCount==0)
			{
			if(__sync_bool_compare_and_swap(&(current->status),2,3))
				return current;
			}
		}
	return NULL;
}

static void unreserveReadBlock(RoutingReadLookupBlock *readBlock)
{
	if(!__sync_bool_compare_and_swap(&(readBlock->status),3,0))
		{
		LOG(LOG_INFO,"Tried to unreserve readblock in wrong state, should never happen");
		exit(1);
		}
}



void queueReadsForSmerLookup(SwqBuffer *rec, int ingressPosition, int ingressSize, int nodeSize, RoutingBuilder *rb)
{

	MemDispenser *disp=dispenserAlloc("RoutingLookup");

	RoutingReadLookupBlock *readBlock=reserveReadBlock(rb);
	readBlock->disp=disp;

	//int smerCount=0;
	int i=0;
	int ingressLast=ingressPosition+ingressSize;

	for(i=0;i<SMER_LOOKUPSLICEGROUPS;i++)
		readBlock->smerIntermediateLookups[i]=allocIntermediateBlock(disp);

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

		assignSmersToIntermediates(readData->smers, maxValidIndex+1, readBlock->smerIntermediateLookups, disp);

		currentRec++;
		readData++;
		}

	readBlock->readCount=ingressLast-ingressPosition;
	readBlock->maxReadLength=maxReadLength;

	assignIntermediatesToEntryLookups(readBlock->smerIntermediateLookups, readBlock->smerEntryLookups, disp);

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

	queueReadBlock(readBlock);
}





int scanForAndDispatchLookupCompleteReadBlocks(RoutingBuilder *rb)
{
	Graph *graph=rb->graph;
	s32 nodeSize=graph->config.nodeSize;

	RoutingReadLookupBlock *readBlock=dequeueCompleteReadBlock(rb);

	if(readBlock==NULL)
		return 0;

	s32 lastIndex=readBlock->maxReadLength-nodeSize;
	s32 maxIndexes=lastIndex+1;
	int *indexes=(int *)alloca(maxIndexes*sizeof(int))+lastIndex;
	SmerId *smers=(SmerId *)alloca(maxIndexes*sizeof(SmerId))+lastIndex;
	u8 *compFlags=(u8 *)alloca(maxIndexes*sizeof(u8))+lastIndex;
	u32 *slices=(u32 *)alloca(maxIndexes*sizeof(u32))+lastIndex;

	extractIntermediatesFromEntryLookups(readBlock->smerEntryLookups, readBlock->smerIntermediateLookups);

	RoutingReadLookupData *readData=readBlock->readData;

	int reads=readBlock->readCount;


	for(int i=0;i<reads;i++)
		{
		RoutingReadLookupData *read=readData+i;
		int smerCount=read->seqLength-nodeSize+1;

		int foundCount=extractIndexedSmerDataFromIntermediates(read->smers, read->compFlags, smerCount, smers, compFlags, slices, indexes, readBlock->smerIntermediateLookups);

		if(i==0)
			{
			LOG(LOG_INFO,"Read length %i found: %i",read->seqLength,foundCount);
			}
		}

	dispenserFree(readBlock->disp);

	unreserveReadBlock(readBlock);

	__sync_fetch_and_add(&(rb->allocatedReadLookupBlocks),-1);

	//LOG(LOG_INFO,"Complete readblock. Reserved: %i",count);

	return 1;
}

int scanForSmerLookups(RoutingBuilder *rb, int startSlice, int endSlice)
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


