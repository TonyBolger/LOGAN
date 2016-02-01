#ifndef __TASKROUTING_H
#define __TASKROUTING_H


#define TR_INGRESS_BLOCKSIZE 1024

#define TR_INGRESS_PER_TIDY_MIN 1000000000
#define TR_INGRESS_PER_TIDY_MAX 1000000000
#define TR_TIDYS_PER_BACKOFF 1


typedef struct RoutingBuilderStr {

	ParallelTask *pt;


} RoutingBuilder;



RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads);
void freeRoutingBuilder(RoutingBuilder *rb);


#endif
