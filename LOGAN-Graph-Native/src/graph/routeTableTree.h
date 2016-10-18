#ifndef __ROUTE_TABLE_TREE_H
#define __ROUTE_TABLE_TREE_H

// Route Table is loosely similar to a B-tree

// Route Table Root
// All modes: 0: prefix, 1: suffix
// Small mode:   4 in total, 2: forward, 3: reverse
// Medium mode: 16 in total, 2-8: forward, 9-15: reverse
// Large mode:  64 in total, 2-32: forward, 33-63: reverse
// Huge mode:  256 in total, 2-128: forward, 129-255: reverse

// Branches contain a min/max upstream id range. Leaves contain a precise upstream ID
//
// Minimum Valid tree: Small Root -> Leaf
//

#define ROUTE_TABLE_TREE_SMALLROOT_ENTRIES 1
#define ROUTE_TABLE_TREE_MEDIUMROOT_ENTRIES 7
#define ROUTE_TABLE_TREE_LARGEROOT_ENTRIES 31
#define ROUTE_TABLE_TREE_HUGEROOT_ENTRIES 127

#define ROUTE_TABLE_TREE_BRANCH_CHILDREN 256
#define ROUTE_TABLE_TREE_LEAF_ENTRIES 256

typedef struct rootTableGenericRootStr
{
	u8 *data[256];
} RouteTableRoot;

typedef struct routeTableSmallRootStr
{
	u8 *prefixData;
	u8 *suffixData;
	u8 *forwardRouteData[ROUTE_TABLE_TREE_SMALLROOT_ENTRIES];
	u8 *reverseRouteData[ROUTE_TABLE_TREE_SMALLROOT_ENTRIES];
} RouteTableSmallRoot;

typedef struct routeTableMediumRootStr
{
	u8 *prefixData;
	u8 *suffixData;
	u8 *forwardRouteData[ROUTE_TABLE_TREE_MEDIUMROOT_ENTRIES];
	u8 *reverseRouteData[ROUTE_TABLE_TREE_MEDIUMROOT_ENTRIES];
} RouteTableMediumRoot;

typedef struct routeTableLargeRootStr
{
	u8 *prefixData;
	u8 *suffixData;
	u8 *forwardRouteData[ROUTE_TABLE_TREE_LARGEROOT_ENTRIES];
	u8 *reverseRouteData[ROUTE_TABLE_TREE_LARGEROOT_ENTRIES];
} RouteTableLargeRoot;

typedef struct routeTableHugeRootStr
{
	u8 *prefixData;
	u8 *suffixData;
	u8 *forwardRouteData[ROUTE_TABLE_TREE_HUGEROOT_ENTRIES];
	u8 *reverseRouteData[ROUTE_TABLE_TREE_HUGEROOT_ENTRIES];
} RouteTableHugeRoot;

extern const s32 ROUTE_TABLE_ROOT_SIZE[];

typedef struct routeTableBranchStr
{
	s16 upstreamMin;
	s16 upstreamMax;
	u8 *childData[ROUTE_TABLE_TREE_BRANCH_CHILDREN];
} RouteTableBranch;

typedef struct routeTableLeafEntryStr
{
	s16 downstream;
	s16 width;
} RouteTableLeafEntry;

typedef struct routeTableLeafStr
{
	s16 upstream;
	RouteTableLeafEntry entries[ROUTE_TABLE_TREE_LEAF_ENTRIES];
} RouteTableLeaf;


typedef struct routeTableTreeElementProxyStr
{
	struct routeTableTreeElementProxyStr *next;

	struct routeTableTreeElementProxyStr *parentProxy; // if attached to branch, NULL if attached to root
	s32 inParentIndex; // Always starts at zero - Corrected for tails and other routing table

	u8 *blockData; // Set if the data in the GC heap, NULL if new

	RouteTableBranch *branchData; // Branches only: May be either temporary or GC heap memory
	struct routeTableTreeElementProxyStr **childProxies; // For branches only, created as needed
	s32 childProxyCount;

	RouteTableLeaf *leafData; // Leaves only: May be either temporary or GC heap memory
	s32 leafEntryCount;

	s32 headerDirty; // Need to rewrite header
	s32 contentDirty; // Need to rewrite header

} RouteTableTreeElementProxy;

typedef struct routeTableTreeProxyStr
{
	MemDispenser *disp;

	RouteTableTreeElementProxy *directChildren[ROUTE_TABLE_TREE_HUGEROOT_ENTRIES];
	s32 directChildrenCount;
	s32 contentDirty;

	RouteTableTreeElementProxy *newBranches;
	RouteTableTreeElementProxy *newLeaves;

} RouteTableTreeProxy;

typedef struct routeTableTreeWalkerStr
{
	RouteTableTreeProxy *treeProxy;
	s32 rootIndex; // Index in root

	RouteTableTreeElementProxy *firstBranchProxy; // If at least one branch level
	s32 firstBranchIndex;

	RouteTableTreeElementProxy *secondBranchProxy; // If two branch levels
	s32 secondBranchIndex;

	RouteTableTreeElementProxy *leafProxy; // If at least one leaf
	s32 leafIndex;

} RouteTableTreeWalker;


struct routeTableTreeBuilderStr
{
	MemDispenser *disp;

	u8 *rootBlockPtr;
	s32 rootSize;

	RouteTableTreeProxy forwardProxy;
	RouteTableTreeProxy reverseProxy;

	RouteTableTreeWalker forwardWalker;
	RouteTableTreeWalker reverseWalker;

	s32 newEntryCount;
};



void rttInitRouteTableTreeBuilder(RouteTableTreeBuilder *builder, HeapDataBlock *dataBlock, MemDispenser *disp);
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
