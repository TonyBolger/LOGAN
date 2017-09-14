
#include "common.h"

/*

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

	//LOG(LOG_INFO,"Alloc of %i %i - total %i",offsetAlloc, entryAlloc, getRouteTableTreeLeafSize_Expected(offsetAlloc, entryAlloc));

	RouteTableTreeLeafBlock *block=dAlloc(disp, getRouteTableTreeLeafSize_Expected(offsetAlloc, entryAlloc));

	block->offsetAlloc=offsetAlloc;
	block->entryAlloc=entryAlloc;

	block->parentBrindex=BRANCH_NINDEX_INVALID;
	block->upstream=-1;
	block->upstreamOffset=0;

	RouteTableTreeLeafOffset *offsetPtr=getRouteTableTreeLeaf_OffsetPtr(block);
	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(block);

	memset(offsetPtr,0,sizeof(RouteTableTreeLeafOffset)*offsetAlloc);

	for(int i=0;i<entryAlloc;i++)
		{
		entryPtr[i].downstream=-2;
		entryPtr[i].width=0;
		}

	//LOG(LOG_INFO,"Alloc Leaf block: %p - Size: %i",block,getRouteTableTreeLeafSize_Expected(offsetAlloc, entryAlloc));

	return block;
}




void getRouteTableTreeLeafProxy_scan(RouteTableTreeLeafBlock *leafBlock, u16 *entryAllocPtr, u16 *entryCountPtr)
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


*/

void flushRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy)
{
//	LOG(LOG_INFO,"Flush %i to %p (%i)",leafProxy->lindex, leafProxy->dataBlock, leafProxy->dataBlock->entryAlloc);

	leafProxy->status=LEAFPROXY_STATUS_DIRTY;
	setBlockArrayEntryProxy(&(treeProxy->leafArrayProxy), leafProxy->lindex, leafProxy, treeProxy->disp);
}


RouteTableTreeLeafProxy *allocRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 upstreamOffsetAlloc, s32 downstreamOffsetAlloc, s32 entryArrayAlloc)
{
	RouteTableUnpackedSingleBlock *unpackedBlock=rtpAllocUnpackedSingleBlock(treeProxy->disp, upstreamOffsetAlloc, downstreamOffsetAlloc);
	rtpAllocUnpackedSingleBlockEntryArray(unpackedBlock, entryArrayAlloc);

	RouteTableTreeLeafProxy *proxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeLeafProxy));
	s32 lindex=appendBlockArrayEntryProxy(&(treeProxy->leafArrayProxy), proxy, treeProxy->disp);

	proxy->leafBlock=NULL;
	proxy->lindex=lindex;

	proxy->status=LEAFPROXY_STATUS_FULLYUNPACKED;
	proxy->unpackedBlock=unpackedBlock;

//	LOG(LOG_INFO,"AllocRouteTableTreeLeaf : %i",lindex);

	return proxy;
}

/*
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



*/


void dumpLeafBlock(RouteTableTreeLeafBlock *leafBlock)
{
	LOG(LOG_INFO,"LeafBlock: Parent Brindex is %i",leafBlock->parentBrindex);

}

void dumpLeafProxy(RouteTableTreeLeafProxy *leafProxy)
{
	LOG(LOG_INFO,"Dump LeafProxy");

	LOG(LOG_INFO,"Parent Brindex is %i",leafProxy->parentBrindex);
	LOG(LOG_INFO,"Lindex is %i",leafProxy->lindex);

	LOG(LOG_INFO,"Status is %i",leafProxy->status);

	if(leafProxy->leafBlock==NULL)
		LOG(LOG_INFO,"LeafBlock is NULL");
	else
		{
		LOG(LOG_INFO,"LeafBlock is not NULL"); // Could dump
		dumpLeafBlock(leafProxy->leafBlock);
		}

	rtpDumpUnpackedSingleBlock(leafProxy->unpackedBlock);

	LOG(LOG_INFO,"End Dump LeafProxy");
	/*
	if(leafBlock!=NULL)
		{
		LOG(LOG_INFO,"Leaf (%p): Parent %i, Upstream %i, Upstream Offset %i, Offset Alloc %i, Entry Alloc %i",
			leafBlock, leafBlock->parentBrindex, leafBlock->upstream, leafBlock->upstreamOffset,
			leafBlock->offsetAlloc, leafBlock->entryAlloc);

		LOGS(LOG_INFO,"Offsets: ");
		RouteTableTreeLeafOffset *offsetPtr=getRouteTableTreeLeaf_OffsetPtr(leafBlock);

		for(int j=0;j<leafBlock->offsetAlloc;j++)
			{
			LOGS(LOG_INFO,"%i ",offsetPtr[j]);
			if((j&0x3F)==0x3F)
				LOGN(LOG_INFO,"");
			}

		LOGN(LOG_INFO,"");

		LOGS(LOG_INFO,"Entries: ");
		RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(leafBlock);

		for(int j=0;j<leafBlock->entryAlloc;j++)
			{
			LOGS(LOG_INFO,"D:%i W:%i  ",entryPtr[j].downstream, entryPtr[j].width);
			if((j&0x1F)==0x1F)
				LOGN(LOG_INFO,"");
			}

		LOGN(LOG_INFO,"");
		}
	else
		LOG(LOG_INFO,"Leaf: NULL");
*/
}

/*
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


void initRouteTableTreeLeafOffsets(RouteTableTreeLeafProxy *leafProxy, s16 upstream)
{
	leafProxy->dataBlock->upstream=upstream;
	leafProxy->dataBlock->upstreamOffset=0;

	RouteTableTreeLeafOffset *offsets=getRouteTableTreeLeaf_OffsetPtr(leafProxy->dataBlock);

	//for(int i=0;i<leafProxy->dataBlock->offsetAlloc; i++)
//		offsets[i]=0;

	memset(offsets, 0, leafProxy->dataBlock->offsetAlloc*sizeof(RouteTableTreeLeafOffset));
}

void recalcRouteTableTreeLeafOffsets(RouteTableTreeLeafProxy *leafProxy)
{
	RouteTableTreeLeafOffset *offsets=getRouteTableTreeLeaf_OffsetPtr(leafProxy->dataBlock);
	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(leafProxy->dataBlock);

	//for(int i=0;i<leafProxy->dataBlock->offsetAlloc; i++)
			//offsets[i]=0;

	//LOG(LOG_INFO,"Recalc for leaf");
	//dumpLeafBlock(leafProxy->dataBlock);

	leafProxy->dataBlock->upstreamOffset=0;

	memset(offsets, 0, leafProxy->dataBlock->offsetAlloc*sizeof(RouteTableTreeLeafOffset));

	for(int i=0;i<leafProxy->entryCount;i++)
		{
		leafProxy->dataBlock->upstreamOffset+=entryPtr[i].width;
		offsets[entryPtr[i].downstream]+=entryPtr[i].width;
		}

	//LOG(LOG_INFO,"Recalc for leaf completed");
}


void validateRouteTableTreeLeafOffsets(RouteTableTreeLeafProxy *leafProxy)
{
	RouteTableTreeLeafOffset *offsets=getRouteTableTreeLeaf_OffsetPtr(leafProxy->dataBlock);
	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(leafProxy->dataBlock);

	//for(int i=0;i<leafProxy->dataBlock->offsetAlloc; i++)
			//offsets[i]=0;

	RouteTableTreeLeafOffset *checkOffsets=alloca(leafProxy->dataBlock->offsetAlloc*sizeof(RouteTableTreeLeafOffset));

	s32 checkUpstreamOffset=0;

	memset(checkOffsets, 0, leafProxy->dataBlock->offsetAlloc*sizeof(RouteTableTreeLeafOffset));

	for(int i=0;i<leafProxy->entryCount;i++)
		{
		checkUpstreamOffset+=entryPtr[i].width;
		checkOffsets[entryPtr[i].downstream]+=entryPtr[i].width;
		}

	int bailFlag=0;

	if(leafProxy->dataBlock->upstreamOffset!=checkUpstreamOffset)
		{
		LOG(LOG_INFO,"Mismatch upstream offset (Found: %i vs Calculated: %i)",leafProxy->dataBlock->upstreamOffset, checkUpstreamOffset);
		bailFlag=1;
		}


	for(int i=0;i<leafProxy->dataBlock->offsetAlloc;i++)
		{
		if(offsets[i]!=checkOffsets[i])
			{
			LOG(LOG_INFO,"Mismatch at offset %i (Found: %i vs Calculated: %i)",i,offsets[i],checkOffsets[i]);
			bailFlag=1;
			}
		}

	if(bailFlag)
	{
		dumpLeafBlock(leafProxy->dataBlock);
		LOG(LOG_CRITICAL,"Bailing out");
	}
}


*/



