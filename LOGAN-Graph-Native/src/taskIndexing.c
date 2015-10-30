/*
 * taskIndexing.c
 *
 *  Created on: Oct 28, 2015
 *      Author: tony
 */

#include "common.h"




static void tiDoRegister(ParallelTask *pt, int workerNo)
{
	LOG("TaskIndexing: DoRegister (%i)",workerNo);
}

static void tiDoDeregister(ParallelTask *pt, int workerNo)
{
	LOG("TaskIndexing: DoDeregister (%i)",workerNo);
}

static int tiDoIngress(ParallelTask *pt, int workerNo, int ingressNo)
{
	LOG("TaskIndexing: DoIngress (%i)",workerNo);

	return 0;
}

static int tiDoIntermediate(ParallelTask *pt, int workerNo, int force)
{
	LOG("TaskIndexing: DoIntermediate (%i)",workerNo);

	return 0;
}

static int tiDoTidy(ParallelTask *pt, int workerNo, int tidyNo)
{
	LOG("TaskIndexing: DoTidy (%i)",workerNo);

	return 0;
}


IndexingBuilder *allocIndexingBuilder(Graph *graph, int threads)
{
	IndexingBuilder *ib=tiIndexingBuilderAlloc();

	int slices=0;

	ParallelTaskConfig *ptc=allocParallelTaskConfig(tiDoRegister,tiDoDeregister,tiDoIngress,tiDoIntermediate,tiDoTidy,threads,slices);
	ib->pt=allocParallelTask(ptc,NULL);

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


