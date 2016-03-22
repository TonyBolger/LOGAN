/*
 * taskRoutingDispatch.c
 *
 *  Created on: Mar 3, 2016
 *      Author: tony
 */

#include "../common.h"


static RoutingDispatchIntermediate *allocDispatchIntermediateBlock(MemDispenser *disp)
{
	RoutingLookupPercolate *block=dAllocCacheAligned(disp, sizeof(RoutingLookupPercolate));

	block->entryCount=0;
	block->entries=dAllocCacheAligned(disp, TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK*sizeof(SmerEntry));

	return block;
}


void initRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState)
{
	MemDispenser *disp=dispenserAlloc("DispatchGroupState");

	dispatchGroupState->status=0;
	dispatchGroupState->forceCount=0;
	dispatchGroupState->disp=disp;

	for(int j=0;j<SMER_DISPATCH_GROUP_SLICES;j++)
		dispatchGroupState->smerInboundDispatches[j]=allocDispatchIntermediateBlock(disp);

	for(int j=0;j<SMER_DISPATCH_GROUPS;j++)
		dispatchGroupState->smerOutboundDispatches[j]=allocDispatchIntermediateBlock(disp);
}

void resetRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState)
{
	if(dispatchGroupState->disp!=NULL)
		dispenserFree(dispatchGroupState->disp);

	initRoutingDispatchGroupState(dispatchGroupState);
}



void queueDispatchForGroup(RoutingBuilder *rb, RoutingDispatch *dispatchForGroup, int groupNum)
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
	while(__sync_bool_compare_and_swap(rb->dispatchPtr+groupNum,current,dispatchForGroup));

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

	dispenserFree(dispatchReadBlock->disp);

	unallocateReadDispatchBlock(dispatchReadBlock);
	unreserveReadDispatchBlock(rb);

	LOG(LOG_INFO,"Dispatch block completed");

	return 1;
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

static void expandIntermediateDispatchBlock(RoutingDispatchIntermediate *block, MemDispenser *disp)
{
	int oldSize=block->entryCount;
	int size=oldSize*2;

	int oldEntrySize=oldSize*sizeof(RoutingReadDispatchData *);

	SmerEntry *entries=dAllocCacheAligned(disp, size*sizeof(RoutingReadDispatchData  *));
	memcpy(entries,block->entries,oldEntrySize);
	block->entries=entries;
	//block->presenceAbsence=NULL;//dAlloc(disp, PAD_BYTELENGTH_8BYTE(PAD_1BITLENGTH_BYTE(size)));
}


static int assignInboundDispatchesToSlices(RoutingDispatch *dispatches, RoutingDispatchGroupState *dispatchGroupState)
{
	RoutingDispatchIntermediate *smerInboundDispatches=dispatchGroupState->smerInboundDispatches;

	while(dispatches!=NULL)
		{
		int i=0;

		RoutingReadDispatchData readData=dispatches->readData[i++];

		while(readData!=NULL)
			{
			int index=readData->currentIndex;

			SmerId smer=readData->smers[index];
			SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
			u64 hash=hashForSmer(smerEntry);
			u32 slice=sliceForSmer(smer,hash);

			u32 inboundIndex=slice & SMER_DISPATCH_GROUP_SLICEMASK;

			u64 entryCount=smerInboundDispatches[inboundIndex]->entryCount;
			if(entryCount>=TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK && !((entryCount & (entryCount - 1))))
				expandIntermediateDispatchBlock(smerInboundDispatches[inboundIndex], dispatchGroupState->disp);

			smerInboundDispatches[inboundIndex]->entries[entryCount]=smerEntry;
			smerInboundDispatches[inboundIndex]->entryCount++;

			readData=dispatches->readData[i++];
			}

		dispatches=dispatches->prevPtr;
		}

	return 0;
}

static void processSlicesForGroup(RoutingBuilder *rb, int groupNum)
{
	RoutingDispatchGroupState *groupState=rb->dispatchGroupState+groupNum;

	for(int i=0;i<SMER_DISPATCH_GROUP_SLICES;i++)
		{
		RoutingDispatchIntermediate *smerInboundDispatches=groupState->smerInboundDispatches+i;



		}
}



static int scanForDispatchesForGroups(RoutingBuilder *rb, int startGroup, int endGroup, int force)
{
	int work=0;

	for(int i=startGroup;i<endGroup;i++)
		{
		if(lockRoutingDispatchGroupState(rb, i))
			{
			RoutingDispatchGroupState *groupState=rb->dispatchGroupState+i;

			RoutingDispatch *dispatchEntry=dequeueDispatchForGroup(rb, i);

			if(dispatchEntry!=NULL)
				{
				work=1;

				RoutingDispatch *reversed=buildPrevLinks(dispatchEntry);

				assignInboundDispatchesToSlices(reversed, groupState->smerInboundDispatches);
  				}



			// think about processing busy slices



			unlockRoutingDispatchGroupState(rb,i);
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

