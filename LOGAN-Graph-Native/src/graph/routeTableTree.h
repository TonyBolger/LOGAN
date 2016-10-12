#ifndef __ROUTE_TABLE_TREE_H
#define __ROUTE_TABLE_TREE_H

#include "../common.h"


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


typedef struct routeTableTreeBuilderStr
{
	MemDispenser *disp;


} RouteTableTreeBuilder;



#endif
