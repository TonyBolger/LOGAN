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

		if(sliceIndex!=SMER_DUMMY)
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

	return indexedData-indexedDataOrig;
}

//static
void dumpExtractedIndexedSmers(DispatchLinkSmer *indexedData, u8 *revCompFlag, s32 smerCount)
{
	LOG(LOG_INFO,"DumpExtractedIndexedSmers");

	for(int i=0;i<smerCount;i++)
		{
		char smerBuffer[SMER_BASES+1];
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
			u32 current=__atomic_load_n(&rb->readLookupBlocks[i].compStat.split.status, __ATOMIC_SEQ_CST);

			if(current==BLOCK_STATUS_IDLE)
				{
				if(__atomic_compare_exchange_n(&(rb->readLookupBlocks[i].compStat.split.status), &current, BLOCK_STATUS_ALLOCATED, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
					return rb->readLookupBlocks+i;
				}
			}
		}

	LOG(LOG_CRITICAL,"Failed to queue lookup readblock, should never happen"); // Can actually happen, just stupidly unlikely
	return NULL;
}



static void queueReadLookupBlock(RoutingReadLookupBlock *readBlock)
{
	u32 current=BLOCK_STATUS_ALLOCATED;
	if(!__atomic_compare_exchange_n(&(readBlock->compStat.split.status), &current, BLOCK_STATUS_ACTIVE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		{
		LOG(LOG_CRITICAL,"Tried to queue lookup readblock in wrong state, should never happen");
		}
}

static int scanForCompleteReadLookupBlock(RoutingBuilder *rb)
{
	RoutingReadLookupBlock *readBlock;
	int i;

	for(i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
		{
		readBlock=rb->readLookupBlocks+i;
		if(readBlock->compStat.split.status == BLOCK_STATUS_ACTIVE && readBlock->compStat.split.completionCount==0)
			return i;
		}
	return -1;
}

static RoutingReadLookupBlock *dequeueCompleteReadLookupBlock(RoutingBuilder *rb, int index)
{
	//LOG(LOG_INFO,"Try dequeue readblock");

	RoutingReadLookupBlock *readBlock=rb->readLookupBlocks+index;

	u64 current=__atomic_load_n(&readBlock->compStat.combined, __ATOMIC_SEQ_CST); // Combined access checks both count and status

	if(current == BLOCK_STATUS_ACTIVE)
		{
		if(__atomic_compare_exchange_n(&(readBlock->compStat.combined), &current, BLOCK_STATUS_COMPLETE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			return readBlock;
		}

	return NULL;
}


static void unallocateReadLookupBlock(RoutingReadLookupBlock *readBlock)
{
	u32 current=BLOCK_STATUS_COMPLETE;
	if(!__atomic_compare_exchange_n(&(readBlock->compStat.split.status), &current, BLOCK_STATUS_IDLE, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		{
		LOG(LOG_CRITICAL,"Tried to unreserve lookup readblock in wrong state, should never happen");
		}
}


int countLookupReadsRemaining(RoutingBuilder *rb)
{
	int readsRemaining=0;

	for(int i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
	{
		int status=__atomic_load_n(&(rb->readLookupBlocks[i].compStat.split.status), __ATOMIC_RELAXED);

		if(status==BLOCK_STATUS_ACTIVE)
			readsRemaining+=__atomic_load_n(&(rb->readLookupBlocks[i].compStat.split.completionCount), __ATOMIC_RELAXED);
	}

	return readsRemaining;
}



static int reserveReadLookupBlock(RoutingBuilder *rb, int reservationMargin)
{
	u64 current=__atomic_load_n(&(rb->allocatedReadLookupBlocks), __ATOMIC_SEQ_CST);

	if((reservationMargin + current)>=TR_READBLOCK_LOOKUPS_INFLIGHT)
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


void populateLookupLinkFromSequenceLink(LookupLink *lookupLink, u32 lookupLinkIndex, u32 sequenceLinkIndex, s32 firstFlag, MemSingleBrickPile *sequencePile)
{
	SequenceLink *sequenceLink=mbSingleBrickFindByIndex(sequencePile, sequenceLinkIndex);

	if(sequenceLink==NULL)
		LOG(LOG_CRITICAL,"Failed to find sequence link with ID %i",sequenceLinkIndex);

	int sequencePos=sequenceLink->position;
	int sequenceLen=sequenceLink->length;
	int sequenceSmers=sequenceLen-sequencePos;

	if((sequenceSmers<LOOKUP_LINK_SMERS) && sequenceLink->nextIndex!=LINK_INDEX_DUMMY)
		LOG(LOG_CRITICAL,"Need to handle chained sequences from %u",sequenceLinkIndex);

	int smerCount=MIN(LOOKUP_LINK_SMERS, sequenceLen-sequencePos);

	int sequencePackedPos=sequencePos>>2;
	int sequenceSkipSmers=sequencePos&0x3;

	SmerId smerBuf[LOOKUP_LINK_SMERS+3];
	u32 revCompBuf=0;

	calculatePossibleSmersAndOrientation(sequenceLink->packedSequence+sequencePackedPos, smerCount+sequenceSkipSmers, smerBuf, &revCompBuf);

	revCompBuf>>=sequenceSkipSmers;

	lookupLink->sourceIndex=sequenceLinkIndex;
	lookupLink->indexType=LINK_INDEXTYPE_SEQ;
	lookupLink->smerCount=smerCount;
	lookupLink->revComp=(u16)revCompBuf | (firstFlag?LOOKUPLINK_FIRSTFLAG:0);

	memcpy(lookupLink->smers, smerBuf+sequenceSkipSmers, smerCount*sizeof(SmerId));

//	trDumpSequenceLink(sequenceLink, sequenceLinkIndex);
//	trDumpLookupLink(lookupLink, lookupLinkIndex);
}


int queueSmerLookupsForIngress(RoutingBuilder *rb, RoutingReadIngressBlock *ingressBlock)
{
	if(!reserveReadLookupBlock(rb, TR_LOOKUP_INGRESS_BLOCKMARGIN))
		{
		//LOG(LOG_INFO,"No lookup blocks");
		return 0;
		}

	u32 sequences=ingressBlock->sequenceCount-ingressBlock->sequencePosition;
	if(sequences>TR_ROUTING_BLOCKSIZE)
		sequences=TR_ROUTING_BLOCKSIZE;

	MemSingleBrickPile *sequencePile=&(rb->sequenceLinkPile);
	MemDoubleBrickPile *lookupPile=&(rb->lookupLinkPile);

	if(!mbCheckDoubleBrickAvailability(lookupPile, sequences+TR_LOOKUP_INGRESS_PILEMARGIN))
		{
		//LOG(LOG_INFO,"No lookup bricks");
		unreserveReadLookupBlock(rb);
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

		populateLookupLinkFromSequenceLink(lookupLink, lookupBlock->lookupLinkIndex[blockIndex], sequenceLinkIndex, 1, sequencePile);

		assignSmersToLookupPercolates(lookupLink->smers, lookupLink->smerCount, lookupBlock->smerEntryLookupsPercolates, disp);

		sequencePosition++;
		blockIndex++;
		sequences--;
		}

	mbDoubleBrickAllocatorCleanup(&alloc);

	ingressBlock->sequencePosition=sequencePosition;
	lookupBlock->readCount=blockIndex;

	assignPercolatesToEntryLookups(lookupBlock->smerEntryLookupsPercolates, lookupBlock->smerEntryLookups, disp);

	int usedSlices=0;
	for(int i=0;i<SMER_SLICES;i++)
		{
		RoutingSmerEntryLookup *lookupForSlice=lookupBlock->smerEntryLookups[i];

		if(lookupForSlice->entryCount>0)
			{
			lookupForSlice->completionCountPtr=&(lookupBlock->compStat.split.completionCount);
			usedSlices++;
			}
		else
			lookupForSlice->completionCountPtr=NULL;
		}

	__atomic_store_n(&(lookupBlock->compStat.split.completionCount), usedSlices, __ATOMIC_SEQ_CST);

	for(int i=0;i<SMER_SLICES;i++)
		{
		RoutingSmerEntryLookup *lookupForSlice=lookupBlock->smerEntryLookups[i];
		if(lookupForSlice->entryCount>0)
			queueLookupForSlice(rb, lookupForSlice, i);
		}

	queueReadLookupBlock(lookupBlock);

//	LOG(LOG_INFO,"queueSmerLookupsForIngress DONE");

	return 0;
}






int scanForAndDispatchLookupCompleteReadLookupBlocks(RoutingBuilder *rb)
{
	//LOG(LOG_INFO,"Begin scanForAndDispatchLookupCompleteReadLookupBlocks");

	//Graph *graph=rb->graph;
	//s32 nodeSize=graph->config.nodeSize;

	int lookupReadBlockIndex=scanForCompleteReadLookupBlock(rb);

	if(lookupReadBlockIndex<0)
		return 0;

	if(!mbCheckDoubleBrickAvailability(&(rb->dispatchLinkPile), TR_ROUTING_DISPATCHES_PER_LOOKUP*TR_ROUTING_BLOCKSIZE))
		return 0;

	MemDoubleBrickAllocator alloc;
	if(!mbInitDoubleBrickAllocator(&alloc, &(rb->dispatchLinkPile), TR_ROUTING_DISPATCHES_PER_LOOKUP*TR_ROUTING_BLOCKSIZE))
		{
		return 0;
		}

	RoutingReadLookupBlock *lookupReadBlock=dequeueCompleteReadLookupBlock(rb, lookupReadBlockIndex);

	if(lookupReadBlock==NULL)
		{
		mbDoubleBrickAllocatorCleanup(&alloc);
		return 0;
		}

	LOG(LOG_INFO,"scanForAndDispatchLookupCompleteReadLookupBlocks");

	extractLookupPercolatesFromEntryLookups(lookupReadBlock->smerEntryLookups, lookupReadBlock->smerEntryLookupsPercolates);

	RoutingReadReferenceBlockDispatchArray *dispArray=allocDispatchArray();

	int reads=lookupReadBlock->readCount;

	for(int i=0;i<reads;i++)
		{
		u32 lookupLinkIndex=lookupReadBlock->lookupLinkIndex[i];

		LookupLink *lookupLink=mbDoubleBrickFindByIndex(&(rb->lookupLinkPile), lookupLinkIndex);

		switch(lookupLink->indexType) // FIXME
			{
			case LINK_INDEXTYPE_LOOKUP:
				LOG(LOG_CRITICAL,"Unable to handle lookup link with lookup sourceIndexType");
				break;

			case LINK_INDEXTYPE_DISPATCH:
				LOG(LOG_CRITICAL,"Unable to handle lookup link with dispatch sourceIndexType");
				break;
			}


		DispatchLinkSmer indexedSmers[LOOKUP_LINK_SMERS+2]; // Allow for possible seq start/end smers
		u8 indexedRevComp[LOOKUP_LINK_SMERS+2];

		s32 extractFlag=((lookupLink->revComp & LOOKUPLINK_FIRSTFLAG)?EXTRACT_FIRST_SMER:0)|
			((lookupLink->sourceIndex==LINK_INDEX_DUMMY)?EXTRACT_LAST_SMER:0);

		int foundCount=extractIndexedSmerDataFromLookupPercolates(lookupLink->smers, lookupLink->smerCount, lookupLink->revComp,
				indexedSmers, indexedRevComp,
				extractFlag, lookupReadBlock->smerEntryLookupsPercolates);
		LOG(LOG_INFO,"Found %i",foundCount);

		dumpExtractedIndexedSmers(indexedSmers, indexedRevComp, foundCount);

		/*

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

		readDispatch->minEdgePosition=TR_INIT_MINEDGEPOSITION;
		readDispatch->maxEdgePosition=TR_INIT_MAXEDGEPOSITION;

		readDispatch->completionCountPtr=&(dispatchReadBlock->completionCount);

		assignToDispatchArrayEntry(dispArray, readDispatch);

		readLookup++;
		readDispatch++;
		*/
		}

	mbDoubleBrickAllocatorCleanup(&alloc);

	queueDispatchArray(rb, dispArray);


	for(int i=0;i<lookupReadBlock->readCount;i++)
		{
		LookupLink *lookupLink=mbDoubleBrickFindByIndex(&(rb->lookupLinkPile), lookupReadBlock->lookupLinkIndex[i]);
		SequenceLink *seqLink=mbSingleBrickFindByIndex(&(rb->sequenceLinkPile), lookupLink->sourceIndex);

		trDumpSequenceLink(seqLink, lookupLink->sourceIndex);
		trDumpLookupLink(lookupLink, lookupReadBlock->lookupLinkIndex[i]);

		mbSingleBrickFreeByIndex(&(rb->sequenceLinkPile), lookupLink->sourceIndex);
		mbDoubleBrickFreeByIndex(&(rb->lookupLinkPile), lookupReadBlock->lookupLinkIndex[i]);
		}

	//LOG(LOG_INFO,"Disp free %p",lookupReadBlock->disp);
	dispenserReset(lookupReadBlock->disp);

	unallocateReadLookupBlock(lookupReadBlock);
	unreserveReadLookupBlock(rb);

	//LOG(LOG_INFO,"Freed lookup block");

	LOG(LOG_INFO,"scanForAndDispatchLookupCompleteReadLookupBlocks DONE");

	return 1;
}



static int scanForSmerLookupsForSlices(RoutingBuilder *rb, int startSlice, int endSlice)
{
	//LOG(LOG_INFO,"Begin scanForSmerLookupsForSlices");

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

//					if(index==-1)
//						LOG(LOG_INFO,"Failed to find smer entry: %lx in %i",lookupEntryScan->entries[j],i);
//					else
//						LOG(LOG_INFO,"Found smer entry: %lx in %i",lookupEntryScan->entries[j],i);

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

//	int position=wState->lookupSliceCurrent;
	int work=0;
	//int completionCount=0;

	work=scanForSmerLookupsForSlices(rb,wState->lookupSliceStart,wState->lookupSliceEnd);
	//if(work<TR_LOOKUP_MAX_WORK)
		//work+=scanForSmerLookupsForSlices(rb, 0, position,  &lastSlice);

	//wState->lookupSliceCurrent=lastSlice;

	return work;

}



