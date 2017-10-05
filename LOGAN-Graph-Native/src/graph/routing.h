#ifndef __ROUTING_H
#define __ROUTING_H




typedef struct smerRoutingStatsStr
{
	SmerId smerId;
	char smerStr[SMER_BASES+1];

	s32 prefixTails;
	s32 prefixBytes;

	s32 suffixTails;
	s32 suffixBytes;

	s32 routeTableFormat; // 0 - null, 1 - array, 2 - tree

	s64 routeTableForwardRouteEntries;
	s64 routeTableForwardRoutes;

	s64 routeTableReverseRouteEntries;
	s64 routeTableReverseRoutes;

	s64 routeTableArrayBytes;

	s64 routeTableTreeTopBytes;
	s64 routeTableTreeArrayBytes;

	s64 routeTableTreeLeafBytes;
	s64 routeTableTreeLeafOffsetBytes;
	s64 routeTableTreeLeafEntryBytes;

	s64 routeTableTreeBranchBytes;
	s64 routeTableTreeBranchOffsetBytes;
	s64 routeTableTreeBranchChildBytes;

	s64 routeTableTotalBytes;
	s64 smerTotalBytes;
} SmerRoutingStats;



//#define ROUTING_TREE_THRESHOLD 4
//#define ROUTING_TREE_THRESHOLD 40
//#define ROUTING_TREE_THRESHOLD 100
#define ROUTING_TREE_THRESHOLD 400
//#define ROUTING_TREE_THRESHOLD 1000
//#define ROUTING_TREE_THRESHOLD 4000
//#define ROUTING_TREE_THRESHOLD 10000
//#define ROUTING_TREE_THRESHOLD 100000
//#define ROUTING_TREE_THRESHOLD 1000000000

int rtRouteReadsForSmer(RoutingIndexedReadReferenceBlock *rdi, SmerArraySlice *slice, RoutingReadData **orderedDispatches, MemDispenser *disp, MemCircHeap *circHeap, u8 sliceTag);

SmerLinked *rtGetLinkedSmer(SmerArray *smerArray, SmerId rootSmerId, MemDispenser *disp);

SmerRoutingStats *rtGetRoutingStats(SmerArraySlice *smerArraySlice, u32 sliceNum, MemDispenser *disp);


#endif

