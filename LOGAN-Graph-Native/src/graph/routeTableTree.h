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

// Two-level trees will need more than s16 indexes (currently 256*1024 possible leaves?)

#define ROUTE_TABLE_TREE_PTR_ARRAY_ENTRIES 256
#define ROUTE_TABLE_TREE_DATA_ARRAY_ENTRIES 1024
#define ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK 4

#define ROUTE_TABLE_TREE_BRANCH_CHILDREN 64
#define ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK 8
//#define ROUTE_TABLE_TREE_BRANCH_CHILDREN 4
//#define ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK 4
//#define ROUTE_TABLE_TREE_BRANCH_CHILDREN 2
//#define ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK 2

#define ROUTE_TABLE_TREE_LEAF_ENTRIES 1024
#define ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK 16
//#define ROUTE_TABLE_TREE_LEAF_ENTRIES 4
//#define ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK 4
//#define ROUTE_TABLE_TREE_LEAF_ENTRIES 2
//#define ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK 2

/* Structs representing the various parts of the tree in heap-block format */

typedef struct rootTableTreeTopBlockStr
{
	u8 *data[8]; // Tail(P,S), Leaf(F,R), Branch(F,R), Offset(F,R)
} __attribute__((packed)) RouteTableTreeTopBlock;

#define ROUTE_TOPINDEX_DIRECT -2
#define ROUTE_TOPINDEX_TOP -1
#define ROUTE_TOPINDEX_PREFIX 0
#define ROUTE_TOPINDEX_SUFFIX 1
#define ROUTE_TOPINDEX_FORWARD_LEAF 2
#define ROUTE_TOPINDEX_REVERSE_LEAF 3
#define ROUTE_TOPINDEX_FORWARD_BRANCH 4
#define ROUTE_TOPINDEX_REVERSE_BRANCH 5
#define ROUTE_TOPINDEX_FORWARD_OFFSET 6
#define ROUTE_TOPINDEX_REVERSE_OFFSET 7

#define ROUTE_TOPINDEX_MAX 8

// Tree-wide Indexes of nodes:
// Brindex = Branch Index, positive, starting from 0 (root)
// Lindex = Leaf Index, positive, starting from 0
// Nindex = Node Index, positive for branches, 0 for root, negative for leaves, -32k for invalid
//
// Sibdex = Sibling Index, position within the parent branch

#define BRANCH_NINDEX_ROOT 0
#define BRANCH_NINDEX_INVALID (-32768)

#define LINDEX_TO_NINDEX(LINDEX) (-(LINDEX)-1)
#define NINDEX_TO_LINDEX(NINDEX) (-(NINDEX)-1)


// For now, all trees use the same size node indexes (s16), tail indexes (s16), width (s16)

typedef struct routeTableTreeBranchBlockStr
{
	s16 childAlloc;
	s16 parentBrindex;

	s16 upstreamMin;
	s16 upstreamMax;

	s16 childNindex[]; // Max is ROUTE_TABLE_TREE_BRANCH_CHILDREN
} __attribute__((packed)) RouteTableTreeBranchBlock;


typedef struct routeTableTreeLeafEntryStr
{
	s16 downstream;
	s16 width;
} __attribute__((packed)) RouteTableTreeLeafEntry;

typedef struct routeTableLeafBlockStr
{
	s16 entryAlloc;
	s16 parentBrindex;

	s16 upstream;
	RouteTableTreeLeafEntry entries[]; // Max is ROUTE_TABLE_TREE_LEAF_ENTRIES
} __attribute__((packed)) RouteTableTreeLeafBlock;


typedef struct routeTableTreeOffsetBlockStr
{
	s16 entryAlloc;

	s16 upstreamMin;
	s16 upstreamCount;
	s16 downstreamMin;
	s16 downstreamCount;

	s32 offsets[];
} __attribute__((packed)) RouteTableTreeOffsetBlock;



typedef struct routeTableTreeArrayBlockStr
{
	u16 dataAlloc;
	u8 *data[];
} __attribute__((packed)) RouteTableTreeArrayBlock;



/* Structs wrapping the heap-format tree, allowing nicer manipulation */

// Possible Array Formats:
// 		Single Level: Shallow Ptr -> Shallow Data (1B sub) [1024]
//      Two Level: Shallow Ptr-> Deep Ptr (1B sub) [256] -> Deep Data (2B sub) [1024]


typedef struct routeTableTreeLeafProxyStr
{
	RouteTableTreeLeafBlock *dataBlock;
	s16 lindex;

	u16 entryAlloc;
	u16 entryCount;

} RouteTableTreeLeafProxy;

typedef struct routeTableTreeBranchProxyStr
{
	RouteTableTreeBranchBlock *dataBlock;
	s16 brindex;

	u16 childAlloc;
	u16 childCount;

} RouteTableTreeBranchProxy;


typedef struct routeTableTreeArrayProxyStr
{
	RouteTableTreeArrayBlock *ptrBlock; // If using two levels
	u16 ptrAlloc;
	u16 ptrCount;

	RouteTableTreeArrayBlock *dataBlock; // With Headers.
	u16 dataAlloc;
	u16 dataCount;

	u8 **newData; // Temporary space for new stuff (no headers)
	u16 newDataAlloc;
	u16 newDataCount;

	HeapDataBlock *heapDataBlock;
	u32 arrayType;
} RouteTableTreeArrayProxy;




typedef struct routeTableTreeProxyStr
{
	MemDispenser *disp;

	RouteTableTreeArrayProxy leafArrayProxy;
	RouteTableTreeArrayProxy branchArrayProxy;
	RouteTableTreeArrayProxy offsetArrayProxy;

	RouteTableTreeBranchProxy *rootProxy;

} RouteTableTreeProxy;

typedef struct routeTableTreeWalkerStr
{
	RouteTableTreeProxy *treeProxy;

	// Current Position
	RouteTableTreeBranchProxy *branchProxy;
	s16 branchChildSibdex;
	RouteTableTreeLeafProxy *leafProxy;
	s16 leafEntry;

	s32 upstreamOffsetCount;
	s32 downstreamOffsetCount;
	s32 *upstreamOffsets;
	s32 *downstreamOffsets;


} RouteTableTreeWalker;


struct routeTableTreeBuilderStr
{
	MemDispenser *disp;

	//HeapDataBlock *topDataBlock;

	HeapDataBlock dataBlocks[8];

	RouteTableTreeProxy forwardProxy;
	RouteTableTreeProxy reverseProxy;

	RouteTableTreeWalker forwardWalker;
	RouteTableTreeWalker reverseWalker;

	s32 newEntryCount;
};



void rttInitRouteTableTreeBuilder(RouteTableTreeBuilder *builder, RouteTableTreeTopBlock *top);
void rttUpgradeToRouteTableTreeBuilder(RouteTableArrayBuilder *arrayBuilder,  RouteTableTreeBuilder *treeBuilder, MemDispenser *disp);

void rttDumpRoutingTable(RouteTableTreeBuilder *builder);

//s32 rttGetRouteTableTreeBuilderDirty(RouteTableTreeBuilder *builder);

void rttBindBlockArrayProxy(RouteTableTreeArrayProxy *arrayProxy, u8 *heapDataPtr, u32 headerSize);

s32 rttGetTopArrayDirty(RouteTableTreeArrayProxy *arrayProxy);
s32 rttGetTopArraySize(RouteTableTreeArrayProxy *arrayProxy);

s32 rttMergeTopArrayUpdates_accumulateSize(RouteTableTreeArrayProxy *arrayProxy, u8 *dataBlock);

RouteTableTreeArrayProxy *rttGetTopArrayByIndex(RouteTableTreeBuilder *builder, s32 topIndex);

void rttMergeRoutes(RouteTableTreeBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 prefixCount, s32 suffixCount, RoutingReadData **orderedDispatches, MemDispenser *disp);

void rttUnpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp);

void rttGetStats(RouteTableTreeBuilder *builder,
		s64 *routeTableForwardRouteEntriesPtr, s64 *routeTableForwardRoutesPtr, s64 *routeTableReverseRouteEntriesPtr, s64 *routeTableReverseRoutesPtr,
		s64 *routeTableTreeTopBytesPtr, s64 *routeTableTreeArrayBytesPtr, s64 *routeTableTreeLeafBytes, s64 *routeTableTreeBranchBytes);


#endif
