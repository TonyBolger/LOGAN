#ifndef __ROUTE_TABLE_TREE_WALKER_H
#define __ROUTE_TABLE_TREE_WALKER_H




typedef struct routeTableTreeWalkerStr
{
	RouteTableTreeProxy *treeProxy;

	// Current Position
	RouteTableTreeBranchProxy *branchProxy;
	RouteTableTreeLeafProxy *leafProxy;
	RouteTableUnpackedEntryArray *leafEntryArray;

	s16 branchChildSibdex;
	s16 leafArrayIndex;
	s16 leafEntryIndex;

	s32 upstreamOffsetCount;
	s32 downstreamOffsetCount;

	s32 *upstreamLeafOffsets;
	s32 *downstreamLeafOffsets;

	s32 *upstreamEntryOffsets;
	s32 *downstreamEntryOffsets;

} RouteTableTreeWalker;



void rttwInitTreeWalker(RouteTableTreeWalker *walker, RouteTableTreeProxy *treeProxy);

void rttwDumpWalker(RouteTableTreeWalker *walker);

void rttwAppendNewLeaf(RouteTableTreeWalker *walker);

void rttwSeekStart(RouteTableTreeWalker *walker);
void rttwSeekEnd(RouteTableTreeWalker *walker);

s32 rttwGetCurrentEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableUnpackedEntry **entry);

//s32 walkerNextLeaf(RouteTableTreeWalker *walker, s16 *upstream, RouteTableUnpackedEntry **entry);
//s32 walkerNextEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableUnpackedEntry **entry, s32 holdUpstream);

s32 rttwAdvanceToUpstreamThenOffsetThenDownstream(RouteTableTreeWalker *walker, s32 targetUpstream, s32 targetDownstream, s32 targetMinOffset, s32 targetMaxOffset,
		s32 *upstreamPtr, RouteTableUnpackedEntry **entryPtr, s32 *upstreamOffsetPtr, s32 *downstreamOffsetPtr);

void rttwResetOffsetArrays(RouteTableTreeWalker *walker);
void rttwInitOffsetArrays(RouteTableTreeWalker *walker, s32 upstreamCount, s32 downstreamCount);

void rttwMergeRoutes_insertEntry(RouteTableTreeWalker *walker, s32 upstream, s32 downstream);
void rttwMergeRoutes_widen(RouteTableTreeWalker *walker);
void rttwMergeRoutes_split(RouteTableTreeWalker *walker, s32 downstream, s32 width1, s32 width2);

void rttwAppendPreorderedEntry(RouteTableTreeWalker *walker, RouteTableEntry *entry, int routingTable);
void rttwAppendPreorderedEntries(RouteTableTreeWalker *walker, RouteTableEntry *entries, u32 entryCount, int routingTable);


#endif
