#ifndef __ROUTE_TABLE_TAGS_H
#define __ROUTE_TABLE_TAGS_H


typedef struct routeTableTagStr
{
	u8 *tagData;
	s32 tailIndex;
	s32 position;

} RouteTableTag;

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

	u32 newForwardEntryAlloc;
	u32 newReverseEntryAlloc;

	s32 totalPackedSize;

};


u8 *rtgScanRouteTableTags(u8 *data);
u8 *rtgInitRouteTableTagBuilder(RouteTableTagBuilder *builder, u8 *data, MemDispenser *disp);

void rtgDumpRoutingTags(RouteTableTagBuilder *builder);

s32 rtgGetRouteTableTagBuilderDirty(RouteTableTagBuilder *builder);

s32 rtgGetRouteTableTagBuilderPackedSize(RouteTableTagBuilder *builder);
u8 *rtgWriteRouteTableTagBuilderPackedData(RouteTableTagBuilder *builder, u8 *data);

void rtgMergeForwardRoutes(RouteTableTagBuilder *builder, RoutePatch *patchPtr, RoutePatch *endPatchPtr);
void rtgMergeReverseRoutes(RouteTableTagBuilder *builder, RoutePatch *patchPtr, RoutePatch *endPatchPtr);

#endif

