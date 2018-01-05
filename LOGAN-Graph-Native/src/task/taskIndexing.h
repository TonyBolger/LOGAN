#ifndef __TASKINDEXING_H
#define __TASKINDEXING_H



#define TI_INGRESS_PER_TIDY_MIN 2
#define TI_INGRESS_PER_TIDY_MAX 256
#define TI_TIDYS_PER_BACKOFF 16

typedef struct IndexingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

} IndexingBuilder;



IndexingBuilder *allocIndexingBuilder(Graph *graph, int threads);
void freeIndexingBuilder(IndexingBuilder *ib);



#endif
