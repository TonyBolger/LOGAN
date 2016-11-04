
#include "common.h"


#define ARRAY_TYPE_SHALLOW_PTR 0
#define ARRAY_TYPE_DEEP_PTR 1
#define ARRAY_TYPE_SHALLOW_DATA 2
#define ARRAY_TYPE_DEEP_DATA 3


RouteTableTreeLeafBlock *reallocRouteTableTreeLeafBlockEntries(RouteTableTreeLeafBlock *oldBlock, MemDispenser *disp, s32 entryAlloc)
{
	if(entryAlloc>ROUTE_TABLE_TREE_LEAF_ENTRIES)
		{
		LOG(LOG_CRITICAL,"Cannot allocate oversize leaf with %i children",entryAlloc);
		}

	RouteTableTreeLeafBlock *block=dAlloc(disp, sizeof(RouteTableTreeLeafBlock)+entryAlloc*sizeof(RouteTableTreeLeafEntry));

	s32 oldBlockEntryAlloc=0;
	if(oldBlock!=NULL)
		{
		oldBlockEntryAlloc=oldBlock->entryAlloc;
		s32 toKeepAlloc=MIN(oldBlockEntryAlloc, entryAlloc);

		memcpy(block,oldBlock, sizeof(RouteTableTreeLeafBlock)+toKeepAlloc*sizeof(RouteTableTreeLeafEntry));
		block->entryAlloc=entryAlloc;
		}

	for(int i=oldBlockEntryAlloc;i<entryAlloc;i++)
		{
		block->entries[i].downstream=-1;
		block->entries[i].width=0;
		}

	block->entryAlloc=entryAlloc;

	return block;
}


RouteTableTreeLeafBlock *allocRouteTableTreeLeafBlock(MemDispenser *disp, s32 entryAlloc)
{
	if(entryAlloc>ROUTE_TABLE_TREE_LEAF_ENTRIES)
			{
			LOG(LOG_CRITICAL,"Cannot allocate oversize leaf with %i children",entryAlloc);
			}

	RouteTableTreeLeafBlock *block=dAlloc(disp, sizeof(RouteTableTreeLeafBlock)+entryAlloc*sizeof(RouteTableTreeLeafEntry));
	block->entryAlloc=entryAlloc;

	block->parentBrindex=BRANCH_NINDEX_INVALID;
	block->upstream=-1;

	for(int i=0;i<entryAlloc;i++)
		{
		block->entries[i].downstream=-1;
		block->entries[i].width=0;
		}

	return block;
}


RouteTableTreeBranchBlock *reallocRouteTableTreeBranchBlockEntries(RouteTableTreeBranchBlock *oldBlock, MemDispenser *disp, s32 childAlloc)
{
	if(childAlloc>ROUTE_TABLE_TREE_BRANCH_CHILDREN)
		{
		LOG(LOG_CRITICAL,"Cannot allocate oversize branch with %i children",childAlloc);
		}

	RouteTableTreeBranchBlock *block=dAlloc(disp, sizeof(RouteTableTreeBranchBlock)+childAlloc*sizeof(s16));
	block->parentBrindex=BRANCH_NINDEX_INVALID;
	block->upstreamMin=-1;
	block->upstreamMax=-1;

	s32 oldBlockChildAlloc=0;
	if(oldBlock!=NULL)
		{
		oldBlockChildAlloc=oldBlock->childAlloc;
		s32 toKeepAlloc=MIN(oldBlockChildAlloc, childAlloc);

		memcpy(block,oldBlock, sizeof(RouteTableTreeLeafBlock)+toKeepAlloc*sizeof(RouteTableTreeLeafEntry));
		}

	for(int i=oldBlockChildAlloc;i<childAlloc;i++)
		block->childNindex[i]=BRANCH_NINDEX_INVALID;

	block->childAlloc=childAlloc;

	return block;
}

RouteTableTreeBranchBlock *allocRouteTableTreeBranchBlock(MemDispenser *disp, s32 childAlloc)
{
	if(childAlloc>ROUTE_TABLE_TREE_BRANCH_CHILDREN)
		{
		LOG(LOG_CRITICAL,"Cannot allocate oversize branch with %i children",childAlloc);
		}

	RouteTableTreeBranchBlock *block=dAlloc(disp, sizeof(RouteTableTreeBranchBlock)+childAlloc*sizeof(s16));
	block->childAlloc=childAlloc;
	block->parentBrindex=BRANCH_NINDEX_INVALID;
	block->upstreamMin=-1;
	block->upstreamMax=-1;

	for(int i=0;i<childAlloc;i++)
		block->childNindex[i]=BRANCH_NINDEX_INVALID;

	return block;
}

RouteTableTreeArrayBlock *allocRouteTableTreeDataArrayBlock(MemDispenser *disp, s32 dataAlloc)
{
	if(dataAlloc>ROUTE_TABLE_TREE_DATA_ARRAY_ENTRIES)
			{
			LOG(LOG_CRITICAL,"Cannot allocate oversize data array with %i children",dataAlloc);
			}

	RouteTableTreeArrayBlock *block=dAlloc(disp, sizeof(RouteTableTreeArrayBlock)+dataAlloc*sizeof(u8 *));
	block->dataAlloc=dataAlloc;

	for(int i=0;i<dataAlloc;i++)
		block->data[i]=NULL;


	return block;
}

RouteTableTreeArrayBlock *allocRouteTableTreePtrArrayBlock(MemDispenser *disp, s32 dataCount)
{
	if(dataCount>ROUTE_TABLE_TREE_PTR_ARRAY_ENTRIES)
			{
			LOG(LOG_CRITICAL,"Cannot allocate oversize data array with %i children",dataCount);
			}

	RouteTableTreeArrayBlock *block=dAlloc(disp, sizeof(RouteTableTreeArrayBlock)+dataCount*sizeof(u8 *));
	block->dataAlloc=dataCount;
	return block;
}










static void initBlockArrayProxy_scan(RouteTableTreeArrayBlock *arrayBlock, u16 *allocPtr, u16 *countPtr)
{
	if(arrayBlock==NULL)
		{
		*allocPtr=0;
		*countPtr=0;
		return;
		}

	u16 alloc=arrayBlock->dataAlloc;
	*allocPtr=alloc;
	u16 count=0;

	for(int i=alloc;i>0;i--)
		{
		if(arrayBlock->data[i-1]!=NULL)
			{
			count=i;
			break;
			}
		}

	*countPtr=count;
}

static void initBlockArrayProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeArrayProxy *arrayProxy, HeapDataBlock *heapDataBlock, u32 arrayType)
{
	arrayProxy->heapDataBlock=heapDataBlock;

	switch(arrayType)
		{
		case ARRAY_TYPE_SHALLOW_PTR:
			LOG(LOG_CRITICAL,"Not implemented");
			break;

		case ARRAY_TYPE_DEEP_PTR:
			LOG(LOG_CRITICAL,"Invalid DeepPtr format for top-level block");
			break;

		case ARRAY_TYPE_SHALLOW_DATA:
			arrayProxy->ptrBlock=NULL;
			if(heapDataBlock->blockPtr!=NULL)
				arrayProxy->dataBlock=(RouteTableTreeArrayBlock *)(heapDataBlock->blockPtr+heapDataBlock->headerSize);
			else
				arrayProxy->dataBlock=NULL;
			break;

		case ARRAY_TYPE_DEEP_DATA:
			LOG(LOG_CRITICAL,"Invalid DeepData format for top-level block");
			break;
		}

	initBlockArrayProxy_scan(arrayProxy->ptrBlock, &arrayProxy->ptrAlloc, &arrayProxy->ptrCount);
	initBlockArrayProxy_scan(arrayProxy->dataBlock, &arrayProxy->dataAlloc, &arrayProxy->dataCount);

	arrayProxy->newData=NULL;
	arrayProxy->newDataAlloc=0;
	arrayProxy->newDataCount=0;

}


//static
s32 getBlockArraySize(RouteTableTreeArrayProxy *arrayProxy)
{
	if(arrayProxy->newDataCount>0)
		return arrayProxy->newDataCount;

	return arrayProxy->dataCount;
}

//static
u8 *getBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, s32 index)
{
	if(arrayProxy->ptrBlock!=NULL)
		{
		LOG(LOG_CRITICAL,"Two level arrays not implemented");
		}

	if(index>=0 && index<arrayProxy->newDataCount)
		{
		u8 *newData=arrayProxy->newData[index];

		if(newData!=NULL)
			return newData;
		}

	if(index>=0 && index<arrayProxy->dataCount)
		return arrayProxy->dataBlock->data[index];

	return NULL;

}

void ensureBlockArrayWritable(RouteTableTreeArrayProxy *arrayProxy, MemDispenser *disp)
{
	if(arrayProxy->newData==NULL) // Haven't yet written anything - make writable
		{
		int newAlloc=arrayProxy->dataAlloc;

		if(arrayProxy->dataCount==newAlloc)
			newAlloc+=ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK;

		u8 **newData=dAlloc(disp, sizeof(u8 *)*newAlloc);

		if(newAlloc>arrayProxy->dataCount)
			memset(newData+sizeof(u8 *)*arrayProxy->dataCount, 0, sizeof(u8 *)*newAlloc);

		arrayProxy->newData=newData;
		arrayProxy->newDataAlloc=newAlloc;
		arrayProxy->newDataCount=arrayProxy->dataCount;
		}

}

//static
void setBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, s32 index, u8 *data, MemDispenser *disp)
{
	ensureBlockArrayWritable(arrayProxy, disp);

	if(arrayProxy->ptrBlock!=NULL)
		{
		LOG(LOG_CRITICAL,"Two level arrays not implemented");
		}

	if(index>=0 && index<arrayProxy->newDataCount)
		arrayProxy->newData[index]=data;
	else
		LOG(LOG_CRITICAL,"Invalid array index %i - Size %i",index,arrayProxy->newDataCount);
}

//static
s32 appendBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, u8 *data, MemDispenser *disp)
{
	ensureBlockArrayWritable(arrayProxy, disp);

	if(arrayProxy->ptrBlock!=NULL)
		{
		LOG(LOG_CRITICAL,"Two level arrays not implemented");
		}

	if(arrayProxy->newDataCount>=arrayProxy->newDataAlloc)
		{
		int newAllocCount=arrayProxy->newDataAlloc+ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK;

		if(newAllocCount>ROUTE_TABLE_TREE_DATA_ARRAY_ENTRIES)
			{
			LOG(LOG_CRITICAL,"Need to allocate more than %i entries (%i)",ROUTE_TABLE_TREE_DATA_ARRAY_ENTRIES, newAllocCount);
			}

		u8 **newData=dAlloc(disp, sizeof(u8 *)*newAllocCount);
		memcpy(newData,arrayProxy->newData, sizeof(u8 *)*newAllocCount);

		arrayProxy->newData=newData;
		arrayProxy->newDataAlloc=newAllocCount;
		}

	s32 index=arrayProxy->newDataCount++;
	arrayProxy->newData[index]=data;

	return index;
}



//static
void getRouteTableTreeBranchProxy_scan(RouteTableTreeBranchBlock *branchBlock, u16 *allocPtr, u16 *countPtr)
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


// static
RouteTableTreeBranchBlock *getRouteTableTreeBranchRaw(RouteTableTreeProxy *treeProxy, s32 brindex)
{
	if(brindex<0)
		{
		LOG(LOG_CRITICAL,"Brindex must be positive");
		}

	u8 *data=getBlockArrayEntry(&(treeProxy->branchArrayProxy), brindex);

	return (RouteTableTreeBranchBlock *)data;
}

// static
RouteTableTreeBranchProxy *getRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, s32 brindex)
{
	if(brindex<0)
		{
		LOG(LOG_CRITICAL,"Brindex must be positive");
		}

	u8 *data=getBlockArrayEntry(&(treeProxy->branchArrayProxy), brindex);

	RouteTableTreeBranchProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeBranchProxy));

	proxy->dataBlock=(RouteTableTreeBranchBlock *)data;
	proxy->brindex=brindex;

	getRouteTableTreeBranchProxy_scan(proxy->dataBlock, &proxy->childAlloc, &proxy->childCount);

	return proxy;
}

// static
void flushRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy)
{
	setBlockArrayEntry(&(treeProxy->branchArrayProxy), branchProxy->brindex, (u8 *)branchProxy->dataBlock, treeProxy->disp);
}

//static
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


//static
void getRouteTableTreeLeafProxy_scan(RouteTableTreeLeafBlock *leafBlock, u16 *allocPtr, u16 *countPtr)
{
	if(leafBlock==NULL)
		{
		*allocPtr=0;
		*countPtr=0;
		return;
		}

	u16 alloc=leafBlock->entryAlloc;
	*allocPtr=alloc;
	u16 count=0;

	for(int i=alloc;i>0;i--)
		{
		if(leafBlock->entries[i-1].width!=0)
			{
			count=i;
			break;
			}
		}

	*countPtr=count;
}


// static
RouteTableTreeLeafBlock *getRouteTableTreeLeafRaw(RouteTableTreeProxy *treeProxy, s32 lindex)
{
	if(lindex<0)
		{
		LOG(LOG_CRITICAL,"Lindex must be positive");
		}

	u8 *data=getBlockArrayEntry(&(treeProxy->leafArrayProxy), lindex);

	return (RouteTableTreeLeafBlock *)data;
}

// static
RouteTableTreeLeafProxy *getRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 lindex)
{
	if(lindex<0)
		{
		LOG(LOG_CRITICAL,"Lindex must be positive");
		}

	u8 *data=getBlockArrayEntry(&(treeProxy->leafArrayProxy), lindex);

	RouteTableTreeLeafProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeLeafProxy));

	proxy->dataBlock=(RouteTableTreeLeafBlock *)data;
	proxy->lindex=lindex;

//	LOG(LOG_INFO,"GetRouteTableTreeLeaf : %i",lindex);

	getRouteTableTreeLeafProxy_scan(proxy->dataBlock, &proxy->entryAlloc, &proxy->entryCount);

	return proxy;
}

// static
void flushRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy)
{
	LOG(LOG_INFO,"Flush %i to %p (%i)",leafProxy->lindex, leafProxy->dataBlock, leafProxy->dataBlock->entryAlloc);

	setBlockArrayEntry(&(treeProxy->leafArrayProxy), leafProxy->lindex, (u8 *)leafProxy->dataBlock, treeProxy->disp);
}

//static
RouteTableTreeLeafProxy *allocRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 entryAlloc)
{
	if(entryAlloc<ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK)
		entryAlloc=ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK;

	RouteTableTreeLeafBlock *dataBlock=allocRouteTableTreeLeafBlock(treeProxy->disp, entryAlloc);
	s32 lindex=appendBlockArrayEntry(&(treeProxy->leafArrayProxy), (u8 *)dataBlock, treeProxy->disp);

	RouteTableTreeLeafProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeLeafProxy));
	proxy->dataBlock=dataBlock;
	proxy->lindex=lindex;

	LOG(LOG_INFO,"AllocRouteTableTreeLeaf : %i",lindex);

	proxy->entryAlloc=entryAlloc;
	proxy->entryCount=0;

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

void expandRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy)
{
	s32 newAlloc=MIN(leafProxy->entryAlloc+ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK, ROUTE_TABLE_TREE_LEAF_ENTRIES);

	RouteTableTreeLeafBlock *leafBlock=reallocRouteTableTreeLeafBlockEntries(leafProxy->dataBlock, treeProxy->disp, newAlloc);

	leafProxy->dataBlock=leafBlock;
	leafProxy->entryAlloc=newAlloc;

	flushRouteTableTreeLeafProxy(treeProxy, leafProxy);
}


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


void leafMakeEntryInsertSpace(RouteTableTreeLeafProxy *leaf, s16 entryPosition, s16 entryCount)
{
	if((leaf->entryCount+entryCount)>leaf->entryAlloc)
			{
			LOG(LOG_CRITICAL,"Insufficient space for branch child insert");
			}


	if(entryPosition<leaf->entryCount)
		{
		s16 entriesToMove=leaf->entryCount-entryPosition;

		RouteTableTreeLeafEntry *entryPtr=leaf->dataBlock->entries+entryPosition;
		memmove(entryPtr+entryCount, entryPtr, sizeof(RouteTableTreeLeafEntry)*entriesToMove);
		}

	leaf->entryCount+=entryCount;
}



//static
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

	LOG(LOG_INFO,"Parent: %i Leaf: %i as %i",parent->brindex,child->lindex,childNindex);

	LOG(LOG_INFO,"Childcount: %i",childCount);


	for(int i=0;i<childCount;i++)
		{
		LOG(LOG_INFO,"Nindex: %i",parent->dataBlock->childNindex[i]);

		if(parent->dataBlock->childNindex[i]==childNindex)
			return i;
		}

	return -1;
}


//static
s16 getBranchChildSibdex_Leaf(RouteTableTreeBranchProxy *parent, RouteTableTreeLeafProxy *child)
{
	s16 childNindex=LINDEX_TO_NINDEX(child->lindex);
	u16 childCount=parent->childCount;

	for(int i=0;i<childCount;i++)
		if(parent->dataBlock->childNindex[i]==childNindex)
			return i;

	return -1;
}




RouteTableTreeBranchProxy *getBranchParent(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *child)
{
	return getRouteTableTreeBranchProxy(treeProxy, child->dataBlock->parentBrindex);
}

RouteTableTreeBranchProxy *getLeafParent(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *child)
{
	return getRouteTableTreeBranchProxy(treeProxy, child->dataBlock->parentBrindex);
}


// static
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


static void initTreeProxy(RouteTableTreeProxy *proxy, HeapDataBlock *leafBlock, HeapDataBlock *branchBlock, HeapDataBlock *offsetBlock, MemDispenser *disp)
{
	proxy->disp=disp;

	initBlockArrayProxy(proxy, &(proxy->leafArrayProxy), leafBlock, 2);
	initBlockArrayProxy(proxy, &(proxy->branchArrayProxy), branchBlock, 2);
	initBlockArrayProxy(proxy, &(proxy->offsetArrayProxy), offsetBlock, 2);

	if(getBlockArraySize(&(proxy->branchArrayProxy))>0)
		proxy->rootProxy=getRouteTableTreeBranchProxy(proxy, BRANCH_NINDEX_ROOT);
	else
		proxy->rootProxy=allocRouteTableTreeBranchProxy(proxy, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

}


static void treeProxySeekStart(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchPtr, s16 *branchChildSibdexPtr, RouteTableTreeLeafProxy **leafPtr)
{
	RouteTableTreeBranchProxy *branchProxy=treeProxy->rootProxy;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 sibdex=-1;

	s16 childNindex=0;

	while(branchProxy->childCount>0 && childNindex>=0)
		{
		childNindex=branchProxy->dataBlock->childNindex[0];

		if(childNindex>0)
			branchProxy=getRouteTableTreeBranchProxy(treeProxy, childNindex);
		}

	if(childNindex<0)
		{
		sibdex=0;

		LOG(LOG_INFO,"Get %i as %i",childNindex, NINDEX_TO_LINDEX(childNindex));

		leafProxy=getRouteTableTreeLeafProxy(treeProxy,NINDEX_TO_LINDEX(childNindex));
		}

	if(leafProxy!=NULL)
		{
		LOG(LOG_INFO,"Branch %i Child %i (%i of %i) Sibdex %i",branchProxy->brindex,leafProxy->lindex,leafProxy->entryCount,leafProxy->entryAlloc, sibdex);
		}
	else
		{
		LOG(LOG_INFO,"Branch %i Null Leaf",branchProxy->brindex);
		}

	if(branchPtr!=NULL)
		*branchPtr=branchProxy;

	if(branchChildSibdexPtr!=NULL)
		*branchChildSibdexPtr=sibdex;

	if(leafPtr!=NULL)
		*leafPtr=leafProxy;
}


static void treeProxySeekEnd(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchPtr,  s16 *branchChildSibdexPtr, RouteTableTreeLeafProxy **leafPtr)
{
	RouteTableTreeBranchProxy *branchProxy=treeProxy->rootProxy;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 sibdex=-1;

	s16 childNindex=0;

	while(branchProxy->childCount>0 && childNindex>=0)
		{
		childNindex=branchProxy->dataBlock->childNindex[branchProxy->childCount-1];

		if(childNindex>0)
			{
			branchProxy=getRouteTableTreeBranchProxy(treeProxy, childNindex);
			}
		}

	if(childNindex<0)
		{
		sibdex=branchProxy->childCount-1;
		leafProxy=getRouteTableTreeLeafProxy(treeProxy,NINDEX_TO_LINDEX(childNindex));
		}

	if(leafProxy!=NULL)
		{
		LOG(LOG_INFO,"Branch %i Child %i Sibdex %i",branchProxy->brindex,leafProxy->lindex,sibdex);
		}
	else
		{
		LOG(LOG_INFO,"Branch %i Null Leaf",branchProxy->brindex);
		}

	if(branchPtr!=NULL)
		*branchPtr=branchProxy;

	if(branchChildSibdexPtr!=NULL)
		*branchChildSibdexPtr=sibdex;

	if(leafPtr!=NULL)
		*leafPtr=leafProxy;

}


//static
void treeProxySplitRoot(RouteTableTreeProxy *treeProxy, s16 childPosition, RouteTableTreeBranchProxy **newParentPtr, s16 *newChildPositionPtr)
{
	LOG(LOG_INFO,"Root Split");

	RouteTableTreeBranchProxy *root=treeProxy->rootProxy;

	if(root->brindex!=BRANCH_NINDEX_ROOT)
		{
		LOG(LOG_CRITICAL,"Asked to root-split a non-root node");
		}

	LOG(LOG_INFO,"Root %i contains %i of %i",root->brindex, root->childCount, root->childAlloc);

	for(int i=0;i<root->childCount;i++)
		{
		LOG(LOG_INFO,"Child %i is %i",i,root->dataBlock->childNindex[i]);
		}

	s32 halfChild=((1+root->childCount)/2);
	s32 otherHalfChild=root->childCount-halfChild;

	s32 halfChildAlloc=halfChild+1;

	RouteTableTreeBranchProxy *branch1=allocRouteTableTreeBranchProxy(treeProxy, halfChildAlloc);
	RouteTableTreeBranchProxy *branch2=allocRouteTableTreeBranchProxy(treeProxy, halfChildAlloc);

	s32 branchBrindex1=branch1->brindex;
	s32 branchBrindex2=branch2->brindex;

	LOG(LOG_INFO,"Split - new branches %i %i",branchBrindex1, branchBrindex2);

	for(int i=0;i<halfChild;i++)
		{
		s32 childNindex=root->dataBlock->childNindex[i];
		branch1->dataBlock->childNindex[i]=childNindex;

		LOG(LOG_INFO,"Moving Child %i to %i",childNindex, branchBrindex1);

		if(childNindex<0)
			{
			RouteTableTreeLeafBlock *leafRaw=getRouteTableTreeLeafRaw(treeProxy, NINDEX_TO_LINDEX(childNindex));

			if(leafRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find leaf with index %i",childNindex);
				}

			leafRaw->parentBrindex=branchBrindex1;
			}
		else
			{
			RouteTableTreeBranchBlock *branchRaw=getRouteTableTreeBranchRaw(treeProxy, childNindex);

			if(branchRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find branch with index %i",childNindex);
				}

			branchRaw->parentBrindex=branchBrindex1;
			}
		}

	branch1->childCount=halfChild;

	for(int i=0;i<otherHalfChild;i++)
		{
		s32 childNindex=root->dataBlock->childNindex[i+halfChild];
		branch2->dataBlock->childNindex[i]=childNindex;

		LOG(LOG_INFO,"Moving Child %i to %i",childNindex, branchBrindex2);

		if(childNindex<0)
			{
			RouteTableTreeLeafBlock *leafRaw=getRouteTableTreeLeafRaw(treeProxy, NINDEX_TO_LINDEX(childNindex));

			if(leafRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find leaf with index %i",childNindex);
				}

			leafRaw->parentBrindex=branchBrindex2;
			}
		else
			{
			RouteTableTreeBranchBlock *branchRaw=getRouteTableTreeBranchRaw(treeProxy, childNindex);

			if(branchRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find branch with index %i",childNindex);
				}

			branchRaw->parentBrindex=branchBrindex2;
			}
		}

	branch2->childCount=otherHalfChild;

	branch1->dataBlock->parentBrindex=BRANCH_NINDEX_ROOT;
	branch2->dataBlock->parentBrindex=BRANCH_NINDEX_ROOT;

	RouteTableTreeBranchBlock *newRoot=reallocRouteTableTreeBranchBlockEntries(NULL, treeProxy->disp, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

	newRoot->parentBrindex=BRANCH_NINDEX_INVALID;
	newRoot->childNindex[0]=branchBrindex1;
	newRoot->childNindex[1]=branchBrindex2;

	root->childCount=2;
	root->childAlloc=newRoot->childAlloc;
	root->dataBlock=newRoot;

	flushRouteTableTreeBranchProxy(treeProxy, root);

	if(childPosition<halfChild)
		{
		*newParentPtr=branch1;
		*newChildPositionPtr=childPosition;
		}
	else
		{
		*newParentPtr=branch2;
		*newChildPositionPtr=childPosition-halfChild;
		}
}



// Split a branch, creating and returning a new sibling which _MUST_ be attached by the caller. Location of an existing child is updated

RouteTableTreeBranchProxy *treeProxySplitBranch(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branch, s16 childPosition,
		RouteTableTreeBranchProxy **newParentPtr, s16 *newChildPositionPtr)
{
	LOG(LOG_INFO,"Non Root Branch Split: %i",branch->brindex);

	s32 halfChild=((1+branch->childCount)/2);
	s32 otherHalfChild=branch->childCount-halfChild;

	s32 halfChildAlloc=halfChild+1;

	RouteTableTreeBranchProxy *newBranch=allocRouteTableTreeBranchProxy(treeProxy, halfChildAlloc);

	s32 newBranchBrindex=newBranch->brindex;
	branch->childCount=halfChild;

	for(int i=0;i<otherHalfChild;i++)
		{
		LOG(LOG_INFO,"Index %i",i);

		s32 childNindex=branch->dataBlock->childNindex[i+halfChild];
		newBranch->dataBlock->childNindex[i]=childNindex;
		branch->dataBlock->childNindex[i+halfChild]=BRANCH_NINDEX_INVALID;

		LOG(LOG_INFO,"Moving Child %i to %i",childNindex, newBranchBrindex);

		if(childNindex<0)
			{
			RouteTableTreeLeafBlock *leafRaw=getRouteTableTreeLeafRaw(treeProxy, NINDEX_TO_LINDEX(childNindex));

			if(leafRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find leaf with index %i",childNindex);
				}

			leafRaw->parentBrindex=newBranchBrindex;
			}
		else
			{
			RouteTableTreeBranchBlock *branchRaw=getRouteTableTreeBranchRaw(treeProxy, childNindex);

			if(branchRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find branch with index %i",childNindex);
				}

			branchRaw->parentBrindex=newBranchBrindex;
			}
		}

	newBranch->childCount=otherHalfChild;

	if(childPosition<halfChild)
		{
		*newParentPtr=branch;
		*newChildPositionPtr=childPosition;
		}
	else
		{
		*newParentPtr=newBranch;
		*newChildPositionPtr=childPosition-halfChild;
		}

	return newBranch;
}



// Split a leaf, creating and returning a new sibling which _MUST_ be attached by the caller. Location of an existing entry is updated

RouteTableTreeLeafProxy *treeProxySplitLeaf(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leaf, s16 leafEntryPosition, s16 space,
		 RouteTableTreeLeafProxy **newLeafPtr, s16 *newEntryPositionPtr)
{
	LOG(LOG_INFO,"Leaf Split: %i",leaf->lindex);

	s32 halfEntry=((1+leaf->entryCount)/2);
	s32 otherHalfEntry=leaf->entryCount-halfEntry;

	s32 halfEntryAlloc=halfEntry+space;

	RouteTableTreeLeafProxy *newLeaf=allocRouteTableTreeLeafProxy(treeProxy, halfEntryAlloc);

	leaf->entryCount=halfEntry;

	memcpy(newLeaf->dataBlock->entries, leaf->dataBlock->entries+halfEntry, sizeof(RouteTableTreeLeafEntry)*otherHalfEntry);
	memset(leaf->dataBlock->entries+halfEntry, 0, sizeof(RouteTableTreeLeafEntry)*otherHalfEntry);

	newLeaf->dataBlock->upstream=leaf->dataBlock->upstream;
	newLeaf->entryCount=otherHalfEntry;

	if(leafEntryPosition<halfEntry)
		{
		*newLeafPtr=leaf;
		*newEntryPositionPtr=leafEntryPosition;
		}
	else
		{
		*newLeafPtr=newLeaf;
		*newEntryPositionPtr=leafEntryPosition-halfEntry;
		}

	return newLeaf;
}





// Insert the given branch child to the branch parent, splitting the parent if needed

//static
void treeProxyInsertBranchChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parent, RouteTableTreeBranchProxy *child, s16 childPosition,
		RouteTableTreeBranchProxy **newParentPtr, s16 *newChildPositionPtr)
{
	LOG(LOG_INFO,"Append Branch Child P: %i C: %i",parent->brindex, child->brindex);


	LOG(LOG_INFO,"Parent %i contains %i of %i",parent->brindex, parent->childCount, parent->childAlloc);

	for(int i=0;i<parent->childCount;i++)
		{
		LOG(LOG_INFO,"Child %i is %i",i,parent->dataBlock->childNindex[i]);
		}


	if(parent->childCount>=parent->childAlloc) // No Space
		{
		if(parent->childAlloc<ROUTE_TABLE_TREE_BRANCH_CHILDREN) // Expand
			{
			expandRouteTableTreeBranchProxy(treeProxy, parent);
			}
		else if(parent->brindex!=BRANCH_NINDEX_ROOT) // Add sibling
			{
			LOG(LOG_INFO,"AppendBranch %i: Sibling",parent->brindex);

			RouteTableTreeBranchProxy *grandParent;
			s32 grandParentBrindex=parent->dataBlock->parentBrindex;

			if(grandParentBrindex!=BRANCH_NINDEX_ROOT)
				grandParent=getRouteTableTreeBranchProxy(treeProxy, grandParentBrindex);
			else
				grandParent=treeProxy->rootProxy;

			s16 parentSibdex=getBranchChildSibdex_Branch(grandParent, parent);
			RouteTableTreeBranchProxy *newParent=treeProxySplitBranch(treeProxy, parent, childPosition, &parent, &childPosition);

			RouteTableTreeBranchProxy *dummyParentPtr; // Don't track splits of grandparent / position
			s16 dummyChildPositionPtr;

			treeProxyInsertBranchChild(treeProxy, grandParent, newParent, parentSibdex+1, &dummyParentPtr, &dummyChildPositionPtr);

			}
		else	// Split root
			{
			LOG(LOG_INFO,"InsertBranchChild %i: Split Root",parent->brindex);

			treeProxySplitRoot(treeProxy, childPosition, &parent, &childPosition);
			}

		}

	if(parent->childCount>=parent->childAlloc)
		{
		LOG(LOG_CRITICAL,"No space after split/sibling add in branch node %i", parent->brindex);
		}

	s16 parentBrindex=parent->brindex;
	s16 childBrindex=child->brindex;

	LOG(LOG_INFO,"Inserting Branch %i to %i",childBrindex, parentBrindex);

	branchMakeChildInsertSpace(parent, childPosition, 1);

	parent->dataBlock->childNindex[childPosition]=childBrindex;
	child->dataBlock->parentBrindex=parentBrindex;

	*newParentPtr=parent;
	*newChildPositionPtr=childPosition;

	LOG(LOG_INFO,"ParentNow %i contains %i of %i",parent->brindex, parent->childCount, parent->childAlloc);

	for(int i=0;i<parent->childCount;i++)
		{
		LOG(LOG_INFO,"ChildNow %i is %i",i,parent->dataBlock->childNindex[i]);
		}

}

// Insert the given leaf child into the branch parent, splitting the parent if needed
static void treeProxyInsertLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parent, RouteTableTreeLeafProxy *child, s16 childPosition,
		RouteTableTreeBranchProxy **newParentPtr, s16 *newChildPositionPtr)
{
	LOG(LOG_INFO,"Append Leaf Child P: %i C: %i",parent->brindex, child->lindex);

	LOG(LOG_INFO,"Parent contains %i of %i",parent->childCount, parent->childAlloc);

	if(parent->childCount>=parent->childAlloc)
		{
		if(parent->childAlloc<ROUTE_TABLE_TREE_BRANCH_CHILDREN)
			{
			expandRouteTableTreeBranchProxy(treeProxy, parent);
			}
		else if(parent->brindex!=BRANCH_NINDEX_ROOT)
			{
			RouteTableTreeBranchProxy *grandParent;
			s32 grandParentBrindex=parent->dataBlock->parentBrindex;

			if(grandParentBrindex!=BRANCH_NINDEX_ROOT)
				grandParent=getRouteTableTreeBranchProxy(treeProxy, grandParentBrindex);
			else
				grandParent=treeProxy->rootProxy;

			LOG(LOG_INFO,"Here");
			s16 parentSibdex=getBranchChildSibdex_Branch(grandParent, parent);
			LOG(LOG_INFO,"Here");
			RouteTableTreeBranchProxy *newParent=treeProxySplitBranch(treeProxy, parent, childPosition, &parent, &childPosition);
			LOG(LOG_INFO,"Here");
			RouteTableTreeBranchProxy *dummyParentPtr; // Don't track splits of grandparent / position
			s16 dummyChildPositionPtr;
			LOG(LOG_INFO,"Here");

			LOG(LOG_INFO,"Old Parent is %i, New parent is %i, Grandparent is %i",parent->brindex, newParent->brindex, grandParentBrindex);

			treeProxyInsertBranchChild(treeProxy, grandParent, newParent, parentSibdex+1, &dummyParentPtr, &dummyChildPositionPtr);
			LOG(LOG_INFO,"Here");
			}
		else
			{
			treeProxySplitRoot(treeProxy, childPosition, &parent, &childPosition);
			}
		}

	if(parent->childCount>=parent->childAlloc)
		{
		LOG(LOG_CRITICAL,"No space after split/sibling add");
		}

	s16 parentBrindex=parent->brindex;
	s16 childLindex=child->lindex;
	branchMakeChildInsertSpace(parent, childPosition, 1);

	parent->dataBlock->childNindex[childPosition]=LINDEX_TO_NINDEX(childLindex);
	child->dataBlock->parentBrindex=parentBrindex;

	*newParentPtr=parent;
	*newChildPositionPtr=childPosition;

}

static void treeProxyAppendLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parent, RouteTableTreeLeafProxy *child,
		RouteTableTreeBranchProxy **newParentPtr, s16 *childPositionPtr)
{
	s16 childPosition=parent->childCount;

	treeProxyInsertLeafChild(treeProxy, parent, child, childPosition, newParentPtr, &childPosition);
	*childPositionPtr=childPosition;
}


// Split a leaf, allowing space to insert one/two leaf entries at the specified position, returning the entry index of the first space, and providing the new context

//static

void treeProxySplitLeafInsertChildEntrySpace(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parent, s16 childPosition, RouteTableTreeLeafProxy *child,
		s16 insertEntryPosition, s16 insertEntryCount,
		RouteTableTreeBranchProxy **newParentPtr, s16 *newChildPositionPtr, RouteTableTreeLeafProxy **newChildPtr, s16 *newEntryPositionPtr)
{
	RouteTableTreeLeafProxy *newLeaf=treeProxySplitLeaf(treeProxy, child, insertEntryPosition, insertEntryCount,  &child, &insertEntryPosition);

	treeProxyInsertLeafChild(treeProxy, parent, newLeaf, childPosition+1, newParentPtr, newChildPositionPtr);

	leafMakeEntryInsertSpace(child, insertEntryPosition, insertEntryCount);

	*newChildPtr=child;
	*newEntryPositionPtr=insertEntryPosition;
}


static s32 getNextBranchSibling(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchPtr)
{
	RouteTableTreeBranchProxy *branch=*branchPtr;

	LOG(LOG_INFO,"getNextBranchSibling for %i",branch->brindex);

	//LOG(LOG_CRITICAL,"FIXME - %i",branch->brindex);

	if(branch->dataBlock->parentBrindex!=BRANCH_NINDEX_INVALID) // Should never happen
		{
		RouteTableTreeBranchProxy *parent=getRouteTableTreeBranchProxy(treeProxy, branch->dataBlock->parentBrindex);

		//LOG(LOG_INFO,"Branch is %i - Parent is %i",branch->brindex,parent->brindex);

		s16 sibdex=getBranchChildSibdex_Branch(parent, branch);
		sibdex++;

		if(sibdex<parent->childCount) // Parent contains more
			{
			branch=getRouteTableTreeBranchProxy(treeProxy, parent->dataBlock->childNindex[sibdex]);
			*branchPtr=branch;

			LOG(LOG_INFO,"getNextBranchSibling direct branch at %i",branch->brindex);

			return 1;
			}
		else if((branch->dataBlock->parentBrindex!=BRANCH_NINDEX_ROOT) && getNextBranchSibling(treeProxy, &parent))
			{
			LOG(LOG_INFO,"getNextBranchSibling recursive parent at %i",parent->brindex);

			if(parent->childCount>0)
				{
				if(parent->dataBlock->childNindex[0]<0)
					LOG(LOG_CRITICAL,"Parent %i contains child %i",parent->brindex, parent->dataBlock->childNindex[0]);

				branch=getRouteTableTreeBranchProxy(treeProxy, parent->dataBlock->childNindex[0]);
				*branchPtr=branch;

				LOG(LOG_INFO,"getNextBranchSibling recursive branch at %i",branch->brindex);

				return 1;
				}
			else
				LOG(LOG_CRITICAL,"Moved to empty branch");

			}
		}

	//LOG(LOG_INFO,"getNextBranchSibling: end of tree");

	return 0;
}



static s32 getNextLeafSibling(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchPtr, s16 *branchChildSibdex, RouteTableTreeLeafProxy **leafPtr)
{
	RouteTableTreeBranchProxy *branch=*branchPtr;
	RouteTableTreeLeafProxy *leaf=*leafPtr;

	s16 sibdexEst=*branchChildSibdex;
	s16 sibdex=getBranchChildSibdex_Leaf_withEstimate(branch, leaf, sibdexEst);

	LOG(LOG_INFO,"getNextLeafSibling from %i %i %i",branch->brindex, sibdex, leaf->lindex);


	if(sibdexEst!=sibdex)
		{
		LOG(LOG_CRITICAL,"Sibdex did not match expected Est: %i Actual: %i",sibdexEst, sibdex);
		}

	sibdex++;
	if(sibdex<branch->childCount)
		{
		LOG(LOG_INFO,"In branch %i, looking at sib %i giving %i",branch->brindex, sibdex, branch->dataBlock->childNindex[sibdex]);

		*branchChildSibdex=sibdex;
		*leafPtr=getRouteTableTreeLeafProxy(treeProxy, NINDEX_TO_LINDEX(branch->dataBlock->childNindex[sibdex]));
		return 1;
		}

	if(getNextBranchSibling(treeProxy, &branch)) // Moved branch
		{
		*branchPtr=branch;
		if(branch->childCount>0)
			{
			LOG(LOG_INFO,"Moved to new branch");
			sibdex=0;
			}
		else
			{
			sibdex=-1;
			LOG(LOG_CRITICAL,"Moved to empty branch");
			}
		}
	else
		sibdex=-1;


	if(sibdex>=0)
		{
		LOG(LOG_INFO,"In branch %i, looking at sib %i giving %i",branch->brindex, sibdex, branch->dataBlock->childNindex[sibdex]);

		*branchChildSibdex=sibdex;
		*leafPtr=getRouteTableTreeLeafProxy(treeProxy, NINDEX_TO_LINDEX(branch->dataBlock->childNindex[sibdex]));
		return 1;
		}
	else
		{
		return 0;
		}

}




static void initTreeWalker(RouteTableTreeWalker *walker, RouteTableTreeProxy *treeProxy)
{
	walker->treeProxy=treeProxy;

	walker->branchProxy=treeProxy->rootProxy;
	walker->branchChildSibdex=-1;

	walker->leafProxy=NULL;
	walker->leafEntry=-1;
}

static void walkerSeekStart(RouteTableTreeWalker *walker)
{
	RouteTableTreeBranchProxy *branchProxy=NULL;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 branchChildSibdex=0;

	treeProxySeekStart(walker->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy);

	walker->branchProxy=branchProxy;
	walker->branchChildSibdex=branchChildSibdex;
	walker->leafProxy=leafProxy;

	if(leafProxy!=NULL)
		{
		LOG(LOG_INFO,"First Leaf: %i %i",leafProxy->entryCount,leafProxy->entryAlloc);
		walker->leafEntry=0;
		}
	else
		walker->leafEntry=-1;
}

static void walkerSeekEnd(RouteTableTreeWalker *walker)
{
	RouteTableTreeBranchProxy *branchProxy=NULL;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 branchChildSibdex=0;

	treeProxySeekEnd(walker->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy);

	walker->branchProxy=branchProxy;
	walker->branchChildSibdex=branchChildSibdex;
	walker->leafProxy=leafProxy;

	if(leafProxy!=NULL)
		walker->leafEntry=leafProxy->entryCount;
	else
		walker->leafEntry=-1;
}

static s32 walkerGetCurrentEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableTreeLeafEntry **entry)
{
	if((walker->leafProxy==NULL)||(walker->leafEntry==-1))
		{
		*upstream=32767;
		*entry=NULL;

		return 0;
		}

	*upstream=walker->leafProxy->dataBlock->upstream;
	*entry=walker->leafProxy->dataBlock->entries+walker->leafEntry;

	return 1;
}

//static
s32 walkerNextEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableTreeLeafEntry **entry, s32 holdUpstream)
{
	if((walker->leafProxy==NULL)||(walker->leafEntry==-1))
		{
		*upstream=32767;
		*entry=NULL;

		return 0;
		}

	walker->leafEntry++;

	if(walker->leafEntry>=walker->leafProxy->entryCount)
		{
		LOG(LOG_INFO,"Next Sib Peek");

		RouteTableTreeBranchProxy *branchProxy=walker->branchProxy;
		s16 branchChildSibdex=walker->branchChildSibdex;
		RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

		if(!getNextLeafSibling(walker->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy))
			{
			LOG(LOG_INFO,"Next Sib failed");

			walker->branchProxy=branchProxy;
			walker->branchChildSibdex=branchChildSibdex;
			walker->leafProxy=leafProxy;

			walker->leafEntry=-1;
			*upstream=32767;
			*entry=NULL;

			return 0;
			}

		if(holdUpstream && (leafProxy->dataBlock->upstream!=*upstream))
			{
			LOG(LOG_INFO,"Hold upstream");

			*entry=NULL;
			return 0;
			}

		walker->branchProxy=branchProxy;
		walker->branchChildSibdex=branchChildSibdex;
		walker->leafProxy=leafProxy;
		walker->leafEntry=0;

		LOG(LOG_INFO,"Next Sib ok %i with %i",walker->leafProxy->lindex,walker->leafProxy->entryCount);
		}

	*upstream=walker->leafProxy->dataBlock->upstream;
	*entry=walker->leafProxy->dataBlock->entries+walker->leafEntry;

	return 1;
}


static void walkerResetOffsetArrays(RouteTableTreeWalker *walker)
{
	memset(walker->upstreamOffsets,0,sizeof(s32)*walker->upstreamOffsetCount);
	memset(walker->downstreamOffsets,0,sizeof(s32)*walker->downstreamOffsetCount);
}

static void walkerInitOffsetArrays(RouteTableTreeWalker *walker, s32 upstreamCount, s32 downstreamCount)
{
	s32 *up=dAlloc(walker->treeProxy->disp, sizeof(s32)*upstreamCount);
	s32 *down=dAlloc(walker->treeProxy->disp, sizeof(s32)*downstreamCount);

	memset(up,0,sizeof(s32)*upstreamCount);
	memset(down,0,sizeof(s32)*downstreamCount);

	walker->upstreamOffsetCount=upstreamCount;
	walker->upstreamOffsets=up;

	walker->downstreamOffsetCount=downstreamCount;
	walker->downstreamOffsets=down;
}



static void walkerAppendNewLeaf(RouteTableTreeWalker *walker, s16 upstream)
{
	LOG(LOG_INFO,"WalkerAppendLeaf");
	RouteTableTreeLeafProxy *leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);

	treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, leafProxy, &walker->branchProxy, &walker->branchChildSibdex);
	leafProxy->dataBlock->upstream=upstream;

	walker->leafProxy=leafProxy;
	walker->leafEntry=leafProxy->entryCount;
}

static void walkerAppendPreorderedEntry(RouteTableTreeWalker *walker, RouteTableEntry *entry, int routingTable)
{
	s32 upstream=routingTable==ROUTING_TABLE_FORWARD?entry->prefix:entry->suffix;
	s32 downstream=routingTable==ROUTING_TABLE_FORWARD?entry->suffix:entry->prefix;

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	if((leafProxy==NULL) || (leafProxy->entryCount>=ROUTE_TABLE_TREE_LEAF_ENTRIES) || (leafProxy->dataBlock->upstream!=upstream))
		{
		walkerAppendNewLeaf(walker, upstream);
		leafProxy=walker->leafProxy;
		}
	else if(leafProxy->entryCount>=leafProxy->entryAlloc) // Expand
		{
		expandRouteTableTreeLeafProxy(walker->treeProxy, leafProxy);
		}

	u16 entryCount=leafProxy->entryCount;

	leafProxy->dataBlock->entries[entryCount].downstream=downstream;
	leafProxy->dataBlock->entries[entryCount].width=entry->width;

	walker->leafEntry=++leafProxy->entryCount;

	LOG(LOG_INFO,"Appended Leaf Entry %i with up %i down %i width %i",leafProxy->entryCount,upstream, downstream,entry->width);
}



static void walkerAppendPreorderedEntries(RouteTableTreeWalker *walker, RouteTableEntry *entries, u32 entryCount, int routingTable)
{
	walkerSeekEnd(walker);

	for(int i=0;i<entryCount;i++)
		walkerAppendPreorderedEntry(walker,entries+i, routingTable);

	LOG(LOG_INFO,"Routing Table with %i entries used %i branches and %i leaves",
		entryCount,walker->treeProxy->branchArrayProxy.newDataCount,walker->treeProxy->leafArrayProxy.newDataCount);
}


void rttInitRouteTableTreeBuilder(RouteTableTreeBuilder *builder, HeapDataBlock *dataBlock, MemDispenser *disp)
{
	builder->disp=disp;
	builder->newEntryCount=0;

	LOG(LOG_CRITICAL,"Not implemented: rttInitRouteTableTreeBuilder");

}

void rttUpgradeToRouteTableTreeBuilder(RouteTableArrayBuilder *arrayBuilder,  RouteTableTreeBuilder *treeBuilder, HeapDataBlock *topDataBlock, MemDispenser *disp)
{
	treeBuilder->disp=arrayBuilder->disp;

	treeBuilder->topDataBlock=topDataBlock;

	for(int i=0;i<ROUTE_TOPINDEX_MAX;i++)
		treeBuilder->top.data[i]=NULL;

	treeBuilder->forwardLeafDataBlock.blockPtr=NULL;
	treeBuilder->reverseLeafDataBlock.blockPtr=NULL;
	treeBuilder->forwardBranchDataBlock.blockPtr=NULL;
	treeBuilder->reverseBranchDataBlock.blockPtr=NULL;
	treeBuilder->forwardOffsetDataBlock.blockPtr=NULL;
	treeBuilder->reverseOffsetDataBlock.blockPtr=NULL;

	initTreeProxy(&(treeBuilder->forwardProxy), &(treeBuilder->forwardLeafDataBlock), &(treeBuilder->forwardBranchDataBlock), &(treeBuilder->forwardOffsetDataBlock), disp);
	initTreeProxy(&(treeBuilder->reverseProxy), &(treeBuilder->reverseLeafDataBlock), &(treeBuilder->reverseBranchDataBlock), &(treeBuilder->reverseOffsetDataBlock), disp);

	initTreeWalker(&(treeBuilder->forwardWalker), &(treeBuilder->forwardProxy));
	initTreeWalker(&(treeBuilder->reverseWalker), &(treeBuilder->reverseProxy));

	LOG(LOG_INFO,"Adding %i %i to tree",arrayBuilder->oldForwardEntryCount, arrayBuilder->oldReverseEntryCount);

	walkerAppendPreorderedEntries(&(treeBuilder->forwardWalker), arrayBuilder->oldForwardEntries, arrayBuilder->oldForwardEntryCount, ROUTING_TABLE_FORWARD);
	walkerAppendPreorderedEntries(&(treeBuilder->reverseWalker), arrayBuilder->oldReverseEntries, arrayBuilder->oldReverseEntryCount, ROUTING_TABLE_REVERSE);

	treeBuilder->newEntryCount=arrayBuilder->oldForwardEntryCount+arrayBuilder->oldReverseEntryCount;
}


void rttDumpRoutingTable(RouteTableTreeBuilder *builder)
{
	LOG(LOG_CRITICAL,"Not implemented: rttDumpRoutingTable");
}

s32 rttGetRouteTableTreeBuilderDirty(RouteTableTreeBuilder *builder)
{
	return builder->newEntryCount>0;
}

s32 rttGetRouteTableTreeBuilderPackedSize(RouteTableTreeBuilder *builder)
{
	return sizeof(RouteTableTreeTopBlock);
}

u8 *rttWriteRouteTableTreeBuilderPackedData(RouteTableTreeBuilder *builder, u8 *data)
{
	LOG(LOG_CRITICAL,"Not implemented: rttWriteRouteTableTreeBuilderPackedData");

	return data;
}


static void mergeRoutes_insertEntry(RouteTableTreeWalker *walker, s32 upstream, s32 downstream)
{
	// New entry, with given up/down and width=1. May require leaf split if full or new leaf if not matching

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;
	s16 leafUpstream=-1;

	if(leafProxy==NULL)
		{
		LOG(LOG_INFO,"Entry Insert: No current leaf - need new leaf append");

		RouteTableTreeLeafProxy *leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);
		treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, leafProxy, &walker->branchProxy, &walker->branchChildSibdex);
		leafProxy->dataBlock->upstream=upstream;

		walker->leafProxy=leafProxy;
		walker->leafEntry=leafProxy->entryCount;
		}
	else
		{
		leafUpstream=leafProxy->dataBlock->upstream;

		if(upstream!=leafUpstream)
			{
			LOG(LOG_INFO,"Entry Insert: Unmatched upstream - need new leaf insert");

			RouteTableTreeLeafProxy *leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);
			treeProxyInsertLeafChild(walker->treeProxy, walker->branchProxy, leafProxy,
					walker->branchChildSibdex, &walker->branchProxy, &walker->branchChildSibdex);
			leafProxy->dataBlock->upstream=upstream;

			walker->leafProxy=leafProxy;
			walker->leafEntry=leafProxy->entryCount;
			}
		else if(leafProxy->entryCount>=ROUTE_TABLE_TREE_LEAF_ENTRIES)
			{
			s16 leafEntry=walker->leafEntry;
			LOG(LOG_INFO,"Entry Insert: Leaf size at maximum - need to split - pos %i",leafEntry);

			RouteTableTreeBranchProxy *newBranchProxy=NULL;
			s16 newBranchChildSibdex=-1;
			RouteTableTreeLeafProxy *newLeafProxy=NULL;
			s16 newLeafEntry=-1;

			treeProxySplitLeafInsertChildEntrySpace(walker->treeProxy, walker->branchProxy, walker->branchChildSibdex, walker->leafProxy,
					walker->leafEntry, 1, &newBranchProxy, &newBranchChildSibdex, &newLeafProxy, &newLeafEntry);

			walker->branchProxy=newBranchProxy;
			walker->branchChildSibdex=newBranchChildSibdex;
			walker->leafProxy=newLeafProxy;
			walker->leafEntry=newLeafEntry;
			}
		else
			{
			if(leafProxy->entryCount>=leafProxy->entryAlloc)
				{
				LOG(LOG_INFO,"Entry Insert: Expand");
				expandRouteTableTreeLeafProxy(walker->treeProxy, leafProxy);
				}
			leafMakeEntryInsertSpace(leafProxy, walker->leafEntry, 1);
			}
		}

	// Space already made - set entry data
	leafProxy->dataBlock->entries[walker->leafEntry].downstream=downstream;
	leafProxy->dataBlock->entries[walker->leafEntry].width=1;

	LOG(LOG_INFO,"Entry Insert: %i",walker->leafEntry);
}

static void mergeRoutes_widen(RouteTableTreeWalker *walker)
{
	// Add one to the current entry width. Hopefully width doesn't wrap

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	if(leafProxy==NULL)
		{
		LOG(LOG_CRITICAL,"Entry Widen: Null leaf, should never happen");
		}

	if(walker->leafEntry<0)
		{
		LOG(LOG_CRITICAL,"Entry Widen: Invalid entry index, should never happen");
		}

	leafProxy->dataBlock->entries[walker->leafEntry].width++;

	LOG(LOG_INFO,"Widened %i %i %i (%i %i) to %i",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry,
			leafProxy->dataBlock->upstream, leafProxy->dataBlock->entries[walker->leafEntry].downstream, leafProxy->dataBlock->entries[walker->leafEntry].width);

}

static void mergeRoutes_split(RouteTableTreeWalker *walker, s32 downstream, s32 width1, s32 width2)
{
	// Add split existing route into two, and insert a new route between

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	s16 newEntryCount=leafProxy->entryCount+2;

	if(newEntryCount>ROUTE_TABLE_TREE_LEAF_ENTRIES)
		{
		s16 leafEntry=walker->leafEntry;
		LOG(LOG_INFO,"Entry Split: Leaf size at maximum - need to split - pos %i",leafEntry);

		RouteTableTreeBranchProxy *newBranchProxy=NULL;
		s16 newBranchChildSibdex=-1;
		RouteTableTreeLeafProxy *newLeafProxy=NULL;
		s16 newLeafEntry=-1;

		treeProxySplitLeafInsertChildEntrySpace(walker->treeProxy, walker->branchProxy, walker->branchChildSibdex, walker->leafProxy,
				walker->leafEntry, 2, &newBranchProxy, &newBranchChildSibdex, &newLeafProxy, &newLeafEntry);

		walker->branchProxy=newBranchProxy;
		walker->branchChildSibdex=newBranchChildSibdex;
		walker->leafProxy=newLeafProxy;
		walker->leafEntry=newLeafEntry;
		}
	else
		{
		if(newEntryCount>leafProxy->entryAlloc)
			{
			LOG(LOG_INFO,"Entry Split: Leaf full - need to expand");
			expandRouteTableTreeLeafProxy(walker->treeProxy, leafProxy);
			}
		LOG(LOG_INFO,"Entry Split: Easy case");
		leafMakeEntryInsertSpace(leafProxy, walker->leafEntry, 1);

		}

	leafProxy->dataBlock->entries[walker->leafEntry++].width=width1;

	leafProxy->dataBlock->entries[walker->leafEntry].downstream=downstream;
	leafProxy->dataBlock->entries[walker->leafEntry++].width=1;

	leafProxy->dataBlock->entries[walker->leafEntry++].width=width2;

	LOG(LOG_INFO,"Entry Split");
}



void mergeRoutes_ordered_forwardSingle(RouteTableTreeWalker *walker, RoutePatch *patch)
{
	s32 targetPrefix=patch->prefixIndex;
	s32 targetSuffix=patch->suffixIndex;
	int minEdgePosition=(*(patch->rdiPtr))->minEdgePosition;
	int maxEdgePosition=(*(patch->rdiPtr))->maxEdgePosition;

	s16 upstream=-1;
	RouteTableTreeLeafEntry *entry=NULL;

	/*
	int res=walkerGetCurrentEntry(walker, &upstream, &entry);
	while(res) 												// Skip lower upstream
		{
		LOG(LOG_INFO,"Currently at %i %i %i %i",walker->branchProxy->brindex, walker->branchChildSibdex, walker->leafProxy->lindex, walker->leafEntry);

		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[upstream]+=entry->width;

		res=walkerNextEntry(walker, &upstream, &entry);
		}
	LOG(LOG_CRITICAL,"Done");
*/

	LOG(LOG_INFO,"*****************");
	LOG(LOG_INFO,"Looking for P %i S %i Min %i Max %i", targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);
	LOG(LOG_INFO,"*****************");

	int res=walkerGetCurrentEntry(walker, &upstream, &entry);
	while(res && upstream < targetPrefix) 												// Skip lower upstream
		{
		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[upstream]+=entry->width;

		LOG(LOG_INFO,"EntryLoop1 %i %i %i", walker->leafProxy->dataBlock->upstream, entry->downstream, entry->width);
		LOG(LOG_INFO,"OffsetsLoop1 U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);

		res=walkerNextEntry(walker, &upstream, &entry, 0);
		}

//	int upstreamEdgeOffset=walker->upstreamOffsets[targetPrefix];
	//int downstreamEdgeOffset=0;

	LOG(LOG_INFO,"OffsetsNow U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);

	if(entry!=NULL)
			LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i down: %i Width: %i",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream, entry->downstream, entry->width);
		else
			LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i - NULL ENTRY",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream);


	while(res && upstream == targetPrefix &&
			((walker->upstreamOffsets[targetPrefix]+entry->width)<minEdgePosition ||
			((walker->upstreamOffsets[targetPrefix]+entry->width)==minEdgePosition && entry->downstream!=targetSuffix))) // Skip earlier upstream
		{
		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[upstream]+=entry->width;

		LOG(LOG_INFO,"EntryLoop2 %i %i %i", walker->leafProxy->dataBlock->upstream, entry->downstream, entry->width);
		LOG(LOG_INFO,"OffsetsLoop2 U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);

		res=walkerNextEntry(walker, &upstream, &entry, 1);
		}

	LOG(LOG_INFO,"OffsetsNow U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);

	if(entry!=NULL)
			LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i down: %i Width: %i",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream, entry->downstream, entry->width);
		else
			LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i - NULL ENTRY",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream);


	while(res && upstream == targetPrefix &&
			(walker->upstreamOffsets[targetPrefix]+entry->width)<=maxEdgePosition && entry->downstream<targetSuffix) // Skip matching upstream with earlier downstream
		{
		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[upstream]+=entry->width;

		LOG(LOG_INFO,"EntryLoop3 %i %i %i", walker->leafProxy->dataBlock->upstream, entry->downstream, entry->width);
		LOG(LOG_INFO,"OffsetsLoop3 U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);

		res=walkerNextEntry(walker, &upstream, &entry, 1);
		}

	LOG(LOG_INFO,"FwdDone: %i %i %i %i",walker->branchProxy->brindex,walker->branchChildSibdex,walker->leafEntry, res);

	LOG(LOG_INFO,"OffsetsNow U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);

	if(entry!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i down: %i Width: %i",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream, entry->downstream, entry->width);
	else
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i - NULL ENTRY",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream);


	s32 upstreamEdgeOffset=walker->upstreamOffsets[targetPrefix];
	s32 downstreamEdgeOffset=walker->downstreamOffsets[targetSuffix];


	if(!res || upstream > targetPrefix || entry == NULL || (entry->downstream!=targetSuffix && upstreamEdgeOffset>=minEdgePosition)) // No suitable existing entry, but can insert/append here
		{
		int minMargin=upstreamEdgeOffset-minEdgePosition; // Margin between current position and minimum position: Must be zero or positive
		int maxMargin=maxEdgePosition-upstreamEdgeOffset; // Margin between current position and maximum position: Must be zero or positive

		if(minMargin<0 || maxMargin<0)
			{
			LOG(LOG_INFO,"Failed to add forward route for prefix %i suffix %i",targetPrefix,targetSuffix);
			LOG(LOG_INFO,"Current edge offset %i minEdgePosition %i maxEdgePosition %i",upstreamEdgeOffset,minEdgePosition,maxEdgePosition);
			LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
			}

		// Map offsets to new entry
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		mergeRoutes_insertEntry(walker, targetPrefix, targetSuffix); // targetPrefix,targetSuffix,1
		}
	else if(upstream==targetPrefix && entry->downstream==targetSuffix) // Existing entry suitable, widen
		{
		int upstreamEdgeOffsetEnd=upstreamEdgeOffset+entry->width;

		// Adjust offsets
		if(minEdgePosition<upstreamEdgeOffset) // Trim upstream range to entry
			minEdgePosition=upstreamEdgeOffset;

		if(maxEdgePosition>upstreamEdgeOffsetEnd) // Trim upstream range to entry
			maxEdgePosition=upstreamEdgeOffsetEnd;

		int minOffset=minEdgePosition-upstreamEdgeOffset; // Offset of minimum position: zero or positive
		int maxOffset=maxEdgePosition-upstreamEdgeOffset; // Offset of maximum position: zero or positive

		if(minOffset<0 || maxOffset<0 || minOffset>maxOffset)
			{
			LOG(LOG_CRITICAL,"Invalid offsets or gap detected in route insert - Min: %i Max: %i",minOffset,maxOffset);
			}

		// Map offsets to new entry
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset+minOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset+maxOffset;

		mergeRoutes_widen(walker); // width ++
		}
	else // Existing entry unsuitable, split and insert
		{
		int targetEdgePosition=entry->downstream>targetSuffix?minEdgePosition:maxEdgePosition; // Early or late split

		int splitWidth1=targetEdgePosition-upstreamEdgeOffset;
		int splitWidth2=entry->width-splitWidth1;

		if(splitWidth1<=0 || splitWidth2<=0)
			{
			LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i from %i",splitWidth1, splitWidth2, entry->width);
			}

		// Map offsets
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		mergeRoutes_split(walker, targetSuffix, splitWidth1, splitWidth2); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2

		//LOG(LOG_CRITICAL,"Entry Split"); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2
		}

	LOG(LOG_INFO,"*****************");
	LOG(LOG_INFO,"mergeRoutes_ordered_forwardSingle done");
	LOG(LOG_INFO,"*****************");

}


void mergeRoutes_ordered_reverseSingle(RouteTableTreeWalker *walker, RoutePatch *patch)
{

	LOG(LOG_CRITICAL,"Reverse: Not Implemented");

}


/*	LOG(LOG_INFO,"Entries: %i %i",forwardRoutePatchCount, reverseRoutePatchCount);

	for(int i=0;i<forwardRoutePatchCount;i++)
		{
		LOG(LOG_INFO,"Fwd: P: %i S: %i pos %i %i",
			forwardRoutePatches[i].prefixIndex,forwardRoutePatches[i].suffixIndex,
			(*(forwardRoutePatches[i].rdiPtr))->minEdgePosition,
			(*(forwardRoutePatches[i].rdiPtr))->maxEdgePosition);
		}

	for(int i=0;i<reverseRoutePatchCount;i++)
		{
		LOG(LOG_INFO,"Rev: P: %i S: %i pos %i %i",
			reverseRoutePatches[i].prefixIndex,reverseRoutePatches[i].suffixIndex,
			(*(reverseRoutePatches[i].rdiPtr))->minEdgePosition,
			(*(reverseRoutePatches[i].rdiPtr))->maxEdgePosition);
		}
*/



void rttMergeRoutes(RouteTableTreeBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 prefixCount, s32 suffixCount, RoutingReadData **orderedDispatches, MemDispenser *disp)
{


	prefixCount++; // Move from 1 based to 0 based
	suffixCount++; // Move from 1 based to 0 based

	// Forward Routes

	if(forwardRoutePatchCount>0)
		{
		RouteTableTreeWalker *walker=&(builder->forwardWalker);

		walkerInitOffsetArrays(walker, prefixCount, suffixCount);

		RoutePatch *patchPtr=forwardRoutePatches;
		RoutePatch *patchEnd=patchPtr+forwardRoutePatchCount;
		RoutePatch *patchPeekPtr=NULL;

		while(patchPtr<patchEnd)
			{
			int targetUpstream=patchPtr->prefixIndex;
			patchPeekPtr=patchPtr+1;

			if(patchPeekPtr<patchEnd && patchPeekPtr->prefixIndex==targetUpstream)
				{
				while(patchPtr<patchEnd && patchPtr->prefixIndex==targetUpstream)
					{
					walkerSeekStart(walker);
					mergeRoutes_ordered_forwardSingle(walker, patchPtr);
					walkerResetOffsetArrays(walker);

					*(orderedDispatches++)=*(patchPtr->rdiPtr);
					patchPtr++;
					}
				}
			else
				{
				walkerSeekStart(walker);
				mergeRoutes_ordered_forwardSingle(walker, patchPtr);

				*(orderedDispatches++)=*(patchPtr->rdiPtr);
				patchPtr++;
				}
			}
		}


	// Reverse Routes

	if(reverseRoutePatchCount>0)
		{
		RouteTableTreeWalker *walker=&(builder->reverseWalker);

		walkerInitOffsetArrays(walker, suffixCount, prefixCount);

		RoutePatch *patchPtr=reverseRoutePatches;
		RoutePatch *patchEnd=patchPtr+reverseRoutePatchCount;
		RoutePatch *patchPeekPtr=NULL;

		while(patchPtr<patchEnd)
			{
			int targetUpstream=patchPtr->suffixIndex;
			patchPeekPtr=patchPtr+1;

			if(patchPeekPtr<patchEnd && patchPeekPtr->suffixIndex==targetUpstream)
				{
				while(patchPtr<patchEnd && patchPtr->suffixIndex==targetUpstream)
					{
					walkerSeekStart(walker);
					mergeRoutes_ordered_reverseSingle(walker, patchPtr);
					walkerResetOffsetArrays(walker);

					*(orderedDispatches++)=*(patchPtr->rdiPtr);
					patchPtr++;
					}
				}
			else
				{
				walkerSeekStart(walker);
				mergeRoutes_ordered_reverseSingle(walker, patchPtr);

				*(orderedDispatches++)=*(patchPtr->rdiPtr);
				patchPtr++;
				}
			}

		}


	LOG(LOG_CRITICAL,"Post merge: Not Implemented");


}


void rttUnpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp)
{
	LOG(LOG_CRITICAL,"Not implemented: rttUnpackRouteTableForSmerLinked");
}

