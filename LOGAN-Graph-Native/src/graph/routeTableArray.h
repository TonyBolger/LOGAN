#ifndef __ROUTE_TABLE_ARRAY_H
#define __ROUTE_TABLE_ARRAY_H



struct routeTableArrayBuilderStr
{
	MemDispenser *disp;

	u32 oldDataSize;

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

};


u8 *rtaScanRouteTableArray(u8 *data);
u8 *rtaInitRouteTableArrayBuilder(RouteTableArrayBuilder *builder, u8 *data, MemDispenser *disp);

void rtaDumpRoutingTableArray(RouteTableArrayBuilder *builder);

s32 rtaGetRouteTableArrayBuilderDirty(RouteTableArrayBuilder *builder);

s32 rtaGetRouteTableArrayBuilderPackedSize(RouteTableArrayBuilder *builder);
u8 *rtaWriteRouteTableArrayBuilderPackedData(RouteTableArrayBuilder *builder, u8 *data);

void rtaMergeRoutes(RouteTableArrayBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 maxNewPrefix, s32 maxNewSuffix, RoutingReadData **orderedDispatches, MemDispenser *disp);

void rtaUnpackRouteTableArrayForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp);



#endif
