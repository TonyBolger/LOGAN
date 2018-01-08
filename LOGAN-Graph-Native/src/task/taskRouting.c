/*
 * taskRouting.c
 *
 *  Created on: Oct 28, 2015
 *      Author: tony
 */

#include "common.h"




static void *trDoRegister(ParallelTask *pt, int workerNo, int totalWorkers)
{
	RoutingWorkerState *workerState=G_ALLOC(sizeof(RoutingWorkerState), MEMTRACKID_ROUTING_WORKERSTATE);

	workerState->lookupSliceStart=(SMER_SLICES*workerNo)/totalWorkers;
	workerState->lookupSliceEnd=(SMER_SLICES*(workerNo+1))/totalWorkers;

	workerState->dispatchGroupStart=(SMER_DISPATCH_GROUPS*workerNo)/totalWorkers;
	workerState->dispatchGroupEnd=(SMER_DISPATCH_GROUPS*(workerNo+1))/totalWorkers;

	LOG(LOG_INFO,"Worker %i of %i - Lookup Range %i %i DispatchGroup Range %i %i",
			workerNo,totalWorkers,
			workerState->lookupSliceStart, workerState->lookupSliceEnd,
			workerState->dispatchGroupStart, workerState->dispatchGroupEnd);


	//workerState->lookupSliceDefault=workerState->lookupSliceCurrent=(workerNo*SMER_SLICE_PRIME)&SMER_SLICE_MASK;
	//workerState->dispatchGroupDefault=workerState->dispatchGroupCurrent=(workerNo*SMER_DISPATCH_GROUP_PRIME)&SMER_DISPATCH_GROUP_MASK;

	return workerState;
}

static void trDoDeregister(ParallelTask *pt, int workerNo, void *wState)
{
	G_FREE(wState, sizeof(RoutingWorkerState), MEMTRACKID_ROUTING_WORKERSTATE);
}

static int trAllocateIngressSlot(ParallelTask *pt, int workerNo)
{
	RoutingBuilder *rb=pt->dataPtr;

	return reserveReadLookupBlock(rb);
}


static int trDoIngress(ParallelTask *pt, int workerNo,void *wState, void *ingressPtr, int ingressPosition, int ingressSize)
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
			if(readDispatchBlock->readData[i]->indexCount!=-1)
				{
				int indexCount=readDispatchBlock->readData[i]->indexCount;

				if(indexCount>=0)
					{
					LOG(LOG_INFO,"INCOMPLETE: Blocked read: %i at indexCount %i in slice %i",i,indexCount, readDispatchBlock->readData[i]->indexedData[indexCount].slice);
					}
				else
					{
					LOG(LOG_INFO,"INCOMPLETE: %i at indexCount %i",i,readDispatchBlock->readData[i]->indexCount);
					}
				}
			}
		}

}

static void dumpUncleanDispatchGroup(int groupNum, RoutingReadReferenceBlockDispatch *dispatchPtr, RoutingDispatchGroupState *dispatchGroupState)
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


static int trDoIntermediate(ParallelTask *pt, int workerNo, void *wState, int force)
{
	RoutingWorkerState *workerState=wState;
	RoutingBuilder *rb=pt->dataPtr;

/*
	if(__atomic_load_n(&rb->pogoSuppressionFlag, __ATOMIC_RELAXED))
		{
		__atomic_store_n(&rb->pogoSuppressionFlag, 0, __ATOMIC_RELAXED);

		int lookupReads=countLookupReadsRemaining(rb);
		int dispatchReads=countDispatchReadsRemaining(rb);

		int arlb=__atomic_load_n(&rb->allocatedReadLookupBlocks, __ATOMIC_SEQ_CST);
		int ardb=__atomic_load_n(&rb->allocatedReadDispatchBlocks, __ATOMIC_SEQ_CST);

		LOG(LOG_INFO,"Lookup %i (%i) Dispatch %i (%i) - force %i",lookupReads,arlb, dispatchReads, ardb, force);
		}
*/

	if(scanForCompleteReadDispatchBlocks(rb))
		{
		//LOG(LOG_INFO,"scanForCompleteReadDispatchBlocks OK");

		if(!force)
			return 1;
		}

	if(scanForAndDispatchLookupCompleteReadLookupBlocks(rb))
		{
		//LOG(LOG_INFO,"scanForAndDispatchLookupCompleteReadLookupBlocks OK");
		if(!force)
			return 1;
		}

	int arlb=__atomic_load_n(&rb->allocatedReadLookupBlocks, __ATOMIC_SEQ_CST);
	int ardb=__atomic_load_n(&rb->allocatedReadDispatchBlocks, __ATOMIC_SEQ_CST);

	if(ardb==TR_READBLOCK_DISPATCHES_INFLIGHT)
		{
		if(scanForDispatches(rb, workerNo, workerState, force))
			{
			//LOG(LOG_INFO,"scanForDispatches 1");
			return 1;
			}
		}


	//if((force)||(rb->allocatedReadLookupBlocks==TR_READBLOCK_LOOKUPS_INFLIGHT))
	if((force)||(arlb==TR_READBLOCK_LOOKUPS_INFLIGHT))
		{
		if(scanForSmerLookups(rb, workerNo, workerState))
			{
			//LOG(LOG_INFO,"scanForSmerLookups");
			return 1;
			}
		}

	if(force)
		{
		if(scanForDispatches(rb, workerNo, workerState, force))
			{
			//LOG(LOG_INFO,"scanForDispatches 2");
			return 1;
			}
		}

	arlb=__atomic_load_n(&rb->allocatedReadLookupBlocks, __ATOMIC_SEQ_CST);
	ardb=__atomic_load_n(&rb->allocatedReadDispatchBlocks, __ATOMIC_SEQ_CST);

//	LOG(LOG_INFO,"trDoIntermediate: %i %i %i %i",workerNo, force, arlb, ardb);

	if(arlb>0 || ardb>0) // If in force mode, and not finished, rally the minions
		return force;

	return 0;
}

/*
int trDoIntermediate_bork(ParallelTask *pt, int workerNo, void *wState, int force)
{
	RoutingWorkerState *workerState=wState;
	RoutingBuilder *rb=pt->dataPtr;

	if(__atomic_load_n(&rb->pogoSuppressionFlag, __ATOMIC_RELAXED))
		{
		int ret=scanForCompleteReadDispatchBlocks(rb);
		while(ret)
			ret=scanForCompleteReadDispatchBlocks(rb);

		if(ret)
			return 1;

//		int lookupReads=countLookupReadsRemaining(rb);
		int dispatchReads=countDispatchReadsRemaining(rb);

//		int arlb=__atomic_load_n(&rb->allocatedReadLookupBlocks, __ATOMIC_RELAXED);
		int ardb=__atomic_load_n(&rb->allocatedReadDispatchBlocks, __ATOMIC_RELAXED);

//		LOG(LOG_INFO,"Lookup %i (%i) Dispatch %i (%i) Limit %i %i",lookupReads,arlb, dispatchReads, ardb,
//				TR_POGOSUPRESSION_LOOKUP_TO_DISPATCH_READ_LIMIT, TR_POGOSUPRESSION_DISPATCH_PRIORITY_READ_LIMIT);

		if(dispatchReads<TR_POGOSUPRESSION_LOOKUP_TO_DISPATCH_READ_LIMIT && ardb<TR_READBLOCK_DISPATCHES_INFLIGHT)
			{
			ret=scanForAndDispatchLookupCompleteReadLookupBlocks(rb);

			while(ret)
				ret=scanForAndDispatchLookupCompleteReadLookupBlocks(rb);

			if(ret)
				return 1;

			//lookupReads=countLookupReadsRemaining(rb);
			dispatchReads=countDispatchReadsRemaining(rb);
			ardb=__atomic_load_n(&rb->allocatedReadDispatchBlocks, __ATOMIC_RELAXED);
			}

		if(dispatchReads>TR_POGOSUPRESSION_DISPATCH_PRIORITY_READ_LIMIT || ardb==TR_READBLOCK_DISPATCHES_INFLIGHT)
			__atomic_store_n(&rb->pogoPriorityFlag, 1, __ATOMIC_RELAXED);
		else
			__atomic_store_n(&rb->pogoPriorityFlag, 0, __ATOMIC_RELAXED);

		__atomic_store_n(&rb->pogoSuppressionFlag, 0, __ATOMIC_RELAXED);
		}

	int priority=__atomic_load_n(&rb->pogoPriorityFlag, __ATOMIC_RELAXED);

	if((force)||(priority==0))
		{
		if(scanForSmerLookups(rb, workerNo, workerState))
			{
			//LOG(LOG_INFO,"scanForSmerLookups");
			return 1;
			}
		}

	if((force) || (priority == 1))
		{
		if(scanForDispatches(rb, workerNo, workerState, force))
			{
			//LOG(LOG_INFO,"scanForDispatches 2");
			return 1;
			}
		}


	int arlb=__atomic_load_n(&rb->allocatedReadLookupBlocks, __ATOMIC_RELAXED);
	int ardb=__atomic_load_n(&rb->allocatedReadDispatchBlocks, __ATOMIC_RELAXED);

//	LOG(LOG_INFO,"trDoIntermediate: %i %i %i %i",workerNo, force, arlb, ardb);

	if(arlb>0 || ardb>0) // If in force mode, and not finished, rally the minions
		return force;

	return 0;
}
*/

static int trDoTidy(ParallelTask *pt, int workerNo, void *wState, int tidyNo)
{
	return 0;
}

static void trDoTickTock(ParallelTask *pt)
{
//	LOG(LOG_INFO,"Tick tock MF");

	RoutingBuilder *rb=pt->dataPtr;

	__atomic_store_n(&(rb->pogoDebugFlag), 1, __ATOMIC_RELAXED);
}


RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads)
{
	RoutingBuilder *rb=tiRoutingBuilderAlloc();

	ParallelTaskConfig *ptc=allocParallelTaskConfig(trDoRegister,trDoDeregister,trAllocateIngressSlot,trDoIngress,trDoIntermediate,trDoTidy,trDoTickTock,
			threads,
			TR_INGRESS_BLOCKSIZE, TR_INGRESS_PER_TIDY_MIN, TR_INGRESS_PER_TIDY_MAX, TR_TIDYS_PER_BACKOFF, 0);//SMER_HASH_SLICES);

	rb->pt=allocParallelTask(ptc,rb);
	rb->graph=graph;

	rb->thread=NULL;
	rb->threadData=NULL;

	rb->pogoDebugFlag=1;

	rb->allocatedReadLookupBlocks=0;
	for(int i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
		rb->readLookupBlocks[i].disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_LOOKUP, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);

	for(int i=0;i<SMER_SLICES;i++)
		rb->smerEntryLookupPtr[i]=NULL;

	rb->allocatedReadDispatchBlocks=0;
	for(int i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		rb->readDispatchBlocks[i].disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_DISPATCH, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		{
		initRoutingDispatchGroupState(rb->dispatchGroupState+i);
		rb->dispatchPtr[i]=NULL;
		}

	return rb;
}


void *runRptWorker(void *voidData)
{
	RptThreadData *data=(RptThreadData *)voidData;
	RoutingBuilder *rb=data->routingBuilder;

    //setitimer(ITIMER_PROF, &(data->profilingTimer), NULL);

	performTask_worker(rb->pt);

	return NULL;
}


void createRoutingBuilderWorkers(RoutingBuilder *rb)
{
	int threadCount=rb->pt->config->expectedThreads;

	rb->thread=G_ALLOC(sizeof(pthread_t)*threadCount, MEMTRACKID_THREADS);
	rb->threadData=G_ALLOC(sizeof(IptThreadData)*threadCount, MEMTRACKID_THREADS);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, THREAD_INDEXING_STACKSIZE);

	int i=0;
	for(i=0;i<threadCount;i++)
		{
		rb->threadData[i].routingBuilder=rb;
		rb->threadData[i].threadIndex=i;

		int err=pthread_create(rb->thread+i,&attr,runRptWorker,rb->threadData+i);

		if(err)
			{
			char errBuf[ERRORBUF];
			strerror_r(err,errBuf,ERRORBUF);

			LOG(LOG_CRITICAL,"Error %i (%s) creating worker thread %i",err,errBuf,i);
			}
		}
}




void freeRoutingBuilder(RoutingBuilder *rb)
{
	dumpUnclean(rb);

	for(int i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
		dispenserFree(rb->readLookupBlocks[i].disp);

	for(int i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		dispenserFree(rb->readDispatchBlocks[i].disp);

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		freeRoutingDispatchGroupState(rb->dispatchGroupState+i);

	ParallelTask *pt=rb->pt;
	ParallelTaskConfig *ptc=pt->config;

	int threadCount=rb->pt->config->expectedThreads;

	void *status;

	for(int i=0;i<threadCount;i++)
		pthread_join(rb->thread[i], &status);

	if(rb->thread!=NULL)
		G_FREE(rb->thread, sizeof(pthread_t)*threadCount, MEMTRACKID_THREADS);

	if(rb->threadData!=NULL)
		G_FREE(rb->threadData, sizeof(IptThreadData)*threadCount, MEMTRACKID_THREADS);

	freeParallelTask(pt);
	freeParallelTaskConfig(ptc);

	tiRoutingBuilderFree(rb);
}


