#ifndef __ROUTE_TABLE_H
#define __ROUTE_TABLE_H

#include "../common.h"


typedef struct routingReadDataStr {
	u8 *packedSeq; // 8
	u8 *quality; // 8
	u32 seqLength; // 4

	s32 indexCount; // 4
	s32 *completionCountPtr; // 8

	// Tracking of edge positions

	s32 minEdgePosition;
	s32 maxEdgePosition;

	// Split into aux structure with []. Add first/last.
	u32 *readIndexes; // 8
	SmerId *fsmers; // 8
	SmerId *rsmers; // 8
	u32 *slices; // 8
	u32 *sliceIndexes; // 8

} __attribute__((aligned (32))) RoutingReadData;


typedef struct routePatchStr
{
	struct routePatchStr *next;
	RoutingReadData **rdiPtr; // Needs double ptr to enable sorting by inbound position

	s32 prefixIndex;
	s32 suffixIndex;
} RoutePatch;


typedef struct routePatchMergeWideReadsetStr // Represents a set of reads with same upstream, flexible positions, but potentially varied downstream
{
	struct routePatchMergeWideReadsetStr *next;

	RoutePatch *firstRoutePatch;

	s32 minEdgeOffset;
	s32 maxEdgeOffset; // Closed interval, includes both max and min

} RoutePatchMergeWideReadset;


typedef struct routePatchMergePositionOrderedReadtreeStr // Represents sets of reads with same upstream and defined, consecutive, relative order.
{
	struct routePatchMergePositionOrderedReadtreeStr *next;

	RoutePatchMergeWideReadset *firstWideReadset;

	s32 minEdgePosition;
	s32 maxEdgePosition; // Closed interval, includes both max and min
} RoutePatchMergePositionOrderedReadtree;







typedef struct routeTableBuilderStr
{
	MemDispenser *disp;

	RouteTableEntry *oldForwardEntries;
	RouteTableEntry *oldReverseEntries;

	RouteTableEntry *newForwardEntries;
	RouteTableEntry *newReverseEntries;

	u32 oldForwardEntryCount;
	u32 oldReverseEntryCount;
	u32 newForwardEntryCount;
	u32 newReverseEntryCount;

	u32 newForwardEntryAlloc;
	u32 newReverseEntryAlloc;

	s32 maxPrefix;
	s32 maxSuffix;
	s32 maxWidth;

	s32 totalPackedSize;

} RouteTableBuilder;



u8 *initRouteTableBuilder(RouteTableBuilder *builder, u8 *data, MemDispenser *disp);

s32 getRouteTableBuilderDirty(RouteTableBuilder *builder);
s32 getRouteTableBuilderPackedSize(RouteTableBuilder *builder);
u8 *writeRouteTableBuilderPackedData(RouteTableBuilder *builder, u8 *data);

void mergeRoutes(RouteTableBuilder *builder, RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 maxNewPrefix, s32 maxNewSuffix, MemDispenser *disp);

void unpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp);

#endif
