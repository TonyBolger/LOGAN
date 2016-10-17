#ifndef __ROUTE_TABLE_TREE_H
#define __ROUTE_TABLE_TREE_H

typedef struct rootTableGenericRootStr
{
	u8 *data[256];
} RootTableGenericRoot;

typedef struct routeTableSmallRootStr
{
	u8 *prefixData;
	u8 *suffixData;
	u8 *forwardRouteData[1];
	u8 *reverseRouteData[1];
} RouteTableSmallRoot;


typedef struct routeTableMediumRootStr
{
	u8 *prefixData;
	u8 *suffixData;
	u8 *forwardRouteData[6];
	u8 *reverseRouteData[6];
} RouteTableMediumRoot;

typedef struct routeTableLargeRootStr
{
	u8 *prefixData;
	u8 *suffixData;
	u8 *forwardRouteData[31];
	u8 *reverseRouteData[31];
} RouteTableLargeRoot;

typedef struct routeTableHugeRootStr
{
	u8 *prefixData;
	u8 *suffixData;
	u8 *forwardRouteData[127];
	u8 *reverseRouteData[127];

} RouteTableHugeRoot;


struct routeTableTreeBuilderStr
{
	MemDispenser *disp;

	RouteTableArrayBuilder *nestedBuilder;
	RouteTableSmallRoot *rootPtr;

};



u8 *rttScanRouteTableTree(u8 *data);
u8 *rttInitRouteTableTreeBuilder(RouteTableTreeBuilder *builder, u8 *routeData, MemDispenser *disp);
void rttUpgradeToRouteTableTreeBuilder(RouteTableArrayBuilder *arrayBuilder,  RouteTableTreeBuilder *treeBuilder, MemDispenser *disp);

void rttDumpRoutingTable(RouteTableTreeBuilder *builder);

s32 rttGetRouteTableTreeBuilderDirty(RouteTableTreeBuilder *builder);

s32 rttGetRouteTableTreeBuilderPackedSize(RouteTableTreeBuilder *builder);
u8 *rttWriteRouteTableTreeBuilderPackedData(RouteTableTreeBuilder *builder, u8 *data);

void rttMergeRoutes(RouteTableTreeBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 maxNewPrefix, s32 maxNewSuffix, RoutingReadData **orderedDispatches, MemDispenser *disp);

void rttUnpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp);




#endif
