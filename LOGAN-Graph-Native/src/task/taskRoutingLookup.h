#ifndef __TASKROUTINGLOOKUP_H
#define __TASKROUTINGLOOKUP_H

int countLookupReadsRemaining(RoutingBuilder *rb);

RoutingReadLookupRecycleBlock *recycleLookupLink(RoutingBuilder *rb, RoutingReadLookupRecycleBlock *recycleBlock, s32 blockType, u32 lookupLinkIndex);
void flushRecycleBlock(RoutingBuilder *rb, RoutingReadLookupRecycleBlock *recycleBlock);

// Entry points for trDoIntermediate

int queueSmerLookupsForIngress(RoutingBuilder *rb, RoutingReadIngressBlock *ingressBlock);

int scanForAndDispatchLookupCompleteReadLookupBlocks(RoutingBuilder *rb);

int scanForAndProcessLookupRecycles(RoutingBuilder *rb, int force);

int scanForSmerLookups(RoutingBuilder *rb, int workerNo, RoutingWorkerState *wState);

#endif
