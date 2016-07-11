#ifndef __ROUTE_TABLE_H
#define __ROUTE_TABLE_H

#include "../common.h"

typedef struct routePatchStr
{
	s32 rdiIndex;

	s32 prefixIndex;
	s32 suffixIndex;
} RoutePatch;


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

void mergeRoutes(RouteTableBuilder *builder, RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount);

void unpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp);

#endif
