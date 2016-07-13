/*
 * taskRouting.c
 *
 *  Created on: Oct 28, 2015
 *      Author: tony
 */

#include "common.h"




static void trDoRegister(ParallelTask *pt, int workerNo)
{

}

static void trDoDeregister(ParallelTask *pt, int workerNo)
{

}

static int trAllocateIngressSlot(ParallelTask *pt, int workerNo)
{
	RoutingBuilder *rb=pt->dataPtr;

	return reserveReadLookupBlock(rb);
}


static int trDoIngress(ParallelTask *pt, int workerNo,void *ingressPtr, int ingressPosition, int ingressSize)
{
	SwqBuffer *rec=ingressPtr;
	RoutingBuilder *rb=pt->dataPtr;
	Graph *graph=rb->graph;
	s32 nodeSize=graph->config.nodeSize;

	queueReadsForSmerLookup(rec,ingressPosition,ingressSize,nodeSize,rb);

	return 1;
}

static void dumpUncleanDispatchReadBlocks(int blockNum, RoutingReadDispatchBlock *readDispatchBlock)
{
	if(readDispatchBlock->completionCount>0)
		{
		LOG(LOG_INFO,"INCOMPLETE: RDB BlockNum: %i Status: %i RemainingCount: %i",blockNum,readDispatchBlock->status,readDispatchBlock->completionCount);

		for(int i=0;i<readDispatchBlock->readCount;i++)
			{
			if(readDispatchBlock->readData[i].indexCount!=-1)
				{
				int indexCount=readDispatchBlock->readData[i].indexCount;

				if(indexCount>=0)
					{
					LOG(LOG_INFO,"INCOMPLETE: Blocked read: %i at indexCount %i in slice %i",i,indexCount, readDispatchBlock->readData[i].slices[indexCount]);
					}
				else
					{
					LOG(LOG_INFO,"INCOMPLETE: %i at indexCount %i",i,readDispatchBlock->readData[i].indexCount);
					}
				}
			}
		}

}

static void dumpUncleanDispatchGroup(int groupNum, RoutingDispatch *dispatchPtr, RoutingDispatchGroupState *dispatchGroupState)
{
	if(dispatchPtr!=NULL)
		{
		LOG(LOG_INFO,"INCOMPLETE: Dispatch ptr not null: %i", groupNum);
		}

	if(dispatchGroupState->status!=0)
		LOG(LOG_INFO,"INCOMPLETE: DispatchGroup Status for %i is %i", groupNum,dispatchGroupState->status);

	//dispatchGroupState->outboundDispatches;

	for(int i=0;i<SMER_DISPATCH_GROUP_SLICES;i++)
		{
		if(dispatchGroupState->smerInboundDispatches[i].entryCount>0)
			LOG(LOG_INFO,"INCOMPLETE: Inbound items: %i %i %i", groupNum, i, dispatchGroupState->smerInboundDispatches[i].entryCount);
		}

}

static void dumpUnclean(RoutingBuilder *rb)
{
	for(int i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		dumpUncleanDispatchReadBlocks(i, rb->readDispatchBlocks+i);

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		dumpUncleanDispatchGroup(i, rb->dispatchPtr[i], rb->dispatchGroupState+i);

}


/* Intermediate:

	Non-force:
		Handle complete dispatch block - top priority, frees up memory

		Handle complete lookup block - second priority, move into dispatch

		if allocated dispatch blocks is max: - if dispatch full, perform
			dispatch scan

		if allocated lookup blocks is max: - if lookup full, perform
			lookup scan


	Force:
		Handle complete dispatch block

		Handle complete lookup block

		if allocated dispatch blocks is max:
			dispatch scan

		lookup scan

		dispatch scan

*/


static int trDoIntermediate(ParallelTask *pt, int workerNo, int force)
{
	RoutingBuilder *rb=pt->dataPtr;

	if(scanForCompleteReadDispatchBlocks(rb))
		{
		//LOG(LOG_INFO,"scanForCompleteReadDispatchBlocks OK");
		return 1;
		}

	if(scanForAndDispatchLookupCompleteReadLookupBlocks(rb))
		{
		//LOG(LOG_INFO,"scanForAndDispatchLookupCompleteReadLookupBlocks OK");
		return 1;
		}

	if(rb->allocatedReadDispatchBlocks==TR_READBLOCK_DISPATCHES_INFLIGHT)
		{
		if(scanForDispatches(rb, workerNo, force))
			{
			//LOG(LOG_INFO,"scanForDispatches 1");
			return 1;
			}
		}

	if((force)||(rb->allocatedReadLookupBlocks==TR_READBLOCK_LOOKUPS_INFLIGHT))
		{
		if(scanForSmerLookups(rb, workerNo))
			{
			//LOG(LOG_INFO,"scanForSmerLookups");
			return 1;
			}
		}

	if(force)
		{
		if(scanForDispatches(rb, workerNo, force))
			{
			//LOG(LOG_INFO,"scanForDispatches 2");
			return 1;
			}
		}

	int arlb=__atomic_load_n(&rb->allocatedReadLookupBlocks, __ATOMIC_SEQ_CST);
	int ardb=__atomic_load_n(&rb->allocatedReadDispatchBlocks, __ATOMIC_SEQ_CST);

//	LOG(LOG_INFO,"trDoIntermediate: %i %i %i %i",workerNo, force, arlb, ardb);

	if(arlb>0 || ardb>0) // If in force mode, and not finished, rally the minions
		return force;

	return 0;
}

static int trDoTidy(ParallelTask *pt, int workerNo, int tidyNo)
{
	return 0;
}


RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads)
{
	RoutingBuilder *rb=tiRoutingBuilderAlloc();

	ParallelTaskConfig *ptc=allocParallelTaskConfig(trDoRegister,trDoDeregister,trAllocateIngressSlot,trDoIngress,trDoIntermediate,trDoTidy,threads,
			TR_INGRESS_BLOCKSIZE, TR_INGRESS_PER_TIDY_MIN, TR_INGRESS_PER_TIDY_MAX, TR_TIDYS_PER_BACKOFF,
			16);//SMER_HASH_SLICES);

	rb->pt=allocParallelTask(ptc,rb);
	rb->graph=graph;

	rb->allocatedReadLookupBlocks=0;
	for(int i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
		rb->readLookupBlocks[i].disp=NULL;

	for(int i=0;i<SMER_SLICES;i++)
		rb->smerEntryLookupPtr[i]=NULL;

	rb->allocatedReadDispatchBlocks=0;
	for(int i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		rb->readDispatchBlocks[i].disp=NULL;

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		{
		initRoutingDispatchGroupState(rb->dispatchGroupState+i);
		rb->dispatchPtr[i]=NULL;
		}

	return rb;
}



void freeRoutingBuilder(RoutingBuilder *rb)
{
	dumpUnclean(rb);

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		freeRoutingDispatchGroupState(rb->dispatchGroupState+i);

	ParallelTask *pt=rb->pt;
	ParallelTaskConfig *ptc=pt->config;

	freeParallelTask(pt);
	freeParallelTaskConfig(ptc);

	tiRoutingBuilderFree(rb);
}


