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
		Handle complete dispatch block
		if allocated dispatch blocks is max:
			Slicegroup dispatch scan
		else
			Handle complete read lookup block
			Slice lookup scan (if read lookup blocks is max)

	Force:
		Handle complete dispatch block
		Slicegroup dispatch scan

		if allocated dispatch blocks is not max:
			Handle complete read lookup block
			Slice lookup scan

*/


static int trDoIntermediate(ParallelTask *pt, int workerNo, int force)
{
	RoutingBuilder *rb=pt->dataPtr;

//	LOG(LOG_INFO,"doIntermediate %i %i %i %i",workerNo,force,countReadBlocks(rb),countCompleteReadBlocks(rb));


	if(scanForAndDispatchLookupCompleteReadLookupBlocks(rb))
		return 1;

	if((force)||(rb->allocatedReadLookupBlocks==TR_READBLOCK_LOOKUPS_INFLIGHT))
		{
		int startPos=(workerNo*SMER_SLICE_PRIME)&SMER_SLICE_MASK;

		int work=0;

		work=scanForSmerLookups(rb,startPos,SMER_SLICES);
		work+=scanForSmerLookups(rb, 0, startPos);

		if(work)
			return 1;
		}

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

	return rb;
}

void freeRoutingBuilder(RoutingBuilder *rb)
{
	ParallelTask *pt=rb->pt;
	ParallelTaskConfig *ptc=pt->config;

	freeParallelTask(pt);
	freeParallelTaskConfig(ptc);

	tiRoutingBuilderFree(rb);
}


