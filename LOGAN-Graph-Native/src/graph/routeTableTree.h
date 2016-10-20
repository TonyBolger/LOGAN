#ifndef __ROUTE_TABLE_TREE_H
#define __ROUTE_TABLE_TREE_H

// Route Table is loosely similar to a B-tree

// Route Table Root: Contains block ptrs as follows:
// 2 tail blocks: prefix and suffix
// Forward Branch block
// Forward Leaf block
// Reverse Branch block
// Reverse Leaf block

// Branches contain a min/max upstream id range. Leaves contain a precise upstream ID
//
// Minimum Valid tree: Empty root

//

#define ROUTE_TABLE_TREE_ARRAY_ENTRIES 1024

#define ROUTE_TABLE_TREE_BRANCH_CHILDREN 64
#define ROUTE_TABLE_TREE_LEAF_ENTRIES 1024

/* Structs representing the various parts of the tree in heap-block format */

typedef struct rootTableTreeTopBlockStr
{
	u8 *prefixData;
	u8 *suffixData;

	u8 *forwardLeaves;   // Either a leaf array or a nested array, each to leaf arrays
	u8 *reverseLeaves;   // Either a leaf array or a nested array, each to leaf arrays

	u8 *forwardBranches; // Either a branch array or a nested array, each to branch arrays
	u8 *reverseBranches; // Either a branch array or a nested array, each to branch arrays

//	u8 *forwardOffsetIndex;
//	u8 *reverseOffsetIndex;
} __attribute__((packed)) RouteTableTreeTopBlock;


typedef struct routeTableTreeBranchBlockStr
{
	s16 childCount;
	s16 parentIndex;

	s16 upstreamMin;
	s16 upstreamMax;

	s16 childIndex[]; // Max is ROUTE_TABLE_TREE_BRANCH_CHILDREN
} __attribute__((packed)) RouteTableTreeBranchBlock;

typedef struct routeTableTreeLeafEntryStr
{
	s16 downstream;
	s16 width;
} __attribute__((packed)) RouteTableTreeLeafEntry;

typedef struct routeTableLeafBlockStr
{
	s16 entryCount;

	s16 parentIndex;
	s16 upstream;
	RouteTableTreeLeafEntry entries[]; // Max is ROUTE_TABLE_TREE_LEAF_ENTRIES
} __attribute__((packed)) RouteTableTreeLeafBlock;


typedef struct routeTableTreeOffsetBlockStr
{
	s16 entryCount;

	s16 upstreamMin;
	s16 upstreamCount;
	s16 downstreamMin;
	s16 downstreamCount;

	s32 offsets[];
} __attribute__((packed)) RouteTableTreeOffsetBlock;



typedef struct routeTableTreeArrayBlockStr
{
	u16 dataCount;
	u8 *data[];
} __attribute__((packed)) RouteTableTreeArrayBlock;






/* Structs wrapping the heap-format tree, allowing nicer manipulation */


typedef struct routeTableTreeProxyStr
{
	MemDispenser *disp;



//	RouteTableBranchArray *branchArray;
//	RouteTableLeafArray *leafArray;
} RouteTableTreeProxy;

typedef struct routeTableTreeWalkerStr
{
	RouteTableTreeProxy *treeProxy;

	s16 currentNodeIndex;
	s16 currentLeafEntry;

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
