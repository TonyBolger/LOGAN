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


RoutingReadReferenceBlockDispatchArray *allocDispatchArray(RoutingReadReferenceBlockDispatchArray *nextPtr)
{
	MemDispenser *disp=dispenserAlloc("RoutingDispatchArray");

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


void assignToDispatchArrayEntry(RoutingReadReferenceBlockDispatchArray *array, RoutingReadData *readData)
{
	SmerId fsmer=readData->fsmers[readData->indexCount];
	SmerId rsmer=readData->rsmers[readData->indexCount];

	SmerId smer=CANONICAL_SMER(fsmer,rsmer);

	SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
	u64 hash=hashForSmer(smerEntry);
	u32 sliceNew=sliceForSmer(smer,hash);

	u32 slice=readData->slices[readData->indexCount];
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
	MemDispenser *disp=dispenserAlloc("DispatchGroupState");

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
	if(dispatchGroupState->disp!=NULL)
		{
		dispenserFree(dispatchGroupState->disp);
		//dispenserFreeLogged(dispatchGroupState->disp);
		//dispatchGroupState->disp=NULL;
		}

	MemDispenser *disp=dispenserAlloc("DispatchGroupState");
	dispatchGroupState->disp=disp;

	for(int j=0;j<SMER_DISPATCH_GROUP_SLICES;j++)
		initDispatchIntermediateBlock(dispatchGroupState->smerInboundDispatches+j, disp);

	dispatchGroupState->outboundDispatches=cleanupRoutingDispatchArrays(dispatchGroupState->outboundDispatches);
}


void freeRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState)
{
	if(dispatchGroupState->disp!=NULL)
		dispenserFree(dispatchGroupState->disp);

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
	__atomic_fetch_sub(&(rb->allocatedReadDispatchBlocks),1, __ATOMIC_SEQ_CST);
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
			dispenserFree(dispatchReadBlock->dispatchArray->disp);
		}

	dispenserFree(dispatchReadBlock->disp);

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
		for(int i=0;i<dispatches->data.entryCount;i++)
			{
			RoutingReadData *readData=dispatches->data.entries[i];

			int indexCount=readData->indexCount;
			u32 slice=readData->slices[indexCount];

			u32 inboundIndex=slice & SMER_DISPATCH_GROUP_SLICEMASK;

			assignReadDataToDispatchIntermediate(smerInboundDispatches+inboundIndex, readData, dispatchGroupState->disp);
			}

		RoutingReadReferenceBlockDispatch *nextDispatches=dispatches->prevPtr;

		__atomic_fetch_sub(dispatches->completionCountPtr,1, __ATOMIC_SEQ_CST);

		dispatches=nextDispatches;
		}

	return 0;
}


static int indexDispatchesForSlice(RoutingReadReferenceBlock *smerInboundDispatches, int sliceSmerCount, MemDispenser *disp,
		RoutingReadReferenceBlock **indexedDispatches, u32 *sliceIndexes)
{
//	int size=sizeof(RoutingDispatchIntermediate *) *sliceSmerCount;
	//RoutingDispatchIntermediate **smerDispatches=dAlloc(disp, size);

//	LOG(LOG_INFO,"%i %p %i %p %p",sliceSmerCount,smerDispatches, size, indexedDispatches, sliceIndexes);

	RoutingReadReferenceBlock **smerDispatches=dAlloc(disp, sizeof(RoutingReadReferenceBlock *)*  sliceSmerCount);

	for(int j=0;j<sliceSmerCount;j++)
		{
//		indexedDispatches[j]=NULL;
//		sliceIndexes[j]=-1;
		smerDispatches[j]=NULL;
		}

	int usedCount=0;
//	int maxCount=0;


	for(int i=0;i<smerInboundDispatches->entryCount;i++)
	{
		RoutingReadData *readData=smerInboundDispatches->entries[i];
		int sliceIndex=readData->sliceIndexes[readData->indexCount];

		if(sliceIndex>=0 && sliceIndex<sliceSmerCount)
			{
			RoutingReadReferenceBlock *rdi=smerDispatches[sliceIndex];

			if(rdi==NULL)
				{
				rdi=allocDispatchIntermediateBlock(disp);
				smerDispatches[sliceIndex]=rdi;
				indexedDispatches[usedCount]=rdi;
				sliceIndexes[usedCount++]=sliceIndex;
				}

			assignReadDataToDispatchIntermediate(rdi,readData,disp);

//			if(rdi->entryCount>maxCount)
//				maxCount=rdi->entryCount;
			}
		else
			{
			LOG(LOG_CRITICAL,"Got invalid slice index %i",sliceIndex);
			}
	}

	return usedCount;
}



/*
void dumpPatches(RoutePatch *patches, int patchCount)
{
	for(int i=0;i<patchCount;i++)
		LOG(LOG_INFO,"Patch: P: %i S: %i #: %i",patches[i].prefixIndex, patches[i].suffixIndex, patches[i].rdiIndex);
}
*/



static void processSlice(RoutingReadReferenceBlock *smerInboundDispatches,  SmerArraySlice *slice, u32 sliceIndex,
		RoutingReadData **orderedDispatches, MemDispenser *disp, MemCircHeap *circHeap)
{
	//for(int i=0;i<SMER_DISPATCH_GROUP_SLICES;i++)
		//{

		if(smerInboundDispatches->entryCount>0)
			{
			// Wasteful approach for groups with few reads - change to qsort based on sliceIndex
			RoutingReadReferenceBlock **indexedDispatches=dAlloc(disp, sizeof(RoutingReadReferenceBlock *)*  slice->smerCount);
			u32 *sliceIndexes=dAlloc(disp, sizeof(u32)*slice->smerCount);
/*
			for(int j=0;j<slice->smerCount;j++)
				{
				indexedDispatches[j]=NULL;
				sliceIndexes[j]=-1;
				}
*/
			int indexLength=indexDispatchesForSlice(smerInboundDispatches, slice->smerCount, disp, indexedDispatches, sliceIndexes);//, smerTmpDispatches);

			u8 sliceTag=(u8)sliceIndex; // In practice, cannot be bigger than SMER_DISPATCH_GROUP_SLICES (256)

			int dispatchOffset=0;
			for(int j=0;j<indexLength;j++)
				{
				int entryCount=rtRouteReadsForSmer(indexedDispatches[j], sliceIndexes[j], slice, orderedDispatches+dispatchOffset, disp, circHeap, sliceTag);
				dispatchOffset+=entryCount;
				}

			}
		//}

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



static int scanForDispatchesForGroups(RoutingBuilder *rb, int startGroup, int endGroup, int force, int workerNo)
{
	int work=0;

	for(int i=startGroup;i<endGroup;i++)
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
					work=1;

					RoutingReadReferenceBlockDispatch *reversed=buildPrevLinks(dispatchEntry);
					assignReversedInboundDispatchesToSlices(reversed, groupState);

					// Currently only processing with new incoming or force mode
					// Longer term think about processing only with busy slices or force mode

					//LOG(LOG_INFO,"Begin group %i",i);

					prepareGroupOutbound(groupState);

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

							processSlice(smerInboundDispatches, slice, j, orderedDispatches, groupState->disp, circHeap);

//							LOG(LOG_INFO,"IndexGroup:");

//							for(int i=0;i<inboundEntryCount;i++)
//								LOG(LOG_INFO,"%i",rdiIndexes[i]);

							work+=gatherSliceOutbound(groupState, j, orderedDispatches);
							}
						}

					queueDispatchArray(rb, groupState->outboundDispatches);

					recycleRoutingDispatchGroupState(groupState);
					}

				unlockRoutingDispatchGroupState(rb,i);
				}
			}

		}

	return work>0;
}



int scanForDispatches(RoutingBuilder *rb, int workerNo, int force)
{
	int startPos=(workerNo*SMER_SLICE_PRIME)&SMER_DISPATCH_GROUP_MASK;

	int work=0;

	work=scanForDispatchesForGroups(rb,startPos,SMER_DISPATCH_GROUPS, force, workerNo);
	work+=scanForDispatchesForGroups(rb, 0, startPos, force, workerNo);

	return work;
}

