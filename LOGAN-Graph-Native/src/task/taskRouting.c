/*
 * taskRouting.c
 *
 *  Created on: Oct 28, 2015
 *      Author: tony
 */

#include "../common.h"




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

	return 0;
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

	LOG(LOG_INFO,"doIntermediate %i %i %i %i",workerNo,force, rb->allocatedReadLookupBlocks, rb->allocatedReadDispatchBlocks);

	if(scanForCompleteReadDispatchBlocks(rb))
		return 1;

	if(scanForAndDispatchLookupCompleteReadLookupBlocks(rb))
		return 1;

	if(rb->allocatedReadDispatchBlocks==TR_READBLOCK_DISPATCHES_INFLIGHT)
		{
		if(scanForDispatches(rb, workerNo, force))
			return 1;
		}

	if((force)||(rb->allocatedReadLookupBlocks==TR_READBLOCK_LOOKUPS_INFLIGHT))
		{
		if(scanForSmerLookups(rb, workerNo))
			return 1;
		}

	if(force)
		return scanForDispatches(rb, workerNo, force);

	showReadDispatchBlocks(rb);

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

static void dumpDispatchReadBlocks(int i, RoutingReadDispatchBlock *readDispatchBlock)
{
	LOG(LOG_INFO,"Cleanup RDF: %i %i %i",i,readDispatchBlock->status,readDispatchBlock->completionCount);
}

void freeRoutingBuilder(RoutingBuilder *rb)
{
	for(int i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		dumpDispatchReadBlocks(i, rb->readDispatchBlocks+i);

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		freeRoutingDispatchGroupState(rb->dispatchGroupState+i);

	ParallelTask *pt=rb->pt;
	ParallelTaskConfig *ptc=pt->config;

	freeParallelTask(pt);
	freeParallelTaskConfig(ptc);

	tiRoutingBuilderFree(rb);
}


