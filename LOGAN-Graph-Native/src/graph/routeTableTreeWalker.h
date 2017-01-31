#ifndef __ROUTE_TABLE_TREE_WALKER_H
#define __ROUTE_TABLE_TREE_WALKER_H




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



void initTreeWalker(RouteTableTreeWalker *walker, RouteTableTreeProxy *treeProxy);

void walkerSeekStart(RouteTableTreeWalker *walker);
void walkerSeekEnd(RouteTableTreeWalker *walker);

s32 walkerGetCurrentEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableTreeLeafEntry **entry);
s32 walkerNextEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableTreeLeafEntry **entry, s32 holdUpstream);

void walkerResetOffsetArrays(RouteTableTreeWalker *walker);
void walkerInitOffsetArrays(RouteTableTreeWalker *walker, s32 upstreamCount, s32 downstreamCount);

void walkerAppendNewLeaf(RouteTableTreeWalker *walker, s16 upstream);
void walkerAppendPreorderedEntry(RouteTableTreeWalker *walker, RouteTableEntry *entry, int routingTable);
void walkerAppendPreorderedEntries(RouteTableTreeWalker *walker, RouteTableEntry *entries, u32 entryCount, int routingTable);


#endif
