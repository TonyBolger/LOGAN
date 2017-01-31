
#include "common.h"


s32 getRouteTableTreeLeafSize_Expected(s16 offsetAlloc, s16 entryAlloc)
{
	return sizeof(RouteTableTreeLeafBlock)+((s32)offsetAlloc)*sizeof(RouteTableTreeLeafOffset)+((s32)entryAlloc)*sizeof(RouteTableTreeLeafEntry);
}


s32 getRouteTableTreeLeafSize_Existing(RouteTableTreeLeafBlock *leafBlock)
{
	return sizeof(RouteTableTreeLeafBlock)+((s32)leafBlock->offsetAlloc)*sizeof(RouteTableTreeLeafOffset)+((s32)leafBlock->entryAlloc)*sizeof(RouteTableTreeLeafEntry);
}

RouteTableTreeLeafOffset *getRouteTableTreeLeaf_OffsetPtr(RouteTableTreeLeafBlock *leafBlock)
{
	return (RouteTableTreeLeafOffset *)(leafBlock->extraData);
}

RouteTableTreeLeafEntry *getRouteTableTreeLeaf_EntryPtr(RouteTableTreeLeafBlock *leafBlock)
{
	return (RouteTableTreeLeafEntry *)(leafBlock->extraData+((s32)leafBlock->offsetAlloc)*sizeof(RouteTableTreeLeafOffset));
}


static RouteTableTreeLeafBlock *reallocRouteTableTreeLeafBlockEntries(RouteTableTreeLeafBlock *oldBlock, MemDispenser *disp, s32 offsetAlloc, s32 entryAlloc)
{
	if(entryAlloc>ROUTE_TABLE_TREE_LEAF_ENTRIES)
		{
		LOG(LOG_CRITICAL,"Cannot allocate oversize leaf with %i children",entryAlloc);
		}

	RouteTableTreeLeafBlock *newBlock=dAlloc(disp, getRouteTableTreeLeafSize_Expected(offsetAlloc, entryAlloc));

	newBlock->offsetAlloc=offsetAlloc;
	newBlock->entryAlloc=entryAlloc;

	s32 oldBlockOffsetAlloc=0;
	s32 oldBlockEntryAlloc=0;

	RouteTableTreeLeafOffset *newOffsetPtr=getRouteTableTreeLeaf_OffsetPtr(newBlock);
	RouteTableTreeLeafEntry *newEntryPtr=getRouteTableTreeLeaf_EntryPtr(newBlock);

	if(oldBlock!=NULL)
		{
		oldBlockOffsetAlloc=oldBlock->offsetAlloc;
		oldBlockEntryAlloc=oldBlock->entryAlloc;

		s32 toKeepOffsets=MIN(oldBlockOffsetAlloc, offsetAlloc);
		s32 toKeepEntries=MIN(oldBlockEntryAlloc, entryAlloc);

//		LOG(LOG_INFO,"Up: %i (%p -> %p): Keeping %i (%i %i) %i (%i %i)",oldBlock->upstream, oldBlock,newBlock,
//				toKeepOffsets, oldBlockOffsetAlloc, offsetAlloc, toKeepEntries, oldBlockEntryAlloc, entryAlloc);

		if(oldBlockOffsetAlloc<0 || oldBlockEntryAlloc<0)
			{
			LOG(LOG_CRITICAL,"Negative array sizes are invalid: Offset: %i Entry: %i",oldBlockOffsetAlloc, oldBlockEntryAlloc);
			}

		if(oldBlockOffsetAlloc > offsetAlloc)
			{
			LOG(LOG_CRITICAL,"Dubious Offset array shrinkage: Was: %i Now: %i",oldBlockOffsetAlloc, offsetAlloc);
			}


		memcpy(newBlock,oldBlock, getRouteTableTreeLeafSize_Expected(toKeepOffsets,0)); // Copy core + offsets

		newBlock->offsetAlloc=offsetAlloc; // Fixup after copy
		newBlock->entryAlloc=entryAlloc;

		RouteTableTreeLeafEntry *oldEntryPtr=getRouteTableTreeLeaf_EntryPtr(oldBlock);
		memcpy(newEntryPtr, oldEntryPtr, toKeepEntries*sizeof(RouteTableTreeLeafEntry)); // Copy entries
		}

	for(int i=oldBlockOffsetAlloc;i<offsetAlloc;i++)
		{
		newOffsetPtr[i]=0;
		}

	for(int i=oldBlockEntryAlloc;i<entryAlloc;i++)
		{
		newEntryPtr[i].downstream=-1;
		newEntryPtr[i].width=0;
		}

	return newBlock;
}


static RouteTableTreeLeafBlock *allocRouteTableTreeLeafBlock(MemDispenser *disp, s32 offsetAlloc, s32 entryAlloc)
{
	if(entryAlloc>ROUTE_TABLE_TREE_LEAF_ENTRIES)
			{
			LOG(LOG_CRITICAL,"Cannot allocate oversize leaf with %i children",entryAlloc);
			}

	RouteTableTreeLeafBlock *block=dAlloc(disp, getRouteTableTreeLeafSize_Expected(offsetAlloc, entryAlloc));

	block->offsetAlloc=offsetAlloc;
	block->entryAlloc=entryAlloc;

	block->parentBrindex=BRANCH_NINDEX_INVALID;
	block->upstream=-1;

	RouteTableTreeLeafOffset *offsetPtr=getRouteTableTreeLeaf_OffsetPtr(block);
	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(block);


	for(int i=0;i<offsetAlloc;i++)
		{
		offsetPtr[i]=0;
		}

	for(int i=0;i<entryAlloc;i++)
		{
		entryPtr[i].downstream=-2;
		entryPtr[i].width=0;
		}

	//LOG(LOG_INFO,"Alloc Leaf block: %p - Size: %i",block,getRouteTableTreeLeafSize_Expected(offsetAlloc, entryAlloc));

	return block;
}




static void getRouteTableTreeLeafProxy_scan(RouteTableTreeLeafBlock *leafBlock, u16 *entryAllocPtr, u16 *entryCountPtr)
{
	if(leafBlock==NULL)
		{
		*entryAllocPtr=0;
		*entryCountPtr=0;
		return;
		}

	u16 entryAlloc=leafBlock->entryAlloc;
	*entryAllocPtr=entryAlloc;

	u16 count=0;

	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(leafBlock);

	for(int i=entryAlloc;i>0;i--)
		{
		if(entryPtr[i-1].width!=0)
			{
			count=i;
			break;
			}
		}

	*entryCountPtr=count;
}



RouteTableTreeLeafBlock *getRouteTableTreeLeafRaw(RouteTableTreeProxy *treeProxy, s32 lindex)
{
	if(lindex<0)
		{
		LOG(LOG_CRITICAL,"Lindex must be positive: %i",lindex);
		}

	u8 *data=getBlockArrayEntry(&(treeProxy->leafArrayProxy), lindex);

	return (RouteTableTreeLeafBlock *)data;
}


RouteTableTreeLeafProxy *getRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 lindex)
{
	if(lindex<0)
		{
		LOG(LOG_CRITICAL,"Lindex must be positive: %i",lindex);
		}

	u8 *data=getBlockArrayEntry(&(treeProxy->leafArrayProxy), lindex);

	RouteTableTreeLeafProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeLeafProxy));

	proxy->dataBlock=(RouteTableTreeLeafBlock *)data;
	proxy->lindex=lindex;

//	LOG(LOG_INFO,"GetRouteTableTreeLeaf : %i",lindex);

	getRouteTableTreeLeafProxy_scan(proxy->dataBlock, &proxy->entryAlloc, &proxy->entryCount);

	return proxy;
}


void flushRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy)
{
//	LOG(LOG_INFO,"Flush %i to %p (%i)",leafProxy->lindex, leafProxy->dataBlock, leafProxy->dataBlock->entryAlloc);

	setBlockArrayEntry(&(treeProxy->leafArrayProxy), leafProxy->lindex, (u8 *)leafProxy->dataBlock, treeProxy->disp);
}


RouteTableTreeLeafProxy *allocRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 offsetAlloc, s32 entryAlloc)
{
	if(entryAlloc<ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK)
		entryAlloc=ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK;

	RouteTableTreeLeafBlock *dataBlock=allocRouteTableTreeLeafBlock(treeProxy->disp, offsetAlloc, entryAlloc);
	s32 lindex=appendBlockArrayEntry(&(treeProxy->leafArrayProxy), (u8 *)dataBlock, treeProxy->disp);

	RouteTableTreeLeafProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeLeafProxy));
	proxy->dataBlock=dataBlock;
	proxy->lindex=lindex;

//	LOG(LOG_INFO,"AllocRouteTableTreeLeaf : %i",lindex);

	proxy->entryAlloc=entryAlloc;
	proxy->entryCount=0;

	return proxy;
}


void ensureRouteTableTreeLeafOffsetCapacity(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy, s32 offsetAlloc)
{
	if(leafProxy->dataBlock->offsetAlloc>=offsetAlloc)
		return;

	//LOG(LOG_INFO,"Resizing offset to %i from %i",offsetAlloc, leafProxy->dataBlock->offsetAlloc);

	leafProxy->dataBlock=reallocRouteTableTreeLeafBlockEntries(leafProxy->dataBlock, treeProxy->disp, offsetAlloc, leafProxy->dataBlock->entryAlloc);
	flushRouteTableTreeLeafProxy(treeProxy, leafProxy);
}

void expandRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy, s32 offsetAlloc)
{
	s16 newEntryAlloc=MIN(leafProxy->entryAlloc+ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK, ROUTE_TABLE_TREE_LEAF_ENTRIES);

	RouteTableTreeLeafBlock *leafBlock=reallocRouteTableTreeLeafBlockEntries(leafProxy->dataBlock, treeProxy->disp, offsetAlloc, newEntryAlloc);

	leafProxy->dataBlock=leafBlock;
	leafProxy->entryAlloc=newEntryAlloc;

	flushRouteTableTreeLeafProxy(treeProxy, leafProxy);
}


/*
void dumpLeafProxy(RouteTableTreeLeafProxy *leafProxy)
{
	LOG(LOG_INFO,"Leaf: Index %i Used %i of %i - Datablock %p",leafProxy->lindex,leafProxy->entryCount,leafProxy->entryAlloc,leafProxy->dataBlock);
	LOG(LOG_INFO,"Upstream %i  BlockAlloc %i",leafProxy->dataBlock->upstream,leafProxy->dataBlock->entryAlloc);

	RouteTableTreeLeafEntry *entryPtr=(RouteTableTreeLeafEntry *)(leafProxy->dataBlock->extraData);

	for(int i=0;i<leafProxy->dataBlock->entryAlloc;i++)
		LOG(LOG_INFO,"Entry %i has DS: %i W %i",i,entryPtr[i].downstream,entryPtr[i].width);
}
*/



void leafMakeEntryInsertSpace(RouteTableTreeLeafProxy *leaf, s16 entryPosition, s16 entryCount)
{
	if((leaf->entryCount+entryCount)>leaf->entryAlloc)
		LOG(LOG_CRITICAL,"Insufficient space for branch child insert");


	if(entryPosition<0)
		LOG(LOG_CRITICAL,"Cannot make entry insert space with negative position %i",entryPosition);

	if(entryPosition<leaf->entryCount)
		{
		s16 entriesToMove=leaf->entryCount-entryPosition;

		RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(leaf->dataBlock);
		RouteTableTreeLeafEntry *entryInsertPtr=entryPtr+entryPosition;

		memmove(entryInsertPtr+entryCount, entryInsertPtr, sizeof(RouteTableTreeLeafEntry)*entriesToMove);
		}

	leaf->entryCount+=entryCount;
}








