#ifndef __TASKINDEXING_H
#define __TASKINDEXING_H



#define TI_INGRESS_BLOCKSIZE 10240

#define TI_INGRESS_PER_TIDY_MIN 1
#define TI_INGRESS_PER_TIDY_MAX 64
#define TI_TIDYS_PER_BACKOFF 4

typedef struct IndexingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

} IndexingBuilder;



IndexingBuilder *allocIndexingBuilder(Graph *graph, int threads);
void freeIndexingBuilder(IndexingBuilder *ib);



#endif
