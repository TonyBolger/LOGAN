#ifndef __ROUTE_TABLE_TAGS_H
#define __ROUTE_TABLE_TAGS_H



struct routeTableTagBuilderStr
{
	MemDispenser *disp;

	u32 oldDataSize;

	RouteTableTag *oldForwardTags;
	RouteTableTag *oldReverseTags;

	RouteTableTag *newForwardTags;
	RouteTableTag *newReverseTags;

	u32 oldForwardTagCount;
	u32 oldReverseTagCount;
	u32 newForwardTagCount;
	u32 newReverseTagCount;

	s32 forwardPackedSize;
	s32 reversePackedSize;

};


u8 *rtgScanRouteTableTags(u8 *data);
u8 *rtgInitRouteTableTagBuilder(RouteTableTagBuilder *builder, u8 *data, MemDispenser *disp);

void rtgDumpRoutingTags(RouteTableTagBuilder *builder);

s32 rtgGetRouteTableTagBuilderDirty(RouteTableTagBuilder *builder);

s32 rtgGetRouteTableTagBuilderPackedSize(RouteTableTagBuilder *builder);
s32 rtgGetNullRouteTableTagPackedSize();

u8 *rtgWriteRouteTableTagBuilderPackedData(RouteTableTagBuilder *builder, u8 *data);
u8 *rtgWriteNullRouteTableTagPackedData(u8 *data);

void rtgMergeForwardRoutes(RouteTableTagBuilder *builder, s32 forwardTagCount, RoutePatch *patchPtr, RoutePatch *endPatchPtr);
void rtgMergeReverseRoutes(RouteTableTagBuilder *builder, s32 reverseTagCount, RoutePatch *patchPtr, RoutePatch *endPatchPtr);

u8 *rtgUnpackRouteTableTagsForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp);

#endif

