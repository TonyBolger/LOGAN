#ifndef __TASKROUTINGDISPATCH_H
#define __TASKROUTINGDISPATCH_H


RoutingReadReferenceBlockDispatchArray *allocDispatchArray();


void assignToDispatchArrayEntry(RoutingReadReferenceBlockDispatchArray *array, u32 dispatchLinkIndex, DispatchLink *readData);

void initRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState);
void freeRoutingDispatchGroupState(RoutingDispatchGroupState *dispatchGroupState);

/*
int countNonEmptyDispatchGroups(RoutingBuilder *rb);
int countDispatchReadsRemaining(RoutingBuilder *rb);

*/

// Entry points for trDoIntermediate

int scanForDispatches(RoutingBuilder *rb, int workerNo, RoutingWorkerState *wState, int force);

void queueDispatchArray(RoutingBuilder *rb, RoutingReadReferenceBlockDispatchArray *dispArray);

#endif
