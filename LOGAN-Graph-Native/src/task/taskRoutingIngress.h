#ifndef __TASKROUTINGDISPATCH_H
#define __TASKROUTINGDISPATCH_H



s32 reserveReadIngressBlock(RoutingBuilder *rb);


void populateReadIngressBlock(SwqBuffer *rec, int ingressPosition, int ingressSize, int nodeSize, RoutingBuilder *rb);

#endif
