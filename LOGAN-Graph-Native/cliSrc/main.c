
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

	pthread_t *threads=malloc(sizeof(pthread_t)*threadCount);

	IptThreadData *data=malloc(sizeof(IptThreadData)*threadCount);

	int i=0;
	for(i=0;i<threadCount;i++)
		{
		data[i].graph=graph;
		data[i].indexingBuilder=ib;

		data[i].threadIndex=i;
		pthread_create(threads+i,NULL,runIptWorker,data+i);
		}

	waitForStartup(ib->pt);


	LOG(LOG_INFO,"Indexing: Ready to parse");

	// Parse stuff here

	static SwqBuffer swqBuffers[PT_INGRESS_BUFFERS];
	static ParallelTaskIngress ingressBuffers[PT_INGRESS_BUFFERS];

	for(i=0;i<PT_INGRESS_BUFFERS;i++)
		initSequenceBuffer(swqBuffers+i, FASTQ_BASES_PER_BATCH, FASTQ_RECORDS_PER_BATCH, FASTQ_MAX_READ_LENGTH);

	u8 *ioBuffer=malloc(FASTQ_IO_RECYCLE_BUFFER+FASTQ_IO_PRIMARY_BUFFER);

	for(i=0;i<fileCount;i++)
		{
		char path[1024];
		sprintf(path,pathTemplate,fileCount,i);

		LOG(LOG_INFO,"Indexing: Parsing %s",path);

		int reads=parseAndProcess(path, FASTQ_MIN_READ_LENGTH, 0, 2000000000,
				ioBuffer, FASTQ_IO_RECYCLE_BUFFER, FASTQ_IO_PRIMARY_BUFFER,
				swqBuffers, ingressBuffers, PT_INGRESS_BUFFERS,
				ib, indexingBuilderDataHandler);

		LOG(LOG_INFO,"Indexing: Parsed %i reads from %s",reads,path);
		}


	queueShutdown(ib->pt);

	waitForShutdown(ib->pt);

	void *status;

	for(i=0;i<threadCount;i++)
		pthread_join(threads[i], &status);

	for(i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		freeSequenceBuffer(swqBuffers+i);
		if(*(ingressBuffers[i].ingressUsageCount)>0)
			LOG(LOG_INFO,"Buffer still in use");
		}

	freeIndexingBuilder(ib);

	free(ioBuffer);
	free(threads);
	free(data);
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

	pthread_t *threads=malloc(sizeof(pthread_t)*threadCount);

	RptThreadData *data=malloc(sizeof(RptThreadData)*threadCount);

	int i=0;
	for(i=0;i<threadCount;i++)
		{
		data[i].graph=graph;
		data[i].routingBuilder=rb;

		data[i].threadIndex=i;
		pthread_create(threads+i,NULL,runRptWorker,data+i);
		}

	waitForStartup(rb->pt);

	LOG(LOG_INFO,"Routing: Ready to parse");

	// Parse stuff here

	static SwqBuffer swqBuffers[PT_INGRESS_BUFFERS];
	static ParallelTaskIngress ingressBuffers[PT_INGRESS_BUFFERS];

	for(i=0;i<PT_INGRESS_BUFFERS;i++)
		initSequenceBuffer(swqBuffers+i, FASTQ_BASES_PER_BATCH, FASTQ_RECORDS_PER_BATCH, FASTQ_MAX_READ_LENGTH);

	u8 *ioBuffer=malloc(FASTQ_IO_RECYCLE_BUFFER+FASTQ_IO_PRIMARY_BUFFER);



	for(i=0;i<fileCount;i++)
		{
		char path[1024];
		sprintf(path,pathTemplate,fileCount,i);

		LOG(LOG_INFO,"Routing: Parsing %s",path);

		int reads=parseAndProcess(path, FASTQ_MIN_READ_LENGTH, 0, 2000000000,
				ioBuffer, FASTQ_IO_RECYCLE_BUFFER, FASTQ_IO_PRIMARY_BUFFER,
				swqBuffers, ingressBuffers, PT_INGRESS_BUFFERS,
				rb, routingBuilderDataHandler);

		LOG(LOG_INFO,"Routing: Parsed %i reads from %s",reads,path);
		}


	queueShutdown(rb->pt);

	waitForShutdown(rb->pt);

	void *status;

	for(i=0;i<threadCount;i++)
		pthread_join(threads[i], &status);

	for(i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		freeSequenceBuffer(swqBuffers+i);

		if(*(ingressBuffers[i].ingressUsageCount)>0)
			LOG(LOG_INFO,"Buffer still in use");
		}

	free(ioBuffer);
	free(threads);
	free(data);

	freeRoutingBuilder(rb);
}









int main(int argc, char **argv)
{
	logInit();

	Graph *graph=allocGraph(23,23,NULL);

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



	//runTpfMaster(fileTemplate, fileCount, graph);

	runIptMaster(fileTemplate, fileCount, threadCountIndexing, graph);

	LOG(LOG_INFO,"Smer count: %i",smGetSmerCount(&(graph->smerMap)));

	//smDumpSmerMap(&(graph->smerMap));

	switchMode(graph);

	runRptMaster(fileTemplate, fileCount, threadCountRouting, graph);

	freeGraph(graph);

	return 0;
}

