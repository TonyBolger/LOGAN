#ifndef __ROUTE_TABLE_TREE_PROXY_H
#define __ROUTE_TABLE_TREE_PROXY_H



struct routeTableTreeProxyStr
{
	MemDispenser *disp;

	RouteTableTreeArrayProxy leafArrayProxy;
	RouteTableTreeArrayProxy branchArrayProxy;
//	RouteTableTreeArrayProxy offsetArrayProxy;

	RouteTableTreeBranchProxy *rootProxy;
};



void initTreeProxy(RouteTableTreeProxy *treeProxy, HeapDataBlock *leafBlock, u8 *leafDataPtr, HeapDataBlock *branchBlock, u8 *branchDataPtr, MemDispenser *disp);

void treeProxySeekStart(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr, s16 *branchChildSibdexPtr, RouteTableTreeLeafProxy **leafProxyPtr);
void treeProxySeekEnd(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr,  s16 *branchChildSibdexPtr, RouteTableTreeLeafProxy **leafProxyPtr);

void treeProxySplitRoot(RouteTableTreeProxy *treeProxy, s16 childPosition, RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *newChildPositionPtr);

RouteTableTreeBranchProxy *treeProxySplitBranch(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy, s16 childPosition,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *newChildPositionPtr);

RouteTableTreeLeafProxy *treeProxySplitLeaf(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy, s16 leafEntryPosition, s16 space,
		 RouteTableTreeLeafProxy **newLeafProxyPtr, s16 *newEntryPositionPtr);

void treeProxyInsertBranchChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeBranchProxy *childBranchProxy, s16 childPosition,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *newChildPositionPtr);

// Insert the given leaf child into the branch parent, splitting the parent if needed
void treeProxyInsertLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy, s16 childPosition,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *newChildPositionPtr);

void treeProxyAppendLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *childPositionPtr);

RouteTableTreeLeafProxy *treeProxySplitLeafInsertChildEntrySpace(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy,
		s16 childPosition, RouteTableTreeLeafProxy *childLeafProxy, s16 insertEntryPosition, s16 insertEntryCount,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *newChildPositionPtr, RouteTableTreeLeafProxy **newChildLeafProxyPtr, s16 *newEntryPositionPtr);

s32 getNextBranchSibling(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr);
s32 getNextLeafSibling(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr, s16 *branchChildSibdex, RouteTableTreeLeafProxy **leafProxyPtr);


#endif
