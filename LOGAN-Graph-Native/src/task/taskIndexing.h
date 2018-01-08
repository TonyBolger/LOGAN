#ifndef __TASKINDEXING_H
#define __TASKINDEXING_H



#define TI_INGRESS_BLOCKSIZE 1000

#define TI_INGRESS_PER_TIDY_MIN 4
#define TI_INGRESS_PER_TIDY_MAX 256
#define TI_TIDYS_PER_BACKOFF 16

typedef struct indexingBuilderStr IndexingBuilder;

typedef struct iptThreadDataStr
{
	IndexingBuilder *indexingBuilder;
	int threadIndex;
} IptThreadData;

struct indexingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

	pthread_t *thread;
	IptThreadData *threadData;

};



IndexingBuilder *allocIndexingBuilder(Graph *graph, int threads);

void createIndexingBuilderWorkers(IndexingBuilder *ib);

void freeIndexingBuilder(IndexingBuilder *ib);



#endif
