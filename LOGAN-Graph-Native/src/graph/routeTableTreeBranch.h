#ifndef __ROUTE_TABLE_TREE_BRANCH_H
#define __ROUTE_TABLE_TREE_BRANCH_H

typedef s16 RouteTableTreeBranchChild;

typedef struct routeTableTreeBranchBlockStr
{
	s16 childAlloc;
	s16 parentBrindex;

	//s16 upstreamMin;
	//s16 upstreamMax;

	s16 childNindex[]; // Max is ROUTE_TABLE_TREE_BRANCH_CHILDREN
} __attribute__((packed)) RouteTableTreeBranchBlock;


struct routeTableTreeBranchProxyStr
{
	RouteTableTreeBranchBlock *dataBlock;
	s16 brindex;

	u16 childAlloc;
	u16 childCount;

};

s32 rttbGetRouteTableTreeBranchSize_Expected(s16 childAlloc);
s32 rttbGetRouteTableTreeBranchSize_Existing(RouteTableTreeBranchBlock *branchBlock);

RouteTableTreeBranchBlock *rttbReallocRouteTableTreeBranchBlockEntries(RouteTableTreeBranchBlock *oldBlock, MemDispenser *disp, s32 childAlloc);

void rttbGetRouteTableTreeBranchProxy_scan(RouteTableTreeBranchBlock *branchBlock, u16 *allocPtr, u16 *countPtr);

void rttbFlushRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy);

RouteTableTreeBranchProxy *rttbAllocRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, s32 childAlloc);

void rttbExpandRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy);

void rttbDumpBranchBlock(RouteTableTreeBranchBlock *branchBlock);

void rttbBranchMakeChildInsertSpace(RouteTableTreeBranchProxy *branchProxy, s16 childPosition, s16 childCount);

s16 rttbGetBranchChildSibdex_Branch_withEstimate(RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeBranchProxy *childBranchProxy, s16 sibdexEstimate);
s16 rttbGetBranchChildSibdex_Branch(RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeBranchProxy *childBranchProxy);

s16 rttbGetBranchChildSibdex_Leaf_withEstimate(RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy, s16 sibdexEstimate);
s16 rttbGetBranchChildSibdex_Leaf(RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy);

s32 rttbGetBranchChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, s16 sibdex,
		RouteTableTreeBranchProxy **childBranchProxyPtr, RouteTableTreeLeafProxy **childLeafProxyPtr);

RouteTableTreeBranchProxy *rttbGetBranchParent(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *childBranchProxy);
RouteTableTreeBranchProxy *rttbGetLeafParent(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *childLeafProxy);



#endif
