/*
 * taskRoutingDispatch.c
 *
 *  Created on: Mar 3, 2016
 *      Author: tony
 */

#include "../common.h"


static void initDispatchIntermediateBlock(RoutingDispatchIntermediate *block, MemDispenser *disp)
{
	block->entryCount=0;
	block->entries=dAllocCacheAligned(disp, TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK*sizeof(RoutingReadDispatchData *));

}

/*
static RoutingDispatchIntermediate *allocDispatchIntermediateBlock(MemDispenser *disp)
{
	RoutingDispatchIntermediate *block=dAllocCacheAligned(disp, sizeof(RoutingLookupPercolate));
	initDispatchIntermediateBlock(block, disp);
	return block;
}
*/

RoutingDispatchArray *allocDispatchArray(RoutingDispatchArray *nextPtr)
{
	MemDispenser *disp=dispenserAlloc("RoutingDispatchArray");

	RoutingDispatchArray *array=dAlloc(disp, sizeof(RoutingDispatchArray));

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

static void expandIntermediateDispatchBlock(RoutingDispatchIntermediate *block, MemDispenser *disp)
{
	int oldSize=block->entryCount;
	int size=oldSize*2;

	int oldEntrySize=oldSize*sizeof(RoutingReadDispatchData *);

	RoutingReadDispatchData **entries=dAllocCacheAligned(disp, size*sizeof(RoutingReadDispatchData  *));
	memcpy(entries,block->entries,oldEntrySize);
	block->entries=entries;

	//block->presenceAbsence=NULL;//dAlloc(disp, PAD_BYTELENGTH_8BYTE(PAD_1BITLENGTH_BYTE(size)));
}



static void assignReadDataToDispatchIntermediate(RoutingDispatchIntermediate *intermediate, RoutingReadDispatchData *readData, MemDispenser *disp)
{
	u64 entryCount=intermediate->entryCount;
	if(entryCount>=TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK && !((entryCount & (entryCount - 1))))
		expandIntermediateDispatchBlock(intermediate, disp);

	intermediate->entries[entryCount]=readData;
	intermediate->entryCount++;
}


void assignToDispatchArrayEntry(RoutingDispatchArray *array, RoutingReadDispatchData *readData)
{
	SmerId smer=readData->smers[readData->indexCount];

	SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
	u64 hash=hashForSmer(smerEntry);
	u32 slice=sliceForSmer(smer,hash);

	u32 group=(slice >> SMER_DISPATCH_GROUP_SHIFT);

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


static RoutingDispatchArray *cleanupRoutingDispatchArrays(RoutingDispatchArray *in)
{
	RoutingDispatchArray **scan=&in;

	while((*scan)!=NULL)
		{
		RoutingDispatchArray *current=(*scan);

		if(current->completionCount==0)
			{
			*scan=current->nextPtr;
			dispenserFree(current->disp);
			}
		else
			scan=&((*scan)->nextPtr);
		}

	return in;
}


void recycleRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState)
{
	if(dispatchGroupState->disp!=NULL)
		dispenserFree(dispatchGroupState->disp);

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



static void queueDispatchForGroup(RoutingBuilder *rb, RoutingDispatch *dispatchForGroup, int groupNum)
{
	RoutingDispatch *current=NULL;
	int loopCount=0;

	do
		{
		if(loopCount++>1000000)
			{
			LOG(LOG_INFO,"Loop Stuck");
			exit(0);
			}

		current=rb->dispatchPtr[groupNum];
		dispatchForGroup->nextPtr=current;
		}
	while(!__sync_bool_compare_and_swap(rb->dispatchPtr+groupNum,current,dispatchForGroup));

}

static RoutingDispatch *dequeueDispatchForGroup(RoutingBuilder *rb, int groupNum)
{
	RoutingDispatch *current=NULL;
	int loopCount=0;

	do
		{
		if(loopCount++>1000000)
			{
			LOG(LOG_INFO,"Loop Stuck");
			exit(0);
			}

		current=rb->dispatchPtr[groupNum];

		if(current==NULL)
			return NULL;
		}
	while(!__sync_bool_compare_and_swap(rb->dispatchPtr+groupNum,current,NULL));

	return current;
}


static int lockRoutingDispatchGroupState(RoutingBuilder *rb, int groupNum)
{
	return __sync_bool_compare_and_swap(&(rb->dispatchGroupState[groupNum].status),0,1);
}

static int unlockRoutingDispatchGroupState(RoutingBuilder *rb, int groupNum)
{
	if(!__sync_bool_compare_and_swap(&(rb->dispatchGroupState[groupNum].status),1,0))
		{
		LOG(LOG_INFO,"Tried to unlock DispatchState without lock, should never happen");
		exit(1);
		}

	return 1;
}





int reserveReadDispatchBlock(RoutingBuilder *rb)
{
	int allocated=rb->allocatedReadDispatchBlocks;

	if(allocated>=TR_READBLOCK_DISPATCHES_INFLIGHT)
		return 0;

	while(!__sync_bool_compare_and_swap(&(rb->allocatedReadDispatchBlocks),allocated,allocated+1))
		{
		allocated=rb->allocatedReadDispatchBlocks;

		if(allocated>=TR_READBLOCK_DISPATCHES_INFLIGHT)
			return 0;
		}

	return 1;
}


void unreserveReadDispatchBlock(RoutingBuilder *rb)
{
	__sync_fetch_and_add(&(rb->allocatedReadDispatchBlocks),-1);
}



RoutingReadDispatchBlock *allocateReadDispatchBlock(RoutingBuilder *rb)
{
	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		int i;

		for(i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
			{
			if(rb->readDispatchBlocks[i].status==0)
				{
				if(__sync_bool_compare_and_swap(&(rb->readDispatchBlocks[i].status),0,1))
					return rb->readDispatchBlocks+i;
				}
			}
		}

	LOG(LOG_INFO,"Failed to queue dispatch readblock, should never happen"); // Can actually happen, just stupidly unlikely
	exit(1);
}


void queueReadDispatchBlock(RoutingReadDispatchBlock *readBlock)
{
	if(!__sync_bool_compare_and_swap(&(readBlock->status),1,2))
		{
		LOG(LOG_INFO,"Tried to queue dispatch readblock in wrong state, should never happen");
		exit(1);
		}
}


void showReadDispatchBlocks(RoutingBuilder *rb)
{
	int i;

	for(i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		{
		LOG(LOG_INFO,"RDF: %i %i %i",i,rb->readDispatchBlocks[i].status,rb->readDispatchBlocks[i].completionCount);
		}
}

int scanForCompleteReadDispatchBlock(RoutingBuilder *rb)
{
	RoutingReadDispatchBlock *current;
	int i;

	for(i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		{
		current=rb->readDispatchBlocks+i;
		if(current->status == 2 && current->completionCount==0)
			return i;

		}
	return -1;
}

RoutingReadDispatchBlock *dequeueCompleteReadDispatchBlock(RoutingBuilder *rb, int index)
{
	RoutingReadDispatchBlock *current=rb->readDispatchBlocks+index;

	if(current->status == 2 && current->completionCount==0)
		{
		if(__sync_bool_compare_and_swap(&(current->status),2,3))
			return current;
		}

	return NULL;
}

void unallocateReadDispatchBlock(RoutingReadDispatchBlock *readBlock)
{
	if(!__sync_bool_compare_and_swap(&(readBlock->status),3,0))
		{
		LOG(LOG_INFO,"Tried to unreserve dispatch readblock in wrong state, should never happen");
		exit(1);
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
		return 0;

	if(dispatchReadBlock->dispatchArray!=NULL)
		{
		if(dispatchReadBlock->dispatchArray->completionCount!=0)
			{
			LOG(LOG_INFO,"EECK - Dispatch array not completed");
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


void queueDispatchArray(RoutingBuilder *rb, RoutingDispatchArray *dispArray)
{
	int count=0;

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		if(dispArray->dispatches[i].data.entryCount>0)
			count++;

	dispArray->completionCount=count;

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		{
		if(dispArray->dispatches[i].data.entryCount>0)
			queueDispatchForGroup(rb,dispArray->dispatches+i,i);
		}
}


static RoutingDispatch *buildPrevLinks(RoutingDispatch *dispatchEntry)
{
	RoutingDispatch *prev=NULL;

	while(dispatchEntry!=NULL)
		{
		dispatchEntry->prevPtr=prev;
		prev=dispatchEntry;
		dispatchEntry=dispatchEntry->nextPtr;
		}

	return prev;
}


static int assignReversedInboundDispatchesToSlices(RoutingDispatch *dispatches, RoutingDispatchGroupState *dispatchGroupState)
{
	RoutingDispatchIntermediate *smerInboundDispatches=dispatchGroupState->smerInboundDispatches;

	while(dispatches!=NULL)
		{
		for(int i=0;i<dispatches->data.entryCount;i++)
			{
			RoutingReadDispatchData *readData=dispatches->data.entries[i];

			int index=readData->indexCount;

			SmerId smer=readData->smers[index];
			SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
			u64 hash=hashForSmer(smerEntry);
			u32 slice=sliceForSmer(smer,hash);

			u32 inboundIndex=slice & SMER_DISPATCH_GROUP_SLICEMASK;

			assignReadDataToDispatchIntermediate(smerInboundDispatches+inboundIndex, readData, dispatchGroupState->disp);
			}

		__sync_fetch_and_add(dispatches->completionCountPtr,-1);

		dispatches=dispatches->prevPtr;
		}

	return 0;
}


static int processSlicesForGroup(RoutingDispatchGroupState *groupState)
{
	int work=0;
	RoutingDispatchArray *dispatchArray=groupState->outboundDispatches;

	for(int i=0;i<SMER_DISPATCH_GROUP_SLICES;i++)
		{
		RoutingDispatchIntermediate *smerInboundDispatches=groupState->smerInboundDispatches+i;

		for(int j=0;j<smerInboundDispatches->entryCount;j++)
			{
			RoutingReadDispatchData *readData=smerInboundDispatches->entries[j];

			if(readData->indexCount==0) // Last Smer -
				__sync_fetch_and_add(readData->completionCountPtr,-1);

			else if(readData->indexCount>0)
				{
				readData->indexCount--;
				assignToDispatchArrayEntry(dispatchArray, readData);
				}

			else // Wrapped
				{
				LOG(LOG_INFO,"Wrapped smer Index");
				exit(1);
				}

			}

		if(smerInboundDispatches->entryCount)
			work=1;

		smerInboundDispatches->entryCount=0;
		}

	return work;
}

/*
static void prepareGroupOutbound(RoutingDispatchGroupState *groupState)
{
	RoutingDispatchArray *outboundDispatches=allocDispatchArray();
	outboundDispatches->nextPtr=groupState->outboundDispatches;
	groupState->outboundDispatches=outboundDispatches;

}

*/

static int scanForDispatchesForGroups(RoutingBuilder *rb, int startGroup, int endGroup, int force)
{
	int work=0;

	for(int i=startGroup;i<endGroup;i++)
		{
		if(lockRoutingDispatchGroupState(rb, i))
			{
			//LOG(LOG_INFO,"Processing dispatch group %i",i);

			RoutingDispatchGroupState *groupState=rb->dispatchGroupState+i;

			RoutingDispatch *dispatchEntry=dequeueDispatchForGroup(rb, i);

			if(dispatchEntry!=NULL)
				{
				work=1;

				RoutingDispatch *reversed=buildPrevLinks(dispatchEntry);

				assignReversedInboundDispatchesToSlices(reversed, groupState);
  				}

			// think about processing only with busy slices

			//prepareGroupOutbound(groupState);

			RoutingDispatchArray *outboundDispatches=allocDispatchArray(groupState->outboundDispatches);
			groupState->outboundDispatches=outboundDispatches;


			work+=processSlicesForGroup(groupState);

			queueDispatchArray(rb, outboundDispatches);


			recycleRoutingDispatchGroupState(groupState);

			unlockRoutingDispatchGroupState(rb,i);

			//LOG(LOG_INFO,"Processed dispatch group %i",i);
			}

		}

	return work>0;
}



int scanForDispatches(RoutingBuilder *rb, int workerNo, int force)
{
	int startPos=(workerNo*SMER_SLICE_PRIME)&SMER_DISPATCH_GROUP_MASK;

	int work=0;

	work=scanForDispatchesForGroups(rb,startPos,SMER_DISPATCH_GROUPS, force);
	work+=scanForDispatchesForGroups(rb, 0, startPos, force);

	return work;
}

