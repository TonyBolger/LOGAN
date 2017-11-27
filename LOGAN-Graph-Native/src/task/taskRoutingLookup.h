#ifndef __TASKROUTINGLOOKUP_H
#define __TASKROUTINGLOOKUP_H

int countLookupReadsRemaining(RoutingBuilder *rb);

// Entry point for trAllocateIngressSlot
int reserveReadLookupBlock(RoutingBuilder *rb);

// Entry points for trDoIntermediate
s32 queueIngressReadsForSmerLookup(RoutingBuilder *rb);



int scanForAndDispatchLookupCompleteReadLookupBlocks(RoutingBuilder *rb);
int scanForSmerLookups(RoutingBuilder *rb, int workerNo, RoutingWorkerState *wState);

#endif
