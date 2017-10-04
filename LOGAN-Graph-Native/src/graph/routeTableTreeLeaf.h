#ifndef __ROUTE_TABLE_TREE_LEAF_H
#define __ROUTE_TABLE_TREE_LEAF_H

/*
typedef s32 RouteTableTreeLeafOffset;

typedef struct routeTableTreeLeafEntryStr
{
	s16 downstream;
	s32 width;
} __attribute__((packed)) RouteTableTreeLeafEntry;

typedef struct routeTableTreeLeafEntryBlockStr
{
	s16 upstream;

	u16 entryAlloc;
	u16 entryCount;
	RouteTableTreeLeafEntry entries[];
} __attribute__((packed)) RouteTableTreeLeafEntryBlock;
*/

typedef struct routeTableLeafBlockStr
{
	s16 parentBrindex;
	u8 packedBlockData[]; // Contains a RouteTablePackedSingleBlock
} __attribute__((packed)) RouteTableTreeLeafBlock;


#define LEAFPROXY_STATUS_FULLYPACKED 0
#define LEAFPROXY_STATUS_OFFSETS 1
#define LEAFPROXY_STATUS_FULLYUNPACKED 2
#define LEAFPROXY_STATUS_DIRTY 3

struct routeTableTreeLeafProxyStr
{
	RouteTableTreeLeafBlock *leafBlock;
	s16 parentBrindex;
	s16 lindex;

	s32 status;
	RouteTableUnpackedSingleBlock *unpackedBlock;
};


void rttlEnsureOffsetUnpacked(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy);
void rttlEnsureFullyUnpacked(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy);
void rttlMarkDirty(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy);

RouteTableTreeLeafProxy *allocRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 upstreamOffsetAlloc, s32 downstreamOffsetAlloc, s32 entryArrayAlloc);

void dumpLeafBlock(RouteTableTreeLeafBlock *leafBlock);

void dumpLeafProxy(RouteTableTreeLeafProxy *leafProxy);

//void getRouteTableTreeLeafProxy_scan(RouteTableTreeLeafBlock *leafBlock, u16 *entryAllocPtr, u16 *entryCountPtr);
//RouteTableTreeLeafProxy *allocRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 offsetAlloc, s32 entryAlloc);

//void ensureRouteTableTreeLeafOffsetCapacity(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy, s32 offsetAlloc);
//void expandRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy, s32 offsetAlloc);

//void leafMakeEntryInsertSpace(RouteTableTreeLeafProxy *leaf, s16 entryPosition, s16 entryCount);

//void initRouteTableTreeLeafOffsets(RouteTableTreeLeafProxy *leafProxy, s16 upstream);
//void recalcRouteTableTreeLeafOffsets(RouteTableTreeLeafProxy *leafProxy);
//void validateRouteTableTreeLeafOffsets(RouteTableTreeLeafProxy *leafProxy);


#endif
