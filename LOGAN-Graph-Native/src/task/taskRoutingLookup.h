#ifndef __TASKROUTINGLOOKUP_H
#define __TASKROUTINGLOOKUP_H

// Entry point for trAllocateIngressSlot
int reserveReadLookupBlock(RoutingBuilder *rb);

// Entry point for trDoIngress
void queueReadsForSmerLookup(SwqBuffer *rec, int ingressPosition, int ingressSize, int nodeSize, RoutingBuilder *rb);

// Entry points for trDoIntermediate
int scanForAndDispatchLookupCompleteReadLookupBlocks(RoutingBuilder *rb);
int scanForSmerLookups(RoutingBuilder *rb, int workerNo);

#endif
