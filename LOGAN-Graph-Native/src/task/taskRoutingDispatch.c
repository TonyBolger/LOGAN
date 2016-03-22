/*
 * taskRoutingDispatch.c
 *
 *  Created on: Mar 3, 2016
 *      Author: tony
 */

#include "../common.h"



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


static int assignInboundDispatchesToSlices(RoutingDispatch *dispatches, RoutingDispatchIntermediate *smerInboundDispatches)
{
//	dispatches->readData

	return 0;
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

