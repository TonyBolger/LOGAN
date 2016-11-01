
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
void treeProxySplitRoot(RouteTableTreeProxy *treeProxy)
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
		LOG(LOG_INFO,"Index %i",i);

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
		LOG(LOG_INFO,"Index %i",i);

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

	LOG(LOG_INFO,"Children offloaded");

	branch1->dataBlock->parentBrindex=BRANCH_NINDEX_ROOT;
	branch2->dataBlock->parentBrindex=BRANCH_NINDEX_ROOT;

	RouteTableTreeBranchBlock *newRoot=reallocRouteTableTreeBranchBlockEntries(NULL, treeProxy->disp, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

	LOG(LOG_INFO,"Got new root");

	newRoot->parentBrindex=BRANCH_NINDEX_INVALID;
	newRoot->childNindex[0]=branchBrindex1;
	newRoot->childNindex[1]=branchBrindex2;

	// Should not be needed
	//for(int i=2;i<newRoot->childAlloc;i++)
	//		newRoot->childNindex[i]=BRANCH_NINDEX_INVALID;

	LOG(LOG_INFO,"Root Block was %p",root->dataBlock);

	root->childCount=2;
	root->childAlloc=newRoot->childAlloc;
	root->dataBlock=newRoot;

	flushRouteTableTreeBranchProxy(treeProxy, root);

	LOG(LOG_INFO,"Root Block now %p",newRoot);
}



s16 treeProxySplitRoot_trackParentBySibdex(RouteTableTreeProxy *treeProxy, s32 sibdex)
{
	s32 trackChildNindex=treeProxy->rootProxy->dataBlock->childNindex[sibdex];
	treeProxySplitRoot(treeProxy);

	s32 newParentBrindex=0;

	if(trackChildNindex>=0)
		newParentBrindex=getRouteTableTreeBranchRaw(treeProxy, trackChildNindex)->parentBrindex;
	else
		newParentBrindex=getRouteTableTreeLeafRaw(treeProxy, NINDEX_TO_LINDEX(trackChildNindex))->parentBrindex;

	return newParentBrindex;
}


// Split a branch, creating a new sibling which _MUST_ be attached by the caller

RouteTableTreeBranchProxy *treeProxySplitBranch(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branch)
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

	return newBranch;
}



// Append the given branch child to the branch parent, splitting the parent if needed

//static
RouteTableTreeBranchProxy *treeProxyAppendBranchChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parent, RouteTableTreeBranchProxy *child)
{
	LOG(LOG_INFO,"Append Branch Child P: %i C: %i",parent->brindex, child->brindex);

	s16 parentBrindex=parent->brindex;
	s16 childBrindex=child->brindex;

	LOG(LOG_INFO,"Parent %i contains %i of %i",parent->brindex, parent->childCount, parent->childAlloc);

	for(int i=0;i<parent->childCount;i++)
		{
		LOG(LOG_INFO,"Child %i is %i",i,parent->dataBlock->childNindex[i]);
		}


	if(parent->childCount>=parent->childAlloc) // No Space
		{
		if(parent->childAlloc<ROUTE_TABLE_TREE_BRANCH_CHILDREN) // Expand
			{
			LOG(LOG_INFO,"AppendBranch %i: Expand",parent->brindex);

			s32 newAlloc=MIN(parent->childAlloc+ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK, ROUTE_TABLE_TREE_BRANCH_CHILDREN);

			RouteTableTreeBranchBlock *parentBlock=reallocRouteTableTreeBranchBlockEntries(parent->dataBlock, treeProxy->disp, newAlloc);

			parent->dataBlock=parentBlock;
			parent->childAlloc=newAlloc;

			flushRouteTableTreeBranchProxy(treeProxy, parent);
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

			RouteTableTreeBranchProxy *newParent=treeProxySplitBranch(treeProxy, parent);
//			RouteTableTreeBranchProxy *newParent=allocRouteTableTreeBranchProxy(treeProxy, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

			treeProxyAppendBranchChild(treeProxy, grandParent, newParent);
			parent=newParent;
			}
		else	// Split root
			{
			LOG(LOG_INFO,"AppendBranch %i: Split Root",parent->brindex);

			//treeProxySplitRoot(treeProxy);
			//parent=getRouteTableTreeBranchProxy(treeProxy, treeProxy->rootProxy->dataBlock->childNindex[1]);

			parentBrindex=treeProxySplitRoot_trackParentBySibdex(treeProxy, parent->childCount-1);
			parent=getRouteTableTreeBranchProxy(treeProxy, parentBrindex);


//			treeProxySeekEnd(treeProxy, &parent, NULL);
			}

		}

	if(parent->childCount>=parent->childAlloc)
		{
		LOG(LOG_CRITICAL,"No space after split/sibling add in branch node %i", parent->brindex);
		}

	LOG(LOG_INFO,"Appended Branch %i",childBrindex);

	parent->dataBlock->childNindex[parent->childCount++]=childBrindex;
	child->dataBlock->parentBrindex=parentBrindex;


	LOG(LOG_INFO,"ParentNow %i contains %i of %i",parent->brindex, parent->childCount, parent->childAlloc);

	for(int i=0;i<parent->childCount;i++)
		{
		LOG(LOG_INFO,"ChildNow %i is %i",i,parent->dataBlock->childNindex[i]);
		}


	return parent;
}



// Append the given leaf child to the branch parent, splitting the parent if needed

static RouteTableTreeBranchProxy *treeProxyAppendLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parent, RouteTableTreeLeafProxy *child)
{
	LOG(LOG_INFO,"Append Leaf Child P: %i C: %i",parent->brindex, child->lindex);

	s16 parentBrindex=parent->brindex;
	s16 childLindex=child->lindex;

	LOG(LOG_INFO,"Parent contains %i of %i",parent->childCount, parent->childAlloc);

	if(parent->childCount>=parent->childAlloc)
		{
		if(parent->childAlloc<ROUTE_TABLE_TREE_BRANCH_CHILDREN)
			{
			s32 newAlloc=MIN(parent->childAlloc+ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK, ROUTE_TABLE_TREE_BRANCH_CHILDREN);

			RouteTableTreeBranchBlock *parentBlock=reallocRouteTableTreeBranchBlockEntries(parent->dataBlock, treeProxy->disp, newAlloc);

			parent->dataBlock=parentBlock;
			parent->childAlloc=newAlloc;

			flushRouteTableTreeBranchProxy(treeProxy, parent);
			}
		else if(parent->brindex!=BRANCH_NINDEX_ROOT)
			{
			RouteTableTreeBranchProxy *grandParent;
			s32 grandParentBrindex=parent->dataBlock->parentBrindex;

			if(grandParentBrindex!=BRANCH_NINDEX_ROOT)
				grandParent=getRouteTableTreeBranchProxy(treeProxy, grandParentBrindex);
			else
				grandParent=treeProxy->rootProxy;

			RouteTableTreeBranchProxy *newParent=treeProxySplitBranch(treeProxy, parent);

			LOG(LOG_INFO,"AppendLeaf %i -> AppendBranch %i",child->lindex,newParent->brindex);

			treeProxyAppendBranchChild(treeProxy, grandParent, newParent);
			parent=newParent;
			}
		else
			{
			treeProxySplitRoot(treeProxy);

			parent=getRouteTableTreeBranchProxy(treeProxy, treeProxy->rootProxy->dataBlock->childNindex[1]);
			}
		}

	if(parent->childCount>=parent->childAlloc)
		{
		LOG(LOG_CRITICAL,"No space after split/sibling add");
		}

	LOG(LOG_INFO,"Appended Leaf %i as %i in %i",childLindex,LINDEX_TO_NINDEX(childLindex),parent->brindex);

	parent->dataBlock->childNindex[parent->childCount++]=LINDEX_TO_NINDEX(childLindex);
	child->dataBlock->parentBrindex=parentBrindex;

	return parent;
}



static s32 getNextBranchSibling(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchPtr)
{
	RouteTableTreeBranchProxy *branch=*branchPtr;

//	LOG(LOG_INFO,"getNextBranchSibling for %i",branch->brindex);

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
			return 1;
			}
		else if((branch->dataBlock->parentBrindex!=BRANCH_NINDEX_ROOT) && getNextBranchSibling(treeProxy, &parent))
			{
			if(parent->childCount>0)
				{
				if(parent->dataBlock->childNindex[0]<0)
					LOG(LOG_CRITICAL,"Parent %i contains child %i",parent->brindex, parent->dataBlock->childNindex[0]);

				branch=getRouteTableTreeBranchProxy(treeProxy, parent->dataBlock->childNindex[0]);
				*branchPtr=branch;
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
//	LOG(LOG_INFO,"getNextLeafSibling");

	RouteTableTreeBranchProxy *branch=*branchPtr;
	RouteTableTreeLeafProxy *leaf=*leafPtr;

	s16 sibdexEst=*branchChildSibdex;
	s16 sibdex=getBranchChildSibdex_Leaf_withEstimate(branch, leaf, sibdexEst);

	if(sibdexEst!=sibdex)
		{
		LOG(LOG_CRITICAL,"Sibdex did not match expected Est: %i Actual: %i",sibdexEst, sibdex);
		}

	sibdex++;
	if(sibdex<branch->childCount)
		{
		//LOG(LOG_INFO,"In branch %i, looking at sib %i giving %i",branch->brindex, sibdex, branch->dataBlock->childNindex[sibdex]);

		*branchChildSibdex=sibdex;
		*leafPtr=getRouteTableTreeLeafProxy(treeProxy, NINDEX_TO_LINDEX(branch->dataBlock->childNindex[sibdex]));
		return 1;
		}

	if(getNextBranchSibling(treeProxy, &branch)) // Moved branch
		{
		*branchPtr=branch;
		if(branch->childCount>0)
			sibdex=0;
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
		//LOG(LOG_INFO,"In branch %i, looking at sib %i giving %i",branch->brindex, sibdex, branch->dataBlock->childNindex[sibdex]);

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
	if(walker->leafProxy==NULL)
		return 0;

	if(walker->leafEntry==-1)
		return 0;

	*upstream=walker->leafProxy->dataBlock->upstream;
	*entry=walker->leafProxy->dataBlock->entries+walker->leafEntry;

	return 1;
}

//static
s32 walkerNextEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableTreeLeafEntry **entry)
{
	if(walker->leafProxy==NULL)
		return 0;

	if(walker->leafEntry==-1)
		return 0;

	walker->leafEntry++;

	if(walker->leafEntry>=walker->leafProxy->entryCount)
		{
		walker->leafEntry=0;

		//LOG(LOG_INFO,"Next Sib");

		if(!getNextLeafSibling(walker->treeProxy, &walker->branchProxy, &walker->branchChildSibdex, &walker->leafProxy))
			return 0;
		}

	*upstream=walker->leafProxy->dataBlock->upstream;
	*entry=walker->leafProxy->dataBlock->entries+walker->leafEntry;

	return 1;
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



static void walkerAppendLeaf(RouteTableTreeWalker *walker, s16 upstream)
{
	LOG(LOG_INFO,"WalkerAppendLeaf");
	RouteTableTreeLeafProxy *leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);

	walker->branchProxy=treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, leafProxy);
	leafProxy->dataBlock->upstream=upstream;

	walker->branchChildSibdex=-1;
	walker->leafProxy=leafProxy;
	walker->leafEntry=leafProxy->entryCount;
}

static void walkerAppendPreorderedEntry(RouteTableTreeWalker *walker, RouteTableEntry *entry, int routingTable)
{
	s32 upstream=routingTable==ROUTING_TABLE_FORWARD?entry->prefix:entry->suffix;
	s32 downstream=routingTable==ROUTING_TABLE_FORWARD?entry->suffix:entry->prefix;

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	if((leafProxy==NULL) || (leafProxy->entryCount>=ROUTE_TABLE_TREE_LEAF_ENTRIES))
		{
		walkerAppendLeaf(walker, upstream);
		leafProxy=walker->leafProxy;
		}
	else if(leafProxy->entryCount>=leafProxy->entryAlloc) // Expand
		{
		s32 newAlloc=MIN(leafProxy->entryAlloc+ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK, ROUTE_TABLE_TREE_LEAF_ENTRIES);

		LOG(LOG_INFO,"Expanding Leaf to %i",newAlloc);

		RouteTableTreeLeafBlock *leafBlock=reallocRouteTableTreeLeafBlockEntries(leafProxy->dataBlock, walker->treeProxy->disp, newAlloc);

		leafProxy->dataBlock=leafBlock;
		leafProxy->entryAlloc=newAlloc;

		LOG(LOG_INFO,"Flushing with %i %i %i",leafProxy->entryCount,leafProxy->entryAlloc,leafProxy->dataBlock->entryAlloc);

		flushRouteTableTreeLeafProxy(walker->treeProxy, leafProxy);
		}

	u16 entryCount=leafProxy->entryCount;

	leafProxy->dataBlock->entries[entryCount].downstream=downstream;
	leafProxy->dataBlock->entries[entryCount].width=entry->width;

	walker->leafEntry=++leafProxy->entryCount;

	LOG(LOG_INFO,"Appended Leaf Entry %i with down %i width %i",leafProxy->entryCount,downstream,entry->width);
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
	LOG(LOG_CRITICAL,"Entry Insert");
}

static void mergeRoutes_widen(RouteTableTreeWalker *walker)
{
	LOG(LOG_CRITICAL,"Entry Widen");
}

static void mergeRoutes_split(RouteTableTreeWalker *walker, s32 downstream, s32 width1, s32 width2)
{
	LOG(LOG_CRITICAL,"Entry Split");
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


	LOG(LOG_INFO,"Looking for P %i S %i Min %i Max %i", targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

	int res=walkerGetCurrentEntry(walker, &upstream, &entry);
	while(res && upstream < targetPrefix) 												// Skip lower upstream
		{
		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[upstream]+=entry->width;

		res=walkerNextEntry(walker, &upstream, &entry);
		}

//	int upstreamEdgeOffset=walker->upstreamOffsets[targetPrefix];
	//int downstreamEdgeOffset=0;

	while(res && upstream == targetPrefix &&
			((walker->upstreamOffsets[targetPrefix]+entry->width)<minEdgePosition ||
			((walker->upstreamOffsets[targetPrefix]+entry->width)==minEdgePosition && entry->downstream!=targetSuffix))) // Skip earlier upstream
		{
		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[targetPrefix]+=entry->width;

		res=walkerNextEntry(walker, &upstream, &entry);
		}

	while(res && upstream == targetPrefix &&
			(walker->upstreamOffsets[targetPrefix]+entry->width)<=maxEdgePosition && entry->downstream<targetSuffix) // Skip matching upstream with earlier downstream
		{
		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[targetPrefix]+=entry->width;

		res=walkerNextEntry(walker, &upstream, &entry);
		}

	LOG(LOG_INFO,"FwdDone: %i %i %i",walker->branchProxy->brindex,walker->branchChildSibdex,walker->leafEntry);

	if(entry!=NULL)
		{
		LOG(LOG_INFO,"Currently at %i %i %i - %i %i %i",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream, entry->downstream, entry->width);
		}

	s32 upstreamEdgeOffset=walker->upstreamOffsets[targetPrefix];
	s32 downstreamEdgeOffset=walker->downstreamOffsets[targetSuffix];

	if(!res || upstream > targetPrefix || (entry->downstream!=targetSuffix && upstreamEdgeOffset>=minEdgePosition)) // No suitable existing entry, but can insert/append here
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
			LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i from %i",splitWidth1, splitWidth2);
			}

		// Map offsets
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		mergeRoutes_split(walker, targetSuffix, splitWidth1, splitWidth2); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2

		//LOG(LOG_CRITICAL,"Entry Split"); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2
		}

	LOG(LOG_CRITICAL,"HMM"); // targetPrefix,targetSuffix,1

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

		walkerSeekStart(walker);
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
					mergeRoutes_ordered_forwardSingle(walker, patchPtr);

					*(orderedDispatches++)=*(patchPtr->rdiPtr);
					patchPtr++;
					}
				}
			else
				{
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

		walkerSeekStart(walker);
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
					mergeRoutes_ordered_reverseSingle(walker, patchPtr);

					*(orderedDispatches++)=*(patchPtr->rdiPtr);
					patchPtr++;
					}
				}
			else
				{
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

