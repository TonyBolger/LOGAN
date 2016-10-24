
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
		memcpy(block,oldBlock, sizeof(RouteTableTreeLeafBlock)+oldBlockEntryAlloc*sizeof(RouteTableTreeLeafEntry));
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
		u8 *newData=arrayProxy->dataBlock->data[index];

		if(newData!=NULL)
			return newData;
		}

	if(index>=0 && index<arrayProxy->dataCount)
		return arrayProxy->dataBlock->data[index];


	LOG(LOG_CRITICAL,"Invalid array index %i - Size %i",index,arrayProxy->newDataCount);

	return NULL;

}

void ensureBlockArrayWritable(RouteTableTreeArrayProxy *arrayProxy, MemDispenser *disp)
{
	if(arrayProxy->newData==NULL) // Haven't yet written anything - make writable
		{
		int newAllocCount=arrayProxy->dataAlloc;

		if(arrayProxy->dataCount==newAllocCount)
			newAllocCount+=ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK;

		u8 **newData=dAlloc(disp, sizeof(u8 *)*newAllocCount);

		if(newAllocCount>arrayProxy->dataCount)
			memset(newData+sizeof(u8 *)*arrayProxy->dataCount, 0, sizeof(u8 *)*newAllocCount);

		arrayProxy->newData=newData;
		arrayProxy->newDataAlloc=newAllocCount;
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

	if(index>=0 && index<arrayProxy->dataCount)
		arrayProxy->newData[index]=data;
	else
		LOG(LOG_CRITICAL,"Invalid array index %i - Size %i",index,arrayProxy->dataCount);
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

		arrayProxy->newData=dAlloc(disp, sizeof(u8 *)*newAllocCount);
		memcpy(newData,arrayProxy->newData, sizeof(u8 *)*newAllocCount);

		arrayProxy->newData=newData;
		arrayProxy->newDataAlloc=newAllocCount;
		}

	s32 index=arrayProxy->newDataCount++;
	arrayProxy->newData[index]=data;
	return arrayProxy->dataAlloc+index;
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
void setRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy)
{
	setBlockArrayEntry(&(treeProxy->branchArrayProxy), branchProxy->index, (u8 *)branchProxy->dataBlock, treeProxy->disp);
}

//static
RouteTableTreeBranchProxy *allocRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy)
{
	RouteTableTreeBranchBlock *dataBlock=allocRouteTableTreeBranchBlock(treeProxy->disp, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);
	s32 index=appendBlockArrayEntry(&(treeProxy->branchArrayProxy), (u8 *)dataBlock, treeProxy->disp);

	RouteTableTreeBranchProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeBranchProxy));
	proxy->dataBlock=dataBlock;
	proxy->index=index;

	proxy->childAlloc=ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK;
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
void setRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy)
{
	setBlockArrayEntry(&(treeProxy->leafArrayProxy), leafProxy->index, (u8 *)leafProxy->dataBlock, treeProxy->disp);
}

//static
RouteTableTreeLeafProxy *allocRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy)
{
	RouteTableTreeLeafBlock *dataBlock=allocRouteTableTreeLeafBlock(treeProxy->disp, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);
	s32 index=appendBlockArrayEntry(&(treeProxy->leafArrayProxy), (u8 *)dataBlock, treeProxy->disp);

	RouteTableTreeLeafProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeLeafProxy));
	proxy->dataBlock=dataBlock;
	proxy->index=index;

	proxy->entryAlloc=ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK;
	proxy->entryCount=0;

	return proxy;
}



static void initTreeProxy(RouteTableTreeProxy *proxy, HeapDataBlock *leafBlock, HeapDataBlock *branchBlock, HeapDataBlock *offsetBlock, MemDispenser *disp)
{
	proxy->disp=disp;

	initBlockArrayProxy(proxy, &(proxy->leafArrayProxy), leafBlock, 2);
	initBlockArrayProxy(proxy, &(proxy->branchArrayProxy), branchBlock, 2);
	initBlockArrayProxy(proxy, &(proxy->offsetArrayProxy), offsetBlock, 2);

	if(getBlockArraySize(&(proxy->branchArrayProxy))>0)
		proxy->rootProxy=getRouteTableTreeBranchProxy(proxy, 0);
	else
		proxy->rootProxy=allocRouteTableTreeBranchProxy(proxy);

}


static RouteTableTreeBranchProxy *treeProxyAppendLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parent, RouteTableTreeLeafProxy *child)
{
	s16 parentIndex=parent->index;
	s16 childIndex=child->index;

	if(parent->childCount>=parent->childAlloc)
		{
		LOG(LOG_INFO,"Not implemented");
		}

	LOG(LOG_INFO,"Appended Leaf");

	parent->dataBlock->childIndex[parent->childCount++]=childIndex;
	child->dataBlock->parentIndex=parentIndex;

	return parent;
}








static void initTreeWalker(RouteTableTreeWalker *walker, RouteTableTreeProxy *treeProxy)
{
	walker->treeProxy=treeProxy;

	walker->branchProxy=treeProxy->rootProxy;
	walker->leafProxy=NULL;

	walker->leafEntry=-1;
}

static void walkerSeekEnd(RouteTableTreeWalker *walker)
{
	RouteTableTreeBranchProxy *branchProxy=walker->treeProxy->rootProxy;
	RouteTableTreeLeafProxy *leafProxy=NULL;

	s16 lastChildIndex=0;

	while(branchProxy->childCount>0 && lastChildIndex>=0)
		{
		lastChildIndex=branchProxy->dataBlock->childIndex[branchProxy->childCount-1];

		if(lastChildIndex>0)
			{
			branchProxy=getRouteTableTreeBranchProxy(walker->treeProxy, lastChildIndex);
			}

		}

	if(lastChildIndex<0)
		{
		leafProxy=getRouteTableTreeLeafProxy(walker->treeProxy,1-lastChildIndex);
		}

	walker->branchProxy=branchProxy;
	walker->leafProxy=leafProxy;

	if(leafProxy!=NULL)
		walker->leafEntry=leafProxy->entryCount;
	else
		walker->leafEntry=-1;
}


static void walkerAppendLeaf(RouteTableTreeWalker *walker, s16 upstream)
{
	RouteTableTreeLeafProxy *leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy);

	walker->branchProxy=treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, leafProxy);
	leafProxy->dataBlock->upstream=upstream;

	walker->leafProxy=leafProxy;
	walker->leafEntry=leafProxy->entryCount;
}

static void walkerAppendPreorderedEntry(RouteTableTreeWalker *walker, RouteTableEntry *entry, int routingTable)
{
	s32 upstream=routingTable==ROUTING_TABLE_FORWARD?entry->prefix:entry->suffix;
	s32 downstream=routingTable==ROUTING_TABLE_FORWARD?entry->suffix:entry->prefix;

	if(walker->leafProxy==NULL || walker->leafProxy->entryCount==ROUTE_TABLE_TREE_LEAF_ENTRIES)
		walkerAppendLeaf(walker, upstream);

	u16 entryCount=walker->leafProxy->entryCount;

	if(walker->leafProxy->entryAlloc==walker->leafProxy->entryCount)
		{
		RouteTableTreeLeafBlock *leaf=reallocRouteTableTreeLeafBlockEntries(walker->leafProxy->dataBlock, walker->treeProxy->disp,
				walker->leafProxy->entryAlloc+ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);

		walker->leafProxy->dataBlock=leaf;
		}

	walker->leafProxy->dataBlock->entries[entryCount].downstream=downstream;
	walker->leafProxy->dataBlock->entries[entryCount].width=entry->width;

	walker->leafEntry=++walker->leafProxy->entryCount;

	LOG(LOG_INFO,"Appended Leaf Entry");
}



static void walkerAppendPreorderedEntries(RouteTableTreeWalker *walker, RouteTableEntry *entries, u32 entryCount, int routingTable)
{
	walkerSeekEnd(walker);

	for(int i=0;i<entryCount;i++)
		walkerAppendPreorderedEntry(walker,entries+i, routingTable);

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



//	treeBuilder->newEntryCount=arrayBuilder->oldForwardEntryCount+arrayBuilder->oldReverseEntryCount;

	LOG(LOG_CRITICAL,"Not implemented: rttUpgradeToRouteTableTreeBuilder");
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

void rttMergeRoutes(RouteTableTreeBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 maxNewPrefix, s32 maxNewSuffix, RoutingReadData **orderedDispatches, MemDispenser *disp)
{
	LOG(LOG_CRITICAL,"Not implemented: rttMergeRoutes");

	}

void rttUnpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp)
{
	LOG(LOG_CRITICAL,"Not implemented: rttUnpackRouteTableForSmerLinked");
}

