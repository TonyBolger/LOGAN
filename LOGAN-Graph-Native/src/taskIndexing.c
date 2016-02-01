/*
 * taskIndexing.c
 *
 *  Created on: Oct 28, 2015
 *      Author: tony
 */

#include "common.h"




static void tiDoRegister(ParallelTask *pt, int workerNo)
{
	LOG(LOG_INFO,"TaskIndexing: DoRegister (%i)",workerNo);
	sleep(1);
}

static void tiDoDeregister(ParallelTask *pt, int workerNo)
{
	LOG(LOG_INFO,"TaskIndexing: DoDeregister (%i)",workerNo);
	sleep(1);
}

static int tiDoIngress(ParallelTask *pt, int workerNo,void *ingressPtr, int ingressPosition, int ingressSize)
{
	LOG(LOG_INFO,"TaskIndexing: DoIngress (%i): %i %i",workerNo,ingressPosition, ingressSize);
	//sleep(1);
	return 0;
}

static int tiDoIntermediate(ParallelTask *pt, int workerNo, int force)
{
	LOG(LOG_INFO,"TaskIndexing: DoIntermediate (%i)",workerNo);
	sleep(1);
	return 0;
}

static int tiDoTidy(ParallelTask *pt, int workerNo, int tidyNo)
{
	LOG(LOG_INFO,"TaskIndexing: DoTidy (%i)",workerNo);
	//sleep(1);
	return 0;
}



IndexingBuilder *allocIndexingBuilder(Graph *graph, int threads)
{
	IndexingBuilder *ib=tiIndexingBuilderAlloc();

	ParallelTaskConfig *ptc=allocParallelTaskConfig(tiDoRegister,tiDoDeregister,tiDoIngress,tiDoIntermediate,tiDoTidy,threads,
			TI_INGRESS_BLOCKSIZE,TI_INGRESS_PER_TIDY_MIN, TI_INGRESS_PER_TIDY_MAX, TI_TIDYS_PER_BACKOFF,
			16);//SMER_HASH_SLICES);

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


