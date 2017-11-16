#ifndef __ROUTE_TABLE_TREE_PROXY_H
#define __ROUTE_TABLE_TREE_PROXY_H



struct routeTableTreeProxyStr
{
	MemDispenser *disp;

	s32 upstreamCount;
	s32 downstreamCount;

	RouteTableTreeArrayProxy leafArrayProxy;
	RouteTableTreeArrayProxy branchArrayProxy;

	RouteTableTreeBranchProxy *rootProxy;
};


void initTreeProxy(RouteTableTreeProxy *treeProxy, HeapDataBlock *leafBlock, u8 *leafDataPtr, HeapDataBlock *branchBlock, u8 *branchDataPtr, MemDispenser *disp);
void updateTreeProxyTailCounts(RouteTableTreeProxy *treeProxy, s32 upstreamCount, s32 downstreamCount);

RouteTableTreeBranchBlock *getRouteTableTreeBranchBlock(RouteTableTreeProxy *treeProxy, s32 brindex);
RouteTableTreeBranchProxy *getRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, s32 brindex);

RouteTableTreeLeafBlock *getRouteTableTreeLeafBlock(RouteTableTreeProxy *treeProxy, s32 lindex);
RouteTableTreeLeafProxy *getRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 lindex);

void treeProxySeekStart(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr, s16 *branchChildSibdexPtr, RouteTableTreeLeafProxy **leafProxyPtr);
void treeProxySeekEnd(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr,  s16 *branchChildSibdexPtr, RouteTableTreeLeafProxy **leafProxyPtr);

void treeProxyInsertBranchChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeBranchProxy *childBranchProxy, s16 childPosition,
		RouteTableTreeBranchProxy **updatedParentBranchProxyPtr, s16 *updatedChildPositionPtr);

// Insert a new leaf child at the position of the existing child, splitting the parent if needed
void treeProxyInsertLeafChildBefore(RouteTableTreeProxy *treeProxy,
		RouteTableTreeBranchProxy *branchProxy, RouteTableTreeLeafProxy *existingChildLeafProxy, s16 existingChildPosition,
		RouteTableTreeLeafProxy *newChildLeafProxy, RouteTableTreeBranchProxy **updatedBranchProxyPtr, s16 *updatedExistingChildPositionPtr, s16 *updatedNewChildPositionPtr);

// Insert the given leaf child into the branch parent, splitting the parent if needed
void treeProxyInsertLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy, s16 childPosition,
		RouteTableTreeBranchProxy **updatedParentBranchProxyPtr, s16 *updatedChildPositionPtr);

void treeProxyAppendLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *childPositionPtr);

RouteTableTreeLeafProxy *treeProxySplitLeaf(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy,
		s16 childPosition, RouteTableTreeLeafProxy *childLeafProxy, s16 insertArrayIndex,
		RouteTableTreeBranchProxy **updatedParentBranchProxyPtr, s16 *updatedChildPositionPtr, RouteTableTreeLeafProxy **updatedChildLeafProxyPtr, s16 *updatedEntryIndexPtr);

s32 hasMoreSiblings(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy, s16 sibdexEst, RouteTableTreeLeafProxy *leafProxy);

s32 getNextBranchSibling(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr);
s32 getNextLeafSibling(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr, s16 *branchChildSibdex, RouteTableTreeLeafProxy **leafProxyPtr);


#endif
