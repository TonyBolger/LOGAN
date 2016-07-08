#ifndef __ROUTE_TABLE_H
#define __ROUTE_TABLE_H

#include "../common.h"

typedef struct routeTableEntryStr
{
	s32 prefix;
	s32 suffix;
	s32 width;
} RouteTableEntry;


typedef struct routeTableBuilderStr
{
	MemDispenser *disp;

	RouteTableEntry *oldForwardRoutes;
	RouteTableEntry *oldReverseRoutes;

	RouteTableEntry *newForwardRoutes;
	RouteTableEntry *newReverseRoutes;

	s32 oldForwardRouteCount;
	s32 oldReverseRouteCount;
	s32 newForwardRouteCount;
	s32 newReverseRouteCount;

	s32 newForwardRouteAlloc;
	s32 newReverseRouteAlloc;

	s32 maxPrefix;
	s32 maxSuffix;
	s32 maxWidth;

	s32 totalPackedSize;

} RouteTableBuilder;



u8 *initRouteTableBuilder(RouteTableBuilder *builder, u8 *data, MemDispenser *disp);

s32 getRouteTableBuilderDirty(RouteTableBuilder *builder);
s32 getRouteTableBuilderPackedSize(RouteTableBuilder *builder);
u8 *writeRouteTableBuilderPackedData(RouteTableBuilder *builder, u8 *data);

s32 mergeRoutes(RouteTableBuilder *builder, SmerId smer, s32 tailLength);



#endif
