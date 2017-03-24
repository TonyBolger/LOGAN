/*
 * taskIndexing.c
 *
 *  Created on: Oct 28, 2015
 *      Author: tony
 */

#include "common.h"



static void *tiDoRegister(ParallelTask *pt, int workerNo, int totalWorkers)
{
	return NULL;
}

static void tiDoDeregister(ParallelTask *pt, int workerNo, void *workerState)
{
	if(workerState!=NULL)
		gFree(workerState);

}

static int tiDoIngress(ParallelTask *pt, int workerNo, void *workerState, void *ingressPtr, int ingressPosition, int ingressSize)
{
	SwqBuffer *rec=ingressPtr;
	IndexingBuilder *ib=pt->dataPtr;

	u8 *packedSeq=malloc(PAD_BYTELENGTH_4BYTE(rec->maxSequenceTotalLength));
//	memset(packedSeq,0,PAD_BYTELENGTH_4BYTE(rec->maxSequenceTotalLength));

	int i=0;
	int ingressLast=ingressPosition+ingressSize;

	Graph *graph=ib->graph;
	SmerMap *smerMap=&(graph->smerMap);
	int nodeSize=graph->config.nodeSize;
	int sparseness=graph->config.sparseness;

	for(i=ingressPosition;i<ingressLast;i++)
		{
		SequenceWithQuality *currentRec=rec->rec+i;

		packSequence(currentRec->seq, packedSeq, currentRec->length);
		smAddPathSmers(smerMap, currentRec->length, packedSeq, nodeSize, sparseness);
		}

	free(packedSeq);

	return 1;
}

/*
static int tiDoIntermediate(ParallelTask *pt, int workerNo, int force)
{
	return 0;
}
*/

static int tiDoTidy(ParallelTask *pt, int workerNo, void *workerState, int tidyNo)
{
	IndexingBuilder *ib=pt->dataPtr;
	Graph *graph=ib->graph;

	smConsiderResize(&(graph->smerMap), tidyNo);

	return 0;
}



IndexingBuilder *allocIndexingBuilder(Graph *graph, int threads)
{
	IndexingBuilder *ib=tiIndexingBuilderAlloc();

	ParallelTaskConfig *ptc=allocParallelTaskConfig(tiDoRegister,tiDoDeregister,NULL,tiDoIngress,NULL,tiDoTidy,threads,
			TI_INGRESS_BLOCKSIZE,TI_INGRESS_PER_TIDY_MIN, TI_INGRESS_PER_TIDY_MAX, TI_TIDYS_PER_BACKOFF,
			SMER_SLICES);

	ib->pt=allocParallelTask(ptc,ib);
	ib->graph=graph;


	return ib;
}


void freeIndexingBuilder(IndexingBuilder *ib)
{
	ParallelTask *pt=ib->pt;
	ParallelTaskConfig *ptc=pt->config;

	freeParallelTask(pt);
	freeParallelTaskConfig(ptc);
	tiIndexingBuilderFree(ib);
}


