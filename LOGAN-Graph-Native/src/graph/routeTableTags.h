#ifndef __ROUTE_TABLE_TAGS_H
#define __ROUTE_TABLE_TAGS_H



struct routeTableTagBuilderStr
{
	MemDispenser *disp;

	u32 oldDataSize;

	RouteTableTag *oldForwardEntries;
	RouteTableTag *oldReverseEntries;

	RouteTableTag *newForwardEntries;
	RouteTableTag *newReverseEntries;

	u32 oldForwardEntryCount;
	u32 oldReverseEntryCount;
	u32 newForwardEntryCount;
	u32 newReverseEntryCount;

	s32 forwardPackedSize;
	s32 reversePackedSize;

};


u8 *rtgScanRouteTableTags(u8 *data);
u8 *rtgInitRouteTableTagBuilder(RouteTableTagBuilder *builder, u8 *data, MemDispenser *disp);

void rtgDumpRoutingTags(RouteTableTagBuilder *builder);

s32 rtgGetRouteTableTagBuilderDirty(RouteTableTagBuilder *builder);

s32 rtgGetRouteTableTagBuilderPackedSize(RouteTableTagBuilder *builder);
u8 *rtgWriteRouteTableTagBuilderPackedData(RouteTableTagBuilder *builder, u8 *data);

void rtgMergeForwardRoutes(RouteTableTagBuilder *builder, s32 forwardTagCount, RoutePatch *patchPtr, RoutePatch *endPatchPtr);
void rtgMergeReverseRoutes(RouteTableTagBuilder *builder, s32 reverseTagCount, RoutePatch *patchPtr, RoutePatch *endPatchPtr);

u8 *rtgUnpackRouteTableTagsForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp);

#endif

