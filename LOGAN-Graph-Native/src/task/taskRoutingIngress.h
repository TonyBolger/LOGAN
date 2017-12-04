#ifndef __TASKROUTINGDISPATCH_H
#define __TASKROUTINGDISPATCH_H



s32 reserveReadIngressBlock(RoutingBuilder *rb);

s32 getAvailableReadIngress(RoutingBuilder *rb);
//s32 consumeReadIngress(RoutingBuilder *rb, s32 readsToConsume, u32 **sequenceLinkIndexPtr, RoutingReadIngressBlock **readBlockPtr);

void populateReadIngressBlock(SwqBuffer *rec, int ingressPosition, int ingressSize, int nodeSize, RoutingBuilder *rb);

s32 processIngressedReads(RoutingBuilder *rb);

#endif
