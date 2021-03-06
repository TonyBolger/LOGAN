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

typedef struct routeTableArrayBufferStr
{
	 RouteTableEntry *oldEntryPtr;
	 RouteTableEntry *oldEntryPtrEnd;

	 RouteTableEntry *newEntryPtr;

	 s32 oldPrefix;
	 s32 oldSuffix;
	 s32 oldWidth;

	 s32 newPrefix;
	 s32 newSuffix;
	 s32 newWidth;

	 s32 totalNewWidth;
	 s32 maxWidth;
} RouteTableArrayBuffer;




u8 *rtaScanRouteTableArray(u8 *data);
u8 *rtaInitRouteTableArrayBuilder(RouteTableArrayBuilder *builder, u8 *data, MemDispenser *disp);

void rtaDumpRoutingTableArray(RouteTableArrayBuilder *builder);

s32 rtaGetRouteTableArrayBuilderDirty(RouteTableArrayBuilder *builder);

s32 rtaGetRouteTableArrayBuilderPackedSize(RouteTableArrayBuilder *builder);
s32 rtaGetNullRouteTableArrayPackedSize();

u8 *rtaWriteRouteTableArrayBuilderPackedData(RouteTableArrayBuilder *builder, u8 *data);
u8 *rtaWriteNullRouteTableArrayPackedData(u8 *data);


void rtaMergeRoutes(RouteTableArrayBuilder *builder, RouteTableTagBuilder *tagBuilder, RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches,
		s32 forwardRoutePatchCount, s32 reverseRoutePatchCount, s32 forwardTagCount, s32 reverseTagCount,
		s32 prefixCount, s32 suffixCount, u32 *orderedDispatches, MemDispenser *disp);

u8 *rtaUnpackRouteTableArrayForSmerLinked(SmerLinked *smerLinked, u8 *data, s64 routeLimit, MemDispenser *disp);

void rtaGetStats(RouteTableArrayBuilder *builder,
		s64 *routeTableForwardRouteEntriesPtr, s64 *routeTableForwardRoutesPtr, s64 *routeTableReverseRouteEntriesPtr, s64 *routeTableReverseRoutesPtr,
		s64 *routeTableArrayBytesPtr);

#endif
