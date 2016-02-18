
#include "cliCommon.h"

typedef struct tpfThreadDataStr
{
	Graph *graph;
	int threadCount;
	int threadIndex;
	char *pathTemplate;
} TpfThreadData;

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



#define FASTQ_MIN_READ_LENGTH 40
#define FASTQ_MAX_READ_LENGTH 10000

//#define FASTQ_RECORDS_PER_BATCH 100
//#define FASTQ_BASES_PER_BATCH 10000

//#define FASTQ_RECORDS_PER_BATCH 1000
//#define FASTQ_BASES_PER_BATCH 100000

//#define FASTQ_RECORDS_PER_BATCH 10000
//#define FASTQ_BASES_PER_BATCH 1000000

#define FASTQ_RECORDS_PER_BATCH 100000
#define FASTQ_BASES_PER_BATCH 10000000

// RECYCLE_BUFFER should be at least FASTQ_MAX_READ_LENGTH * 4

#define FASTQ_IO_RECYCLE_BUFFER (64*1024)
#define FASTQ_IO_PRIMARY_BUFFER (1024*1024)





void tpfAddPathSmersHandler(SwqBuffer *buffer, void *context)
{
	Graph *graph=context;

	//GraphBatchContext *ctx=context;

	int numRecords=buffer->numSequences;

	u8 *packedSeq=alloca(buffer->maxSequenceLength);
	int i=0;

	for(i=0;i<numRecords;i++)
		{
		SequenceWithQuality *currentRec=buffer->rec+i;

		packSequence(currentRec->seq, packedSeq, currentRec->length);
		addPathSmers(graph, currentRec->length, packedSeq);
		}
}

void tpfAddPathRoutesHandler(SequenceWithQuality *rec, int numRecords, void *context)
{
	/*
	GraphBatchContext *ctx=context;

	int i=0;

	u8 *packPtr=ctx->packedSequences;

	for(i=0;i<numRecords;i++)
		{
		FastqRecord *currentRec=rec+i;

		int packLen=packSequence(currentRec->seq, packPtr, currentRec->length);

		ctx->reads[i].readLength=currentRec->length;
		ctx->reads[i].packedSequence=packPtr;

		packPtr+=packLen;
		}

	ctx->numReads=numRecords;
	graphBatchAddPathRoutes(ctx);

	for(i=0;i<numRecords;i++)
		addPathRoutes(ctx->graph, ctx->reads[i].readLength, ctx->reads[i].packedSequence);
*/
}


void *tpfWorker(void *voidData)
{
	TpfThreadData *data=(TpfThreadData *)voidData;

	char path[1024];
	sprintf(path,data->pathTemplate,data->threadCount,data->threadIndex);

	LOG(LOG_INFO,"Thread %i of %i started for %s",data->threadIndex,data->threadCount,path);

	//char *path, int minSeqLength, int recordsToSkip, int recordsToUse, void *handlerContext, void (*handler)
/*
	int bufSize=FASTQ_BASES_PER_BATCH;

	SwqBuffer buffer;

	buffer.seqBuffer=malloc(bufSize);
	buffer.qualBuffer=malloc(bufSize);
	buffer.rec=malloc(sizeof(SequenceWithQuality)*FASTQ_RECORDS_PER_BATCH);

	buffer.maxSequenceTotalLength=bufSize;
	buffer.maxSequences=FASTQ_RECORDS_PER_BATCH;
	buffer.maxSequenceLength=FASTQ_MAX_READ_LENGTH;
	buffer.numSequences=0;
	buffer.usageCount=0;


	int reads=parseAndProcess(path, FASTQ_MIN_READ_LENGTH, 0, 250000000,
			&buffer, 1, data->graph, tpfAddPathSmersHandler);
	*/
//	int reads=parseAndProcess(path, minLength, 0, 1, smerCtx, addPathSmersHandler);


	//LOG(LOG_INFO,"Thread %i of %i processed %i",data->threadIndex,data->threadCount,reads);


	return NULL;
}



void runTpfMaster(char *pathTemplate, int fileCount, Graph *graph)
{
	pthread_t *threads=malloc(sizeof(pthread_t)*fileCount);
	TpfThreadData *data=malloc(sizeof(TpfThreadData)*fileCount);

	int i=0;
	for(i=0;i<fileCount;i++)
		{
		data[i].graph=graph;
		data[i].threadCount=fileCount;
		data[i].threadIndex=i;
		data[i].pathTemplate=pathTemplate;

		pthread_create(threads+i,NULL,tpfWorker,data+i);
		}


	void *status;

	for(i=0;i<fileCount;i++)
		pthread_join(threads[i], &status);
}







void *runIptWorker(void *voidData)
{
	IptThreadData *data=(IptThreadData *)voidData;
	IndexingBuilder *ib=data->indexingBuilder;

	//setitimer(ITIMER_PROF, &(data->profilingTimer), NULL);

	performTask(ib->pt);

	return NULL;
}


void iptHandler(SwqBuffer *buffer, void *context)
{
	IndexingBuilder *ib=(IndexingBuilder *)context;

	//LOG(LOG_INFO," ********************* IPT HANDLER GOT %i ****************************", buffer->numSequences);

	queueIngress(ib->pt, buffer, buffer->numSequences, &buffer->usageCount);
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

	SwqBuffer buffers[PT_INGRESS_BUFFERS];

	int bufSize=FASTQ_BASES_PER_BATCH;

	for(int i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		buffers[i].seqBuffer=malloc(bufSize);
		buffers[i].qualBuffer=malloc(bufSize);
		buffers[i].rec=malloc(sizeof(SequenceWithQuality)*FASTQ_RECORDS_PER_BATCH);

		buffers[i].maxSequenceTotalLength=bufSize;
		buffers[i].maxSequences=FASTQ_RECORDS_PER_BATCH;
		buffers[i].maxSequenceLength=FASTQ_MAX_READ_LENGTH;
		buffers[i].numSequences=0;
		buffers[i].usageCount=0;
		}

	u8 *ioBuffer=malloc(FASTQ_IO_RECYCLE_BUFFER+FASTQ_IO_PRIMARY_BUFFER);

	for(i=0;i<fileCount;i++)
		{
		char path[1024];
		sprintf(path,pathTemplate,fileCount,i);

		LOG(LOG_INFO,"Indexing: Parsing %s",path);

		int reads=parseAndProcess(path, FASTQ_MIN_READ_LENGTH, 0, 2000000000,
				ioBuffer, FASTQ_IO_RECYCLE_BUFFER, FASTQ_IO_PRIMARY_BUFFER,
				buffers, PT_INGRESS_BUFFERS,
				ib, iptHandler);

		LOG(LOG_INFO,"Indexing: Parsed %i reads from %s",reads,path);
		}


	queueShutdown(ib->pt);

	waitForShutdown(ib->pt);

	void *status;

	for(i=0;i<threadCount;i++)
		pthread_join(threads[i], &status);

	for(int i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		free(buffers[i].seqBuffer);
		free(buffers[i].qualBuffer);
		free(buffers[i].rec);
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

	performTask(rb->pt);

	return NULL;
}



void rptHandler(SwqBuffer *buffer, void *context)
{
	RoutingBuilder *rb=(RoutingBuilder *)context;

	//LOG(LOG_INFO," ********************* RPT HANDLER GOT %i ****************************", buffer->numSequences);

	queueIngress(rb->pt, buffer, buffer->numSequences, &buffer->usageCount);
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

	SwqBuffer buffers[PT_INGRESS_BUFFERS];

	int bufSize=FASTQ_BASES_PER_BATCH;

	for(int i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		buffers[i].seqBuffer=malloc(bufSize);
		buffers[i].qualBuffer=malloc(bufSize);
		buffers[i].rec=malloc(sizeof(SequenceWithQuality)*FASTQ_RECORDS_PER_BATCH);

		buffers[i].maxSequenceTotalLength=bufSize;
		buffers[i].maxSequences=FASTQ_RECORDS_PER_BATCH;
		buffers[i].maxSequenceLength=FASTQ_MAX_READ_LENGTH;
		buffers[i].numSequences=0;
		buffers[i].usageCount=0;
		}

	u8 *ioBuffer=malloc(FASTQ_IO_RECYCLE_BUFFER+FASTQ_IO_PRIMARY_BUFFER);



	for(i=0;i<fileCount;i++)
		{
		char path[1024];
		sprintf(path,pathTemplate,fileCount,i);

		LOG(LOG_INFO,"Routing: Parsing %s",path);

		int reads=parseAndProcess(path, FASTQ_MIN_READ_LENGTH, 0, 250000000,
				ioBuffer, FASTQ_IO_RECYCLE_BUFFER, FASTQ_IO_PRIMARY_BUFFER,
				buffers, PT_INGRESS_BUFFERS,
				rb, rptHandler);

		LOG(LOG_INFO,"Routing: Parsed %i reads from %s",reads,path);
		}


	queueShutdown(rb->pt);

	waitForShutdown(rb->pt);

	void *status;

	for(i=0;i<threadCount;i++)
		pthread_join(threads[i], &status);

	for(int i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		free(buffers[i].seqBuffer);
		free(buffers[i].qualBuffer);
		free(buffers[i].rec);
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
	int threadCount=0;

	if(argc==3)
		{
		fileTemplate=argv[1];
		fileCount=atoi(argv[2]);
		threadCount=fileCount;
		}
	else if(argc==4)
		{
		fileTemplate=argv[1];
		fileCount=atoi(argv[2]);
		threadCount=atoi(argv[3]);
		}
	else
		{
		LOG(LOG_CRITICAL,"Expected arguments: template files");
		return 1;
		}



	//runTpfMaster(fileTemplate, fileCount, graph);

	runIptMaster(fileTemplate, fileCount, threadCount, graph);

	LOG(LOG_INFO,"Smer count: %i",smGetSmerCount(&(graph->smerMap)));
	//smDumpSmerMap(&(graph->smerMap));

	switchMode(graph);

	runRptMaster(fileTemplate, fileCount, threadCount, graph);

	freeGraph(graph);

	return 0;
}

