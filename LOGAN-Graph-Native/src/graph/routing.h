#ifndef __ROUTING_H
#define __ROUTING_H





typedef struct routingReadReferenceBlockStr {
	u64 entryCount; // 8
	RoutingReadData **entries; // 8
} __attribute__((aligned (16))) RoutingReadReferenceBlock;




int rtRouteReadsForSmer(RoutingReadReferenceBlock *rdi, u32 sliceIndex, SmerArraySlice *slice, RoutingReadData **orderedDispatches, MemDispenser *disp, MemColHeap *colHeap);


SmerLinked *rtGetLinkedSmer(SmerArray *smerArray, SmerId rootSmerId, MemDispenser *disp);


#endif

