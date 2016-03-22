#ifndef __TASKROUTINGDISPATCH_H
#define __TASKROUTINGDISPATCH_H


void initRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState);

// Entry points for scanForAndDispatchLookupCompleteReadLookupBlocks
int reserveReadDispatchBlock(RoutingBuilder *rb);
void unreserveReadDispatchBlock(RoutingBuilder *rb);

RoutingReadDispatchBlock *allocateReadDispatchBlock(RoutingBuilder *rb);
void queueReadDispatchBlock(RoutingReadDispatchBlock *readBlock);

// Entry points for trDoIntermediate
int scanForCompleteReadDispatchBlocks(RoutingBuilder *rb);
int scanForDispatches(RoutingBuilder *rb, int workerNo, int force);


#endif
