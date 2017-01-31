
#include "common.h"


s32 getRouteTableTreeBranchSize_Expected(s16 childAlloc)
{
	return sizeof(RouteTableTreeBranchBlock)+((s32)childAlloc)*sizeof(s16);
}


s32 getRouteTableTreeBranchSize_Existing(RouteTableTreeBranchBlock *branchBlock)
{
	return sizeof(RouteTableTreeBranchBlock)+((s32)branchBlock->childAlloc)*sizeof(s16);
}





RouteTableTreeBranchBlock *reallocRouteTableTreeBranchBlockEntries(RouteTableTreeBranchBlock *oldBlock, MemDispenser *disp, s32 childAlloc)
{
	if(childAlloc>ROUTE_TABLE_TREE_BRANCH_CHILDREN)
		{
		LOG(LOG_CRITICAL,"Cannot allocate oversize branch with %i children",childAlloc);
		}

	RouteTableTreeBranchBlock *block=dAlloc(disp, getRouteTableTreeBranchSize_Expected(childAlloc));
	block->parentBrindex=BRANCH_NINDEX_INVALID;
//	block->upstreamMin=-1;
//	block->upstreamMax=-1;

	s32 oldBlockChildAlloc=0;
	if(oldBlock!=NULL)
		{
		oldBlockChildAlloc=oldBlock->childAlloc;
		s32 toKeepChildAlloc=MIN(oldBlockChildAlloc, childAlloc);

		memcpy(block,oldBlock, getRouteTableTreeBranchSize_Expected(toKeepChildAlloc));
		}

	for(int i=oldBlockChildAlloc;i<childAlloc;i++)
		block->childNindex[i]=BRANCH_NINDEX_INVALID;

	block->childAlloc=childAlloc;

	return block;
}

static RouteTableTreeBranchBlock *allocRouteTableTreeBranchBlock(MemDispenser *disp, s32 childAlloc)
{
	if(childAlloc>ROUTE_TABLE_TREE_BRANCH_CHILDREN)
		{
		LOG(LOG_CRITICAL,"Cannot allocate oversize branch with %i children",childAlloc);
		}

	RouteTableTreeBranchBlock *block=dAlloc(disp, getRouteTableTreeBranchSize_Expected(childAlloc));
	block->childAlloc=childAlloc;
	block->parentBrindex=BRANCH_NINDEX_INVALID;
//	block->upstreamMin=-1;
//	block->upstreamMax=-1;

	for(int i=0;i<childAlloc;i++)
		block->childNindex[i]=BRANCH_NINDEX_INVALID;

	return block;
}


static void getRouteTableTreeBranchProxy_scan(RouteTableTreeBranchBlock *branchBlock, u16 *allocPtr, u16 *countPtr)
{
	if(branchBlock==NULL)
		{
		*allocPtr=0;
		*countPtr=0;
		return;
		}

	u16 alloc=branchBlock->childAlloc;
	*allocPtr=alloc;
	u16 count=0;

	for(int i=alloc;i>0;i--)
		{
		if(branchBlock->childNindex[i-1]!=BRANCH_NINDEX_INVALID)
			{
			count=i;
			break;
			}
		}

	*countPtr=count;
}



RouteTableTreeBranchBlock *getRouteTableTreeBranchRaw(RouteTableTreeProxy *treeProxy, s32 brindex)
{
	if(brindex<0)
		{
		LOG(LOG_CRITICAL,"Brindex must be positive: %i",brindex);
		}

	u8 *data=getBlockArrayEntry(&(treeProxy->branchArrayProxy), brindex);

	return (RouteTableTreeBranchBlock *)data;
}


RouteTableTreeBranchProxy *getRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, s32 brindex)
{
	if(brindex<0)
		{
		LOG(LOG_CRITICAL,"Brindex must be positive: %i",brindex);
		}

	u8 *data=getBlockArrayEntry(&(treeProxy->branchArrayProxy), brindex);

	RouteTableTreeBranchProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeBranchProxy));

	proxy->dataBlock=(RouteTableTreeBranchBlock *)data;
	proxy->brindex=brindex;

	getRouteTableTreeBranchProxy_scan(proxy->dataBlock, &proxy->childAlloc, &proxy->childCount);

	return proxy;
}


void flushRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy)
{
	setBlockArrayEntry(&(treeProxy->branchArrayProxy), branchProxy->brindex, (u8 *)branchProxy->dataBlock, treeProxy->disp);
}


RouteTableTreeBranchProxy *allocRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, s32 childAlloc)
{
	if(childAlloc<ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK)
		childAlloc=ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK;

	RouteTableTreeBranchBlock *dataBlock=allocRouteTableTreeBranchBlock(treeProxy->disp, childAlloc);
	s32 brindex=appendBlockArrayEntry(&(treeProxy->branchArrayProxy), (u8 *)dataBlock, treeProxy->disp);

	RouteTableTreeBranchProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeBranchProxy));
	proxy->dataBlock=dataBlock;
	proxy->brindex=brindex;

	proxy->childAlloc=childAlloc;
	proxy->childCount=0;

	return proxy;
}



void expandRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy)
{
	s32 newAlloc=MIN(branchProxy->childAlloc+ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK, ROUTE_TABLE_TREE_BRANCH_CHILDREN);

	RouteTableTreeBranchBlock *parentBlock=reallocRouteTableTreeBranchBlockEntries(branchProxy->dataBlock, treeProxy->disp, newAlloc);

	branchProxy->dataBlock=parentBlock;
	branchProxy->childAlloc=newAlloc;

	flushRouteTableTreeBranchProxy(treeProxy, branchProxy);
}


/*
void dumpBranchProxy(RouteTableTreeBranchProxy *branchProxy)
{
	LOG(LOG_INFO,"Branch: Index %i Used %i of %i - Datablock %p",branchProxy->brindex,branchProxy->childCount,branchProxy->childAlloc,branchProxy->dataBlock);
	LOG(LOG_INFO,"BlockAlloc %i",branchProxy->dataBlock->childAlloc);

	for(int i=0;i<branchProxy->dataBlock->childAlloc;i++)
		LOG(LOG_INFO,"Entry %i has %i",i,branchProxy->dataBlock->childNindex[i]);
}
*/



void branchMakeChildInsertSpace(RouteTableTreeBranchProxy *branch, s16 childPosition, s16 childCount)
{
	if((branch->childCount+childCount)>branch->childAlloc)
		{
		LOG(LOG_CRITICAL,"Insufficient space for branch child insert");
		}

	if(childPosition<branch->childCount)
		{
		s16 entriesToMove=branch->childCount-childPosition;

		s16 *childPtr=branch->dataBlock->childNindex+childPosition;
		memmove(childPtr+childCount, childPtr, sizeof(s16)*entriesToMove);
		}

	branch->childCount+=childCount;
}





s16 getBranchChildSibdex_Branch_withEstimate(RouteTableTreeBranchProxy *parent, RouteTableTreeBranchProxy *child, s16 sibdexEstimate)
{
	s16 childNindex=child->brindex;
	u16 childCount=parent->childCount;

	if(sibdexEstimate>=0 && sibdexEstimate<childCount && parent->dataBlock->childNindex[sibdexEstimate]==childNindex)
		return sibdexEstimate;

	for(int i=0;i<childCount;i++)
		if(parent->dataBlock->childNindex[i]==childNindex)
			return i;

	return -1;
}

//static
s16 getBranchChildSibdex_Branch(RouteTableTreeBranchProxy *parent, RouteTableTreeBranchProxy *child)
{
	s16 childNindex=child->brindex;
	u16 childCount=parent->childCount;

	for(int i=0;i<childCount;i++)
		if(parent->dataBlock->childNindex[i]==childNindex)
			return i;

	return -1;
}


//static
s16 getBranchChildSibdex_Leaf_withEstimate(RouteTableTreeBranchProxy *parent, RouteTableTreeLeafProxy *child, s16 sibdexEstimate)
{
	s16 childNindex=LINDEX_TO_NINDEX(child->lindex);
	u16 childCount=parent->childCount;

	if(sibdexEstimate>=0 && sibdexEstimate<childCount && parent->dataBlock->childNindex[sibdexEstimate]==childNindex)
		return sibdexEstimate;

//	LOG(LOG_INFO,"Parent: %i Leaf: %i as %i",parent->brindex,child->lindex,childNindex);

//	LOG(LOG_INFO,"Childcount: %i",childCount);


	for(int i=0;i<childCount;i++)
		{
//		LOG(LOG_INFO,"Nindex: %i",parent->dataBlock->childNindex[i]);

		if(parent->dataBlock->childNindex[i]==childNindex)
			return i;
		}

	return -1;
}


s16 getBranchChildSibdex_Leaf(RouteTableTreeBranchProxy *parent, RouteTableTreeLeafProxy *child)
{
	s16 childNindex=LINDEX_TO_NINDEX(child->lindex);
	u16 childCount=parent->childCount;

	for(int i=0;i<childCount;i++)
		if(parent->dataBlock->childNindex[i]==childNindex)
			return i;

	return -1;
}



s32 getBranchChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parent, s16 sibdex, RouteTableTreeBranchProxy **branchChild, RouteTableTreeLeafProxy **leafChild)
{
	u16 childCount=parent->childCount;

	if(sibdex<0 || sibdex>=childCount)
		{
		*branchChild=NULL;
		*leafChild=NULL;
		return 0;
		}

	s16 childNindex=parent->dataBlock->childNindex[sibdex];

	if(childNindex>=0)
		{
		*branchChild=getRouteTableTreeBranchProxy(treeProxy, childNindex);
		*leafChild=NULL;
		}
	else
		{
		*branchChild=NULL;
		*leafChild=getRouteTableTreeLeafProxy(treeProxy, childNindex);
		}

	return 1;

}

