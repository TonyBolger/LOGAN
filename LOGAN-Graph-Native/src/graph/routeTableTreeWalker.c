#include "common.h"




void initTreeWalker(RouteTableTreeWalker *walker, RouteTableTreeProxy *treeProxy)
{
	walker->treeProxy=treeProxy;

	walker->branchProxy=treeProxy->rootProxy;
	walker->branchChildSibdex=-1;

	walker->leafProxy=NULL;
//	LOG(LOG_INFO,"leafEntry -1");
	walker->leafEntry=-1;

	walker->upstreamOffsetCount=0;
	walker->upstreamOffsets=NULL;

	walker->downstreamOffsetCount=0;
	walker->downstreamOffsets=NULL;

}


void dumpWalker(RouteTableTreeWalker *walker)
{
	LOG(LOG_INFO,"Walker Dump: Starts");

	if(walker->branchProxy!=NULL)
		{
		LOG(LOG_INFO,"Branch: %i ",walker->branchProxy->brindex);

		}
	else
		LOG(LOG_INFO,"Branch: NULL");

	if(walker->leafProxy!=NULL)
		{
		LOG(LOG_INFO,"Leaf: %i Entry: %i",walker->leafProxy->lindex, walker->leafEntry);
		dumpLeafBlock(walker->leafProxy->dataBlock);
		}
	else
		LOG(LOG_INFO,"Leaf: NULL");

	if(walker->upstreamOffsets!=NULL)
		{
		LOGS(LOG_INFO,"Walker UpstreamOffsets: ");

		for(int j=0;j<walker->upstreamOffsetCount;j++)
			{
			LOGS(LOG_INFO,"%i ",walker->upstreamOffsets[j]);
			if((j&0x1F)==0x1F)
				LOGN(LOG_INFO,"");
			}
		LOGN(LOG_INFO,"");
		}
	else
		LOG(LOG_INFO,"Walker UpstreamOffsets: NULL");

	if(walker->downstreamOffsets!=NULL)
		{
		LOGS(LOG_INFO,"Walker DownstreamOffsets: ");

		for(int j=0;j<walker->downstreamOffsetCount;j++)
			{
			LOGS(LOG_INFO,"%i ",walker->downstreamOffsets[j]);
			if((j&0x1F)==0x1F)
				LOGN(LOG_INFO,"");
			}
		LOGN(LOG_INFO,"");
		}
	else
		LOG(LOG_INFO,"Walker DownstreamOffsets: NULL");

	LOG(LOG_INFO,"Walker Dump: Ends");
}


void walkerSeekStart(RouteTableTreeWalker *walker)
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
		//LOG(LOG_INFO,"First Leaf: %i %i",leafProxy->entryCount,leafProxy->entryAlloc);
		walker->leafEntry=0;
//		validateRouteTableTreeLeafOffsets(leafProxy);
		}
	else
		{
//		LOG(LOG_INFO,"leafEntry -1");
		walker->leafEntry=-1;
		}
}

void walkerSeekEnd(RouteTableTreeWalker *walker)
{
	RouteTableTreeBranchProxy *branchProxy=NULL;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 branchChildSibdex=0;

	treeProxySeekEnd(walker->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy);

	walker->branchProxy=branchProxy;
	walker->branchChildSibdex=branchChildSibdex;
	walker->leafProxy=leafProxy;

	if(leafProxy!=NULL)
		{
		walker->leafEntry=leafProxy->entryCount;
//		validateRouteTableTreeLeafOffsets(leafProxy);
		}
	else
		{
//		LOG(LOG_INFO,"leafEntry -1");
		walker->leafEntry=-1;
		}
}

s32 walkerGetCurrentEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableTreeLeafEntry **entry)
{
	if((walker->leafProxy==NULL)||(walker->leafEntry==-1))
		{
		*upstream=32767;
		*entry=NULL;

		return 0;
		}

	*upstream=walker->leafProxy->dataBlock->upstream;

	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(walker->leafProxy->dataBlock);

	*entry=entryPtr+walker->leafEntry;

	return 1;
}


s32 walkerNextLeaf(RouteTableTreeWalker *walker, s16 *upstream, RouteTableTreeLeafEntry **entry)
{
	RouteTableTreeBranchProxy *branchProxy=walker->branchProxy;
	s16 branchChildSibdex=walker->branchChildSibdex;
	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	if(!getNextLeafSibling(walker->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy))
		{
//		LOG(LOG_INFO,"Next Sib failed");

		walker->branchProxy=branchProxy;
		walker->branchChildSibdex=branchChildSibdex;
		walker->leafProxy=leafProxy;

		//LOG(LOG_INFO,"leafEntry at end");

		walker->leafEntry=walker->leafProxy->entryCount;
		*upstream=32767;
		*entry=NULL;

		return 0;
		}

	if(leafProxy->dataBlock==NULL)
		{
		LOG(LOG_CRITICAL,"Leaf Proxy without datablock %i (%i of %i)",leafProxy->lindex, leafProxy->entryCount, leafProxy->entryAlloc);
		}

	walker->branchProxy=branchProxy;
	walker->branchChildSibdex=branchChildSibdex;
	walker->leafProxy=leafProxy;
	walker->leafEntry=0;

	*upstream=walker->leafProxy->dataBlock->upstream;
	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(walker->leafProxy->dataBlock);
	*entry=entryPtr+walker->leafEntry;

	return 1;

}

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
//		LOG(LOG_INFO,"Next Sib Peek");

		RouteTableTreeBranchProxy *branchProxy=walker->branchProxy;
		s16 branchChildSibdex=walker->branchChildSibdex;
		RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

		if(!getNextLeafSibling(walker->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy))
			{
			//LOG(LOG_INFO,"Next Sib failed");

			walker->branchProxy=branchProxy;
			walker->branchChildSibdex=branchChildSibdex;
			walker->leafProxy=leafProxy;

			//LOG(LOG_INFO,"leafEntry at end");

			walker->leafEntry=walker->leafProxy->entryCount;
			*upstream=32767;
			*entry=NULL;

			return 0;
			}

		if(leafProxy->dataBlock==NULL)
			{
			LOG(LOG_CRITICAL,"Leaf Proxy without datablock %i (%i of %i)",leafProxy->lindex, leafProxy->entryCount, leafProxy->entryAlloc);
			}

		if(holdUpstream && leafProxy->dataBlock!=NULL && (leafProxy->dataBlock->upstream!=*upstream))
			{
//			LOG(LOG_INFO,"Hold upstream");

			*entry=NULL;
			return 0;
			}

		walker->branchProxy=branchProxy;
		walker->branchChildSibdex=branchChildSibdex;
		walker->leafProxy=leafProxy;
		walker->leafEntry=0;

//		validateRouteTableTreeLeafOffsets(leafProxy);

//		LOG(LOG_INFO,"Next Sib ok %i with %i",walker->leafProxy->lindex,walker->leafProxy->entryCount);
		}

	*upstream=walker->leafProxy->dataBlock->upstream;

	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(walker->leafProxy->dataBlock);

	*entry=entryPtr+walker->leafEntry;

	return 1;
}


void walkerResetOffsetArrays(RouteTableTreeWalker *walker)
{
//	LOG(LOG_INFO,"Walker Offset Reset %i %i",walker->upstreamOffsetCount, walker->downstreamOffsetCount);

	memset(walker->upstreamOffsets,0,sizeof(s32)*walker->upstreamOffsetCount);
	memset(walker->downstreamOffsets,0,sizeof(s32)*walker->downstreamOffsetCount);
}

void walkerInitOffsetArrays(RouteTableTreeWalker *walker, s32 upstreamCount, s32 downstreamCount)
{
	//LOG(LOG_INFO,"Walker Offset Init %i %i",upstreamCount, downstreamCount);

	s32 *up=dAlloc(walker->treeProxy->disp, sizeof(s32)*upstreamCount);
	s32 *down=dAlloc(walker->treeProxy->disp, sizeof(s32)*downstreamCount);

	memset(up,0,sizeof(s32)*upstreamCount);
	memset(down,0,sizeof(s32)*downstreamCount);

	walker->upstreamOffsetCount=upstreamCount;
	walker->upstreamOffsets=up;

	walker->downstreamOffsetCount=downstreamCount;
	walker->downstreamOffsets=down;
}


void walkerAccumulateLeafOffsets(RouteTableTreeWalker *walker)
{
	if(walker->leafProxy==NULL)
		return;

	s16 upstream=walker->leafProxy->dataBlock->upstream;
	walker->upstreamOffsets[upstream]+=walker->leafProxy->dataBlock->upstreamOffset;

	int commonOffsets=MIN(walker->downstreamOffsetCount, walker->leafProxy->dataBlock->offsetAlloc);

	RouteTableTreeLeafOffset *offsets=getRouteTableTreeLeaf_OffsetPtr(walker->leafProxy->dataBlock);

	for(int i=0;i<commonOffsets;i++)
		walker->downstreamOffsets[i]+=offsets[i];
}


void walkerAppendNewLeaf(RouteTableTreeWalker *walker, s16 upstream)
{
	//LOG(LOG_INFO,"WalkerAppendLeaf - new leaf with");
	RouteTableTreeLeafProxy *leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->downstreamOffsetCount, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);

	treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, leafProxy, &walker->branchProxy, &walker->branchChildSibdex);
	leafProxy->dataBlock->upstream=upstream;
	leafProxy->dataBlock->upstreamOffset=0;

	walker->leafProxy=leafProxy;
	walker->leafEntry=leafProxy->entryCount;
}


void walkerAppendPreorderedEntry(RouteTableTreeWalker *walker, RouteTableEntry *entry, int routingTable)
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
		expandRouteTableTreeLeafProxy(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);
		}

	u16 entryCount=leafProxy->entryCount;

	if(downstream>32000 || entry->width>32000)
		LOG(LOG_CRITICAL,"Cannot append entry with large downstream/width %i %i",downstream, entry->width);

	RouteTableTreeLeafOffset *offsetPtr=getRouteTableTreeLeaf_OffsetPtr(leafProxy->dataBlock);
	offsetPtr[downstream]+=entry->width;

	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(leafProxy->dataBlock);

	entryPtr[entryCount].downstream=downstream;
	entryPtr[entryCount].width=entry->width;

	leafProxy->dataBlock->upstreamOffset+=entry->width;
	walker->leafEntry=++leafProxy->entryCount;

//	LOG(LOG_INFO,"Appended Leaf Entry %i with up %i down %i width %i",leafProxy->entryCount,upstream, downstream,entry->width);
}



void walkerAppendPreorderedEntries(RouteTableTreeWalker *walker, RouteTableEntry *entries, u32 entryCount, int routingTable)
{
	walkerSeekEnd(walker);

	for(int i=0;i<entryCount;i++)
		walkerAppendPreorderedEntry(walker,entries+i, routingTable);

	//LOG(LOG_INFO,"Routing Table with %i entries used %i branches and %i leaves",
//		entryCount,walker->treeProxy->branchArrayProxy.newDataCount,walker->treeProxy->leafArrayProxy.newDataCount);
}
