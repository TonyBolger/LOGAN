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

void walkerAppendNewLeaf(RouteTableTreeWalker *walker)
{
	if(walker->branchProxy==NULL)
		LOG(LOG_CRITICAL,"Not on a valid branch");

	walker->leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->treeProxy->upstreamCount, walker->treeProxy->downstreamCount, ROUTEPACKING_ENTRYARRAYS_CHUNK);
	s16 childPositionPtr;
	treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, walker->leafProxy, &walker->branchProxy, &childPositionPtr);

	walker->leafArrayIndex=-1;
	walker->leafEntryIndex=-1;
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
		walkerAppendNewLeaf(walker);
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


static s32 ensureSpaceForArray(RouteTableTreeWalker *walker)
{
	RouteTableUnpackedSingleBlock *block=walker->leafProxy->unpackedBlock;

	if(block->entryArrayCount>=ROUTEPACKING_ENTRYARRAYS_MAX)
		{
		treeProxySplitLeaf(walker->treeProxy, walker->branchProxy, walker->branchChildSibdex, walker->leafProxy, walker->leafArrayIndex,
				&walker->branchProxy, &walker->branchChildSibdex, &walker->leafProxy, &walker->leafArrayIndex);

		return 1;
		}

	return 0;
}


static s32 ensureSpaceForEntries(RouteTableTreeWalker *walker, int entryCount)
{
	RouteTableUnpackedSingleBlock *block=walker->leafProxy->unpackedBlock;

	if(walker->leafArrayIndex<0 || walker->leafArrayIndex>block->entryArrayCount)
		LOG(LOG_CRITICAL,"Invalid array index %i",walker->leafArrayIndex);

	RouteTableUnpackedEntryArray *array=block->entryArrays[walker->leafArrayIndex];

	if((array->entryCount + entryCount) > ROUTEPACKING_ENTRYS_MAX)
		{
		ensureSpaceForArray(walker);
		rtpSplitArray(block, walker->leafArrayIndex, walker->leafEntryIndex, &(walker->leafArrayIndex), &(walker->leafEntryIndex));

		return 1;
		}

	return 0;
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

	LOG(LOG_INFO,"Check for empty leaf");
	if(leafProxy->unpackedBlock->entryArrayCount==0) // At empty leaf - may be more robust if after leaf selection
		{
		LOG(LOG_INFO,"Found empty leaf");

		walker->leafArrayIndex=0;
		walker->leafEntryIndex=-1;

		*entryPtr=NULL;
		*upstreamPtr=-1;

		*upstreamOffsetPtr=0;
		*downstreamOffsetPtr=walker->downstreamLeafOffsets[targetDownstream];

		LOG(LOG_INFO,"Done, at empty leaf");

		return 0;
		}

	LOG(LOG_INFO,"Non empty leaf");

	LOG(LOG_INFO,"Advancing to U %i D %i Offset (%i %i)",targetUpstream,targetDownstream,targetMinOffset,targetMaxOffset);
//	dumpLeafProxy(leafProxy);

	s32 arrayIndex=leafProxy->unpackedBlock->entryArrayCount-1;
	s32 upstream=(arrayIndex>=0)?leafProxy->unpackedBlock->entryArrays[arrayIndex]->upstream:-1;

	LOG(LOG_INFO,"About to move leaf (upstream) %i want %i",upstream,targetUpstream);

	while(upstream < targetUpstream) // One leaf at a time, for upstream
		{
		LOG(LOG_INFO,"Moving one leaf");

		walkerApplyLeafOffsets(walker, leafProxy);

		if(!getNextLeafSibling(walker->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy)) // No more leaves
			{
			LOG(LOG_INFO,"No more leaves");

			walker->leafProxy=NULL;
			walker->leafArrayIndex=-1;
			walker->leafEntryIndex=-1;

			*entryPtr=NULL;
			*upstreamPtr=-1;

			*upstreamOffsetPtr=0;
			*downstreamOffsetPtr=walker->downstreamLeafOffsets[targetDownstream];

			return 0;
			}

		walker->branchProxy=branchProxy;
		walker->branchChildSibdex=branchChildSibdex;
		walker->leafProxy=leafProxy;

//		dumpLeafProxy(leafProxy);

		arrayIndex=leafProxy->unpackedBlock->entryArrayCount-1;
		upstream=leafProxy->unpackedBlock->entryArrays[arrayIndex]->upstream;
		}


	walkerTransferOffsetsLeafToEntry(walker);

	if(leafProxy->unpackedBlock->entryArrays==NULL)
		LOG(LOG_CRITICAL,"Null leaf arrays");

	if(leafProxy->unpackedBlock->entryArrayCount==0)
		LOG(LOG_CRITICAL,"Empty leaf");

	arrayIndex=0;
	LOG(LOG_INFO,"Array Index %i",arrayIndex);

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


	while(upstream == targetUpstream && snoopUpstreamOffset<=targetMinOffset) // One entry at a time
		{
		currentUpstreamOffset=snoopUpstreamOffset;
		walker->downstreamLeafOffsets[downstream]+=array->entries[entryIndex].width;

		entryIndex++;

		if(entryIndex>=array->entryCount) // Ran out of entries
			{
			s32 snoopArrayIndex=arrayIndex+1; // Consider moving array
			if(snoopArrayIndex<block->entryArrayCount && block->entryArrays[snoopArrayIndex]->upstream==upstream)
				{
				LOG(LOG_INFO,"No more entries - next array. New is %i (offset %i)",snoopArrayIndex, currentUpstreamOffset);

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

		snoopUpstreamOffset=currentUpstreamOffset+array->entries[entryIndex].width;
		downstream=array->entries[entryIndex].downstream;
		}



	LOG(LOG_INFO,"About to move array (downstream) At %i Next %i Want %i",currentUpstreamOffset,snoopUpstreamOffset,targetMinOffset);

	//walker->upstreamOffsets[targetPrefix]+entry->width)<=maxEdgePosition && entry->downstream<targetSuffix

	while(upstream == targetUpstream && snoopUpstreamOffset<=targetMaxOffset && downstream < targetDownstream)
		{
		currentUpstreamOffset=snoopUpstreamOffset;
		walker->downstreamLeafOffsets[downstream]+=array->entries[entryIndex].width;

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

		snoopUpstreamOffset=currentUpstreamOffset+array->entries[entryIndex].width;
		downstream=array->entries[entryIndex].downstream;
		}

	LOG(LOG_INFO,"After move array Up %i (%i) Down %i (%i) Offset %i (%i %i) Width %i",
			upstream, targetUpstream, downstream, targetDownstream, currentUpstreamOffset, targetMinOffset, targetMaxOffset, array->entries[entryIndex].width);

	LOG(LOG_INFO,"Indexes are %i %i",arrayIndex,entryIndex);

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
		LOG(LOG_INFO,"Entry Insert: No current leaf - need new leaf append");

		leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->upstreamOffsetCount, walker->downstreamOffsetCount, ROUTEPACKING_ENTRYARRAYS_CHUNK);
		treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, leafProxy, &walker->branchProxy, &walker->branchChildSibdex);

		walker->leafProxy=leafProxy;

		block=leafProxy->unpackedBlock;
		rtpInsertNewEntryArray(block, 0, upstream, ROUTEPACKING_ENTRYS_CHUNK);

		walker->leafArrayIndex=0;
		walker->leafEntryIndex=0;

		LOG(LOG_INFO,"Entry Insert: Done new leaf append");
		}
	else
		{
		block=walker->leafProxy->unpackedBlock;


		if(walker->leafArrayIndex>=block->entryArrayCount)
			{
			if(ensureSpaceForArray(walker))
				block=walker->leafProxy->unpackedBlock;

			rtpInsertNewEntryArray(block, walker->leafArrayIndex, upstream, ROUTEPACKING_ENTRYS_CHUNK);
			walker->leafEntryIndex=0;

			LOG(LOG_INFO,"ArrayAppend: %i %i TODO",walker->leafArrayIndex,block->entryArrayCount);
			}
		else
			{
			RouteTableUnpackedEntryArray *array=block->entryArrays[walker->leafArrayIndex];
			s32 arrayUpstream=array->upstream;

			if(upstream<arrayUpstream) // Insert new array before specified, possibly including splitting leaf and branches
				{
				if(ensureSpaceForArray(walker))
					block=walker->leafProxy->unpackedBlock;

				array=rtpInsertNewEntryArray(block, walker->leafArrayIndex, upstream, ROUTEPACKING_ENTRYS_CHUNK);
				walker->leafEntryIndex=0;
				}
			else if(upstream>arrayUpstream) // Insert new array after specified (problem)
				{
				LOG(LOG_CRITICAL,"ArrayInsertAfter: How???");
				}
			else // Otherwise upstream matching, check for space
				{
				ensureSpaceForEntries(walker,1);
				}
			}
		}


	if(downstream<0)
		LOG(LOG_CRITICAL,"Insert invalid downstream %i",downstream);


	rtpInsertNewEntry(block, walker->leafArrayIndex, walker->leafEntryIndex, downstream, 1);

	block->upstreamOffsets[upstream]++;
	block->downstreamOffsets[downstream]++;

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

	ensureSpaceForEntries(walker,2);

	RouteTableUnpackedSingleBlock *block=leafProxy->unpackedBlock;
	RouteTableUnpackedEntryArray *array=block->entryArrays[walker->leafArrayIndex];

	s16 splitDownstream=array->entries[walker->leafEntryIndex].downstream;
	array->entries[walker->leafEntryIndex].width=width1;

	// Old entry is (splitDownstream,width1), new entry is (downstream,1) second is (splitDownstream, width2)
	rtpInsertNewDoubleEntry(block, walker->leafArrayIndex, walker->leafEntryIndex+1, downstream, 1, splitDownstream, width2);

	leafProxy->unpackedBlock->upstreamOffsets[array->upstream]++;
	leafProxy->unpackedBlock->downstreamOffsets[downstream]++;

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
		walkerAppendNewLeaf(walker);

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


