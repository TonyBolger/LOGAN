#ifndef __TASKROUTINGDISPATCH_H
#define __TASKROUTINGDISPATCH_H


RoutingReadReferenceBlockDispatchArray *allocDispatchArray(RoutingReadReferenceBlockDispatchArray *nextPtr);


void assignToDispatchArrayEntry(RoutingReadReferenceBlockDispatchArray *array, u32 dispatchLinkIndex, DispatchLink *readData);
RoutingReadReferenceBlockDispatchArray *cleanupRoutingDispatchArrays(RoutingReadReferenceBlockDispatchArray *in);

void initRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState);
void freeRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState);

/*
int countNonEmptyDispatchGroups(RoutingBuilder *rb);
int countDispatchReadsRemaining(RoutingBuilder *rb);

*/

// Entry points for trDoIntermediate

int scanForDispatches(RoutingBuilder *rb, u64 workerToken, int workerNo, RoutingWorkerState *wState, int force);

void queueDispatchArray(RoutingBuilder *rb, RoutingReadReferenceBlockDispatchArray *dispArray);

#endif
