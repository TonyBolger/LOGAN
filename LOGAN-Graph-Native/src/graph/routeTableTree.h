#ifndef __ROUTE_TABLE_TREE_H
#define __ROUTE_TABLE_TREE_H


// Route Table is loosely similar to a B-tree

// Route Table Root: Contains block ptrs as follows:
// 2 tail blocks: prefix and suffix
// Forward Leaf block
// Reverse Leaf block
// Forward Branch block
// Reverse Leaf block

// Offsets?

// Leaves contain a single upstream ID, branches generally include leafs with multiple upstream IDs
//
// Minimum Valid tree: Empty root

// Two-level trees may need more than s16 indexes (currently 256*(>256) possible leaves)

#define ROUTE_TABLE_TREE_PTR_ARRAY_ENTRIES 256

// Can be used to force 2-level mode on modest datasets
//#define ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES 64
//#define ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES 256
#define ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES 1024

#define ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_SHIFT 2
#define ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_RANGE (1<<ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_SHIFT)

//#define ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_SHIFT 4
#define ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_SHIFT 8
#define ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES (1<<ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_SHIFT)

#define ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_MASK (ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES-1)

#define ROUTE_TABLE_TREE_MAX_ARRAY_ENTRIES (ROUTE_TABLE_TREE_PTR_ARRAY_ENTRIES*ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES)
#define ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK 8

// Long term, should be at least 128 to allow 1024 branches contain 65536 leaves (at 50%)

#define ROUTE_TABLE_TREE_BRANCH_CHILDREN 1024
//#define ROUTE_TABLE_TREE_BRANCH_CHILDREN 256
#define ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK 4
//#define ROUTE_TABLE_TREE_BRANCH_CHILDREN 4
//#define ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK 4



//#define ROUTE_TABLE_TREE_LEAF_ENTRIES 2048
//#define ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK 16

#define ROUTE_TABLE_TREE_LEAF_ENTRIES 1024
#define ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK 8

// Can be used to force 2-level mode on modest datasets
//#define ROUTE_TABLE_TREE_LEAF_ENTRIES 16
//#define ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK 4


/* Structs representing the various parts of the tree in heap-block format */

typedef struct rootTableTreeTopBlockStr
{
	u8 *data[6]; // Tail(P,S), Branch(F,R), Leaf(F,R)
} __attribute__((packed)) RouteTableTreeTopBlock;


// This (monster) represents the state of RouteTableTree during patch merge

typedef struct routeTableLeafEntryBufferStr
{
	RouteTableTreeProxy *treeProxy;
	int upstreamOffsetCount;
	int downstreamOffsetCount;

	// Current Position within old entries (reading)
	RouteTableTreeBranchProxy *oldBranchProxy;
	RouteTableTreeLeafProxy *oldLeafProxy;
	s16 oldBranchChildSibdex;

	// Old EntryArrays/Entries (reading)
	RouteTableUnpackedEntryArray **oldEntryArraysPtr;
	RouteTableUnpackedEntryArray **oldEntryArraysPtrEnd;
	RouteTableUnpackedEntry *oldEntryPtr;
	RouteTableUnpackedEntry *oldEntryPtrEnd;
	s32 oldUpstream;
	s32 oldDownstream;
	s32 oldWidth;

	// Accumulated offsets of old entries
	s32 *accumUpstreamOffsets;
	s32 *accumDownstreamOffsets;

	// Current Position within new entries (writing)
	RouteTableTreeBranchProxy *newBranchProxy;
	RouteTableTreeLeafProxy *newLeafProxy;
	s16 newBranchChildSibdex;

	// New EntryArrays/Entries (writing)
	RouteTableUnpackedEntryArray **newEntryArraysBlockPtr;
	RouteTableUnpackedEntryArray **newEntryArraysPtr;
	RouteTableUnpackedEntryArray **newEntryArraysPtrEnd;

	RouteTableUnpackedEntryArray *newEntryCurrentArrayPtr;
	RouteTableUnpackedEntry *newEntryPtr;
	RouteTableUnpackedEntry *newEntryPtrEnd;

	s32 newUpstream;
	s32 newDownstream;
	s32 newWidth;

	// Offset of current new entry in leaf
	s32 *newLeafUpstreamOffsets;
	s32 *newLeafDownstreamOffsets;

	s32 maxWidth;
} RouteTableTreeEntryBuffer;




// Negative types are pseudo-indexes
#define ROUTE_PSEUDO_INDEX_DIRECT -2
#define ROUTE_PSEUDO_INDEX_TOP -1

// These types are real top indexes

#define ROUTE_TOPINDEX_PREFIX 0
#define ROUTE_TOPINDEX_SUFFIX 1
#define ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0 2
#define ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0 3
#define ROUTE_TOPINDEX_FORWARD_LEAF_ARRAY_0 4
#define ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0 5

#define ROUTE_TOPINDEX_MAX 6

// Remaining types are pseudo-indexes
#define ROUTE_PSEUDO_INDEX_FORWARD_LEAF_ARRAY_1 6
#define ROUTE_PSEUDO_INDEX_REVERSE_LEAF_ARRAY_1 7
#define ROUTE_PSEUDO_INDEX_FORWARD_BRANCH_1 8
#define ROUTE_PSEUDO_INDEX_REVERSE_BRANCH_1 9
#define ROUTE_PSEUDO_INDEX_FORWARD_LEAF_1 10
#define ROUTE_PSEUDO_INDEX_REVERSE_LEAF_1 11
#define ROUTE_PSEUDO_INDEX_FORWARD_LEAF_2 12
#define ROUTE_PSEUDO_INDEX_REVERSE_LEAF_2 13

#define ROUTE_PSEUDO_INDEX_MID_ADJUST 2
#define ROUTE_PSEUDO_INDEX_ENTITY_1_ADJUST 6
#define ROUTE_PSEUDO_INDEX_LEAF_2_ADJUST 8

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


// For now, all trees use the same size tail indexes (s16), width (s32). Longer term, this should be per-node


/*
typedef struct routeTableTreeOffsetBlockStr
{
	s16 entryAlloc;

	s16 upstreamMin;
	s16 upstreamCount;
	s16 downstreamMin;
	s16 downstreamCount;

	s32 offsets[];
} __attribute__((packed)) RouteTableTreeOffsetBlock;
*/



struct routeTableTreeBuilderStr
{
	MemDispenser *disp;

	//HeapDataBlock *topDataBlock;

	HeapDataBlock dataBlocks[6];

	RouteTableTreeProxy forwardProxy;
	RouteTableTreeProxy reverseProxy;

	RouteTableTreeWalker forwardWalker;
	RouteTableTreeWalker reverseWalker;

	s32 newEntryCount;
};



void rttInitRouteTableTreeBuilder(RouteTableTreeBuilder *builder, RouteTableTreeTopBlock *top);
void rttUpgradeToRouteTableTreeBuilder(RouteTableArrayBuilder *arrayBuilder,  RouteTableTreeBuilder *treeBuilder, s32 sliceIndex, s32 prefixCount, s32 suffixCount, MemDispenser *disp);

void rttUpdateRouteTableTreeBuilderTailCounts(RouteTableTreeBuilder *builder, s32 prefixCount, s32 suffixCount);

void rttDumpRoutingTable(RouteTableTreeBuilder *builder);

//s32 rttGetRouteTableTreeBuilderDirty(RouteTableTreeBuilder *builder);

void rttBindBlockArrayProxy(RouteTableTreeArrayProxy *arrayProxy, u8 *heapDataPtr, u32 headerSize, u32 isIndirect);

s32 rttGetTopArraySize(RouteTableTreeArrayProxy *arrayProxy);

s32 rttMergeTopArrayUpdates_accumulateSize(RouteTableTreeArrayProxy *arrayProxy, u8 *dataBlock);

RouteTableTreeArrayProxy *rttGetTopArrayByIndex(RouteTableTreeBuilder *builder, s32 topIndex);

void rttMergeRoutes(RouteTableTreeBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 prefixCount, s32 suffixCount, RoutingReadData **orderedDispatches, MemDispenser *disp);

void rttUnpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp);

void rttGetStats(RouteTableTreeBuilder *builder,
		s64 *routeTableForwardRouteEntriesPtr, s64 *routeTableForwardRoutesPtr, s64 *routeTableReverseRouteEntriesPtr, s64 *routeTableReverseRoutesPtr,
		s64 *routeTableTreeTopBytesPtr, s64 *routeTableTreeArrayBytesPtr,
		s64 *routeTableTreeLeafBytes, s64 *routeTableTreeLeafOffsetBytes, s64 *routeTableTreeLeafEntryBytes,
		s64 *routeTableTreeBranchBytes, s64 *routeTableTreeBranchOffsetBytes, s64 *routeTableTreeBranchChildBytes);


#endif
