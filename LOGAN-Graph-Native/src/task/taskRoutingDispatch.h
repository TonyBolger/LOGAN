#ifndef __TASKROUTINGDISPATCH_H
#define __TASKROUTINGDISPATCH_H


RoutingReadReferenceBlockDispatchArray *allocDispatchArray();

/*
void assignToDispatchArrayEntry(RoutingReadReferenceBlockDispatchArray *array, RoutingReadData *readData);

void initRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState);
void freeRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState);

int countNonEmptyDispatchGroups(RoutingBuilder *rb);
int countDispatchReadsRemaining(RoutingBuilder *rb);

*/
// Entry points for scanForAndDispatchLookupCompleteReadLookupBlocks

//int reserveReadDispatchBlock(RoutingBuilder *rb);
//void unreserveReadDispatchBlock(RoutingBuilder *rb);

//RoutingReadDispatchBlock *allocateReadDispatchBlock(RoutingBuilder *rb);
//void queueReadDispatchBlock(RoutingReadDispatchBlock *readBlock);

//void showReadDispatchBlocks(RoutingBuilder *rb);

// Entry points for trDoIntermediate
int scanForCompleteReadDispatchBlocks(RoutingBuilder *rb);
int scanForDispatches(RoutingBuilder *rb, int workerNo, RoutingWorkerState *wState, int force);

void queueDispatchArray(RoutingBuilder *rb, RoutingReadReferenceBlockDispatchArray *dispArray);

#endif
