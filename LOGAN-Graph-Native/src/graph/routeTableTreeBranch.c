
#include "common.h"


s32 getRouteTableTreeBranchSize_Expected(s16 childAlloc)
{
	return sizeof(RouteTableTreeBranchBlock)+((s32)childAlloc)*sizeof(RouteTableTreeBranchChild);
}


s32 getRouteTableTreeBranchSize_Existing(RouteTableTreeBranchBlock *branchBlock)
{
	return sizeof(RouteTableTreeBranchBlock)+((s32)branchBlock->childAlloc)*sizeof(RouteTableTreeBranchChild);
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

	RouteTableTreeBranchProxy *branchBroxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeBranchProxy));

	branchBroxy->dataBlock=(RouteTableTreeBranchBlock *)data;
	branchBroxy->brindex=brindex;

	getRouteTableTreeBranchProxy_scan(branchBroxy->dataBlock, &branchBroxy->childAlloc, &branchBroxy->childCount);

	return branchBroxy;
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

	RouteTableTreeBranchProxy *branchBroxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeBranchProxy));
	branchBroxy->dataBlock=dataBlock;
	branchBroxy->brindex=brindex;

	branchBroxy->childAlloc=childAlloc;
	branchBroxy->childCount=0;

	return branchBroxy;
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


void dumpBranchBlock(RouteTableTreeBranchBlock *branchBlock)
{
	if(branchBlock!=NULL)
		{
		LOG(LOG_INFO,"Branch (%p): Parent %i, Child Alloc %i",branchBlock, branchBlock->parentBrindex, branchBlock->childAlloc);

		LOGS(LOG_INFO,"Children: ");

		for(int j=0;j<branchBlock->childAlloc;j++)
			{
			LOGS(LOG_INFO,"%i ",branchBlock->childNindex[j]);
			if((j&0x1F)==0x1F)
				LOGN(LOG_INFO,"");
			}

		LOGN(LOG_INFO,"");

		}
	else
		{
		LOG(LOG_INFO,"Branch: NULL");
		}
}



void branchMakeChildInsertSpace(RouteTableTreeBranchProxy *branchProxy, s16 childPosition, s16 childCount)
{
	if((branchProxy->childCount+childCount)>branchProxy->childAlloc)
		{
		LOG(LOG_CRITICAL,"Insufficient space for branch child insert");
		}

	if(childPosition<branchProxy->childCount)
		{
		s16 entriesToMove=branchProxy->childCount-childPosition;

		s16 *childPtr=branchProxy->dataBlock->childNindex+childPosition;
		memmove(childPtr+childCount, childPtr, sizeof(s16)*entriesToMove);
		}

	branchProxy->childCount+=childCount;
}





s16 getBranchChildSibdex_Branch_withEstimate(RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeBranchProxy *childBranchProxy, s16 sibdexEstimate)
{
	s16 childNindex=childBranchProxy->brindex;
	u16 childCount=parentBranchProxy->childCount;

	if(sibdexEstimate>=0 && sibdexEstimate<childCount && parentBranchProxy->dataBlock->childNindex[sibdexEstimate]==childNindex)
		return sibdexEstimate;

	for(int i=0;i<childCount;i++)
		if(parentBranchProxy->dataBlock->childNindex[i]==childNindex)
			return i;

	return -1;
}

//static
s16 getBranchChildSibdex_Branch(RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeBranchProxy *childBranchProxy)
{
	s16 childNindex=childBranchProxy->brindex;
	u16 childCount=parentBranchProxy->childCount;

	for(int i=0;i<childCount;i++)
		if(parentBranchProxy->dataBlock->childNindex[i]==childNindex)
			return i;

	return -1;
}


//static
s16 getBranchChildSibdex_Leaf_withEstimate(RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy, s16 sibdexEstimate)
{
	s16 childNindex=LINDEX_TO_NINDEX(childLeafProxy->lindex);
	u16 childCount=parentBranchProxy->childCount;

	if(sibdexEstimate>=0 && sibdexEstimate<childCount && parentBranchProxy->dataBlock->childNindex[sibdexEstimate]==childNindex)
		return sibdexEstimate;

//	LOG(LOG_INFO,"Parent: %i Leaf: %i as %i",parent->brindex,child->lindex,childNindex);

//	LOG(LOG_INFO,"Childcount: %i",childCount);


	for(int i=0;i<childCount;i++)
		{
//		LOG(LOG_INFO,"Nindex: %i",parent->dataBlock->childNindex[i]);

		if(parentBranchProxy->dataBlock->childNindex[i]==childNindex)
			return i;
		}

	return -1;
}


s16 getBranchChildSibdex_Leaf(RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy)
{
	s16 childNindex=LINDEX_TO_NINDEX(childLeafProxy->lindex);
	u16 childCount=parentBranchProxy->childCount;

	for(int i=0;i<childCount;i++)
		if(parentBranchProxy->dataBlock->childNindex[i]==childNindex)
			return i;

	return -1;
}



s32 getBranchChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, s16 sibdex,
		RouteTableTreeBranchProxy **childBranchProxyPtr, RouteTableTreeLeafProxy **childLeafProxyPtr)
{
	u16 childCount=parentBranchProxy->childCount;

	if(sibdex<0 || sibdex>=childCount)
		{
		*childBranchProxyPtr=NULL;
		*childLeafProxyPtr=NULL;
		return 0;
		}

	s16 childNindex=parentBranchProxy->dataBlock->childNindex[sibdex];

	if(childNindex>=0)
		{
		*childBranchProxyPtr=getRouteTableTreeBranchProxy(treeProxy, childNindex);
		*childLeafProxyPtr=NULL;
		}
	else
		{
		*childBranchProxyPtr=NULL;
		*childLeafProxyPtr=getRouteTableTreeLeafProxy(treeProxy, childNindex);
		}

	return 1;

}


RouteTableTreeBranchProxy *getBranchParent(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *childBranchProxy)
{
	return getRouteTableTreeBranchProxy(treeProxy, childBranchProxy->dataBlock->parentBrindex);
}


RouteTableTreeBranchProxy *getLeafParent(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *childLeafProxy)
{
	return getRouteTableTreeBranchProxy(treeProxy, childLeafProxy->dataBlock->parentBrindex);
}



