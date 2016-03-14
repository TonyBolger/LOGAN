#ifndef __TASKROUTINGLOOKUP_H
#define __TASKROUTINGLOOKUP_H

int reserveReadLookupBlock(RoutingBuilder *rb);

void queueReadsForSmerLookup(SwqBuffer *rec, int ingressPosition, int ingressSize, int nodeSize, RoutingBuilder *rb);

int scanForAndDispatchLookupCompleteReadLookupBlocks(RoutingBuilder *rb);
int scanForSmerLookups(RoutingBuilder *rb, int startSlice, int endSlice);

#endif
