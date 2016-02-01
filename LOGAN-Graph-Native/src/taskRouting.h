#ifndef __TASKROUTING_H
#define __TASKROUTING_H


#define TR_INGRESS_BLOCKSIZE 1024


typedef struct RoutingBuilderStr {

	ParallelTask *pt;


} RoutingBuilder;



RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads);
void freeRoutingBuilder(RoutingBuilder *rb);


#endif
