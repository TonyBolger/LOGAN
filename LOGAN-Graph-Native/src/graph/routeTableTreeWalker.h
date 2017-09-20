#ifndef __ROUTE_TABLE_TREE_WALKER_H
#define __ROUTE_TABLE_TREE_WALKER_H




typedef struct routeTableTreeWalkerStr
{
	RouteTableTreeProxy *treeProxy;

	// Current Position
	RouteTableTreeBranchProxy *branchProxy;
	s16 branchChildSibdex;
	RouteTableTreeLeafProxy *leafProxy;
	s16 leafArrayIndex;
	s16 leafEntryIndex;

	s32 upstreamOffsetCount;
	s32 downstreamOffsetCount;

	s32 *upstreamLeafOffsets;
	s32 *downstreamLeafOffsets;

	s32 *upstreamEntryOffsets;
	s32 *downstreamEntryOffsets;

} RouteTableTreeWalker;



void initTreeWalker(RouteTableTreeWalker *walker, RouteTableTreeProxy *treeProxy);

void dumpWalker(RouteTableTreeWalker *walker);

void walkerAppendNewLeaf(RouteTableTreeWalker *walker);

void walkerSeekStart(RouteTableTreeWalker *walker);
void walkerSeekEnd(RouteTableTreeWalker *walker);

s32 walkerGetCurrentEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableUnpackedEntry **entry);

//s32 walkerNextLeaf(RouteTableTreeWalker *walker, s16 *upstream, RouteTableUnpackedEntry **entry);
//s32 walkerNextEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableUnpackedEntry **entry, s32 holdUpstream);

s32 walkerAdvanceToUpstreamThenOffsetThenDownstream(RouteTableTreeWalker *walker, s32 targetUpstream, s32 targetDownstream, s32 targetMinOffset, s32 targetMaxOffset,
		s32 *upstreamPtr, RouteTableUnpackedEntry **entryPtr, s32 *upstreamOffsetPtr, s32 *downstreamOffsetPtr);

void walkerResetOffsetArrays(RouteTableTreeWalker *walker);
void walkerInitOffsetArrays(RouteTableTreeWalker *walker, s32 upstreamCount, s32 downstreamCount);

void walkerMergeRoutes_insertEntry(RouteTableTreeWalker *walker, s32 upstream, s32 downstream);
void walkerMergeRoutes_widen(RouteTableTreeWalker *walker);
void walkerMergeRoutes_split(RouteTableTreeWalker *walker, s32 downstream, s32 width1, s32 width2);

void walkerAppendPreorderedEntry(RouteTableTreeWalker *walker, RouteTableEntry *entry, int routingTable);
void walkerAppendPreorderedEntries(RouteTableTreeWalker *walker, RouteTableEntry *entries, u32 entryCount, int routingTable);


#endif
