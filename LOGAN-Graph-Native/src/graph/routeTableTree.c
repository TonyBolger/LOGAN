
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
	block->entryAlloc=entryAlloc;

	s32 oldBlockEntryAlloc=0;
	if(oldBlock!=NULL)
		{
		oldBlockEntryAlloc=oldBlock->entryAlloc;
		s32 toKeepAlloc=MIN(oldBlockEntryAlloc, entryAlloc);

		memcpy(block,oldBlock, sizeof(RouteTableTreeLeafBlock)+toKeepAlloc*sizeof(RouteTableTreeLeafEntry));
		}

	for(int i=oldBlockEntryAlloc;i<entryAlloc;i++)
		{
		block->entries[i].downstream=-1;
		block->entries[i].width=0;
		}

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

	block->parentIndex=BRANCH_INDEX_INVALID;
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
	block->childAlloc=childAlloc;
	block->parentIndex=BRANCH_INDEX_INVALID;
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
		block->childIndex[i]=BRANCH_INDEX_INVALID;

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
	block->parentIndex=BRANCH_INDEX_INVALID;
	block->upstreamMin=-1;
	block->upstreamMax=-1;

	for(int i=0;i<childAlloc;i++)
		block->childIndex[i]=BRANCH_INDEX_INVALID;

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
		if(branchBlock->childIndex[i-1]!=BRANCH_INDEX_INVALID)
			{
			count=i;
			break;
			}
		}

	*countPtr=count;
}


// static
RouteTableTreeBranchBlock *getRouteTableTreeBranchRaw(RouteTableTreeProxy *treeProxy, s32 index)
{
	u8 *data=getBlockArrayEntry(&(treeProxy->branchArrayProxy), index);

	return (RouteTableTreeBranchBlock *)data;
}

// static
RouteTableTreeBranchProxy *getRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, s32 index)
{
	u8 *data=getBlockArrayEntry(&(treeProxy->branchArrayProxy), index);

	RouteTableTreeBranchProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeBranchProxy));

	proxy->dataBlock=(RouteTableTreeBranchBlock *)data;
	proxy->index=index;

	getRouteTableTreeBranchProxy_scan(proxy->dataBlock, &proxy->childAlloc, &proxy->childCount);

	return proxy;
}

// static
void flushRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy)
{
	setBlockArrayEntry(&(treeProxy->branchArrayProxy), branchProxy->index, (u8 *)branchProxy->dataBlock, treeProxy->disp);
}

//static
RouteTableTreeBranchProxy *allocRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, s32 childAlloc)
{
	if(childAlloc<ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK)
		childAlloc=ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK;

	RouteTableTreeBranchBlock *dataBlock=allocRouteTableTreeBranchBlock(treeProxy->disp, childAlloc);
	s32 index=appendBlockArrayEntry(&(treeProxy->branchArrayProxy), (u8 *)dataBlock, treeProxy->disp);

	RouteTableTreeBranchProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeBranchProxy));
	proxy->dataBlock=dataBlock;
	proxy->index=index;

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
RouteTableTreeLeafBlock *getRouteTableTreeLeafRaw(RouteTableTreeProxy *treeProxy, s32 index)
{
	u8 *data=getBlockArrayEntry(&(treeProxy->leafArrayProxy), index);

	return (RouteTableTreeLeafBlock *)data;
}

// static
RouteTableTreeLeafProxy *getRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 index)
{
	u8 *data=getBlockArrayEntry(&(treeProxy->leafArrayProxy), index);

	RouteTableTreeLeafProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeLeafProxy));

	proxy->dataBlock=(RouteTableTreeLeafBlock *)data;
	proxy->index=index;

	getRouteTableTreeLeafProxy_scan(proxy->dataBlock, &proxy->entryAlloc, &proxy->entryCount);

	return proxy;
}

// static
void flushRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy)
{
	setBlockArrayEntry(&(treeProxy->leafArrayProxy), leafProxy->index, (u8 *)leafProxy->dataBlock, treeProxy->disp);
}

//static
RouteTableTreeLeafProxy *allocRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 entryAlloc)
{
	if(entryAlloc<ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK)
		entryAlloc=ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK;

	RouteTableTreeLeafBlock *dataBlock=allocRouteTableTreeLeafBlock(treeProxy->disp, entryAlloc);
	s32 index=appendBlockArrayEntry(&(treeProxy->leafArrayProxy), (u8 *)dataBlock, treeProxy->disp);

	RouteTableTreeLeafProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeLeafProxy));
	proxy->dataBlock=dataBlock;
	proxy->index=index;

	proxy->entryAlloc=entryAlloc;
	proxy->entryCount=0;

	return proxy;
}



//static
s16 getBranchChildPositionBranch(RouteTableTreeBranchProxy *parent, RouteTableTreeBranchProxy *child, s16 positionEstimate)
{
	return 0;
}

//static
s16 getBranchChildPositionLeaf(RouteTableTreeBranchProxy *parent, RouteTableTreeLeafProxy *child, s16 positionEstimate)
{
	return 0;
}



static void initTreeProxy(RouteTableTreeProxy *proxy, HeapDataBlock *leafBlock, HeapDataBlock *branchBlock, HeapDataBlock *offsetBlock, MemDispenser *disp)
{
	proxy->disp=disp;

	initBlockArrayProxy(proxy, &(proxy->leafArrayProxy), leafBlock, 2);
	initBlockArrayProxy(proxy, &(proxy->branchArrayProxy), branchBlock, 2);
	initBlockArrayProxy(proxy, &(proxy->offsetArrayProxy), offsetBlock, 2);

	if(getBlockArraySize(&(proxy->branchArrayProxy))>0)
		proxy->rootProxy=getRouteTableTreeBranchProxy(proxy, BRANCH_INDEX_ROOT);
	else
		proxy->rootProxy=allocRouteTableTreeBranchProxy(proxy, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

}


static void treeProxySeekStart(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchPtr, s16 *branchChildPtr, RouteTableTreeLeafProxy **leafPtr)
{
	RouteTableTreeBranchProxy *branchProxy=treeProxy->rootProxy;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 branchChild=-1;

	s16 lastChildIndex=0;

	while(branchProxy->childCount>0 && lastChildIndex>=0)
		{
		lastChildIndex=branchProxy->dataBlock->childIndex[0];

		if(lastChildIndex>0)
			branchProxy=getRouteTableTreeBranchProxy(treeProxy, lastChildIndex);
		}

	if(lastChildIndex<0)
		{
		branchChild=0;
		leafProxy=getRouteTableTreeLeafProxy(treeProxy,1-lastChildIndex);
		}

	if(branchPtr!=NULL)
		*branchPtr=branchProxy;

	if(branchChildPtr!=NULL)
		*branchChildPtr=branchChild;

	if(leafPtr!=NULL)
		*leafPtr=leafProxy;
}


static void treeProxySeekEnd(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchPtr,  s16 *branchChildPtr, RouteTableTreeLeafProxy **leafPtr)
{
	RouteTableTreeBranchProxy *branchProxy=treeProxy->rootProxy;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 branchChild=-1;

	s16 lastChildIndex=0;

	while(branchProxy->childCount>0 && lastChildIndex>=0)
		{
		lastChildIndex=branchProxy->dataBlock->childIndex[branchProxy->childCount-1];

		LOG(LOG_INFO,"Last Child Array Index %i found %i",branchProxy->childCount-1, lastChildIndex);

		if(lastChildIndex>0)
			{
			branchChild=branchProxy->childCount-1;
			branchProxy=getRouteTableTreeBranchProxy(treeProxy, lastChildIndex);

			LOG(LOG_INFO,"End Seek now %i",branchProxy->index);
			}
		}

	if(lastChildIndex<0)
		{
		leafProxy=getRouteTableTreeLeafProxy(treeProxy,1-lastChildIndex);
		}

	if(branchPtr!=NULL)
		*branchPtr=branchProxy;

	if(branchChildPtr!=NULL)
		*branchChildPtr=branchChild;

	if(leafPtr!=NULL)
		*leafPtr=leafProxy;

}


//static
void treeProxySplitRoot(RouteTableTreeProxy *treeProxy)
{
	LOG(LOG_INFO,"Root Split");

	RouteTableTreeBranchProxy *root=treeProxy->rootProxy;

	if(root->index!=BRANCH_INDEX_ROOT)
		{
		LOG(LOG_CRITICAL,"Asked to root-split a non-root node");
		}

	LOG(LOG_INFO,"Root %i contains %i of %i",root->index, root->childCount, root->childAlloc);

	s32 halfChild=((1+root->childCount)/2);
	s32 otherHalfChild=root->childCount-halfChild;

	s32 halfChildAlloc=halfChild+1;

	RouteTableTreeBranchProxy *branch1=allocRouteTableTreeBranchProxy(treeProxy, halfChildAlloc);
	RouteTableTreeBranchProxy *branch2=allocRouteTableTreeBranchProxy(treeProxy, halfChildAlloc);

	s32 branchIndex1=branch1->index;
	s32 branchIndex2=branch2->index;

	LOG(LOG_INFO,"Split - new branches %i %i",branchIndex1, branchIndex2);

	for(int i=0;i<halfChild;i++)
		{
		LOG(LOG_INFO,"Index %i",i);

		s32 childIndex=root->dataBlock->childIndex[i];
		branch1->dataBlock->childIndex[i]=childIndex;

		LOG(LOG_INFO,"Moving Child %i to %i",childIndex, branchIndex1);

		if(childIndex<0)
			{
			RouteTableTreeLeafBlock *leafRaw=getRouteTableTreeLeafRaw(treeProxy, -childIndex-1);

			if(leafRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find leaf with index %i",childIndex);
				}

			leafRaw->parentIndex=branchIndex1;
			}
		else
			{
			RouteTableTreeBranchBlock *branchRaw=getRouteTableTreeBranchRaw(treeProxy, childIndex);

			if(branchRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find branch with index %i",childIndex);
				}

			branchRaw->parentIndex=branchIndex1;
			}
		}

	branch1->childCount=halfChild;

	for(int i=0;i<otherHalfChild;i++)
		{
		LOG(LOG_INFO,"Index %i",i);

		s32 childIndex=root->dataBlock->childIndex[i+halfChild];
		branch2->dataBlock->childIndex[i]=childIndex;

		LOG(LOG_INFO,"Moving Child %i to %i",childIndex, branchIndex2);

		if(childIndex<0)
			{
			RouteTableTreeLeafBlock *leafRaw=getRouteTableTreeLeafRaw(treeProxy, -childIndex-1);

			if(leafRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find leaf with index %i",childIndex);
				}

			leafRaw->parentIndex=branchIndex2;
			}
		else
			{
			RouteTableTreeBranchBlock *branchRaw=getRouteTableTreeBranchRaw(treeProxy, childIndex);

			if(branchRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find branch with index %i",childIndex);
				}

			branchRaw->parentIndex=branchIndex2;
			}
		}

	branch2->childCount=otherHalfChild;

	LOG(LOG_INFO,"Children offloaded");

	branch1->dataBlock->parentIndex=BRANCH_INDEX_ROOT;
	branch2->dataBlock->parentIndex=BRANCH_INDEX_ROOT;

	RouteTableTreeBranchBlock *newRoot=reallocRouteTableTreeBranchBlockEntries(NULL, treeProxy->disp, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

	LOG(LOG_INFO,"Got new root");

	newRoot->parentIndex=BRANCH_INDEX_INVALID;
	newRoot->childIndex[0]=branchIndex1;
	newRoot->childIndex[1]=branchIndex2;

	LOG(LOG_INFO,"Root Block was %p",root->dataBlock);

	root->childCount=2;
	root->childAlloc=newRoot->childAlloc;
	root->dataBlock=newRoot;

	flushRouteTableTreeBranchProxy(treeProxy, root);

	LOG(LOG_INFO,"Root Block now %p",newRoot);
}



// Split a branch, creating a new sibling which _MUST_ be attached by the caller

RouteTableTreeBranchProxy *treeProxySplitBranch(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branch)
{
	s32 halfChild=((1+branch->childCount)/2);
	s32 otherHalfChild=branch->childCount-halfChild;

	s32 halfChildAlloc=halfChild+1;

	RouteTableTreeBranchProxy *newBranch=allocRouteTableTreeBranchProxy(treeProxy, halfChildAlloc);

	s32 newBranchIndex=newBranch->index;
	branch->childCount=halfChild;

	for(int i=0;i<otherHalfChild;i++)
		{
		LOG(LOG_INFO,"Index %i",i);

		s32 childIndex=branch->dataBlock->childIndex[i+halfChild];
		newBranch->dataBlock->childIndex[i]=childIndex;

		LOG(LOG_INFO,"Moving Child %i to %i",childIndex, newBranchIndex);

		if(childIndex<0)
			{
			RouteTableTreeLeafBlock *leafRaw=getRouteTableTreeLeafRaw(treeProxy, -childIndex-1);

			if(leafRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find leaf with index %i",childIndex);
				}

			leafRaw->parentIndex=newBranchIndex;
			}
		else
			{
			RouteTableTreeBranchBlock *branchRaw=getRouteTableTreeBranchRaw(treeProxy, childIndex);

			if(branchRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find branch with index %i",childIndex);
				}

			branchRaw->parentIndex=newBranchIndex;
			}
		}

	newBranch->childCount=otherHalfChild;

	return newBranch;
}



// Append the given branch child to the branch parent, splitting the parent if needed

//static
RouteTableTreeBranchProxy *treeProxyAppendBranchChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parent, RouteTableTreeBranchProxy *child)
{
	LOG(LOG_INFO,"Append Branch Child P: %i C: %i",parent->index, child->index);

	s16 parentIndex=parent->index;
	s16 childIndex=child->index;

	LOG(LOG_INFO,"Parent %i contains %i of %i",parent->index, parent->childCount, parent->childAlloc);

	if(parent->childCount>=parent->childAlloc) // No Space
		{
		if(parent->childAlloc<ROUTE_TABLE_TREE_BRANCH_CHILDREN) // Expand
			{
			LOG(LOG_INFO,"AppendBranch %i: Expand",parent->index);

			s32 newAlloc=MIN(parent->childAlloc+ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK, ROUTE_TABLE_TREE_BRANCH_CHILDREN);

			RouteTableTreeBranchBlock *parentBlock=reallocRouteTableTreeBranchBlockEntries(parent->dataBlock, treeProxy->disp, newAlloc);

			parent->dataBlock=parentBlock;
			parent->childAlloc=newAlloc;

			flushRouteTableTreeBranchProxy(treeProxy, parent);
			}
		else if(parent->index!=BRANCH_INDEX_ROOT) // Add sibling
			{
			LOG(LOG_INFO,"AppendBranch %i: Sibling",parent->index);

			RouteTableTreeBranchProxy *grandParent;
			s32 grandParentIndex=parent->dataBlock->parentIndex;

			if(grandParentIndex!=BRANCH_INDEX_ROOT)
				grandParent=getRouteTableTreeBranchProxy(treeProxy, grandParentIndex);
			else
				grandParent=treeProxy->rootProxy;

			RouteTableTreeBranchProxy *newParent=treeProxySplitBranch(treeProxy, parent);
//			RouteTableTreeBranchProxy *newParent=allocRouteTableTreeBranchProxy(treeProxy, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

			treeProxyAppendBranchChild(treeProxy, grandParent, newParent);
			parent=newParent;
			}
		else	// Split root
			{
			LOG(LOG_INFO,"AppendBranch %i: Split Root",parent->index);

			treeProxySplitRoot(treeProxy);

			parent=getRouteTableTreeBranchProxy(treeProxy, treeProxy->rootProxy->dataBlock->childIndex[1]);

//			treeProxySeekEnd(treeProxy, &parent, NULL);
			}

		}

	if(parent->childCount>=parent->childAlloc)
		{
		LOG(LOG_CRITICAL,"No space after split/sibling add in branch node %i", parent->index);
		}

	LOG(LOG_INFO,"Appended Branch %i",childIndex);

	parent->dataBlock->childIndex[parent->childCount++]=-1-childIndex;
	child->dataBlock->parentIndex=parentIndex;

	return parent;
}



// Append the given leaf child to the branch parent, splitting the parent if needed

static RouteTableTreeBranchProxy *treeProxyAppendLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parent, RouteTableTreeLeafProxy *child)
{
	LOG(LOG_INFO,"Append Leaf Child P: %i C: %i",parent->index, child->index);

	s16 parentIndex=parent->index;
	s16 childIndex=child->index;

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
		else if(parent->index!=BRANCH_INDEX_ROOT)
			{
			RouteTableTreeBranchProxy *grandParent;
			s32 grandParentIndex=parent->dataBlock->parentIndex;

			if(grandParentIndex!=BRANCH_INDEX_ROOT)
				grandParent=getRouteTableTreeBranchProxy(treeProxy, grandParentIndex);
			else
				grandParent=treeProxy->rootProxy;

			RouteTableTreeBranchProxy *newParent=treeProxySplitBranch(treeProxy, parent);
//			RouteTableTreeBranchProxy *newParent=allocRouteTableTreeBranchProxy(treeProxy, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

			treeProxyAppendBranchChild(treeProxy, grandParent, newParent);

			LOG(LOG_INFO,"AppendLeaf %i -> AppendBranch %i",child->index,newParent->index);

			treeProxyAppendBranchChild(treeProxy, grandParent, newParent);
			parent=newParent;
			}
		else
			{
			treeProxySplitRoot(treeProxy);

			parent=getRouteTableTreeBranchProxy(treeProxy, treeProxy->rootProxy->dataBlock->childIndex[1]);
			}
		}

	if(parent->childCount>=parent->childAlloc)
		{
		LOG(LOG_CRITICAL,"No space after split/sibling add");
		}

	LOG(LOG_INFO,"Appended Leaf %i",childIndex);

	parent->dataBlock->childIndex[parent->childCount++]=-1-childIndex;
	child->dataBlock->parentIndex=parentIndex;

	return parent;
}





static void getNextLeafSibling(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchPtr, s16 *branchChild, RouteTableTreeLeafProxy **leafPtr)
{



}




static void initTreeWalker(RouteTableTreeWalker *walker, RouteTableTreeProxy *treeProxy)
{
	walker->treeProxy=treeProxy;

	walker->branchProxy=treeProxy->rootProxy;
	walker->branchChild=-1;

	walker->leafProxy=NULL;
	walker->leafEntry=-1;
}

static void walkerSeekStart(RouteTableTreeWalker *walker)
{
	RouteTableTreeBranchProxy *branchProxy=NULL;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 branchChild=0;

	treeProxySeekStart(walker->treeProxy, &branchProxy, &branchChild, &leafProxy);

	walker->branchProxy=branchProxy;
	walker->branchChild=branchChild;
	walker->leafProxy=leafProxy;

	if(leafProxy!=NULL)
		walker->leafEntry=0;
	else
		walker->leafEntry=-1;
}

static void walkerSeekEnd(RouteTableTreeWalker *walker)
{
	RouteTableTreeBranchProxy *branchProxy=NULL;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 branchChild=0;

	treeProxySeekEnd(walker->treeProxy, &branchProxy, &branchChild, &leafProxy);

	walker->branchProxy=branchProxy;
	walker->branchChild=branchChild;
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

	if(walker->leafEntry<walker->leafProxy->entryCount)
		{
		walker->leafEntry++;
		}
	else
		{
		getNextLeafSibling(walker->treeProxy, &walker->branchProxy, &walker->branchChild, &walker->leafProxy);
		walker->leafEntry=0;
		}

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

	walker->branchChild=-1;
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
	else if(leafProxy->entryAlloc<ROUTE_TABLE_TREE_LEAF_ENTRIES) // Expand
		{
		s32 newAlloc=MIN(leafProxy->entryAlloc+ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK, ROUTE_TABLE_TREE_LEAF_ENTRIES);

		RouteTableTreeLeafBlock *leafBlock=reallocRouteTableTreeLeafBlockEntries(leafProxy->dataBlock, walker->treeProxy->disp, newAlloc);

		leafProxy->dataBlock=leafBlock;
		leafProxy->entryAlloc=newAlloc;

		flushRouteTableTreeLeafProxy(walker->treeProxy, leafProxy);
		}

	u16 entryCount=leafProxy->entryCount;

	leafProxy->dataBlock->entries[entryCount].downstream=downstream;
	leafProxy->dataBlock->entries[entryCount].width=entry->width;

	walker->leafEntry=++leafProxy->entryCount;

//	LOG(LOG_INFO,"Appended Leaf Entry");
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

void mergeRoutes_ordered_forwardSingle(RouteTableTreeWalker *walker, RoutePatch *patch)
{
	int targetPrefix=patch->prefixIndex;
	//int targetSuffix=patch->suffixIndex;
	//int minEdgePosition=(*(patch->rdiPtr))->minEdgePosition;
	//int maxEdgePosition=(*(patch->rdiPtr))->maxEdgePosition;

	s16 currentUpstream=-1;
	RouteTableTreeLeafEntry *currentEntry;

	int res=walkerGetCurrentEntry(walker, &currentUpstream, &currentEntry);
	while(res && currentUpstream < targetPrefix)
		{

		res=walkerNextEntry(walker, &currentUpstream, &currentEntry);
		}


}


void rttMergeRoutes(RouteTableTreeBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 maxNewPrefix, s32 maxNewSuffix, RoutingReadData **orderedDispatches, MemDispenser *disp)
{
	LOG(LOG_INFO,"Entries: %i %i",forwardRoutePatchCount, reverseRoutePatchCount);

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

	// Forward Routes

	if(forwardRoutePatchCount>0)
		{
		RouteTableTreeWalker *walker=&(builder->forwardWalker);

		walkerSeekStart(walker);
		walkerInitOffsetArrays(walker, maxNewPrefix+1, maxNewSuffix+1);

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
		walkerInitOffsetArrays(walker, maxNewSuffix+1, maxNewPrefix+1);

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

	LOG(LOG_CRITICAL,"Not Implemented");
}


void rttUnpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp)
{
	LOG(LOG_CRITICAL,"Not implemented: rttUnpackRouteTableForSmerLinked");
}

