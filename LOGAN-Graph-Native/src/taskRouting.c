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

static int trDoIngress(ParallelTask *pt, int workerNo,void *ingressPtr, int ingressPosition, int ingressSize)
{
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

	ParallelTaskConfig *ptc=allocParallelTaskConfig(trDoRegister,trDoDeregister,trDoIngress,trDoIntermediate,trDoTidy,threads,TR_INGRESS_BLOCKSIZE,16);//SMER_HASH_SLICES);

	rb->pt=allocParallelTask(ptc,NULL);

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


