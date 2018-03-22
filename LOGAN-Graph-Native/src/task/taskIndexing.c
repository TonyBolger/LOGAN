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
		LOG(LOG_CRITICAL,"Unexpected non-NULL workerState during indexing");
}

static int tiDoIngress(ParallelTask *pt, int workerNo, void *workerState, void *ingressPtr, int ingressPosition, int ingressSize)
{
	SwqBuffer *rec=ingressPtr;
	IndexingBuilder *ib=pt->dataPtr;

	//int paddedLength=PAD_BYTELENGTH_4BYTE(rec->maxSequenceTotalLength);
	int paddedLength=PAD_BYTELENGTH_4BYTE(rec->maxSequenceLength);
	u8 *packedSeq=G_ALLOC(paddedLength, MEMTRACKID_INDEXING_PACKSEQ);

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

		packSequence(currentRec->seq, packedSeq, currentRec->seqLength);
		smAddPathSmers(smerMap, currentRec->seqLength, packedSeq, nodeSize, sparseness);
		}

	G_FREE(packedSeq, paddedLength, MEMTRACKID_INDEXING_PACKSEQ);

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

	ParallelTaskConfig *ptc=allocParallelTaskConfig(tiDoRegister,tiDoDeregister,NULL,tiDoIngress,NULL,tiDoTidy,NULL,
			threads,
			TI_INGRESS_BLOCKSIZE,TI_INGRESS_PER_TIDY_MIN, TI_INGRESS_PER_TIDY_MAX, TI_TIDYS_PER_BACKOFF,
			SMER_SLICES);

	ib->pt=allocParallelTask(ptc,ib);
	ib->graph=graph;

	ib->thread=NULL;
	ib->threadData=NULL;

	return ib;
}


static void *runIptWorker(void *voidData)
{
	IptThreadData *data=(IptThreadData *)voidData;
	IndexingBuilder *ib=data->indexingBuilder;

	//setitimer(ITIMER_PROF, &(data->profilingTimer), NULL);

	performTask_worker(ib->pt);

	return NULL;
}


void createIndexingBuilderWorkers(IndexingBuilder *ib)
{
	int threadCount=ib->pt->config->expectedThreads;

	ib->thread=G_ALLOC(sizeof(pthread_t)*threadCount, MEMTRACKID_THREADS);
	ib->threadData=G_ALLOC(sizeof(IptThreadData)*threadCount, MEMTRACKID_THREADS);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, THREAD_INDEXING_STACKSIZE);

	int i=0;
	for(i=0;i<threadCount;i++)
		{
		ib->threadData[i].indexingBuilder=ib;
		ib->threadData[i].threadIndex=i;

		int err=pthread_create(ib->thread+i,&attr,runIptWorker,ib->threadData+i);

		if(err)
			{
			char errBuf[ERRORBUF];
			strerror_r(err,errBuf,ERRORBUF);

			LOG(LOG_CRITICAL,"Error %i (%s) creating worker thread %i",err,errBuf,i);
			}
		}
}


void freeIndexingBuilder(IndexingBuilder *ib)
{
	ParallelTask *pt=ib->pt;
	ParallelTaskConfig *ptc=pt->config;

	int threadCount=ib->pt->config->expectedThreads;

	if(ib->thread!=NULL)
	    {
	    void *status;

   	    for(int i=0;i<threadCount;i++)
		pthread_join(ib->thread[i], &status);

	    G_FREE(ib->thread, sizeof(pthread_t)*threadCount, MEMTRACKID_THREADS);
	    }

	if(ib->threadData!=NULL)
		G_FREE(ib->threadData, sizeof(IptThreadData)*threadCount, MEMTRACKID_THREADS);

	freeParallelTask(pt);
	freeParallelTaskConfig(ptc);

	tiIndexingBuilderFree(ib);
}


