#ifndef __ROUTE_TABLE_TREE_BRANCH_H
#define __ROUTE_TABLE_TREE_BRANCH_H

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

s32 getRouteTableTreeBranchSize_Expected(s16 childAlloc);
s32 getRouteTableTreeBranchSize_Existing(RouteTableTreeBranchBlock *branchBlock);

RouteTableTreeBranchBlock *reallocRouteTableTreeBranchBlockEntries(RouteTableTreeBranchBlock *oldBlock, MemDispenser *disp, s32 childAlloc);

RouteTableTreeBranchBlock *getRouteTableTreeBranchRaw(RouteTableTreeProxy *treeProxy, s32 brindex);
RouteTableTreeBranchProxy *getRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, s32 brindex);

void flushRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy);

RouteTableTreeBranchProxy *allocRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, s32 childAlloc);

void expandRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy);

void branchMakeChildInsertSpace(RouteTableTreeBranchProxy *branchProxy, s16 childPosition, s16 childCount);

s16 getBranchChildSibdex_Branch_withEstimate(RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeBranchProxy *childBranchProxy, s16 sibdexEstimate);
s16 getBranchChildSibdex_Branch(RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeBranchProxy *childBranchProxy);

s16 getBranchChildSibdex_Leaf_withEstimate(RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy, s16 sibdexEstimate);
s16 getBranchChildSibdex_Leaf(RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy);

s32 getBranchChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, s16 sibdex,
		RouteTableTreeBranchProxy **childBranchProxyPtr, RouteTableTreeLeafProxy **childLeafProxyPtr);

RouteTableTreeBranchProxy *getBranchParent(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *childBranchProxy);
RouteTableTreeBranchProxy *getLeafParent(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *childLeafProxy);



#endif
