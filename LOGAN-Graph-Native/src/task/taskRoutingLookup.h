#ifndef __TASKROUTINGLOOKUP_H
#define __TASKROUTINGLOOKUP_H



void queueReadsForSmerLookup(SwqBuffer *rec, int ingressPosition, int ingressSize, int nodeSize, RoutingBuilder *rb);

int scanForAndDispatchLookupCompleteReadBlocks(RoutingBuilder *rb);
int scanForSmerLookups(RoutingBuilder *rb, int startSlice, int endSlice);

#endif
