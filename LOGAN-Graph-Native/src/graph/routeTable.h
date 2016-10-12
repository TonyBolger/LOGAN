#ifndef __ROUTE_TABLE_H
#define __ROUTE_TABLE_H

#include "../common.h"




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

	RouteTableArrayBuilder *arrayBuilder;

} RouteTableBuilder;




u8 *scanRouteTable(u8 *data);
u8 *initRouteTableBuilder(RouteTableBuilder *builder, u8 *data, MemDispenser *disp);

void dumpRoutingTable(RouteTableBuilder *builder);

s32 getRouteTableBuilderDirty(RouteTableBuilder *builder);

s32 getRouteTableBuilderPackedSize(RouteTableBuilder *builder);
u8 *writeRouteTableBuilderPackedData(RouteTableBuilder *builder, u8 *data);

void mergeRoutes(RouteTableBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 maxNewPrefix, s32 maxNewSuffix, RoutingReadData **orderedDispatches, MemDispenser *disp);

void unpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp);

#endif
