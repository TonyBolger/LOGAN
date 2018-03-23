/*
 * taskRoutingDispatch.c
 *
 *  Created on: Mar 3, 2016
 *      Author: tony
 */

#include "common.h"


static void initDispatchIntermediateBlock(RoutingDispatchLinkIndexBlock *block, MemDispenser *disp)
{
	block->entryCount=0;
	block->entries=dAllocCacheAligned(disp, TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK*sizeof(u32));
}

RoutingDispatchLinkIndexBlock *allocDispatchIntermediateBlock(MemDispenser *disp)
{
	RoutingDispatchLinkIndexBlock *block=dAlloc(disp, sizeof(RoutingDispatchLinkIndexBlock));
	initDispatchIntermediateBlock(block, disp);
	return block;
}

static void initDispatchIndexedIntermediateBlock(RoutingIndexedDispatchLinkIndexBlock *block, MemDispenser *disp)
{
	block->entryCount=0;
	block->linkIndexEntries=dAllocCacheAligned(disp, TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK*sizeof(u32));
	block->linkEntries=dAllocCacheAligned(disp, TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK*sizeof(DispatchLink *));

}

RoutingIndexedDispatchLinkIndexBlock *allocDispatchIndexedIntermediateBlock(MemDispenser *disp)
{
	RoutingIndexedDispatchLinkIndexBlock *block=dAlloc(disp, sizeof(RoutingIndexedDispatchLinkIndexBlock));
	initDispatchIndexedIntermediateBlock(block, disp);
	return block;
}


RoutingReadReferenceBlockDispatchArray *allocDispatchArray(RoutingReadReferenceBlockDispatchArray *nextPtr)
{
	MemDispenser *disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_DISPATCH_ARRAY, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE); // SMALL or MEDIUM

	RoutingReadReferenceBlockDispatchArray *array=dAlloc(disp, sizeof(RoutingReadReferenceBlockDispatchArray));

	array->nextPtr=nextPtr;
	array->disp=disp;
    array->completionCount=0;

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		{
		array->dispatches[i].nextPtr=NULL;
		array->dispatches[i].prevPtr=NULL;
        array->dispatches[i].completionCountPtr=&(array->completionCount);

		initDispatchIntermediateBlock(&(array->dispatches[i].data),disp);
		}

	return array;
}


static void expandIntermediateDispatchBlock(RoutingDispatchLinkIndexBlock *block, MemDispenser *disp)
{
	int oldSize=block->entryCount;
	int size=oldSize*2;

	int oldEntrySize=oldSize*sizeof(u32);

	//LOG(LOG_INFO,"DISP_ROUTING_DISPATCH_GROUPSTATE Alloc: %i",size*sizeof(u32));
	u32 *entries=dAllocCacheAligned(disp, size*sizeof(u32));
	memcpy(entries,block->entries,oldEntrySize);
	block->entries=entries;

}

static void assignReadDataToDispatchIntermediate(RoutingDispatchLinkIndexBlock *intermediate, u32 dispatchLinkIndex, MemDispenser *disp)
{
	u64 entryCount=intermediate->entryCount;
	if(entryCount>=TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK && !((entryCount & (entryCount - 1))))
		expandIntermediateDispatchBlock(intermediate, disp);

	intermediate->entries[entryCount]=dispatchLinkIndex;
	intermediate->entryCount++;
}


static void expandIndexedIntermediateDispatchBlock(RoutingIndexedDispatchLinkIndexBlock *block, MemDispenser *disp)
{
	int oldSize=block->entryCount;
	int size=oldSize*2;

	int oldlinkIndexSize=oldSize*sizeof(u32);
	u32 *linkIndexEntries=dAllocCacheAligned(disp, size*sizeof(u32));
	memcpy(linkIndexEntries,block->linkIndexEntries,oldlinkIndexSize);
	block->linkIndexEntries=linkIndexEntries;

	int oldlinkSize=oldSize*sizeof(DispatchLink *);
	DispatchLink **linkEntries=dAllocCacheAligned(disp, size*sizeof(DispatchLink  *));
	memcpy(linkEntries,block->linkEntries,oldlinkSize);
	block->linkEntries=linkEntries;
}


//static
void assignReadDataToDispatchIndexedIntermediate(RoutingIndexedDispatchLinkIndexBlock *intermediate, u32 dispatchLinkIndex, DispatchLink *dispatchLink, MemDispenser *disp)
{
	s32 entryCount=intermediate->entryCount;
	if(entryCount>=TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK && !((entryCount & (entryCount - 1))))
		{
		expandIndexedIntermediateDispatchBlock(intermediate, disp);
		}

	intermediate->linkIndexEntries[entryCount]=dispatchLinkIndex;
	intermediate->linkEntries[entryCount]=dispatchLink;
	intermediate->entryCount++;
}



void assignToDispatchArrayEntry(RoutingReadReferenceBlockDispatchArray *array, u32 dispatchLinkIndex, DispatchLink *dispatchLink)
{
	int index=dispatchLink->position+1;
	SmerId smer=dispatchLink->smers[index].smer;

	SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
	u64 hash=hashForSmer(smerEntry);
	u32 sliceNew=sliceForSmer(smer,hash);

	u32 slice=dispatchLink->smers[index].slice;
	u32 group=(slice >> SMER_DISPATCH_GROUP_SHIFT);

	if(sliceNew != slice)
		{
		LOG(LOG_CRITICAL,"SLICE MISMATCH: %08lx Slice %i %i Group %i %i",smer, sliceNew, slice, group, dispatchLink->length);
		}

	assignReadDataToDispatchIntermediate(&(array->dispatches[group].data), dispatchLinkIndex, array->disp);
}



static void initDispatchLinkQueue(RoutingSmerAssignedDispatchLinkQueue *queue, s32 sliceIndex, u32 boost, MemDispenser *disp)
{
	queue->sliceIndex=sliceIndex;
	queue->entryCount=0;
	queue->position=0;
	queue->boost=boost;
	queue->dispatchLinkIndexEntries=dAllocCacheAligned(disp, TR_DISPATCH_READS_PER_QUEUE_BLOCK*sizeof(u32));
}

RoutingSmerAssignedDispatchLinkQueue *allocDispatchLinkQueue(MemDispenser *disp,  s32 sliceIndex, u32 boost)
{
	RoutingSmerAssignedDispatchLinkQueue *queue=dAlloc(disp, sizeof(RoutingSmerAssignedDispatchLinkQueue));
	initDispatchLinkQueue(queue, sliceIndex, boost, disp);
	return queue;
}

static void expandDispatchLinkQueue(RoutingSmerAssignedDispatchLinkQueue *queue, MemDispenser *disp)
{
	int oldSize=queue->entryCount;
	int size=oldSize*2;

	int oldEntrySize=oldSize*sizeof(u32);

	//LOG(LOG_INFO,"DISP_ROUTING_DISPATCH_GROUPSTATE Alloc: %i",size*sizeof(u32));
	u32 *entries=dAllocCacheAligned(disp, size*sizeof(u32));
	memcpy(entries,queue->dispatchLinkIndexEntries,oldEntrySize);
	queue->dispatchLinkIndexEntries=entries;

}

static void assignDispatchLinkToQueue(RoutingSmerAssignedDispatchLinkQueue *queue, u32 dispatchLinkIndex, MemDispenser *disp)
{
	u64 entryCount=queue->entryCount;
	if(entryCount>=TR_DISPATCH_READS_PER_QUEUE_BLOCK && !((entryCount & (entryCount - 1))))
		expandDispatchLinkQueue(queue, disp);

	queue->dispatchLinkIndexEntries[entryCount]=dispatchLinkIndex;
	queue->entryCount++;
}


void initRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState)
{
	MemDispenser *disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_DISPATCH_GROUPSTATE, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);

	dispatchGroupState->status=0;
	dispatchGroupState->forceCount=0;
	dispatchGroupState->disp=disp;

	dispatchGroupState->swapDisp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_DISPATCH_GROUPSTATE, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);

	for(int j=0;j<SMER_DISPATCH_GROUP_SLICES;j++)
		initDispatchIntermediateBlock(dispatchGroupState->smerInboundDispatches+j, disp);

	dispatchGroupState->outboundDispatches=NULL;

	for(int j=0;j<SMER_DISPATCH_GROUP_SLICES;j++)
		dispatchGroupState->dispatchLinkQueue.smerQueueMap[j]=NULL;

}



RoutingReadReferenceBlockDispatchArray *cleanupRoutingDispatchArrays(RoutingReadReferenceBlockDispatchArray *in)
{
	RoutingReadReferenceBlockDispatchArray **scan=&in;

	while((*scan)!=NULL)
		{
		RoutingReadReferenceBlockDispatchArray *current=(*scan);

		int compCount=__atomic_load_n(&current->completionCount, __ATOMIC_SEQ_CST);
		if(compCount==0)
			{
			*scan=current->nextPtr;

			//dumpBigDispenser(current->disp);
			dispenserFree(current->disp);
			}
		else
			{
			//LOG(LOG_INFO,"DispatchArray %p has %i",current, compCount);
			scan=&(current->nextPtr);
			}
		}

	return in;
}


static void recycleDispatchLinkQueue(RoutingSliceAssignedDispatchLinkQueue *dispatchLinkQueue, MemDispenser *disp)
{
	// Use new dispenser to create new maps/smerAssignedDispatchQueue for each non-finished smerAssignedDispatchQueue, and transfer entries

	int recycleCount=0;

	for(int i=0;i<SMER_DISPATCH_GROUP_SLICES;i++)
		{
		IohHash *oldMap=dispatchLinkQueue->smerQueueMap[i];
		IohHash *newMap=NULL;

		if(oldMap!=NULL)
			{
			int sliceIndex=0;
			RoutingSmerAssignedDispatchLinkQueue *oldDispatchQueue=NULL;

			int nextIndex=iohGetNext(oldMap, 0, &sliceIndex, (void **)&oldDispatchQueue);
			while(nextIndex>0)
				{
				int position=oldDispatchQueue->position;
				int entryCount=oldDispatchQueue->entryCount;

				if(position<entryCount)
					{
					RoutingSmerAssignedDispatchLinkQueue *newDispatchQueue=allocDispatchLinkQueue(disp, oldDispatchQueue->sliceIndex, oldDispatchQueue->boost);

					recycleCount+=(entryCount-position);
					for(int j=position;j<entryCount;j++)
						assignDispatchLinkToQueue(newDispatchQueue, oldDispatchQueue->dispatchLinkIndexEntries[j], disp);

					if(newMap==NULL)
						newMap=iohInitMap(disp, 6);

					iohPut(newMap, newDispatchQueue->sliceIndex, newDispatchQueue);
					}

				nextIndex=iohGetNext(oldMap, nextIndex, &sliceIndex, (void **)&oldDispatchQueue);
				}
			}
		dispatchLinkQueue->smerQueueMap[i]=newMap;
		}

	//if(recycleCount>0)
//		LOG(LOG_INFO,"Recycled %i entries", recycleCount);
}

void recycleRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState)
{
	MemDispenser *disp=dispatchGroupState->disp;

	if(disp==NULL)
		{
		disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_DISPATCH_GROUPSTATE, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);
		dispatchGroupState->disp=disp;
		dispatchGroupState->swapDisp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_DISPATCH_GROUPSTATE, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);
		}
	else
		{
		recycleDispatchLinkQueue(&(dispatchGroupState->dispatchLinkQueue), dispatchGroupState->swapDisp);

		//dumpBigDispenser(disp);
		dispenserReset(disp);

		//if(dispatchGroupState->swapDisp->allocated > 1000000)
//			LOG(LOG_INFO,"Swap now %i", dispatchGroupState->swapDisp->allocated);

		dispatchGroupState->disp=dispatchGroupState->swapDisp;
		dispatchGroupState->swapDisp=disp;

		disp=dispatchGroupState->disp;
		}

	for(int j=0;j<SMER_DISPATCH_GROUP_SLICES;j++)
		initDispatchIntermediateBlock(dispatchGroupState->smerInboundDispatches+j, disp);

	dispatchGroupState->outboundDispatches=cleanupRoutingDispatchArrays(dispatchGroupState->outboundDispatches);
}


void freeRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState)
{
	if(dispatchGroupState->disp!=NULL)
		dispenserFree(dispatchGroupState->disp);

	if(dispatchGroupState->swapDisp!=NULL)
		dispenserFree(dispatchGroupState->swapDisp);


	dispatchGroupState->outboundDispatches=cleanupRoutingDispatchArrays(dispatchGroupState->outboundDispatches);

}


static void queueDispatchForGroup(RoutingBuilder *rb, RoutingReadReferenceBlockDispatch *dispatchForGroup, int groupNum)
{
	RoutingReadReferenceBlockDispatch *current=NULL;
	int loopCount=0;

	do
		{
		if(loopCount++>1000000)
			{
			LOG(LOG_CRITICAL,"Loop Stuck");
			}

		current= __atomic_load_n(rb->dispatchPtr+groupNum, __ATOMIC_SEQ_CST);
		__atomic_store_n(&(dispatchForGroup->nextPtr), current, __ATOMIC_SEQ_CST);
		}
	while(!__atomic_compare_exchange_n(rb->dispatchPtr+groupNum, &current, dispatchForGroup, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

}


static RoutingReadReferenceBlockDispatch *dequeueDispatchForGroupList(RoutingBuilder *rb, int groupNum)
{
	RoutingReadReferenceBlockDispatch *current=NULL;
	int loopCount=0;

	do
		{
		if(loopCount++>1000000)
			{
			LOG(LOG_CRITICAL,"Loop Stuck");
			}

		current= __atomic_load_n(rb->dispatchPtr+groupNum, __ATOMIC_SEQ_CST);

		if(current==NULL)
			return NULL;
		}
	while(!__atomic_compare_exchange_n(rb->dispatchPtr+groupNum, &current, NULL, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

	return current;
}

/*
int countDispatchReadsRemaining(RoutingBuilder *rb)
{
	int readsRemaining=0;

	for(int i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
	{
		int status=__atomic_load_n(&(rb->readDispatchBlocks[i].status), __ATOMIC_RELAXED);

		if(status==2)
			readsRemaining+=__atomic_load_n(&(rb->readDispatchBlocks[i].completionCount), __ATOMIC_RELAXED);
	}

	return readsRemaining;
}


int countNonEmptyDispatchGroups(RoutingBuilder *rb)
{
	int count=0;

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		{
		if(rb->dispatchPtr[i]!=NULL)
			count++;
		}

	return count;
}
*/

static int lockRoutingDispatchGroupState(RoutingBuilder *rb, int groupNum)
{
	u32 current=0;
	return __atomic_compare_exchange_n(&(rb->dispatchGroupState[groupNum].status), &current, 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static int unlockRoutingDispatchGroupState(RoutingBuilder *rb, int groupNum)
{
	u32 current=1;
	if(!__atomic_compare_exchange_n(&(rb->dispatchGroupState[groupNum].status), &current, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		{
		LOG(LOG_CRITICAL,"Tried to unlock DispatchState without lock, should never happen");
		}

	return 1;
}




/*
int reserveReadDispatchBlock(RoutingBuilder *rb)
{
	u64 allocated=__atomic_load_n(&(rb->allocatedReadDispatchBlocks), __ATOMIC_SEQ_CST);

	if(allocated>=TR_READBLOCK_DISPATCHES_INFLIGHT)
		return 0;

	while(!__atomic_compare_exchange_n(&(rb->allocatedReadDispatchBlocks),&allocated,allocated+1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		{
		allocated=rb->allocatedReadDispatchBlocks;

		if(allocated>=TR_READBLOCK_DISPATCHES_INFLIGHT)
			return 0;
		}

	return 1;
}


void unreserveReadDispatchBlock(RoutingBuilder *rb)
{
	__atomic_fetch_sub(&(rb->allocatedReadDispatchBlocks),1, __ATOMIC_RELEASE);
}
*/
/*

RoutingReadDispatchBlock *allocateReadDispatchBlock(RoutingBuilder *rb)
{
	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		int i;

		for(i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
			{
			u32 current=__atomic_load_n(&(rb->readDispatchBlocks[i].status), __ATOMIC_SEQ_CST);

			if(current==0)
				{
				if(__atomic_compare_exchange_n(&(rb->readDispatchBlocks[i].status),&current,1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
					return rb->readDispatchBlocks+i;
				}
			}
		}

	LOG(LOG_CRITICAL,"Failed to queue dispatch readblock, should never happen"); // Can actually happen, just stupidly unlikely
	return NULL;
}


void queueReadDispatchBlock(RoutingReadDispatchBlock *readBlock)
{
	u32 current=1;

	if(!__atomic_compare_exchange_n(&(readBlock->status),&current,2, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		{
		LOG(LOG_CRITICAL,"Tried to queue dispatch readblock in wrong state, should never happen");
		}
}


int scanForCompleteReadDispatchBlock(RoutingBuilder *rb)
{
	RoutingReadDispatchBlock *readBlock;
	int i;

	for(i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		{
		readBlock=rb->readDispatchBlocks+i;

		if(__atomic_load_n(&readBlock->status, __ATOMIC_SEQ_CST) == 2 && __atomic_load_n(&readBlock->completionCount, __ATOMIC_SEQ_CST) == 0)
			return i;

		}
	return -1;
}

RoutingReadDispatchBlock *dequeueCompleteReadDispatchBlock(RoutingBuilder *rb, int index)
{
	RoutingReadDispatchBlock *readBlock=rb->readDispatchBlocks+index;

	u32 current=__atomic_load_n(&readBlock->status, __ATOMIC_SEQ_CST);

	if(current == 2 && __atomic_load_n(&readBlock->completionCount, __ATOMIC_SEQ_CST)==0)
		{
		if(__atomic_compare_exchange_n(&(readBlock->status),&current, 3, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			return readBlock;
		}

	return NULL;
}

void unallocateReadDispatchBlock(RoutingReadDispatchBlock *readBlock)
{
	u32 current=3;

	if(!__atomic_compare_exchange_n(&(readBlock->status),&current, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		{
		LOG(LOG_CRITICAL,"Tried to unreserve dispatch readblock in wrong state, should never happen");
		}
}


*/



void queueDispatchArray(RoutingBuilder *rb, RoutingReadReferenceBlockDispatchArray *dispArray)
{
    int count=0;

    for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
    	if(dispArray->dispatches[i].data.entryCount>0)
    		count++;

    dispArray->completionCount=count;

    for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
    	{
    	if(dispArray->dispatches[i].data.entryCount>0)
    		{
    		queueDispatchForGroup(rb,dispArray->dispatches+i,i);
    		}
    	}
}

static RoutingReadReferenceBlockDispatch *buildPrevLinks(RoutingReadReferenceBlockDispatch *dispatchEntry)
{
	RoutingReadReferenceBlockDispatch *prev=NULL;
	int count=0;

	while(dispatchEntry!=NULL)
		{
		dispatchEntry->prevPtr=prev;
		prev=dispatchEntry;
		dispatchEntry=dispatchEntry->nextPtr;
		count++;
		}

//	LOG(LOG_INFO,"Count is %i",count);

	return prev;
}


static int assignReversedInboundDispatchesToSlices(MemDoubleBrickPile *dispatchPile,
		RoutingReadReferenceBlockDispatch *dispatches, RoutingDispatchGroupState *dispatchGroupState)
{
	RoutingDispatchLinkIndexBlock *smerInboundDispatches=dispatchGroupState->smerInboundDispatches;

	//LOG(LOG_INFO,"assignReversedInboundDispatchesToSlices");

	while(dispatches!=NULL)
		{
		RoutingReadReferenceBlockDispatch *nextDispatches=dispatches->prevPtr;
		__builtin_prefetch(nextDispatches,1,3);

		//for(int i=0;i<dispatches->data.entryCount;i++)
		//	__builtin_prefetch(dispatches->data.entries[i],0,3);

		for(int i=0;i<dispatches->data.entryCount;i++)
			{
			u32 dispatchLinkIndex=dispatches->data.entries[i];
			DispatchLink *dispatchLink=mbDoubleBrickFindByIndex(dispatchPile, dispatchLinkIndex);

			int index=dispatchLink->position+1;
			u32 slice=dispatchLink->smers[index].slice;

			u32 inboundIndex=slice & SMER_DISPATCH_GROUP_SLICEMASK;

			assignReadDataToDispatchIntermediate(smerInboundDispatches+inboundIndex, dispatchLinkIndex, dispatchGroupState->disp);
			}

		__atomic_fetch_sub(dispatches->completionCountPtr,1, __ATOMIC_RELEASE);

		dispatches=nextDispatches;
		}

	//LOG(LOG_INFO,"assignReversedInboundDispatchesToSlices done");

	return 0;
}


//static
int indexedReadReferenceBlockSorter(const void *a, const void *b)
{
	RoutingIndexedDispatchLinkIndexBlock **pa=(RoutingIndexedDispatchLinkIndexBlock **)a;
	RoutingIndexedDispatchLinkIndexBlock **pb=(RoutingIndexedDispatchLinkIndexBlock **)b;

	return (*pa)->sliceIndex-(*pb)->sliceIndex;
}


/*
static int indexDispatchesForSlice(RoutingReadReferenceBlock *smerInboundDispatches, int sliceSmerCount, MemDispenser *disp,
		RoutingIndexedReadReferenceBlock ***indexedDispatchesPtr)
{
	RoutingIndexedReadReferenceBlock **indexedDispatches=dAlloc(disp, sizeof(RoutingIndexedReadReferenceBlock *)*  sliceSmerCount);
	*indexedDispatchesPtr=indexedDispatches;

	RoutingIndexedReadReferenceBlock **smerDispatches=dAlloc(disp, sizeof(RoutingIndexedReadReferenceBlock *)*  sliceSmerCount);

	for(int j=0;j<sliceSmerCount;j++)
		smerDispatches[j]=NULL;

	int usedCount=0;


	for(int i=0;i<smerInboundDispatches->entryCount;i++)
	{
		RoutingReadData *readData=smerInboundDispatches->entries[i];
		int sliceIndex=readData->sliceIndexes[readData->indexCount];

		if(sliceIndex>=0 && sliceIndex<sliceSmerCount)
			{
			RoutingIndexedReadReferenceBlock *rdi=smerDispatches[sliceIndex];

			if(rdi==NULL)
				{
				rdi=allocDispatchIndexedIntermediateBlock(disp);
				rdi->sliceIndex=sliceIndex;

				smerDispatches[sliceIndex]=rdi;
				indexedDispatches[usedCount++]=rdi;
				}

			assignReadDataToDispatchIndexedIntermediate(rdi,readData,disp);
			}
		else
			{
			LOG(LOG_CRITICAL,"Got invalid slice index %i",sliceIndex);
			}
	}


	qsort(indexedDispatches, usedCount, sizeof(RoutingIndexedReadReferenceBlock *), indexedReadReferenceBlockSorter);

	return usedCount;
}
*/

/*
static int calcCapacityForIndexedEntries(int entryCount)
{
	if(entryCount<16)
		return 6;

	int clz=__builtin_clz(entryCount);
	int size=5+((32-clz)>>1);

	return size;
}
*/
/*
static int indexDispatchesForSlice(MemDoubleBrickPile *dispatchPile, RoutingDispatchLinkIndexBlock *smerInboundDispatches,
		int sliceSmerCount, MemDispenser *disp,
		RoutingIndexedDispatchLinkIndexBlock ***indexedDispatchesPtr)
{
	//for(int i=0;i<smerInboundDispatches->entryCount;i++)
	//	__builtin_prefetch(smerInboundDispatches->entries[i],0,3);

	IohHash *map=iohInitMap(disp, calcCapacityForIndexedEntries(smerInboundDispatches->entryCount));

	for(int i=0;i<smerInboundDispatches->entryCount;i++)
		{
		u32 dispatchLinkIndex=smerInboundDispatches->entries[i];
		DispatchLink *dispatchLink=mbDoubleBrickFindByIndex(dispatchPile, dispatchLinkIndex);

		int index=dispatchLink->position+1;
		int sliceIndex=dispatchLink->smers[index].sliceIndex;

		if(sliceIndex>=0 && sliceIndex<sliceSmerCount)
			{
			RoutingIndexedDispatchLinkIndexBlock *rdi=iohGet(map,sliceIndex);

			if(rdi==NULL)
				{
				rdi=allocDispatchIndexedIntermediateBlock(disp);
				rdi->sliceIndex=sliceIndex;

				iohPut(map,sliceIndex,rdi);
				}

			assignReadDataToDispatchIndexedIntermediate(rdi,dispatchLinkIndex,dispatchLink,disp);
			}
		else
			{
			LOG(LOG_CRITICAL,"Got invalid slice index %i",sliceIndex);
			}
	}

	RoutingIndexedDispatchLinkIndexBlock **indexedDispatches=(RoutingIndexedDispatchLinkIndexBlock **)iohGetAllValues(map);
	*indexedDispatchesPtr=indexedDispatches;

	qsort(indexedDispatches, map->entryCount, sizeof(RoutingIndexedDispatchLinkIndexBlock *), indexedReadReferenceBlockSorter);

	return map->entryCount;
}
*/

/*
void dumpPatches(RoutePatch *patches, int patchCount)
{
	for(int i=0;i<patchCount;i++)
		LOG(LOG_INFO,"Patch: P: %i S: %i #: %i",patches[i].prefixIndex, patches[i].suffixIndex, patches[i].rdiIndex);
}
*/

//static
void dumpDispatchLinkQueue(RoutingSliceAssignedDispatchLinkQueue *dispatchLinkQueue)
{
	LOG(LOG_INFO,"Dispatch Link Queue Dump");

	for(int i=0;i<SMER_DISPATCH_GROUP_SLICES;i++)
		{
		IohHash *map=dispatchLinkQueue->smerQueueMap[i];

		if(map!=NULL)
			LOG(LOG_INFO,"Slice %i: Map %p contains %i", i, map, map->entryCount);
//		else
//			LOG(LOG_INFO,"Slice %i: Map NULL",i);
		}

}


static void assignInboundDispatchesToLinkQueue(MemDoubleBrickPile *dispatchPile, RoutingDispatchLinkIndexBlock *smerInboundDispatches,
		int sliceSmerCount, MemDispenser *disp, RoutingSliceAssignedDispatchLinkQueue *dispatchLinkQueue, s32 sliceWithinGroupIndex)
{
	IohHash *map=dispatchLinkQueue->smerQueueMap[sliceWithinGroupIndex];

	if(map==NULL)
		{
//		LOG(LOG_INFO,"DISP_ROUTING_DISPATCH_GROUPSTATE Alloc Map");
		map=iohInitMap(disp, 8);
		dispatchLinkQueue->smerQueueMap[sliceWithinGroupIndex]=map;
		}

	for(int i=0;i<smerInboundDispatches->entryCount;i++)
		{
		u32 dispatchLinkIndex=smerInboundDispatches->entries[i];
		DispatchLink *dispatchLink=mbDoubleBrickFindByIndex(dispatchPile, dispatchLinkIndex);

		int index=dispatchLink->position+1;
		int sliceIndex=dispatchLink->smers[index].sliceIndex;

		RoutingSmerAssignedDispatchLinkQueue *dispatchQueue=iohGet(map, sliceIndex);

		if(dispatchQueue==NULL)
			{
			dispatchQueue=allocDispatchLinkQueue(disp, sliceIndex, DISPATCH_LINK_QUEUE_DEFAULT_BOOST);
			iohPut(map, sliceIndex, dispatchQueue);
			}

		assignDispatchLinkToQueue(dispatchQueue, dispatchLinkIndex, disp);
		}

}

static int countLinkQueueDispatchesForSlice(RoutingSliceAssignedDispatchLinkQueue *dispatchLinkQueue, int sliceWithinGroupIndex)
{
	IohHash *map=dispatchLinkQueue->smerQueueMap[sliceWithinGroupIndex];
	if(map==NULL)
		return 0;

	int totalCount=0;
	int sliceIndex=0;
	RoutingSmerAssignedDispatchLinkQueue *dispatchQueue=NULL;

	int nextIndex=iohGetNext(map, 0, &sliceIndex, (void **)&dispatchQueue);
	while(nextIndex>0)
		{
		totalCount+=dispatchQueue->entryCount;
		nextIndex=iohGetNext(map, nextIndex, &sliceIndex, (void **)&dispatchQueue);
		}

	return totalCount;
}


/*
static void shuffleProcessedDispatchLinkEntries(DispatchLink *dispatchLink)
{
	// Keep entries smer[length-2] and smer[length-1], and adjust offset

	int total=0;

	int firstToKeep=dispatchLink->length-2;

	for(int i=0;i<firstToKeep;i++)
		total+=dispatchLink->smers[i].seqIndexOffset;

	memcpy(dispatchLink->smers, dispatchLink->smers+firstToKeep, sizeof(DispatchLinkSmer)*2);

	dispatchLink->smers[0].seqIndexOffset+=total;
	dispatchLink->revComp>>=firstToKeep;
	dispatchLink->length=2;
	dispatchLink->position=0;
}
*/


static void mergeDispatchLinks(DispatchLink *oldDispatchLink, DispatchLink *newDispatchLink)
{
	int firstToKeep=oldDispatchLink->length-2;

	int total=0;
	for(int i=0;i<=firstToKeep;i++)
		total+=oldDispatchLink->smers[i].seqIndexOffset;

	memcpy(newDispatchLink->smers, oldDispatchLink->smers+firstToKeep, sizeof(DispatchLinkSmer)*2);
	newDispatchLink->smers[0].seqIndexOffset=total; // Combine offsets from old dispatch link

	newDispatchLink->smers[2].seqIndexOffset-=(total+newDispatchLink->smers[1].seqIndexOffset); // Correct new offsets

	newDispatchLink->revComp|=((oldDispatchLink->revComp)>>firstToKeep)&0x3;

	newDispatchLink->minEdgePosition=oldDispatchLink->minEdgePosition;
	newDispatchLink->maxEdgePosition=oldDispatchLink->maxEdgePosition;
}

static u32 findLookupLinkInDispatchChain(MemDoubleBrickPile *dispatchPile,  DispatchLink *dispatchLink)
{
	while(dispatchLink->indexType==LINK_INDEXTYPE_DISPATCH)
		{
		dispatchLink=mbDoubleBrickFindByIndex(dispatchPile, dispatchLink->nextOrSourceIndex);
		}

	if(dispatchLink->indexType==LINK_INDEXTYPE_LOOKUP)
		return dispatchLink->nextOrSourceIndex;

	return LINK_INDEX_DUMMY;
}

static int transferFromCompletedLookup(RoutingBuilder *rb,
		u32 *dispatchLinkIndexPtr, DispatchLink **dispatchLinkPtr, RoutingReadLookupRecycleBlock **postDispatchRecycleBlockPtr)
{
	MemDoubleBrickPile *lookupPile=&(rb->lookupLinkPile);
	MemDoubleBrickPile *dispatchPile=&(rb->dispatchLinkPile);

	u32 oldDispatchLinkIndex=*dispatchLinkIndexPtr;
	DispatchLink *oldDispatchLink=*dispatchLinkPtr;

	if(oldDispatchLink->indexType!=LINK_INDEXTYPE_LOOKUP)
		LOG(LOG_CRITICAL,"Expected lookup-type link - found %i", oldDispatchLink->indexType);

	u32 oldLookupLinkIndex=oldDispatchLink->nextOrSourceIndex;
	LookupLink *oldLookupLink=mbDoubleBrickFindByIndex(lookupPile, oldLookupLinkIndex);

	if(oldLookupLink->smerCount<0)
		{
		//LOG(LOG_INFO,"Lookup not completed %i",oldDispatchLinkIndex);
		//trDumpDispatchLink(oldDispatchLink, oldDispatchLinkIndex);
		return 0;
		}

	if(oldLookupLink->indexType==LINK_INDEXTYPE_SEQ)
		{
		MemSingleBrickPile *seqPile=&(rb->sequenceLinkPile);
		SequenceLink *seqLink=mbSingleBrickFindByIndex(seqPile, oldLookupLink->sourceIndex);
		trDumpSequenceLinkChain(seqPile, seqLink, oldLookupLink->sourceIndex);

		LOG(LOG_CRITICAL,"Unexpected SequenceLink type");
		}
	else if(oldLookupLink->indexType==LINK_INDEXTYPE_LOOKUP)  // Old Dispatch -> Old Lookup -> -> New Lookup -> Seq* -> null
		{
		LOG(LOG_CRITICAL,"Handle LookupLink type");
		}
	else				// Old Dispatch -> Old Lookup -> New Dispatch* -> New Lookup -> Seq* -> null
		{
		//LOG(LOG_INFO,"Handle DispatchLink type");

		u32 newDispatchLinkIndex=oldLookupLink->sourceIndex;
		DispatchLink *newDispatchLink=mbDoubleBrickFindByIndex(dispatchPile, newDispatchLinkIndex);

		//LOG(LOG_INFO,"Pre merge old");
		//trDumpDispatchLink(oldDispatchLink, oldDispatchLinkIndex);

		//LOG(LOG_INFO,"Pre merge new");
		//trDumpDispatchLink(newDispatchLink, newDispatchLinkIndex);

		mergeDispatchLinks(oldDispatchLink, newDispatchLink);

		//LOG(LOG_INFO,"Post merge new");
		//trDumpDispatchLink(newDispatchLink, newDispatchLinkIndex);

		*dispatchLinkIndexPtr=newDispatchLinkIndex;
		*dispatchLinkPtr=newDispatchLink;

		u32 newLookupLinkIndex=findLookupLinkInDispatchChain(dispatchPile,  newDispatchLink);

		if(newLookupLinkIndex!=LINK_INDEX_DUMMY)
			*postDispatchRecycleBlockPtr=recycleLookupLink(rb, *postDispatchRecycleBlockPtr, LOOKUP_RECYCLE_BLOCK_POSTDISPATCH, newLookupLinkIndex);
		//else
		//	LOG(LOG_CRITICAL,"Failed to find lookup link");

		//LOG(LOG_INFO,"Free double lookup");
		mbDoubleBrickFreeByIndex(lookupPile, oldLookupLinkIndex);

		//LOG(LOG_INFO,"Free double disp");
		mbDoubleBrickFreeByIndex(dispatchPile, oldDispatchLinkIndex);
		}

	//LOG(LOG_INFO,"Lookup completed - handled");

	return 1;
}


static int processSlice(RoutingBuilder *rb, RoutingSliceAssignedDispatchLinkQueue *dispatchLinkQueue,  SmerArraySlice *slice, u32 sliceWithinGroupIndex,
		u32 *orderedDispatches, RoutingReadLookupRecycleBlock **postDispatchRecycleBlockPtr, MemDispenser *stateDisp, MemDispenser *routingDisp, MemHeap *heap)
{
	//MemSingleBrickPile *sequencePile=&(rb->sequenceLinkPile);
	//MemDoubleBrickPile *lookupPile=&(rb->lookupLinkPile);
	MemDoubleBrickPile *dispatchPile=&(rb->dispatchLinkPile);

	IohHash *map=dispatchLinkQueue->smerQueueMap[sliceWithinGroupIndex];

	if(map==NULL)
		LOG(LOG_CRITICAL,"Process slice - map is NULL");

	//LOG(LOG_INFO,"Process slice - %p %i", map, map->entryCount);

	int sliceIndex=0;
	int dispatchOffset=0;

	RoutingSmerAssignedDispatchLinkQueue *dispatchQueue;

	int nextIndex=iohGetNext(map, 0, &sliceIndex, (void **)&dispatchQueue);
	while(nextIndex>0)
		{
//		LOG(LOG_INFO,"DispatchQueue: %i entries for slice %i",dispatchQueue->entryCount,dispatchQueue->sliceIndex);

		if(dispatchQueue->entryCount==0)
			LOG(LOG_CRITICAL,"DispatchQueue: %i entries for slice %i",dispatchQueue->entryCount,dispatchQueue->sliceIndex);

		if(dispatchQueue->position!=0)
			LOG(LOG_CRITICAL,"DispatchQueue: %i entries for slice %i with invalid position %i",dispatchQueue->entryCount,dispatchQueue->sliceIndex, dispatchQueue->position);

		RoutingIndexedDispatchLinkIndexBlock *indexBlock=allocDispatchIndexedIntermediateBlock(routingDisp);
		indexBlock->sliceIndex=sliceIndex;

		//int threshold=MIN(dispatchQueue->entryCount-dispatchQueue->position, DISPATCH_LINK_QUEUE_FORCE_THRESHOLD);

		for(int i=dispatchQueue->position;i<dispatchQueue->entryCount;i++)
			{
			u32 dispatchLinkIndex=dispatchQueue->dispatchLinkIndexEntries[i];
			DispatchLink *dispatchLink=mbDoubleBrickFindByIndex(dispatchPile, dispatchLinkIndex);

			if(dispatchLink->position==dispatchLink->length-2) // Not enough to work with
				{
				if(transferFromCompletedLookup(rb, &dispatchLinkIndex, &dispatchLink, postDispatchRecycleBlockPtr))
					{
					dispatchQueue->dispatchLinkIndexEntries[i]=dispatchLinkIndex;
					}
				else
					break;
				}

			//LOG(LOG_INFO,"Adding %i", dispatchLinkIndex);
			//trDumpDispatchLink(dispatchLink, dispatchLinkIndex);

			assignReadDataToDispatchIndexedIntermediate(indexBlock, dispatchLinkIndex, dispatchLink, routingDisp);

			//LOG(LOG_INFO,"Added %i", dispatchLinkIndex);
			}

		if(indexBlock->entryCount>0)
		//if((indexBlock->entryCount*dispatchQueue->boost)>=threshold)
			{
			//for(int i=0;i<indexBlock->entryCount;i++)
			//	LOG(LOG_INFO,"Entry is %i", indexBlock->linkIndexEntries[i]);

			if(indexBlock->entryCount>50000)
				LOG(LOG_INFO,"Dispatching %i of %i",indexBlock->entryCount, dispatchQueue->entryCount);

			dispatchQueue->boost=DISPATCH_LINK_QUEUE_DEFAULT_BOOST;
			dispatchQueue->position+=indexBlock->entryCount;

			u8 sliceTag=(u8)sliceWithinGroupIndex;

			rtRouteReadsForSmer(indexBlock, 0, indexBlock->entryCount, slice, orderedDispatches+dispatchOffset, routingDisp, heap, sliceTag);
			dispatchOffset+=indexBlock->entryCount;

			/*
			u32 entriesRemaining=indexBlock->entryCount;
			u32 indexBlockOffset=0;

			while(entriesRemaining>0)
				{
				//u32 entriesToRoute=MIN(entriesRemaining, 1000);

				rtRouteReadsForSmer(indexBlock, indexBlockOffset, entriesToRoute, slice, orderedDispatches+dispatchOffset, routingDisp, circHeap, sliceTag);

				indexBlockOffset+=entriesToRoute;
				entriesRemaining-=entriesToRoute;
				dispatchOffset+=entriesToRoute;
				}
				*/
			}
		else
			{
//			if(indexBlock->entryCount>0)
//				LOG(LOG_INFO,"Deferring with %i dispatchable (%i boost) below threshold %i",indexBlock->entryCount, dispatchQueue->boost, threshold);
			dispatchQueue->boost++;
			}
		/*
		else
			{
			if(indexBlock->entryCount>1)
				LOG(LOG_INFO,"Deferring with %i dispatchable",indexBlock->entryCount);
			}
*/

		/*
		if(dispatchQueue->position<dispatchQueue->entryCount)
			{
			LOG(LOG_INFO,"Recycling %i dispatchables", dispatchQueue->entryCount-dispatchQueue->position);
			}
*/

		nextIndex=iohGetNext(map, nextIndex, &sliceIndex, (void **)&dispatchQueue);
		}

	return dispatchOffset;



	/*
	for(int i=0;i<smerInboundDispatches->entryCount;i++)
		{
		u32 dispatchLinkIndex=smerInboundDispatches->entries[i];
		DispatchLink *dispatchLink=mbDoubleBrickFindByIndex(dispatchPile, dispatchLinkIndex);

		if(dispatchLink->length<3)
			{
			LOG(LOG_CRITICAL,"Insufficient smers in dispatchLink");
			}
		}

	if(smerInboundDispatches->entryCount>0)
		{
		RoutingIndexedDispatchLinkIndexBlock **indexedDispatches=NULL;

		int indexLength=indexDispatchesForSlice(dispatchPile, smerInboundDispatches, slice->smerCount, disp, &indexedDispatches);

		u8 sliceTag=(u8)sliceIndex; // In practice, cannot be bigger than SMER_DISPATCH_GROUP_SLICES (256)

		int dispatchOffset=0;

		for(int j=0;j<indexLength;j++)
			{
			int entryCount=rtRouteReadsForSmer(indexedDispatches[j], slice, orderedDispatches+dispatchOffset, routingDisp, circHeap, sliceTag);
			dispatchOffset+=entryCount;
			}
		}

		*/
}




/*
for(int j=0;j<smerInboundDispatches->entryCount;j++)
	{
	RoutingReadDispatchData *readData=smerInboundDispatches->entries[j];

	int index=readData->indexCount;

*/

	/*
	int index=readData->indexCount;

	SmerId prevFmer=readData->fsmers[index+1];
	SmerId currFmer=readData->fsmers[index];
	SmerId nextFmer=readData->fsmers[index-1];

	char bufferP[SMER_BASES+1]={0};
	char bufferC[SMER_BASES+1]={0};
	char bufferN[SMER_BASES+1]={0};

	unpackSmer(prevFmer, bufferP);
	unpackSmer(currFmer, bufferC);
	unpackSmer(nextFmer, bufferN);

	LOG(LOG_INFO,"%s %s %s",bufferP, bufferC, bufferN);
	*/


static void prepareGroupOutbound(RoutingDispatchGroupState *groupState)
{
	RoutingReadReferenceBlockDispatchArray *outboundDispatches=allocDispatchArray(groupState->outboundDispatches);
	groupState->outboundDispatches=outboundDispatches;
}



static void freeSequenceLinkChain(MemSingleBrickPile *sequencePile, u32 sequenceLinkIndex)
{
	while(sequenceLinkIndex!=LINK_INDEX_DUMMY)
		{
		SequenceLink *sequenceLink=mbSingleBrickFindByIndex(sequencePile, sequenceLinkIndex);
		u32 sequenceLinkNextIndex=sequenceLink->nextIndex;

		mbSingleBrickFreeByIndex(sequencePile, sequenceLinkIndex);
		sequenceLinkIndex=sequenceLinkNextIndex;
		}
}



static s32 calculateDispatchLinkChainOffsetAdjustment(DispatchLink *dispatchLink)
{
	s32 total=0;

	for(int i=0;i<(dispatchLink->length-2);i++)
		total+=dispatchLink->smers[i].seqIndexOffset;

	return total;
}

static int validateSequenceLinkConsumed(SequenceLink *sequenceLink, u32 sequenceLinkIndex, MemSingleBrickPile *sequencePile)
{
	int diff=sequenceLink->seqLength-sequenceLink->position;
	u32 nextIndex=sequenceLink->nextIndex;

	while(nextIndex!=LINK_INDEX_DUMMY)
		{
		sequenceLinkIndex=nextIndex;
		sequenceLink=mbSingleBrickFindByIndex(sequencePile, nextIndex);

		diff+=sequenceLink->seqLength-sequenceLink->position;
		nextIndex=sequenceLink->nextIndex;
		}

	if(diff!=SMER_BASES-1)
		{
		LOG(LOG_INFO,"Sequence not fully consumed: %i",diff);
		LOG(LOG_INFO,"SeqLink: %i Position %i of %i", sequenceLinkIndex, sequenceLink->position, sequenceLink->seqLength);
		return 0;
		}


	return 1;
}


static int gatherSliceOutbound(MemSingleBrickPile *sequencePile, MemDoubleBrickPile *lookupPile, MemDoubleBrickPile *dispatchPile,
		RoutingDispatchGroupState *groupState, int sliceNum, u32 *orderedDispatches, int sliceDispatches)
{
	int completedCount=0;

	RoutingReadReferenceBlockDispatchArray *dispatchArray=groupState->outboundDispatches;
	RoutingDispatchLinkIndexBlock *smerInboundDispatches=groupState->smerInboundDispatches+sliceNum;

	for(int i=0;i<sliceDispatches;i++)
		{
		u32 dispatchLinkIndex=orderedDispatches[i];

		DispatchLink *dispatchLink=mbDoubleBrickFindByIndex(dispatchPile, dispatchLinkIndex);
		int nextIndexCount=__atomic_add_fetch(&(dispatchLink->position),1, __ATOMIC_SEQ_CST); // Needed?

		if(nextIndexCount<(dispatchLink->length-2))
			assignToDispatchArrayEntry(dispatchArray, dispatchLinkIndex, dispatchLink);
		else
			{
			switch(dispatchLink->indexType)
				{
				case LINK_INDEXTYPE_SEQ: // Dispatch -> Seq* -> null - should be at end of sequence
					{
					if(!(dispatchLink->revComp & 0x80))
						LOG(LOG_CRITICAL,"End of sequence: No marker. DispatchLink RevComp %02x", dispatchLink->revComp);

					u32 sequenceLinkIndex=dispatchLink->nextOrSourceIndex;

					SequenceLink *sequenceLink=mbSingleBrickFindByIndex(sequencePile, dispatchLink->nextOrSourceIndex);

					if(!validateSequenceLinkConsumed(sequenceLink, dispatchLink->nextOrSourceIndex, sequencePile))
						LOG(LOG_CRITICAL,"Sequence not fully consumed");

					/*
					if(sequenceLink->nextIndex==LINK_INDEX_DUMMY)
						LOG(LOG_INFO,"Single seq tag: %i", sequenceLink->tagLength);
					else
						{
						s32 tagLength=sequenceLink->tagLength;
						u32 nextIndex=sequenceLink->nextIndex;

						while(nextIndex!=LINK_INDEX_DUMMY)
							{
							SequenceLink *nextSeqLink=mbSingleBrickFindByIndex(sequencePile, nextIndex);
							tagLength+=nextSeqLink->tagLength;
							nextIndex=nextSeqLink->nextIndex;
							}

						LOG(LOG_INFO,"Multi seq tag: %i", tagLength);
						}
*/

					freeSequenceLinkChain(sequencePile, sequenceLinkIndex); // Possibly more than one seqLink in chain

					mbDoubleBrickFreeByIndex(dispatchPile, dispatchLinkIndex);
					completedCount++;
					break;
					}

				case LINK_INDEXTYPE_LOOKUP: // Dispatch -> Lookup -> Seq* -> null - shuffle 'used' dispatch link and dispatch
					{
					//shuffleProcessedDispatchLinkEntries(dispatchLink);
					assignToDispatchArrayEntry(dispatchArray, dispatchLinkIndex, dispatchLink);

					//LookupLink *lookupLink=mbDoubleBrickFindByIndex(lookupPile, dispatchLink->nextOrSourceIndex);
					//LOG(LOG_CRITICAL,"TODO: Lookup type: SmerCount: %i",lookupLink->smerCount);
					break;
					}

				case LINK_INDEXTYPE_DISPATCH: // Dispatch -> Dispatch -> ... - dispatch from later dispatch link
					{
					u32 nextDispatchLinkIndex=dispatchLink->nextOrSourceIndex;
					DispatchLink *nextDispatchLink=mbDoubleBrickFindByIndex(dispatchPile, nextDispatchLinkIndex);

					s32 offsetAdjust=calculateDispatchLinkChainOffsetAdjustment(dispatchLink);
					nextDispatchLink->smers[0].seqIndexOffset+=offsetAdjust; // Should try to remove seqLinks here too

					nextDispatchLink->minEdgePosition=dispatchLink->minEdgePosition;
					nextDispatchLink->maxEdgePosition=dispatchLink->maxEdgePosition;

					mbDoubleBrickFreeByIndex(dispatchPile, dispatchLinkIndex);

					assignToDispatchArrayEntry(dispatchArray, nextDispatchLinkIndex, nextDispatchLink);

					break;
					}

				default:
					LOG(LOG_CRITICAL,"Unexpected type %i", dispatchLink->indexType);
				}

			}

		/*
		if(nextIndexCount==(readData->length-1)) // Past last Smer - read is done
			{
			__atomic_fetch_sub(readData->completionCountPtr,1, __ATOMIC_SEQ_CST);
			}
		else if(nextIndexCount>0)
			{
			assignToDispatchArrayEntry(dispatchArray, readData);
			}
		else // Wrapped
			{
			LOG(LOG_CRITICAL,"Wrapped smer Index %i",nextIndexCount);
			}
		*/
		}


	smerInboundDispatches->entryCount=0;

	return completedCount;
}



static int scanForDispatchesForGroups(RoutingBuilder *rb, int startGroup, int endGroup, int force, int workerNo, int *lastGroupPtr)
{
	int work=0;
	int i;

	//LOG(LOG_INFO,"scanForDispatchesForGroups");

	MemSingleBrickPile *sequencePile=&(rb->sequenceLinkPile);
	MemDoubleBrickPile *lookupPile=&(rb->lookupLinkPile);
	MemDoubleBrickPile *dispatchPile=&(rb->dispatchLinkPile);

	MemDispenser *routingDisp=NULL;

	RoutingReadLookupRecycleBlock *postdispatchRecycleBlock=NULL;

	for(i=startGroup;i<endGroup;i++)
		{
		RoutingReadReferenceBlockDispatch *tmpPtr=__atomic_load_n(rb->dispatchPtr+i, __ATOMIC_SEQ_CST);

		if(force || tmpPtr != NULL)
			{
			if(lockRoutingDispatchGroupState(rb, i))
				{
				int completedCount=0;

				RoutingDispatchGroupState *groupState=rb->dispatchGroupState+i;

				RoutingReadReferenceBlockDispatch *dispatchEntry=dequeueDispatchForGroupList(rb, i);

				if(routingDisp==NULL)
					routingDisp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING, DISPENSER_BLOCKSIZE_LARGE, DISPENSER_BLOCKSIZE_HUGE); // MEDIUM or LARGE

				if(dispatchEntry!=NULL)
					{
					RoutingReadReferenceBlockDispatch *reversed=buildPrevLinks(dispatchEntry);
					assignReversedInboundDispatchesToSlices(dispatchPile, reversed, groupState);
					}

				// Currently only processing with new incoming or force mode
				// Longer term think about processing only with busy slices or force mode

				//LOG(LOG_INFO,"Begin group %i",i);

				prepareGroupOutbound(groupState);

				MemHeap *heap=rb->graph->smerArray.heaps[i];

				SmerArraySlice *baseSlice=rb->graph->smerArray.slice+(i << SMER_DISPATCH_GROUP_SHIFT);
				for(int j=0;j<SMER_DISPATCH_GROUP_SLICES;j++)
					{
					RoutingDispatchLinkIndexBlock *smerInboundDispatches=groupState->smerInboundDispatches+j;
					int inboundEntryCount=smerInboundDispatches->entryCount;
					SmerArraySlice *slice=baseSlice+j;

					if(inboundEntryCount>0)
						{
						assignInboundDispatchesToLinkQueue(dispatchPile, smerInboundDispatches, inboundEntryCount,
								groupState->disp, &(groupState->dispatchLinkQueue), j);
						}

					int sliceDispatches=countLinkQueueDispatchesForSlice(&(groupState->dispatchLinkQueue), j);

					//dumpDispatchLinkQueue(&(groupState->dispatchLinkQueue));
					//int sliceNumber=j+(i << SMER_DISPATCH_GROUP_SHIFT);
					//LOG(LOG_INFO,"Processing slice %i",sliceNumber);

					if(sliceDispatches>0)
						{
						//LOG(LOG_INFO,"DISP_ROUTING_DISPATCH_GROUPSTATE Alloc: %i",  sizeof(u32)*sliceDispatches);
						u32 *orderedDispatches=dAlloc(groupState->disp, sizeof(u32)*sliceDispatches);
						int dispatched=processSlice(rb, &(groupState->dispatchLinkQueue), slice, j, orderedDispatches, &postdispatchRecycleBlock,
								groupState->disp, routingDisp, heap);

						if(dispatched>0)
							work++;

//						LOG(LOG_INFO,"Dispatched %i",dispatched);

//						dumpBigDispenser(routingDisp);
						dispenserReset(routingDisp);
						completedCount+=gatherSliceOutbound(sequencePile, lookupPile, dispatchPile, groupState, j, orderedDispatches, dispatched);
						}

					}

				queueDispatchArray(rb, groupState->outboundDispatches);
				recycleRoutingDispatchGroupState(groupState);
				unlockRoutingDispatchGroupState(rb,i);

				__atomic_fetch_sub(&rb->sequencesInFlight, completedCount, __ATOMIC_SEQ_CST);
				}
			}
		}

	flushRecycleBlock(rb, postdispatchRecycleBlock);

	if(routingDisp!=NULL)
		{
//		dumpBigDispenser(routingDisp);
		dispenserFree(routingDisp);
		}

	//LOG(LOG_INFO,"scanForDispatchesForGroups done");

	return work;
}



int scanForDispatches(RoutingBuilder *rb, u64 workToken, int workerNo, RoutingWorkerState *wState, int force)
{
//	LOG(LOG_INFO,"scanForDispatches enter %i",workerNo);

	//int position=wState->dispatchGroupCurrent;
	int work=0;
	int lastGroup=-1;

//	LOG(LOG_INFO,"Dispatch %i %i",wState->dispatchGroupStart, wState->dispatchGroupEnd);
/*
	if(force)
		{
		work=scanForDispatchesForGroups(rb,wState->dispatchGroupStart, SMER_DISPATCH_GROUPS, force, workerNo, &lastGroup);
		work+=scanForDispatchesForGroups(rb, 0, wState->dispatchGroupEnd, force, workerNo, &lastGroup);
		}
	else*/

	// SMER_DISPATCH_GROUPS

	int startGroup=workToken&SMER_DISPATCH_GROUP_MASK;
	int endGroup=startGroup+1;

	//LOG(LOG_INFO,"Worker %i Dispatch %i %i",workerNo, startGroup, endGroup);
	work=scanForDispatchesForGroups(rb, startGroup, endGroup, force, workerNo, &lastGroup);


	//int dispatchGroupStart=wState->dispatchGroupCurrent;
	//int dispatchGroupEnd=dispatchGroupStart+1;
	//wState->dispatchGroupCurrent=(dispatchGroupEnd<wState->dispatchGroupEnd)?dispatchGroupEnd:wState->dispatchGroupStart;
	//work=scanForDispatchesForGroups(rb,dispatchGroupStart, dispatchGroupEnd, force, workerNo, &lastGroup);


	//if(work<TR_DISPATCH_MAX_WORK)
//		work+=scanForDispatchesForGroups(rb, 0, position, force, workerNo, &lastGroup);

//	wState->dispatchGroupCurrent=lastGroup;

//	LOG(LOG_INFO,"scanForDispatches exit %i",workerNo);

	return work;

}

