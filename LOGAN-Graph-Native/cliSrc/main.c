
#include "cliCommon.h"

typedef struct threadDataStr
{
	Graph *graph;
	int threadCount;
	int threadIndex;
	char *pathTemplate;
} ThreadData;


void addPathSmersHandler(FastqRecord *rec, int numRecords, void *context)
{
	GraphBatchContext *ctx=context;

	u8 *packedSeq=alloca(ctx->maxSeqLength);
	int i=0;

	for(i=0;i<numRecords;i++)
		{
		FastqRecord *currentRec=rec+i;

		packSequence(currentRec->seq, packedSeq, currentRec->length);
		addPathSmers(ctx->graph, currentRec->length, packedSeq);
		}
}

void addPathRoutesHandler(FastqRecord *rec, int numRecords, void *context)
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


void *runThread(void *voidData)
{
	ThreadData *data=(ThreadData *)voidData;

	char path[1024];
	sprintf(path,data->pathTemplate,data->threadCount,data->threadIndex);

	LOG(LOG_INFO,"Thread %i of %i started for %s",data->threadIndex,data->threadCount,path);

	int minLength=40;
	int maxLength=270;
	int batchSize=10000;

	//char *path, int minSeqLength, int recordsToSkip, int recordsToUse, void *handlerContext, void (*handler)
	GraphBatchContext *smerCtx=graphBatchInitContext(data->graph, maxLength,batchSize,GRAPH_MODE_INDEX);
	int reads=parseAndProcess(path, minLength, 0, 250000000, smerCtx, addPathSmersHandler);
//	int reads=parseAndProcess(path, minLength, 0, 1, smerCtx, addPathSmersHandler);
	graphBatchFreeContext(smerCtx);

	LOG(LOG_INFO,"Thread %i of %i processed %i",data->threadIndex,data->threadCount,reads);


	return NULL;
}



void runThreadPerFileImpl(char *fileTemplate, int fileCount, Graph *graph)
{
	pthread_t *threads=malloc(sizeof(pthread_t)*fileCount);
	ThreadData *data=malloc(sizeof(ThreadData)*fileCount);

	void *status;

	int i=0;
	for(i=0;i<fileCount;i++)
		{
		data[i].graph=graph;
		data[i].threadCount=fileCount;
		data[i].threadIndex=i;
		data[i].pathTemplate=fileTemplate;

		pthread_create(threads+i,NULL,runThread,data+i);
		}


	for(i=0;i<fileCount;i++)
		pthread_join(threads[i], &status);
}



void runParallelTaskImpl(char *fileTemplate, int fileCount, Graph *graph)
{
	int threadCount=fileCount;
	int hashSlices=16;

	IndexingBuilder *ib=allocIndexingBuilder(graph, threadCount, hashSlices);

	pthread_t *threads=malloc(sizeof(pthread_t)*fileCount);
	ThreadData *data=malloc(sizeof(ThreadData)*fileCount);



	freeIndexingBuilder(ib);
}




int main(int argc, char **argv)
{
	logInit();

	Graph *graph=allocGraph(23,23,NULL);

	if(argc!=3)
		{
		LOG(LOG_CRITICAL,"Expected arguments: template files");
		return 1;
		}

	char *fileTemplate=argv[1];
	int fileCount=atoi(argv[2]);


	//runThreadPerFileImpl(fileTemplate, fileCount, graph);
	runParallelTaskImpl(fileTemplate, fileCount, graph);

	LOG(LOG_INFO,"Smer count: %i",smGetSmerCount(&(graph->smerMap)));


	freeGraph(graph);

	return 0;
}

