#include "common.h"




void initTreeWalker(RouteTableTreeWalker *walker, RouteTableTreeProxy *treeProxy)
{
	walker->treeProxy=treeProxy;

	walker->branchProxy=treeProxy->rootProxy;
	walker->branchChildSibdex=-1;

	walker->leafProxy=NULL;
	walker->leafArrayIndex=-1;
	walker->leafEntryIndex=-1;

	walker->upstreamOffsetCount=0;
	walker->upstreamLeafOffsets=NULL;
	walker->upstreamEntryOffsets=NULL;

	walker->downstreamOffsetCount=0;
	walker->downstreamLeafOffsets=NULL;
	walker->downstreamEntryOffsets=NULL;
}


void dumpWalker(RouteTableTreeWalker *walker)
{
	LOG(LOG_CRITICAL,"PackLeaf: dumpWalker TODO");

	/*
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
	*/
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
		walker->leafArrayIndex=0;
		walker->leafEntryIndex=0;
//		validateRouteTableTreeLeafOffsets(leafProxy);
		}
	else
		{
		walker->leafArrayIndex=-1;
		walker->leafEntryIndex=-1;
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
		LOG(LOG_CRITICAL,"PackLeaf: walkerSeekEnd TODO");
//		walker->leafEntry=leafProxy->entryCount;
//		validateRouteTableTreeLeafOffsets(leafProxy);
		}
	else
		{
		walker->leafArrayIndex=-1;
		walker->leafEntryIndex=-1;
		}

}

s32 walkerGetCurrentEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableUnpackedEntry **entry)
{
	if((walker->leafProxy==NULL)||(walker->leafArrayIndex==-1)||(walker->leafEntryIndex==-1))
		{
		*upstream=32767;
		*entry=NULL;

		return 0;
		}

	RouteTableUnpackedEntryArray *array=walker->leafProxy->unpackedBlock->entryArrays[walker->leafArrayIndex];

	*upstream=array->upstream;
	*entry=array->entries+walker->leafEntryIndex;

	return 1;
}


s32 walkerNextLeaf(RouteTableTreeWalker *walker, s16 *upstream, RouteTableUnpackedEntry **entry)
{
	LOG(LOG_CRITICAL,"PackLeaf: walkerNextLeaf TODO");

	/*
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
*/
	return 1;

}

s32 walkerNextEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableUnpackedEntry **entry, s32 holdUpstream)
{
	LOG(LOG_CRITICAL,"PackLeaf: walkerNextEntry TODO");

	/*
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

*/

	return 1;
}


static void walkerApplyLeafOffsets(RouteTableTreeWalker *walker, RouteTableTreeLeafProxy *leafProxy)
{
	RouteTableUnpackedSingleBlock *block=leafProxy->unpackedBlock;

	for(int i=0;i<walker->upstreamOffsetCount;i++)
		walker->upstreamLeafOffsets[i]+=block->upstreamOffsets[i];

	for(int i=0;i<walker->downstreamOffsetCount;i++)
			walker->downstreamLeafOffsets[i]+=block->downstreamOffsets[i];
}

static void walkerApplyArrayOffsets(RouteTableTreeWalker *walker, RouteTableTreeLeafProxy *leafProxy, int arrayIndex)
{
	RouteTableUnpackedSingleBlock *block=leafProxy->unpackedBlock;
	RouteTableUnpackedEntryArray *array=block->entryArrays[arrayIndex];

	s32 *downstreamOffsets=walker->downstreamEntryOffsets;
	s32 upstreamOffset=0;

	for(int i=0;i<array->entryCount;i++)
		{
		s32 downstream=array->entries[i].downstream;
		s32 width=array->entries[i].width;

		downstreamOffsets[downstream]+=width;
		upstreamOffset+=width;
		}

	walker->upstreamEntryOffsets[array->upstream]+=upstreamOffset;

}

//static
void walkerTransferOffsetsLeafToEntry(RouteTableTreeWalker *walker)
{
	memcpy(walker->upstreamEntryOffsets, walker->upstreamLeafOffsets, sizeof(s32)*walker->upstreamOffsetCount);
	memcpy(walker->downstreamEntryOffsets, walker->downstreamLeafOffsets, sizeof(s32)*walker->downstreamOffsetCount);
}

// Move to last entry that has upstream<=targetUpstream, and upstreamOffset<=targetOffset

s32 walkerAdvanceToUpstreamThenOffsetThenDownstream(RouteTableTreeWalker *walker, s32 targetUpstream, s32 targetDownstream, s32 targetMinOffset, s32 targetMaxOffset,
		s32 *upstreamPtr, RouteTableUnpackedEntry **entryPtr, s32 *upstreamOffsetPtr, s32 *downstreamOffsetPtr)
{
	RouteTableTreeBranchProxy *branchProxy=walker->branchProxy;
	s16 branchChildSibdex=walker->branchChildSibdex;
	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	if(leafProxy==NULL) // Already past end, fail
		{
		LOG(LOG_INFO,"Null proxy");

		return 0;
		}


	LOG(LOG_INFO,"Advancing to U %i D %i Offset (%i %i)",targetUpstream,targetDownstream,targetMinOffset,targetMaxOffset);
	dumpLeafProxy(leafProxy);

	s32 arrayIndex=leafProxy->unpackedBlock->entryArrayCount-1;
	s32 upstream=leafProxy->unpackedBlock->entryArrays[arrayIndex]->upstream;

	LOG(LOG_INFO,"About to move leaf (upstream) %i want %i",upstream,targetUpstream);

	while(upstream < targetUpstream) // One leaf at a time, for upstream
		{
		LOG(LOG_INFO,"Moving one leaf");

		walkerApplyLeafOffsets(walker, leafProxy);

		if(!getNextLeafSibling(walker->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy)) // No more leaves
			{
			LOG(LOG_INFO,"No more leaves");

			*entryPtr=NULL;
			*upstreamPtr=-1;

			*upstreamOffsetPtr=0;
			*downstreamOffsetPtr=walker->downstreamLeafOffsets[targetDownstream];

			return 0;
			}

		arrayIndex=leafProxy->unpackedBlock->entryArrayCount-1;
		upstream=leafProxy->unpackedBlock->entryArrays[arrayIndex]->upstream;
		}

	walkerTransferOffsetsLeafToEntry(walker);

	if(leafProxy->unpackedBlock->entryArrays==NULL)
		LOG(LOG_CRITICAL,"Null leaf arrays");

	if(leafProxy->unpackedBlock->entryArrayCount==0)
		LOG(LOG_CRITICAL,"Empty leaf");

	arrayIndex=0;
	LOG(LOG_INFO,"Array Index %ip",arrayIndex);

	RouteTableUnpackedSingleBlock *block=leafProxy->unpackedBlock;

	upstream=block->entryArrays[arrayIndex]->upstream;

	LOG(LOG_INFO,"About to move array (upstream) %i want %i",upstream,targetUpstream);

	while(upstream < targetUpstream) // One array at a time, for upstream
		{
		LOG(LOG_INFO,"Moving one array");

		walkerApplyArrayOffsets(walker, leafProxy, arrayIndex);

		arrayIndex++;
		if(arrayIndex>=block->entryArrayCount)
			{
			LOG(LOG_INFO,"No more arrays");

			walker->leafArrayIndex=arrayIndex;
			walker->leafEntryIndex=-1;

			*entryPtr=NULL;
			*upstreamPtr=-1;

			*upstreamOffsetPtr=0;
			*downstreamOffsetPtr=walker->downstreamEntryOffsets[targetDownstream];

			return 0;
			}

		upstream=block->entryArrays[arrayIndex]->upstream;
		}

	s32 currentUpstreamOffset=walker->upstreamEntryOffsets[upstream];

	RouteTableUnpackedEntryArray *array=block->entryArrays[arrayIndex];
	s32 entryIndex=0;

	s32 downstream=array->entries[entryIndex].downstream;
	s32 snoopUpstreamOffset=currentUpstreamOffset+array->entries[entryIndex].width;

	LOG(LOG_INFO,"About to move entry (offset) At %i Next %i Want %i",currentUpstreamOffset,snoopUpstreamOffset,targetMinOffset);


	while(upstream == targetUpstream && snoopUpstreamOffset<targetMinOffset) // One entry at a time
		{
		entryIndex++;

		if(entryIndex>=array->entryCount)
			{
			s32 snoopArrayIndex=arrayIndex+1; // Consider moving array
			if(snoopArrayIndex<block->entryArrayCount && block->entryArrays[snoopArrayIndex]->upstream==upstream)
				{
				LOG(LOG_INFO,"No more entries - next array");

				arrayIndex++;
				array=block->entryArrays[arrayIndex];
				entryIndex=0;
				}
			else
				{
				LOG(LOG_INFO,"No more entries");

				walker->leafArrayIndex=arrayIndex;
				walker->leafEntryIndex=entryIndex;

				*entryPtr=NULL;
				*upstreamPtr=-1;

				*upstreamOffsetPtr=currentUpstreamOffset;
				walker->upstreamEntryOffsets[upstream]=currentUpstreamOffset;

				*downstreamOffsetPtr=walker->downstreamEntryOffsets[targetDownstream];

				return 0;
				}
			}

		currentUpstreamOffset=snoopUpstreamOffset;
		s32 width=array->entries[entryIndex].width;
		snoopUpstreamOffset=currentUpstreamOffset+width;

		walker->downstreamLeafOffsets[downstream]+=width;
		downstream=array->entries[entryIndex].downstream;
		}



	LOG(LOG_INFO,"About to move array (downstream) At %i Next %i Want %i",currentUpstreamOffset,snoopUpstreamOffset,targetMinOffset);

	//walker->upstreamOffsets[targetPrefix]+entry->width)<=maxEdgePosition && entry->downstream<targetSuffix

	while(upstream == targetUpstream && snoopUpstreamOffset<=targetMaxOffset && downstream < targetDownstream)
		{
		entryIndex++;

		if(entryIndex>=array->entryCount)
			{
			s32 snoopArrayIndex=arrayIndex+1; // Consider moving array
			if(snoopArrayIndex<block->entryArrayCount && block->entryArrays[snoopArrayIndex]->upstream==upstream)
				{
				LOG(LOG_INFO,"No more entries - next array");

				arrayIndex++;
				array=block->entryArrays[arrayIndex];
				entryIndex=0;
				}
			else
				{
				LOG(LOG_INFO,"No more entries");

				walker->leafArrayIndex=arrayIndex;
				walker->leafEntryIndex=entryIndex;

				*entryPtr=NULL;
				*upstreamPtr=-1;

				*upstreamOffsetPtr=currentUpstreamOffset;
				walker->upstreamEntryOffsets[upstream]=currentUpstreamOffset;

				*downstreamOffsetPtr=walker->downstreamEntryOffsets[targetDownstream];

				return 0;
				}
			}

		currentUpstreamOffset=snoopUpstreamOffset;

		s32 width=array->entries[entryIndex].width;
		snoopUpstreamOffset=currentUpstreamOffset+width;

		walker->downstreamLeafOffsets[downstream]+=width;
		downstream=array->entries[entryIndex].downstream;
		}

	LOG(LOG_INFO,"After move array Up %i (%i) Down %i (%i) Offset %i (%i %i) Width %i",
			upstream, targetUpstream, downstream, targetDownstream, currentUpstreamOffset, targetMinOffset, targetMaxOffset, array->entries[entryIndex].width);

	walker->leafArrayIndex=arrayIndex;
	walker->leafEntryIndex=entryIndex;

	*entryPtr=&(array->entries[entryIndex]);
	*upstreamPtr=upstream;

	*upstreamOffsetPtr=currentUpstreamOffset;
	walker->upstreamEntryOffsets[upstream]=currentUpstreamOffset;

	*downstreamOffsetPtr=walker->downstreamEntryOffsets[targetDownstream];

	return 1;
}



void walkerResetOffsetArrays(RouteTableTreeWalker *walker)
{
	memset(walker->upstreamLeafOffsets,0,sizeof(s32)*walker->upstreamOffsetCount);
	memset(walker->downstreamLeafOffsets,0,sizeof(s32)*walker->downstreamOffsetCount);

	memset(walker->upstreamEntryOffsets,0,sizeof(s32)*walker->upstreamOffsetCount);
	memset(walker->downstreamEntryOffsets,0,sizeof(s32)*walker->downstreamOffsetCount);

}

void walkerInitOffsetArrays(RouteTableTreeWalker *walker, s32 upstreamCount, s32 downstreamCount)
{
	s32 *upLeaf=dAlloc(walker->treeProxy->disp, sizeof(s32)*upstreamCount);
	s32 *downLeaf=dAlloc(walker->treeProxy->disp, sizeof(s32)*downstreamCount);

	s32 *upEntry=dAlloc(walker->treeProxy->disp, sizeof(s32)*upstreamCount);
	s32 *downEntry=dAlloc(walker->treeProxy->disp, sizeof(s32)*downstreamCount);

	memset(upLeaf,0,sizeof(s32)*upstreamCount);
	memset(downLeaf,0,sizeof(s32)*downstreamCount);

	memset(upEntry,0,sizeof(s32)*upstreamCount);
	memset(downEntry,0,sizeof(s32)*downstreamCount);

	walker->upstreamOffsetCount=upstreamCount;
	walker->upstreamLeafOffsets=upLeaf;
	walker->upstreamEntryOffsets=upEntry;

	walker->downstreamOffsetCount=downstreamCount;
	walker->downstreamLeafOffsets=downLeaf;
	walker->downstreamEntryOffsets=downEntry;
}


void walkerAccumulateLeafOffsets(RouteTableTreeWalker *walker)
{
	LOG(LOG_CRITICAL,"PackLeaf: walkerAccumulateLeafOffsets TODO");

	/*
	if(walker->leafProxy==NULL)
		return;

	s16 upstream=walker->leafProxy->dataBlock->upstream;
	walker->upstreamOffsets[upstream]+=walker->leafProxy->dataBlock->upstreamOffset;

	int commonOffsets=MIN(walker->downstreamOffsetCount, walker->leafProxy->dataBlock->offsetAlloc);

	RouteTableTreeLeafOffset *offsets=getRouteTableTreeLeaf_OffsetPtr(walker->leafProxy->dataBlock);

	for(int i=0;i<commonOffsets;i++)
		walker->downstreamOffsets[i]+=offsets[i];

		*/
}




void walkerMergeRoutes_insertEntry(RouteTableTreeWalker *walker, s32 upstream, s32 downstream)
{
	// New entry, with given up/down and width=1. May require array/leaf/branch split if full or new leaf if not matching

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	RouteTableUnpackedSingleBlock *block=NULL;

	if(leafProxy==NULL)
		{
//		LOG(LOG_INFO,"Entry Insert: No current leaf - need new leaf append");

		leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->downstreamOffsetCount, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);
		treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, leafProxy, &walker->branchProxy, &walker->branchChildSibdex);

		walker->leafProxy=leafProxy;
		walker->leafArrayIndex=0;
		walker->leafEntryIndex=0;

		block=walker->leafProxy->unpackedBlock;
		}
	else
		{
		block=walker->leafProxy->unpackedBlock;

		RouteTableUnpackedEntryArray *array=block->entryArrays[walker->leafArrayIndex];
		s32 arrayUpstream=array->upstream;

		//LOG(LOG_INFO,"InsertEntry: Leaf: %i Entry: %i",walker->leafProxy->lindex,walker->leafEntry);

		if(upstream<arrayUpstream) // Insert new array before specified, possibly including splitting leaf and branches
			{
			LOG(LOG_CRITICAL,"ArrayInsertBefore: TODO");

//			LOG(LOG_INFO,"Entry Insert: Lower Unmatched upstream %i %i - need new leaf insert",upstream,leafUpstream);

/*

			leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->downstreamOffsetCount, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);
			treeProxyInsertLeafChild(walker->treeProxy, walker->branchProxy, leafProxy,
					walker->branchChildSibdex, &walker->branchProxy, &walker->branchChildSibdex);

			initRouteTableTreeLeafOffsets(leafProxy, upstream);

			walker->leafProxy=leafProxy;
*/
			//LOG(LOG_INFO,"Entry Insert: Need new leaf %i added to %i",walker->leafProxy->lindex, walker->branchProxy->brindex);
			}
		else if(upstream>arrayUpstream) // Insert new array after specified, possibly including splitting leaf and branches
			{
			// RouteTableUnpackedEntryArray *rtpInsertNewEntryArray(RouteTableUnpackedSingleBlock *unpackedBlock, s32 arrayIndex, s32 upstream, s32 entryAlloc)

			walker->leafArrayIndex++;
			walker->leafEntryIndex=0;

			array=rtpInsertNewEntryArray(block, walker->leafArrayIndex, upstream, ROUTEPACKING_ENTRYS_CHUNK);



			/*
//			LOG(LOG_INFO,"Entry Insert: Higher Unmatched upstream %i %i - need new leaf append",upstream,leafUpstream);

			leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->downstreamOffsetCount, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);
			treeProxyInsertLeafChild(walker->treeProxy, walker->branchProxy, leafProxy,
					walker->branchChildSibdex+1, &walker->branchProxy, &walker->branchChildSibdex);

			initRouteTableTreeLeafOffsets(leafProxy, upstream);

			leafProxy->entryCount=1;

			walker->leafProxy=leafProxy;
			walker->leafEntry=0;
*/
			}

		else // Make room in existing array, possibly including splitting arrays, leaves and branches
			{

			//LOG(LOG_CRITICAL,"ExistingArray: TODO");

			//s16 leafEntry=walker->leafEntry;
			//LOG(LOG_INFO,"Entry Insert: Leaf size at maximum - need to split - pos %i",walker->leafEntry);

/*			RouteTableTreeBranchProxy *targetBranchProxy=NULL;
			s16 targetBranchChildSibdex=-1;
			RouteTableTreeLeafProxy *targetLeafProxy=NULL;
			s16 targetLeafEntry=-1;

//			ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);

			ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, downstream+1);

			//RouteTableTreeLeafProxy *newLeafProxy=

			treeProxySplitLeafInsertChildEntrySpace(walker->treeProxy, walker->branchProxy, walker->branchChildSibdex, walker->leafProxy,
					walker->leafEntry, 1, &targetBranchProxy, &targetBranchChildSibdex, &targetLeafProxy, &targetLeafEntry);


			walker->branchProxy=targetBranchProxy;
			walker->branchChildSibdex=targetBranchChildSibdex;
			walker->leafProxy=targetLeafProxy;
			walker->leafEntry=targetLeafEntry;

			leafProxy=targetLeafProxy;
			*/

			/*
			if(leafProxy->entryCount>=leafProxy->entryAlloc)
				{
				//LOG(LOG_INFO,"Entry Insert: Expand");
				expandRouteTableTreeLeafProxy(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);
				}
			else
				//ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);
				ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, downstream+1);

			leafMakeEntryInsertSpace(leafProxy, walker->leafEntry, 1);
			*/
			}
		}

	if(downstream<0)
		LOG(LOG_CRITICAL,"Insert invalid downstream %i",downstream);


	rtpInsertNewEntry(block, walker->leafArrayIndex, walker->leafEntryIndex, downstream, 1);

	block->upstreamOffsets[upstream]++;
	block->downstreamOffsets[downstream]++;

	// Space already made - set entry data
/*
	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(leafProxy->dataBlock);

	entryPtr[walker->leafEntry].downstream=downstream;
	entryPtr[walker->leafEntry].width=1;

	leafProxy->dataBlock->upstreamOffset++;
	getRouteTableTreeLeaf_OffsetPtr(leafProxy->dataBlock)[downstream]++;

	//LOG(LOG_INFO,"Entry Insert: Entry: %i D: %i Width %i (U %i) with %i used of %i",walker->leafEntry, downstream, 1, walker->leafProxy->dataBlock->upstream,walker->leafProxy->entryCount,walker->leafProxy->entryAlloc);

*/
	//dumpLeafProxy(leafProxy);

//	LOG(LOG_CRITICAL,"PackLeaf: walkerMergeRoutes_insertEntry TODO");
}


void walkerMergeRoutes_widen(RouteTableTreeWalker *walker)
{
	// Add one to the current entry width. Hopefully width doesn't wrap

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	if(leafProxy==NULL)
		{
		LOG(LOG_CRITICAL,"Entry Widen: Null leaf, should never happen");
		}

	if(walker->leafArrayIndex < 0 || walker->leafEntryIndex<0)
		{
		LOG(LOG_CRITICAL,"Entry Widen: Invalid array / entry index, should never happen");
		}

	//ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);

	LOG(LOG_INFO,"Walker Indexes %i %i",walker->leafArrayIndex, walker->leafEntryIndex);

	RouteTableUnpackedEntryArray *arrayPtr=walker->leafProxy->unpackedBlock->entryArrays[walker->leafArrayIndex];
	RouteTableUnpackedEntry *entryPtr=&(arrayPtr->entries[walker->leafEntryIndex]);

	s32 upstream=arrayPtr->upstream;
	s32 downstream=entryPtr->downstream;

	if(upstream<0)
		LOG(LOG_CRITICAL,"Entry Widen: Invalid downstream, should never happen");

	if(downstream<0)
		LOG(LOG_CRITICAL,"Entry Widen: Invalid downstream, should never happen");

	if(entryPtr->width>2000000000)
		LOG(LOG_CRITICAL,"Entry Widen: About to wrap width %i",entryPtr->width);

	LOG(LOG_INFO,"Entry Widen: U %i D %i W %i",upstream, downstream, entryPtr->width);


	entryPtr->width++;

	leafProxy->unpackedBlock->upstreamOffsets[upstream]++;
	leafProxy->unpackedBlock->downstreamOffsets[downstream]++;
}

void walkerMergeRoutes_split(RouteTableTreeWalker *walker, s32 downstream, s32 width1, s32 width2)
{
	// Add split existing route into two, and insert a new route between

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	RouteTableUnpackedSingleBlock *block=walker->leafProxy->unpackedBlock;

	RouteTableUnpackedEntryArray *array=block->entryArrays[walker->leafArrayIndex];
	s32 arrayUpstream=array->upstream;


	s16 newEntryCount=+2;

	if(newEntryCount>ROUTE_TABLE_TREE_LEAF_ENTRIES)
		{
		/*
		//s16 leafEntry=walker->leafEntry;
		//LOG(LOG_INFO,"Entry Split: Leaf size at maximum - need to split - pos %i",leafEntry);

		//LOG(LOG_INFO,"Adding downstream %i, with %i vs %i",downstream,width1,width2);

		//LOG(LOG_INFO,"Pre split:");
		//dumpLeafProxy(leafProxy);

		RouteTableTreeBranchProxy *targetBranchProxy=NULL;
		s16 targetBranchChildSibdex=-1;
		RouteTableTreeLeafProxy *targetLeafProxy=NULL;
		s16 targetLeafEntry=-1;

//		ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);
		ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, downstream+1);

		//RouteTableTreeLeafProxy *newLeafProxy=
		treeProxySplitLeafInsertChildEntrySpace(walker->treeProxy, walker->branchProxy, walker->branchChildSibdex, walker->leafProxy,
				walker->leafEntry, 2, &targetBranchProxy, &targetBranchChildSibdex, &targetLeafProxy, &targetLeafEntry);

		LOG(LOG_INFO,"Post split: Old");
		dumpLeafProxy(leafProxy);

		LOG(LOG_INFO,"Post split: New");
		dumpLeafProxy(newLeafProxy);

		LOG(LOG_INFO,"Post split: Target");
		dumpLeafProxy(targetLeafProxy);

		walker->branchProxy=targetBranchProxy;
		walker->branchChildSibdex=targetBranchChildSibdex;
		walker->leafProxy=targetLeafProxy;
		walker->leafEntry=targetLeafEntry;

		leafProxy=targetLeafProxy;
//		LOG(LOG_INFO,"Post split: Old");
//		dumpLeafProxy(leafProxy);
		}
	else
		{
		if(newEntryCount>leafProxy->entryAlloc)
			{
			//LOG(LOG_INFO,"Entry Split: Leaf full - need to expand");
			expandRouteTableTreeLeafProxy(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);
			}
		else
			//ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);
			ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, downstream+1);

		//LOG(LOG_INFO,"Entry Split: Easy case");
		leafMakeEntryInsertSpace(leafProxy, walker->leafEntry, 2);

		}

	//LOG(LOG_INFO,"Pre insert: - %i",walker->leafEntry);
	//dumpLeafProxy(leafProxy);

	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(leafProxy->dataBlock);

	s16 splitDownstream=entryPtr[walker->leafEntry].downstream;
	entryPtr[walker->leafEntry++].width=width1;

	entryPtr[walker->leafEntry].downstream=downstream;
	entryPtr[walker->leafEntry++].width=1;

	entryPtr[walker->leafEntry].downstream=splitDownstream;
	entryPtr[walker->leafEntry++].width=width2;

	leafProxy->dataBlock->upstreamOffset++;
	getRouteTableTreeLeaf_OffsetPtr(leafProxy->dataBlock)[downstream]++;



	//LOG(LOG_INFO,"Post Insert:");
	//dumpLeafProxy(leafProxy);

*/
//	LOG(LOG_INFO,"Entry Split");

	LOG(LOG_CRITICAL,"PackLeaf: walkerMergeRoutes_split TODO");
}


























void walkerAppendNewLeaf(RouteTableTreeWalker *walker, s16 upstream)
{
	LOG(LOG_CRITICAL,"PackLeaf: walkerAppendNewLeaf TODO");

	/*
	//LOG(LOG_INFO,"WalkerAppendLeaf - new leaf with");
	RouteTableTreeLeafProxy *leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->downstreamOffsetCount, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);

	treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, leafProxy, &walker->branchProxy, &walker->branchChildSibdex);
	leafProxy->dataBlock->upstream=upstream;
	leafProxy->dataBlock->upstreamOffset=0;

	walker->leafProxy=leafProxy;
	walker->leafEntry=leafProxy->entryCount;
	*/
}


void walkerAppendPreorderedEntry(RouteTableTreeWalker *walker, RouteTableEntry *entry, int routingTable)
{
	s32 upstream=routingTable==ROUTING_TABLE_FORWARD?entry->prefix:entry->suffix;
	s32 downstream=routingTable==ROUTING_TABLE_FORWARD?entry->suffix:entry->prefix;

	if(walker->leafProxy!=NULL) // Check for space
		{
		RouteTableUnpackedSingleBlock *block=walker->leafProxy->unpackedBlock;

		if(walker->leafArrayIndex<0)
			LOG(LOG_CRITICAL,"Invalid Array Index %i for append- count is %i",walker->leafArrayIndex,block->entryArrayCount);

		if(walker->leafArrayIndex!=(block->entryArrayCount-1))
			LOG(LOG_CRITICAL,"Invalid Array Index %i for append- count is %i",walker->leafArrayIndex,block->entryArrayCount);

		RouteTableUnpackedEntryArray *array=block->entryArrays[walker->leafArrayIndex];
		if(array==NULL)
			LOG(LOG_CRITICAL,"Invalid Null EntryArray at Index %i",walker->leafArrayIndex);

		if((array->upstream!=upstream || array->entryCount==ROUTEPACKING_ENTRYS_MAX) &&
				block->entryArrayCount>=(ROUTEPACKING_ENTRYARRAYS_MAX-1)) // Not suitable upstream / full array, and no more arrays
			walker->leafProxy=NULL;
		}

	if(walker->leafProxy==NULL)
		{
		if(walker->branchProxy==NULL)
			LOG(LOG_CRITICAL,"Not on a valid branch");

		walker->leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->treeProxy->upstreamCount, walker->treeProxy->downstreamCount);
		s16 childPositionPtr;
		treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, walker->leafProxy, &walker->branchProxy, &childPositionPtr);

		walker->leafArrayIndex=-1;
		walker->leafEntryIndex=-1;
		}

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;
	RouteTableUnpackedSingleBlock *block=leafProxy->unpackedBlock;

	RouteTableUnpackedEntryArray *array=NULL;

	if(walker->leafArrayIndex<0)
		{
		array=rtpInsertNewEntryArray(block, 0, upstream, ROUTEPACKING_ENTRYS_CHUNK);
		walker->leafArrayIndex=0;
		walker->leafEntryIndex=-1;
		}
	else
		{
		array=block->entryArrays[walker->leafArrayIndex];

		if(array->upstream!=upstream || array->entryCount==ROUTEPACKING_ENTRYS_MAX)
			{
			walker->leafArrayIndex++;
			array=rtpInsertNewEntryArray(block, walker->leafArrayIndex, upstream, ROUTEPACKING_ENTRYS_CHUNK);
			walker->leafEntryIndex=-1;
			}
		}

	walker->leafEntryIndex++;


	rtpInsertNewEntry(block, walker->leafArrayIndex, walker->leafEntryIndex, downstream, entry->width);

	block->upstreamOffsets[upstream]+=entry->width;
	block->downstreamOffsets[downstream]+=entry->width;

}



void walkerAppendPreorderedEntries(RouteTableTreeWalker *walker, RouteTableEntry *entries, u32 entryCount, int routingTable)
{
	walkerSeekEnd(walker);

	for(int i=0;i<entryCount;i++)
		walkerAppendPreorderedEntry(walker,entries+i, routingTable);

	//LOG(LOG_INFO,"Routing Table with %i entries used %i branches and %i leaves",
//		entryCount,walker->treeProxy->branchArrayProxy.newDataCount,walker->treeProxy->leafArrayProxy.newDataCount);


}


