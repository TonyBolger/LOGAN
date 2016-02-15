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

static void ingressOne(Graph *graph, u8 *packedSeq, int length)
{

	s32 nodeSize=graph->config.nodeSize;
	s32 maxValidIndex=length-nodeSize;

	s32 *indexes=alloca((maxValidIndex+1)*sizeof(u32));
	u32 indexCount=0;

	SmerId *smerIds=alloca((maxValidIndex+1)*sizeof(SmerId));
	u32 *compFlags=alloca((maxValidIndex+1)*sizeof(u32));

	calculatePossibleSmers(packedSeq, maxValidIndex, smerIds, compFlags);
	indexCount=saFindIndexesOfExistingSmers(&(graph->smerArray),packedSeq, maxValidIndex, indexes, smerIds);

	saVerifyIndexing(graph->config.sparseness, indexes, indexCount, length, maxValidIndex);

//	if(indexCount>0)
//		LOG(LOG_INFO,"Found %i",indexCount);


}

static int trDoIngress(ParallelTask *pt, int workerNo,void *ingressPtr, int ingressPosition, int ingressSize)
{
	SwqBuffer *rec=ingressPtr;
	RoutingBuilder *rb=pt->dataPtr;
	Graph *graph=rb->graph;

	u8 *packedSeq=malloc(rec->maxSequenceTotalLength+4);

	int i=0;
	int ingressLast=ingressPosition+ingressSize;

	for(i=ingressPosition;i<ingressLast;i++)
		{
		SequenceWithQuality *currentRec=rec->rec+i;
		packSequence(currentRec->seq, packedSeq, currentRec->length);

		ingressOne(graph, packedSeq, currentRec->length);
		}

	free(packedSeq);

	return 0;
}

static int trDoIntermediate(ParallelTask *pt, int workerNo, int force)
{
	return 0;
}

static int trDoTidy(ParallelTask *pt, int workerNo, int tidyNo)
{
	return 0;
}


RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads)
{
	RoutingBuilder *rb=tiRoutingBuilderAlloc();

	ParallelTaskConfig *ptc=allocParallelTaskConfig(trDoRegister,trDoDeregister,trDoIngress,trDoIntermediate,trDoTidy,threads,
			TR_INGRESS_BLOCKSIZE, TR_INGRESS_PER_TIDY_MIN, TR_INGRESS_PER_TIDY_MAX, TR_TIDYS_PER_BACKOFF,
			16);//SMER_HASH_SLICES);

	rb->pt=allocParallelTask(ptc,rb);
	rb->graph=graph;

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


