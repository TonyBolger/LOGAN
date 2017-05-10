/*
 * taskRoutingDispatch.c
 *
 *  Created on: Mar 3, 2016
 *      Author: tony
 */

#include "common.h"


static void initDispatchIntermediateBlock(RoutingReadReferenceBlock *block, MemDispenser *disp)
{
	block->entryCount=0;
	block->entries=dAllocCacheAligned(disp, TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK*sizeof(RoutingReadData *));
}

RoutingReadReferenceBlock *allocDispatchIntermediateBlock(MemDispenser *disp)
{
	RoutingReadReferenceBlock *block=dAlloc(disp, sizeof(RoutingReadReferenceBlock));
	initDispatchIntermediateBlock(block, disp);
	return block;
}

static void initDispatchIndexedIntermediateBlock(RoutingIndexedReadReferenceBlock *block, MemDispenser *disp)
{
	block->entryCount=0;
	block->entries=dAllocCacheAligned(disp, TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK*sizeof(RoutingReadData *));

}

RoutingIndexedReadReferenceBlock *allocDispatchIndexedIntermediateBlock(MemDispenser *disp)
{
	RoutingIndexedReadReferenceBlock *block=dAlloc(disp, sizeof(RoutingIndexedReadReferenceBlock));
	initDispatchIndexedIntermediateBlock(block, disp);
	return block;
}


RoutingReadReferenceBlockDispatchArray *allocDispatchArray(RoutingReadReferenceBlockDispatchArray *nextPtr)
{
	MemDispenser *disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_DISPATCH_ARRAY, DISPENSER_BLOCKSIZE_SMALL, DISPENSER_BLOCKSIZE_LARGE);

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

static void expandIntermediateDispatchBlock(RoutingReadReferenceBlock *block, MemDispenser *disp)
{
	int oldSize=block->entryCount;
	int size=oldSize*2;

	int oldEntrySize=oldSize*sizeof(RoutingReadData *);

	RoutingReadData **entries=dAllocCacheAligned(disp, size*sizeof(RoutingReadData  *));
	memcpy(entries,block->entries,oldEntrySize);
	block->entries=entries;

}

static void assignReadDataToDispatchIntermediate(RoutingReadReferenceBlock *intermediate, RoutingReadData *readData, MemDispenser *disp)
{
	u64 entryCount=intermediate->entryCount;
	if(entryCount>=TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK && !((entryCount & (entryCount - 1))))
		expandIntermediateDispatchBlock(intermediate, disp);

	intermediate->entries[entryCount]=readData;
	intermediate->entryCount++;
}


static void expandIndexedIntermediateDispatchBlock(RoutingIndexedReadReferenceBlock *block, MemDispenser *disp)
{
	int oldSize=block->entryCount;
	int size=oldSize*2;

	int oldEntrySize=oldSize*sizeof(RoutingReadData *);

	RoutingReadData **entries=dAllocCacheAligned(disp, size*sizeof(RoutingReadData  *));
	memcpy(entries,block->entries,oldEntrySize);
	block->entries=entries;

}

static void assignReadDataToDispatchIndexedIntermediate(RoutingIndexedReadReferenceBlock *intermediate, RoutingReadData *readData, MemDispenser *disp)
{
	s32 entryCount=intermediate->entryCount;
	if(entryCount>=TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK && !((entryCount & (entryCount - 1))))
		{
		expandIndexedIntermediateDispatchBlock(intermediate, disp);
		}

	intermediate->entries[entryCount]=readData;
	intermediate->entryCount++;
}



void assignToDispatchArrayEntry(RoutingReadReferenceBlockDispatchArray *array, RoutingReadData *readData)
{
	SmerId fsmer=readData->indexedData[readData->indexCount].fsmer;
	SmerId rsmer=readData->indexedData[readData->indexCount].rsmer;

	SmerId smer=CANONICAL_SMER(fsmer,rsmer);

	SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
	u64 hash=hashForSmer(smerEntry);
	u32 sliceNew=sliceForSmer(smer,hash);

	u32 slice=readData->indexedData[readData->indexCount].slice;
	u32 group=(slice >> SMER_DISPATCH_GROUP_SHIFT);

	if(sliceNew != slice)
		{
		LOG(LOG_INFO,"SLICE MISMATCH: %08lx %08lx Slice %i %i Group %i %i",fsmer, rsmer, sliceNew, slice, group, readData->indexCount);
		exit(1);
		}

	assignReadDataToDispatchIntermediate(&(array->dispatches[group].data), readData, array->disp);
}



void initRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState)
{
	MemDispenser *disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_DISPATCH_GROUPSTATE, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);

	dispatchGroupState->status=0;
	dispatchGroupState->forceCount=0;
	dispatchGroupState->disp=disp;

	for(int j=0;j<SMER_DISPATCH_GROUP_SLICES;j++)
		initDispatchIntermediateBlock(dispatchGroupState->smerInboundDispatches+j, disp);

	dispatchGroupState->outboundDispatches=NULL;
}

/*
void initRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState)
{
	dispatchGroupState->status=0;
	dispatchGroupState->forceCount=0;
	dispatchGroupState->disp=NULL;

	for(int j=0;j<SMER_DISPATCH_GROUP_SLICES;j++)
		{
		dispatchGroupState->smerInboundDispatches[j].entryCount=0;
		dispatchGroupState->smerInboundDispatches[j].entries=NULL;
		}

	dispatchGroupState->outboundDispatches=NULL;
}
*/

static RoutingReadReferenceBlockDispatchArray *cleanupRoutingDispatchArrays(RoutingReadReferenceBlockDispatchArray *in)
{
	RoutingReadReferenceBlockDispatchArray **scan=&in;

	while((*scan)!=NULL)
		{
		RoutingReadReferenceBlockDispatchArray *current=(*scan);

		int compCount=__atomic_load_n(&current->completionCount, __ATOMIC_SEQ_CST);
		if(compCount==0)
			{
			*scan=current->nextPtr;

			dispenserFree(current->disp);
			}
		else
			scan=&(current->nextPtr);
		}


	return in;
}

void recycleRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState)
{
	MemDispenser *disp=dispatchGroupState->disp;

	if(disp==NULL)
		{
		disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_DISPATCH_GROUPSTATE, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);
		dispatchGroupState->disp=disp;
		}
	else
		{
		dispenserReset(disp);
		}

	for(int j=0;j<SMER_DISPATCH_GROUP_SLICES;j++)
		initDispatchIntermediateBlock(dispatchGroupState->smerInboundDispatches+j, disp);

	dispatchGroupState->outboundDispatches=cleanupRoutingDispatchArrays(dispatchGroupState->outboundDispatches);
}


void freeRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState)
{
	if(dispatchGroupState->disp!=NULL)
		{
		dispenserFree(dispatchGroupState->disp);
		}

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


int scanForCompleteReadDispatchBlocks(RoutingBuilder *rb)
{
//	Graph *graph=rb->graph;

	int dispatchReadBlockIndex=scanForCompleteReadDispatchBlock(rb);

	if(dispatchReadBlockIndex<0)
		return 0;

	RoutingReadDispatchBlock *dispatchReadBlock=dequeueCompleteReadDispatchBlock(rb, dispatchReadBlockIndex);

	if(dispatchReadBlock==NULL)
		return 1;

	if(dispatchReadBlock->dispatchArray!=NULL)
		{
		int dispCompletion=__atomic_load_n(&(dispatchReadBlock->dispatchArray->completionCount), __ATOMIC_SEQ_CST);

		if(dispCompletion!=0)
			{
			LOG(LOG_CRITICAL,"EECK - Dispatch array not completed");
			}

		else
			{
			dispenserFree(dispatchReadBlock->dispatchArray->disp);
			}
		}

	dispenserReset(dispatchReadBlock->disp);

	unallocateReadDispatchBlock(dispatchReadBlock);
	unreserveReadDispatchBlock(rb);

	//LOG(LOG_INFO,"Dispatch block completed");

	return 1;
}


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


static int assignReversedInboundDispatchesToSlices(RoutingReadReferenceBlockDispatch *dispatches, RoutingDispatchGroupState *dispatchGroupState)
{
	RoutingReadReferenceBlock *smerInboundDispatches=dispatchGroupState->smerInboundDispatches;

	while(dispatches!=NULL)
		{
		RoutingReadReferenceBlockDispatch *nextDispatches=dispatches->prevPtr;
		__builtin_prefetch(nextDispatches,1,3);

		for(int i=0;i<dispatches->data.entryCount;i++)
			__builtin_prefetch(dispatches->data.entries[i],0,3);

		for(int i=0;i<dispatches->data.entryCount;i++)
			{
			RoutingReadData *readData=dispatches->data.entries[i];

			int indexCount=readData->indexCount;
			u32 slice=readData->indexedData[indexCount].slice;

			u32 inboundIndex=slice & SMER_DISPATCH_GROUP_SLICEMASK;

			assignReadDataToDispatchIntermediate(smerInboundDispatches+inboundIndex, readData, dispatchGroupState->disp);
			}

		__atomic_fetch_sub(dispatches->completionCountPtr,1, __ATOMIC_RELEASE);

		dispatches=nextDispatches;
		}

	return 0;
}


static int indexedReadReferenceBlockSorter(const void *a, const void *b)
{
	RoutingIndexedReadReferenceBlock **pa=(RoutingIndexedReadReferenceBlock **)a;
	RoutingIndexedReadReferenceBlock **pb=(RoutingIndexedReadReferenceBlock **)b;

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


static int calcCapacityForIndexedEntries(int entryCount)
{
	if(entryCount<16)
		return 6;

	int clz=__builtin_clz(entryCount);
	int size=5+((32-clz)>>1);

	return size;
}

static int indexDispatchesForSlice(RoutingReadReferenceBlock *smerInboundDispatches, int sliceSmerCount, MemDispenser *disp,
		RoutingIndexedReadReferenceBlock ***indexedDispatchesPtr)
{
	for(int i=0;i<smerInboundDispatches->entryCount;i++)
		__builtin_prefetch(smerInboundDispatches->entries[i],0,3);

	IohHash *map=iohInitMap(disp, calcCapacityForIndexedEntries(smerInboundDispatches->entryCount));

	for(int i=0;i<smerInboundDispatches->entryCount;i++)
		{
		RoutingReadData *readData=smerInboundDispatches->entries[i];
		int sliceIndex=readData->indexedData[readData->indexCount].sliceIndex;

		if(sliceIndex>=0 && sliceIndex<sliceSmerCount)
			{
			RoutingIndexedReadReferenceBlock *rdi=iohGet(map,sliceIndex);

			if(rdi==NULL)
				{
				rdi=allocDispatchIndexedIntermediateBlock(disp);
				rdi->sliceIndex=sliceIndex;

				iohPut(map,sliceIndex,rdi);
				}

			assignReadDataToDispatchIndexedIntermediate(rdi,readData,disp);
			}
		else
			{
			LOG(LOG_CRITICAL,"Got invalid slice index %i",sliceIndex);
			}
	}

	RoutingIndexedReadReferenceBlock **indexedDispatches=(RoutingIndexedReadReferenceBlock **)iohGetAllValues(map);
	*indexedDispatchesPtr=indexedDispatches;

	qsort(indexedDispatches, map->entryCount, sizeof(RoutingIndexedReadReferenceBlock *), indexedReadReferenceBlockSorter);

	return map->entryCount;
}

/*
void dumpPatches(RoutePatch *patches, int patchCount)
{
	for(int i=0;i<patchCount;i++)
		LOG(LOG_INFO,"Patch: P: %i S: %i #: %i",patches[i].prefixIndex, patches[i].suffixIndex, patches[i].rdiIndex);
}
*/



static void processSlice(RoutingReadReferenceBlock *smerInboundDispatches,  SmerArraySlice *slice, u32 sliceIndex,
		RoutingReadData **orderedDispatches, MemDispenser *disp, MemDispenser *routingDisp, MemCircHeap *circHeap)
{
	if(smerInboundDispatches->entryCount>0)
		{
		RoutingIndexedReadReferenceBlock **indexedDispatches=NULL;

		int indexLength=indexDispatchesForSlice(smerInboundDispatches, slice->smerCount, disp, &indexedDispatches);//, smerTmpDispatches); ?????? which disp?

		u8 sliceTag=(u8)sliceIndex; // In practice, cannot be bigger than SMER_DISPATCH_GROUP_SLICES (256)

		int dispatchOffset=0;

		for(int j=0;j<indexLength;j++)
			{
			int entryCount=rtRouteReadsForSmer(indexedDispatches[j], slice, orderedDispatches+dispatchOffset, routingDisp, circHeap, sliceTag);
			dispatchOffset+=entryCount;
			}
		}
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

static int gatherSliceOutbound(RoutingDispatchGroupState *groupState, int sliceNum, RoutingReadData **orderedDispatches)
{
	int work=0;
	RoutingReadReferenceBlockDispatchArray *dispatchArray=groupState->outboundDispatches;

//	for(int i=0;i<SMER_DISPATCH_GROUP_SLICES;i++)
//		{
		RoutingReadReferenceBlock *smerInboundDispatches=groupState->smerInboundDispatches+sliceNum;

		for(int j=0;j<smerInboundDispatches->entryCount;j++)
			{
//			int entryIndex=rdiIndexes[j];

			//if(entryIndex!=j)
//				LOG(LOG_INFO,"Mismatch %i vs %i",entryIndex,j);

			RoutingReadData *readData=orderedDispatches[j];

			int nextIndexCount=__atomic_sub_fetch(&(readData->indexCount),1, __ATOMIC_SEQ_CST);

			if(nextIndexCount==0) // Past last Smer - read is done
				{
				__atomic_fetch_sub(readData->completionCountPtr,1, __ATOMIC_SEQ_CST);
				}

			else if(nextIndexCount>0)
				{
				assignToDispatchArrayEntry(dispatchArray, readData);
				}

			else // Wrapped
				{
				LOG(LOG_INFO,"Wrapped smer Index %i",nextIndexCount);
				exit(1);
				}
			}

		if(smerInboundDispatches->entryCount)
			work=1;

		smerInboundDispatches->entryCount=0;
//		}

	return work;
}



static int scanForDispatchesForGroups(RoutingBuilder *rb, int startGroup, int endGroup, int force, int workerNo, int *lastGroupPtr)
{
	int work=0;
	int i;

	MemDispenser *routingDisp=NULL;

	for(i=startGroup;i<endGroup;i++)
		{
		RoutingReadReferenceBlockDispatch *tmpPtr=__atomic_load_n(rb->dispatchPtr+i, __ATOMIC_SEQ_CST);

		if(force || tmpPtr != NULL)
			{
			if(lockRoutingDispatchGroupState(rb, i))
				{
				RoutingDispatchGroupState *groupState=rb->dispatchGroupState+i;

				RoutingReadReferenceBlockDispatch *dispatchEntry=dequeueDispatchForGroupList(rb, i);

				if(dispatchEntry!=NULL)
					{
					if(routingDisp==NULL)
						routingDisp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);

					work++;

					RoutingReadReferenceBlockDispatch *reversed=buildPrevLinks(dispatchEntry);
					assignReversedInboundDispatchesToSlices(reversed, groupState);

					// Currently only processing with new incoming or force mode
					// Longer term think about processing only with busy slices or force mode

					//LOG(LOG_INFO,"Begin group %i",i);

					prepareGroupOutbound(groupState);

					//array->dispatches[group].data

					MemCircHeap *circHeap=rb->graph->smerArray.heaps[i];
					SmerArraySlice *baseSlice=rb->graph->smerArray.slice+(i << SMER_DISPATCH_GROUP_SHIFT);
					for(int j=0;j<SMER_DISPATCH_GROUP_SLICES;j++)
						{
						RoutingReadReferenceBlock *smerInboundDispatches=groupState->smerInboundDispatches+j;
						int inboundEntryCount=smerInboundDispatches->entryCount;
						SmerArraySlice *slice=baseSlice+j;

						if(inboundEntryCount>0)
							{
							RoutingReadData **orderedDispatches=dAlloc(groupState->disp, sizeof(RoutingReadData *)*inboundEntryCount);

//							int sliceNumber=j+(i << SMER_DISPATCH_GROUP_SHIFT);
//							LOG(LOG_INFO,"Processing slice %i",sliceNumber);

							processSlice(smerInboundDispatches, slice, j, orderedDispatches, groupState->disp, routingDisp, circHeap);
							dispenserReset(routingDisp);

							gatherSliceOutbound(groupState, j, orderedDispatches);
							}
						}

					queueDispatchArray(rb, groupState->outboundDispatches);
					recycleRoutingDispatchGroupState(groupState);
					}

				unlockRoutingDispatchGroupState(rb,i);
				}
			}
		}

	if(routingDisp!=NULL)
		{
		dispenserFree(routingDisp);
		}

	return work;
}



int scanForDispatches(RoutingBuilder *rb, int workerNo, RoutingWorkerState *wState, int force)
{
	//int position=wState->dispatchGroupCurrent;
	int work=0;
	int lastGroup=-1;

	work=scanForDispatchesForGroups(rb,wState->dispatchGroupStart, wState->dispatchGroupEnd, force, workerNo, &lastGroup);

	//if(work<TR_DISPATCH_MAX_WORK)
//		work+=scanForDispatchesForGroups(rb, 0, position, force, workerNo, &lastGroup);

//	wState->dispatchGroupCurrent=lastGroup;

	return work;
}

