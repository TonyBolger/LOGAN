
#include "cliCommon.h"

typedef struct iptThreadDataStr
{
	Graph *graph;
	IndexingBuilder *indexingBuilder;

	int threadIndex;
} IptThreadData;

typedef struct rptThreadDataStr
{
	Graph *graph;
	RoutingBuilder *routingBuilder;

	int threadIndex;
} RptThreadData;








void *runIptWorker(void *voidData)
{
	IptThreadData *data=(IptThreadData *)voidData;
	IndexingBuilder *ib=data->indexingBuilder;

	//setitimer(ITIMER_PROF, &(data->profilingTimer), NULL);

	performTask_worker(ib->pt);

	return NULL;
}




void runIptMaster(char *pathTemplate, int fileCount, int threadCount, Graph *graph)
{
	IndexingBuilder *ib=allocIndexingBuilder(graph, threadCount);

	pthread_t *threads=G_ALLOC(sizeof(pthread_t)*threadCount, MEMTRACKID_THREADS);
	IptThreadData *data=G_ALLOC(sizeof(IptThreadData)*threadCount, MEMTRACKID_THREADS);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, THREAD_INDEXING_STACKSIZE);

	int i=0;
	for(i=0;i<threadCount;i++)
		{
		data[i].graph=graph;
		data[i].indexingBuilder=ib;

		data[i].threadIndex=i;

		int err=pthread_create(threads+i,&attr,runIptWorker,data+i);

		if(err)
			{
			char errBuf[ERRORBUF];
			strerror_r(err,errBuf,ERRORBUF);

			LOG(LOG_CRITICAL,"Error %i (%s) creating worker thread %i",err,errBuf,i);
			}
		}

	waitForStartup(ib->pt);


	LOG(LOG_INFO,"Indexing: Ready to parse");

	// Parse stuff here

	static SwqBuffer swqBuffers[PT_INGRESS_BUFFERS];
	static ParallelTaskIngress ingressBuffers[PT_INGRESS_BUFFERS];

	for(i=0;i<PT_INGRESS_BUFFERS;i++)
		initSequenceBuffer(swqBuffers+i, FASTQ_BASES_PER_BATCH, FASTQ_RECORDS_PER_BATCH, FASTQ_MAX_READ_LENGTH);

	u8 *ioBuffer=G_ALLOC(FASTQ_IO_RECYCLE_BUFFER+FASTQ_IO_PRIMARY_BUFFER, MEMTRACKID_IOBUF);

	void (*monitor)() = NULL;

#ifdef FEATURE_ENABLE_MEMTRACK
	monitor=mtDump;
#endif

	for(i=0;i<fileCount;i++)
		{
		char path[1024];
		sprintf(path,pathTemplate,fileCount,i);

		LOG(LOG_INFO,"Indexing: Parsing %s",path);

		int reads=parseAndProcess(path, FASTQ_MIN_READ_LENGTH, 0, 2000000000,
				ioBuffer, FASTQ_IO_RECYCLE_BUFFER, FASTQ_IO_PRIMARY_BUFFER,
				swqBuffers, ingressBuffers, PT_INGRESS_BUFFERS,
				ib, indexingBuilderDataHandler, monitor);

		LOG(LOG_INFO,"Indexing: Parsed %i reads from %s",reads,path);
		}


	#ifdef FEATURE_ENABLE_MEMTRACK
		mtDump();
	#endif

	queueShutdown(ib->pt);

	waitForShutdown(ib->pt);

	void *status;

	for(i=0;i<threadCount;i++)
		pthread_join(threads[i], &status);

	for(i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		freeSequenceBuffer(swqBuffers+i);
		if(ingressBuffers[i].ingressUsageCount!=NULL && *(ingressBuffers[i].ingressUsageCount)>0)
			LOG(LOG_INFO,"Buffer still in use");
		}

	freeIndexingBuilder(ib);

	G_FREE(ioBuffer, FASTQ_IO_RECYCLE_BUFFER+FASTQ_IO_PRIMARY_BUFFER, MEMTRACKID_IOBUF);
	G_FREE(threads, sizeof(pthread_t)*threadCount, MEMTRACKID_THREADS);
	G_FREE(data, sizeof(IptThreadData)*threadCount, MEMTRACKID_THREADS);

}


void *runRptWorker(void *voidData)
{
	RptThreadData *data=(RptThreadData *)voidData;
	RoutingBuilder *rb=data->routingBuilder;

    //setitimer(ITIMER_PROF, &(data->profilingTimer), NULL);

	performTask_worker(rb->pt);

	return NULL;
}






void runRptMaster(char *pathTemplate, int fileCount, int threadCount, Graph *graph)
{
	RoutingBuilder *rb=allocRoutingBuilder(graph, threadCount);

	pthread_t *threads=G_ALLOC(sizeof(pthread_t)*threadCount, MEMTRACKID_THREADS);
	RptThreadData *data=G_ALLOC(sizeof(RptThreadData)*threadCount, MEMTRACKID_THREADS);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,THREAD_ROUTING_STACKSIZE);

	int i=0;
	for(i=0;i<threadCount;i++)
		{
		data[i].graph=graph;
		data[i].routingBuilder=rb;

		data[i].threadIndex=i;
		int err=pthread_create(threads+i,&attr,runRptWorker,data+i);

		if(err)
			{
			char errBuf[ERRORBUF];
			strerror_r(err,errBuf,ERRORBUF);

			LOG(LOG_CRITICAL,"Error %i (%s) creating worker thread %i",err,errBuf,i);
			}
		}

	waitForStartup(rb->pt);

	LOG(LOG_INFO,"Routing: Ready to parse");

	// Parse stuff here

	static SwqBuffer swqBuffers[PT_INGRESS_BUFFERS];
	static ParallelTaskIngress ingressBuffers[PT_INGRESS_BUFFERS];

	for(i=0;i<PT_INGRESS_BUFFERS;i++)
		initSequenceBuffer(swqBuffers+i, FASTQ_BASES_PER_BATCH, FASTQ_RECORDS_PER_BATCH, FASTQ_MAX_READ_LENGTH);

	u8 *ioBuffer=G_ALLOC(FASTQ_IO_RECYCLE_BUFFER+FASTQ_IO_PRIMARY_BUFFER, MEMTRACKID_IOBUF);

	void (*monitor)() = NULL;

#ifdef FEATURE_ENABLE_MEMTRACK
	monitor=mtDump;
#endif

	for(i=0;i<fileCount;i++)
		{
		char path[1024];
		sprintf(path,pathTemplate,fileCount,i);

		LOG(LOG_INFO,"Routing: Parsing %s",path);

		int reads=parseAndProcess(path, FASTQ_MIN_READ_LENGTH, 0, 2000000000,
				ioBuffer, FASTQ_IO_RECYCLE_BUFFER, FASTQ_IO_PRIMARY_BUFFER,
				swqBuffers, ingressBuffers, PT_INGRESS_BUFFERS,
				rb, routingBuilderDataHandler, monitor);

		LOG(LOG_INFO,"Routing: Parsed %i reads from %s",reads,path);
		}


	#ifdef FEATURE_ENABLE_MEMTRACK
		mtDump();
	#endif

	queueShutdown(rb->pt);

	waitForShutdown(rb->pt);

	void *status;

	for(i=0;i<threadCount;i++)
		pthread_join(threads[i], &status);

	for(i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		freeSequenceBuffer(swqBuffers+i);

		if(ingressBuffers[i].ingressUsageCount!=NULL && *(ingressBuffers[i].ingressUsageCount)>0)
			LOG(LOG_INFO,"Buffer still in use");
		}

	G_FREE(ioBuffer, FASTQ_IO_RECYCLE_BUFFER+FASTQ_IO_PRIMARY_BUFFER, MEMTRACKID_IOBUF);
	G_FREE(threads, sizeof(pthread_t)*threadCount, MEMTRACKID_THREADS);
	G_FREE(data, sizeof(RptThreadData)*threadCount, MEMTRACKID_THREADS);

	freeRoutingBuilder(rb);
}









int main(int argc, char **argv)
{
	logInit();

	char *fileTemplate=NULL;
	int fileCount=0;
	int threadCountIndexing=0,threadCountRouting=0;

	if(argc==3)
		{
		fileTemplate=argv[1];
		fileCount=atoi(argv[2]);
		threadCountIndexing=threadCountRouting=fileCount;
		}
	else if(argc==4)
		{
		fileTemplate=argv[1];
		fileCount=atoi(argv[2]);
		threadCountIndexing=threadCountRouting=atoi(argv[3]);
		}
	else if(argc==5)
		{
		fileTemplate=argv[1];
		fileCount=atoi(argv[2]);
		threadCountIndexing=atoi(argv[3]);
		threadCountRouting=atoi(argv[4]);
		}
	else
		{
		LOG(LOG_CRITICAL,"Expected arguments: template files");
		return 1;
		}
/*
	LOG(LOG_INFO,"RoutingReadLookupData: %i (20) RoutingReadData: %i (20) RoutingReadIndexedDataEntry: %i (28)",
			sizeof(RoutingReadLookupData), sizeof(RoutingReadData), sizeof(RoutingReadIndexedDataEntry));

	LOG(LOG_INFO,"SequenceLink: %i (64) LookupLink: %i (128) DispatchLink: %i (128)",
			sizeof(SequenceLink), sizeof(LookupLink), sizeof(DispatchLink));
*/

	LOG(LOG_INFO,"MemSingleBrickChunk: %i (2097152) MemDoubleBrickChunk %i (4194304)", sizeof(MemSingleBrickChunk), sizeof(MemDoubleBrickChunk));


#ifdef FEATURE_ENABLE_MEMTRACK
	mtInit();
	mtDump();
#endif

	Graph *graph=grAllocGraph(23,23,NULL);

	//runTpfMaster(fileTemplate, fileCount, graph);

#ifdef FEATURE_ENABLE_MEMTRACK
	mtDump();
#endif

	runIptMaster(fileTemplate, fileCount, threadCountIndexing, graph);

#ifdef FEATURE_ENABLE_MEMTRACK
	mtDump();
#endif

	LOG(LOG_INFO,"Smer count: %i",smGetSmerCount(&(graph->smerMap)));

	//smDumpSmerMap(&(graph->smerMap));

	grSwitchMode(graph);

#ifdef FEATURE_ENABLE_MEMTRACK
	mtDump();
#endif

	runRptMaster(fileTemplate, fileCount, threadCountRouting, graph);

#ifdef FEATURE_ENABLE_MEMTRACK
	mtDump();
//	LOG(LOG_INFO,"Graph construction complete");
//	sleep(60);

#endif

	grFreeGraph(graph);

#ifdef FEATURE_ENABLE_MEMTRACK
	mtDump();
#endif

	return 0;
}

