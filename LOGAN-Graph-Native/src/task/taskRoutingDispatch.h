#ifndef __TASKROUTINGDISPATCH_H
#define __TASKROUTINGDISPATCH_H


RoutingDispatchArray *allocDispatchArray();
void assignToDispatchArrayEntry(RoutingDispatchArray *array, RoutingReadDispatchData *readData);

void initRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState);
void freeRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState);

// Entry points for scanForAndDispatchLookupCompleteReadLookupBlocks
int reserveReadDispatchBlock(RoutingBuilder *rb);
void unreserveReadDispatchBlock(RoutingBuilder *rb);

RoutingReadDispatchBlock *allocateReadDispatchBlock(RoutingBuilder *rb);
void queueReadDispatchBlock(RoutingReadDispatchBlock *readBlock);

void showReadDispatchBlocks(RoutingBuilder *rb);

// Entry points for trDoIntermediate
int scanForCompleteReadDispatchBlocks(RoutingBuilder *rb);
int scanForDispatches(RoutingBuilder *rb, int workerNo, int force);

void queueDispatchArray(RoutingBuilder *rb, RoutingDispatchArray *dispArray);

#endif
