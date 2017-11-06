
#include "common.h"






void rttInitRouteTableTreeBuilder(RouteTableTreeBuilder *treeBuilder, RouteTableTreeTopBlock *top)
{
//	LOG(LOG_INFO,"Regular");

	/*
	for(int i=0;i<ROUTE_TOPINDEX_MAX;i++)
		{
		treeBuilder->dataBlocks[i].headerSize=0;
		treeBuilder->dataBlocks[i].dataSize=0;
		}
*/

	//LOG(LOG_INFO,"rttInitRouteTableTreeBuilder: Forward %p %p %p",top->data[ROUTE_TOPINDEX_FORWARD_LEAF],top->data[ROUTE_TOPINDEX_FORWARD_LEAF],top->data[ROUTE_TOPINDEX_FORWARD_OFFSET]);
	initTreeProxy(&(treeBuilder->forwardProxy),
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_FORWARD_LEAF_ARRAY_0]),top->data[ROUTE_TOPINDEX_FORWARD_LEAF_ARRAY_0],
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0]),top->data[ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0],
			treeBuilder->disp);

	//LOG(LOG_INFO,"rttInitRouteTableTreeBuilder: Reverse %p %p %p",top->data[ROUTE_TOPINDEX_REVERSE_LEAF],top->data[ROUTE_TOPINDEX_REVERSE_LEAF],top->data[ROUTE_TOPINDEX_REVERSE_OFFSET]);
	initTreeProxy(&(treeBuilder->reverseProxy),
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0]),top->data[ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0],
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0]),top->data[ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0],
			treeBuilder->disp);




	//LOG(LOG_INFO,"rttInitRouteTableTreeBuilder");
	rttwInitTreeWalker(&(treeBuilder->forwardWalker), &(treeBuilder->forwardProxy));
	//LOG(LOG_INFO,"rttInitRouteTableTreeBuilder");
	rttwInitTreeWalker(&(treeBuilder->reverseWalker), &(treeBuilder->reverseProxy));

//	LOG(LOG_INFO,"Forward: %i Leaves, %i Branches", treeBuilder->forwardProxy.leafArrayProxy.dataCount, treeBuilder->forwardProxy.branchArrayProxy.dataCount);
	//LOG(LOG_INFO,"Reverse: %i Leaves, %i Branches", treeBuilder->reverseProxy.leafArrayProxy.dataCount, treeBuilder->reverseProxy.branchArrayProxy.dataCount);



}

void rttUpgradeToRouteTableTreeBuilder(RouteTableArrayBuilder *arrayBuilder,  RouteTableTreeBuilder *treeBuilder, s32 sliceIndex, s32 prefixCount, s32 suffixCount, MemDispenser *disp)
{
//	LOG(LOG_INFO,"Upgrade to Tree");

	treeBuilder->disp=arrayBuilder->disp;

	for(int i=0;i<ROUTE_TOPINDEX_MAX;i++)
		rtInitHeapDataBlock(treeBuilder->dataBlocks+i, sliceIndex);

	initTreeProxy(&(treeBuilder->forwardProxy),
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_FORWARD_LEAF_ARRAY_0]),NULL,
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0]),NULL,
			disp);

	updateTreeProxyTailCounts(&(treeBuilder->forwardProxy),prefixCount, suffixCount);

	initTreeProxy(&(treeBuilder->reverseProxy),
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0]),NULL,
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0]),NULL,
			disp);

	updateTreeProxyTailCounts(&(treeBuilder->reverseProxy),suffixCount, prefixCount);

	rttwInitTreeWalker(&(treeBuilder->forwardWalker), &(treeBuilder->forwardProxy));
	rttwInitOffsetArrays(&(treeBuilder->forwardWalker), prefixCount, suffixCount);

	rttwInitTreeWalker(&(treeBuilder->reverseWalker), &(treeBuilder->reverseProxy));
	rttwInitOffsetArrays(&(treeBuilder->reverseWalker), suffixCount, prefixCount);

	//LOG(LOG_INFO,"Upgrading tree with %i forward entries and %i reverse entries to tree",arrayBuilder->oldForwardEntryCount, arrayBuilder->oldReverseEntryCount);

//	LOG(LOG_INFO,"Adding %i forward entries to tree",arrayBuilder->oldForwardEntryCount);
	rttwAppendPreorderedEntries(&(treeBuilder->forwardWalker), arrayBuilder->oldForwardEntries, arrayBuilder->oldForwardEntryCount, ROUTING_TABLE_FORWARD);

//	LOG(LOG_INFO,"Adding %i reverse entries to tree",arrayBuilder->oldReverseEntryCount);
	rttwAppendPreorderedEntries(&(treeBuilder->reverseWalker), arrayBuilder->oldReverseEntries, arrayBuilder->oldReverseEntryCount, ROUTING_TABLE_REVERSE);

//	LOG(LOG_INFO,"Upgrade completed F: %i R: %i",arrayBuilder->oldForwardEntryCount,arrayBuilder->oldReverseEntryCount);

	treeBuilder->newEntryCount=arrayBuilder->oldForwardEntryCount+arrayBuilder->oldReverseEntryCount;

	//rttDumpRoutingTable(treeBuilder);
}


void rttUpdateRouteTableTreeBuilderTailCounts(RouteTableTreeBuilder *builder, s32 prefixCount, s32 suffixCount)
{
	updateTreeProxyTailCounts(&(builder->forwardProxy), prefixCount, suffixCount);
	updateTreeProxyTailCounts(&(builder->reverseProxy), suffixCount, prefixCount);
}


int dumpRoutingTableTree_ArrayProxy(RouteTableTreeArrayProxy *arrayProxy, char *name)
{
	if(arrayProxy==NULL)
	{
		LOG(LOG_INFO,"Array %s is NULL",name);
		return 0;
	}

	int count=0;

	if(arrayProxy->ptrBlock!=NULL)
		{
		LOG(LOG_CRITICAL,"Array is indirect - TODO");
		}

	if(arrayProxy->dataBlock!=NULL)
		{
		LOG(LOG_INFO,"Array %s existing datablock %p contains %i of %i items",name,arrayProxy->dataBlock,arrayProxy->oldDataCount,arrayProxy->oldDataAlloc);
		count=MAX(count, arrayProxy->oldDataCount);
		}
	else
		LOG(LOG_INFO,"Array %s existing datablock is NULL",name);

	if(arrayProxy->newEntries!=NULL)
		{
		LOG(LOG_INFO,"Array %s new datablock %p contains %i of %i items",name,arrayProxy->newEntries,arrayProxy->newDataCount,arrayProxy->newDataAlloc);
		count=MAX(count, arrayProxy->newDataCount);
		}
	else
		LOG(LOG_INFO,"Array %s new datablock is NULL",name);

	return count;

}

void dumpRoutingTableTree(RouteTableTreeProxy *treeProxy)
{
	RouteTableTreeArrayProxy *branchArrayProxy=&(treeProxy->branchArrayProxy);
	int branchCount=dumpRoutingTableTree_ArrayProxy(branchArrayProxy, "BranchArray: ");

	RouteTableTreeArrayProxy *leafArrayProxy=&(treeProxy->leafArrayProxy);
	int leafCount=dumpRoutingTableTree_ArrayProxy(leafArrayProxy, "LeafArray: ");

	for(int i=0;i<branchCount;i++)
		{
		RouteTableTreeBranchBlock *branchBlock=getRouteTableTreeBranchBlock(treeProxy, i);

		dumpBranchBlock(branchBlock);
		}


	for(int i=0;i<leafCount;i++)
		{
		RouteTableTreeLeafBlock *leafBlock=getRouteTableTreeLeafBlock(treeProxy, i);

		dumpLeafBlock(leafBlock);
		}

}

void rttDumpRoutingTable(RouteTableTreeBuilder *builder)
{
	LOG(LOG_INFO,"***********************************************");
	LOG(LOG_INFO,"Dumping Forward Tree");
	LOG(LOG_INFO,"***********************************************");
	dumpRoutingTableTree(&(builder->forwardProxy));
	LOG(LOG_INFO,"***********************************************");

	LOG(LOG_INFO,"***********************************************");
	LOG(LOG_INFO,"Dumping Reverse Tree");
	LOG(LOG_INFO,"***********************************************");
	dumpRoutingTableTree(&(builder->reverseProxy));
	LOG(LOG_INFO,"***********************************************");



}

/*s32 rttGetRouteTableTreeBuilderDirty(RouteTableTreeBuilder *builder)
{
	return builder->newEntryCount>0;
}
*/




s32 rttGetTopArraySize(RouteTableTreeArrayProxy *arrayProxy)
{
	int entryCount=rttGetArrayEntries(arrayProxy);

	if(entryCount>ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)
		{
		entryCount=rttCalcFirstLevelArrayEntries(entryCount);
		}

	return rttCalcArraySize(entryCount);
}



RouteTableTreeArrayProxy *rttGetTopArrayByIndex(RouteTableTreeBuilder *builder, s32 topIndex)
{
	switch(topIndex)
	{
	case ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0:
		return &(builder->forwardProxy.branchArrayProxy);

	case ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0:
		return &(builder->reverseProxy.branchArrayProxy);

	case ROUTE_TOPINDEX_FORWARD_LEAF_ARRAY_0:
		return &(builder->forwardProxy.leafArrayProxy);

	case ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0:
		return &(builder->reverseProxy.leafArrayProxy);


//	case ROUTE_TOPINDEX_FORWARD_OFFSET:
//		return &(builder->forwardProxy.offsetArrayProxy);

//	case ROUTE_TOPINDEX_REVERSE_OFFSET:
//		return &(builder->reverseProxy.offsetArrayProxy);

	}

	LOG(LOG_CRITICAL,"rttGetTopArrayByIndex for invalid index %i",topIndex);

	return 0;
}


static void appendStarterLeaf(RouteTableTreeEntryBuffer *buf)
{
	if(buf->oldBranchProxy==NULL)
		LOG(LOG_CRITICAL,"Not on a valid branch");

	buf->oldLeafProxy=allocRouteTableTreeLeafProxy(buf->treeProxy, buf->upstreamOffsetCount, buf->downstreamOffsetCount, ROUTEPACKING_ENTRYARRAYS_CHUNK);
	treeProxyAppendLeafChild(buf->treeProxy, buf->oldBranchProxy, buf->oldLeafProxy, &buf->oldBranchProxy, &buf->oldBranchChildSibdex);
}

/*
static void initOldLeafWalker(RouteTableTreeLeafWalker *walker, RouteTableTreeProxy *treeProxy, int upstreamOffsetCount, int downstreamOffsetCount)
{
	walker->treeProxy=treeProxy;

	walker->upstreamOffsetCount=upstreamOffsetCount;
	walker->downstreamOffsetCount=downstreamOffsetCount;

	walker->upstreamOffsets=dAlloc(walker->treeProxy->disp, sizeof(s32)*upstreamOffsetCount);
	walker->downstreamOffsets=dAlloc(walker->treeProxy->disp, sizeof(s32)*downstreamOffsetCount);

	memset(walker->upstreamOffsets,0,sizeof(s32)*upstreamOffsetCount);
	memset(walker->downstreamOffsets,0,sizeof(s32)*downstreamOffsetCount);

	treeProxySeekStart(walker->treeProxy, &walker->branchProxy, &walker->branchChildSibdex, &walker->leafProxy);

	if(walker->leafProxy==NULL)
		appendNewLeaf(walker, upstreamOffsetCount, downstreamOffsetCount);
}

static void initOldEntryBuffer(RouteTableLeafEntryBuffer *oldEntryBuf, RouteTableTreeProxy *treeProxy, int upstreamOffsetCount, int downstreamOffsetCount)
{
	initOldLeafWalker(&(oldEntryBuf->walker), treeProxy, upstreamOffsetCount, downstreamOffsetCount);

	oldEntryBuf->entryArraysPtr=NULL;
	oldEntryBuf->entryArraysPtrEnd=NULL;
	oldEntryBuf->entryPtr=NULL;
	oldEntryBuf->entryPtrEnd=NULL;

	oldEntryBuf->upstream=-1;
}
*/
/*
static int snoopOldEntryBufferLastUpstream(RouteTableLeafEntryBuffer *entryBuf)
{
	LOG(LOG_CRITICAL,"TODO");
	return 0;
}
*/
/*
static void initNewLeafWalker(RouteTableTreeLeafWalker *walker, RouteTableTreeLeafWalker *oldWalker)
{
	walker->treeProxy=oldWalker->treeProxy;

	int upstreamOffsetCount=oldWalker->upstreamOffsetCount;
	int downstreamOffsetCount=oldWalker->downstreamOffsetCount;

	walker->upstreamOffsetCount=upstreamOffsetCount;
	walker->downstreamOffsetCount=downstreamOffsetCount;

	walker->upstreamOffsets=dAlloc(walker->treeProxy->disp, sizeof(s32)*upstreamOffsetCount);
	walker->downstreamOffsets=dAlloc(walker->treeProxy->disp, sizeof(s32)*downstreamOffsetCount);

	memset(walker->upstreamOffsets,0,sizeof(s32)*upstreamOffsetCount);
	memset(walker->downstreamOffsets,0,sizeof(s32)*downstreamOffsetCount);

	oldWalker->branchProxy=

	treeProxySeekStart(walker->treeProxy, &walker->branchProxy, &walker->branchChildSibdex, &walker->leafProxy);

	if(walker->leafProxy==NULL)
		appendNewLeaf(walker, upstreamOffsetCount, downstreamOffsetCount);
}


static void initNewEntryBuffer(RouteTableLeafEntryBuffer *newEntryBuf, RouteTableLeafEntryBuffer *oldEntryBuf)
{
	RouteTableTreeLeafWalker *newWalker=&(newEntryBuf->walker);
	RouteTableTreeLeafWalker *oldWalker=&(oldEntryBuf->walker);

	newWalker->


	newEntryBuf->entryArraysPtr=NULL;
	newEntryBuf->entryArraysPtrEnd=NULL;
	newEntryBuf->entryPtr=NULL;
	newEntryBuf->entryPtrEnd=NULL;

	newEntryBuf->upstream=-1;
}
*/


static void initTreeEntryBuffer(RouteTableTreeEntryBuffer *buf, RouteTableTreeProxy *treeProxy, int upstreamOffsetCount, int downstreamOffsetCount)
{
	buf->treeProxy=treeProxy;
	buf->upstreamOffsetCount=upstreamOffsetCount;
	buf->downstreamOffsetCount=downstreamOffsetCount;

	treeProxySeekStart(buf->treeProxy, &buf->oldBranchProxy, &buf->oldBranchChildSibdex, &buf->oldLeafProxy);

	if(buf->oldLeafProxy==NULL)
		appendStarterLeaf(buf);

	buf->oldEntryArraysPtr=NULL;
	buf->oldEntryArraysPtrEnd=NULL;
	buf->oldEntryPtr=NULL;
	buf->oldEntryPtrEnd=NULL;

	buf->oldUpstream=-1;
	buf->oldDownstream=-1;
	buf->oldWidth=0;

	buf->accumUpstreamOffsets=dAlloc(treeProxy->disp, sizeof(s32)*upstreamOffsetCount);
	buf->accumDownstreamOffsets=dAlloc(treeProxy->disp, sizeof(s32)*downstreamOffsetCount);
	memset(buf->accumUpstreamOffsets,0,sizeof(s32)*upstreamOffsetCount);
	memset(buf->accumDownstreamOffsets,0,sizeof(s32)*downstreamOffsetCount);

	buf->newBranchProxy=buf->oldBranchProxy;
	buf->newLeafProxy=buf->oldLeafProxy;
	buf->newBranchChildSibdex=buf->oldBranchChildSibdex;

	buf->newEntryArraysPtr=NULL;
	buf->newEntryArraysPtrEnd=NULL;
	buf->newEntryPtr=NULL;
	buf->newEntryPtrEnd=NULL;

	buf->newUpstream=-1;
	buf->newDownstream=-1;
	buf->newWidth=0;

	buf->newLeafUpstreamOffsets=dAlloc(treeProxy->disp, sizeof(s32)*upstreamOffsetCount);
	buf->newLeafDownstreamOffsets=dAlloc(treeProxy->disp, sizeof(s32)*downstreamOffsetCount);
	memset(buf->newLeafUpstreamOffsets,0,sizeof(s32)*upstreamOffsetCount);
	memset(buf->newLeafDownstreamOffsets,0,sizeof(s32)*downstreamOffsetCount);

	buf->maxWidth=0;
}


static void accumulateOldLeafOffsets(RouteTableTreeEntryBuffer *buf)
{
	RouteTableUnpackedSingleBlock *block=buf->oldLeafProxy->unpackedBlock;

	int upstreamValidOffsets=MIN(buf->upstreamOffsetCount, block->upstreamOffsetAlloc);

	for(int i=0;i<upstreamValidOffsets;i++)
		buf->accumUpstreamOffsets[i]+=block->upstreamLeafOffsets[i];

	int downstreamValidOffsets=MIN(buf->downstreamOffsetCount, block->downstreamOffsetAlloc);

	for(int i=0;i<downstreamValidOffsets;i++)
		buf->accumDownstreamOffsets[i]+=block->downstreamLeafOffsets[i];
}



static s32 advanceToNextLeaf_Old(RouteTableTreeEntryBuffer *buf)
{
	if(buf->oldLeafProxy==NULL)
		return 0;

	if(buf->oldEntryArraysPtr==NULL) // Not current bound
		{
		accumulateOldLeafOffsets(buf);

		if(!getNextLeafSibling(buf->treeProxy, &(buf->oldBranchProxy), &(buf->oldBranchChildSibdex), &(buf->oldLeafProxy))) // No more leaves
			{
//			LOG(LOG_INFO,"No more leaves");
			return 0;
			}

		return 1;
		}

	LOG(LOG_CRITICAL,"TODO: advance over bound leaf");

	return 0;
}


/*
static void bindOldEntries(RouteTableTreeEntryBuffer *buf)
{
	rttlEnsureFullyUnpacked(buf->treeProxy, buf->oldLeafProxy);

	RouteTableUnpackedSingleBlock *block=buf->oldLeafProxy->unpackedBlock;

	buf->oldEntryArraysPtr=block->entryArrays;
	buf->oldEntryArraysPtrEnd=block->entryArrays+block->entryArrayCount;

	if(block->entryArrayCount>0)
		{
		RouteTableUnpackedEntryArray *array=buf->oldEntryArraysPtr[0];

		buf->oldEntryPtr=array->entries;
		buf->oldEntryPtrEnd=array->entries+array->entryCount;
		}
	else
		{
		buf->oldEntryPtr=buf->oldEntryPtrEnd=NULL;
		}
}
*/

static s32 snoopOldLeafLastUpstream(RouteTableTreeEntryBuffer *buf)
{
	RouteTableUnpackedSingleBlock *block=buf->oldLeafProxy->unpackedBlock;

	for(int i=block->upstreamOffsetAlloc;i>0;--i)
		{
		if(block->upstreamLeafOffsets[i]>0)
			return i;
		}
	return -1;
}


static int advanceOldLeafToUpstream(RouteTableTreeEntryBuffer *buf, int upstream, int minEdgePosition)
{
	while(buf->oldLeafProxy!=NULL)
		{
		int snoopUpstream=snoopOldLeafLastUpstream(buf);

		if(snoopUpstream>upstream)
			return 1;

		if(snoopUpstream==upstream)
			{
			RouteTableUnpackedSingleBlock *block=buf->oldLeafProxy->unpackedBlock;
			int snoopOffset=buf->accumUpstreamOffsets[upstream]+block->upstreamLeafOffsets[upstream];

			if(snoopOffset>=minEdgePosition)
				return 1;
			}

		advanceToNextLeaf_Old(buf);
		}

	return 0;
}




/*
static void bindOldEntryBufferForReading(RouteTableLeafEntryBuffer *oldEntryBuf)
{
	RouteTableTreeLeafProxy *leafProxy=oldEntryBuf->walker->leafProxy;

	rttlEnsureFullyUnpacked(leafProxy, oldEntryBuf->walker->treeProxy);

	RouteTableUnpackedSingleBlock unpackedBlock=leafProxy->unpackedBlock;

	oldEntryBuf->entryArraysPtr=unpackedBlock->entryArrays;
	oldEntryBuf->entryArraysPtrEnd=unpackedBlock->entryArrays+unpackedBlock->entryArrayCount;

	if(unpackedBlock->entryArrayCount>0)
		{
		RouteTableUnpackedEntryArray *array=unpackedBlock->entryArrays[0];

		oldEntryBuf->entryPtr=array->entries;
		oldEntryBuf->entryPtrEnd=array->entries+array->entryCount;
		oldEntryBuf->upstream=array->upstream;
		}
	else
		{
		oldEntryBuf->entryPtr=NULL;
		oldEntryBuf->entryPtrEnd=NULL;
		oldEntryBuf->upstream=-1;
		}
}
*/

/*

static s32 treeBufferPollInput(RouteTableTreeBuffer *buf)
{
	RouteTableEntry *oldEntryPtr=buf->oldEntryPtr;



	if(oldEntryPtr<buf->oldEntryPtrEnd)
		{
		buf->oldPrefix=oldEntryPtr->prefix;
		buf->oldSuffix=oldEntryPtr->suffix;
		buf->oldWidth=oldEntryPtr->width;

		buf->oldEntryPtr++;


		return buf->oldWidth;
		}
	else
		{
		buf->oldPrefix=0;
		buf->oldSuffix=0;
		buf->oldWidth=0;

		return 0;
		}

}
*/












/* No Buffer version

static void rttMergeRoutes_ordered_forwardSingle(RouteTableTreeBuilder *builder, RouteTableTreeWalker *walker, RoutePatch *patch)
{
	s32 targetPrefix=patch->prefixIndex;
	s32 targetSuffix=patch->suffixIndex;
	s32 minEdgePosition=(*(patch->rdiPtr))->minEdgePosition;
	s32 maxEdgePosition=(*(patch->rdiPtr))->maxEdgePosition;

	s32 upstream=-1;
	RouteTableUnpackedEntry *entry=NULL;

//	LOG(LOG_INFO,"Forward Route Patch: %i %i (%i %i)",targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

	s32 upstreamEdgeOffset=-1;
	s32 downstreamEdgeOffset=-1;

	int res=rttwAdvanceToUpstreamThenOffsetThenDownstream(walker, targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition,
			&upstream, &entry, &upstreamEdgeOffset, &downstreamEdgeOffset);


	//LOG(LOG_CRITICAL,"PackLeaf: rttMergeRoutes_ordered_forwardSingle (leaf skip) TODO: Res %i Upstream %i Entry %p",res,upstream,entry);

	if(!res || upstream > targetPrefix || entry == NULL || (entry->downstream!=targetSuffix && upstreamEdgeOffset>=minEdgePosition))
		{
		int minMargin=upstreamEdgeOffset-minEdgePosition; // Margin between current position and minimum position: Must be zero or positive
		int maxMargin=maxEdgePosition-upstreamEdgeOffset; // Margin between current position and maximum position: Must be zero or positive

		if(minMargin<0 || maxMargin<0)
			{
			LOG(LOG_INFO,"Failed to add forward route for prefix %i suffix %i",targetPrefix,targetSuffix);
			LOG(LOG_INFO,"Current edge offset %i minEdgePosition %i maxEdgePosition %i",upstreamEdgeOffset,minEdgePosition,maxEdgePosition);
			LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets to new entry
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		rttwMergeRoutes_insertEntry(walker, targetPrefix, targetSuffix); // targetPrefix,targetSuffix,1
		}
	else if(upstream==targetPrefix && entry->downstream==targetSuffix)
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

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

		// Map offsets to new entry
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset+minOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset+maxOffset;

		rttwMergeRoutes_widen(walker); // width ++
		}
	else
		{
		int targetEdgePosition=entry->downstream>targetSuffix?minEdgePosition:maxEdgePosition; // Early or late split

		int splitWidth1=targetEdgePosition-upstreamEdgeOffset;
		int splitWidth2=entry->width-splitWidth1;

		if(splitWidth1<=0 || splitWidth2<=0)
			{
			LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i from %i",splitWidth1, splitWidth2, entry->width);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		rttwMergeRoutes_split(walker, targetSuffix, splitWidth1, splitWidth2); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2
		}

}
*/




static void rttMergeRoutes_ordered_forwardMulti(RouteTableTreeBuilder *builder, RoutePatch *patchPtr, RoutePatch *patchPtrEnd,
		s32 prefixCount, s32 suffixCount)
{
	RouteTableTreeEntryBuffer buf;
	initTreeEntryBuffer(&buf, &(builder->forwardProxy), prefixCount, suffixCount);

	while(patchPtr<patchPtrEnd)
	{
		s32 targetPrefix=patchPtr->prefixIndex;
		s32 targetSuffix=patchPtr->suffixIndex;
		s32 minEdgePosition=(*(patchPtr->rdiPtr))->minEdgePosition;
		s32 maxEdgePosition=(*(patchPtr->rdiPtr))->maxEdgePosition;

		int expectedMaxEdgePosition=maxEdgePosition+1;
		RoutePatch *patchGroupPtr=patchPtr+1;  // Make groups of compatible inserts for combined processing

		if(minEdgePosition!=TR_INIT_MINEDGEPOSITION || maxEdgePosition!=TR_INIT_MAXEDGEPOSITION)
			{
			while(patchGroupPtr<patchPtrEnd && patchGroupPtr->prefixIndex==targetPrefix && patchGroupPtr->suffixIndex==targetSuffix &&
					(*(patchGroupPtr->rdiPtr))->minEdgePosition == minEdgePosition && (*(patchGroupPtr->rdiPtr))->maxEdgePosition == expectedMaxEdgePosition)
				{
				patchGroupPtr++;
				expectedMaxEdgePosition++;
				}
			}
		else
			{
			while(patchGroupPtr<patchPtrEnd && patchGroupPtr->prefixIndex==targetPrefix && patchGroupPtr->suffixIndex==targetSuffix &&
					(*(patchGroupPtr->rdiPtr))->minEdgePosition == TR_INIT_MINEDGEPOSITION && (*(patchGroupPtr->rdiPtr))->maxEdgePosition == TR_INIT_MAXEDGEPOSITION)
				patchGroupPtr++;
			}

		advanceOldLeafToUpstream(&buf, targetPrefix, minEdgePosition);





		LOG(LOG_CRITICAL,"TODO");



	}


//	LOG(LOG_INFO,"Forward Route Patch: %i %i (%i %i)",targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

/*

	while(res && upstream < targetPrefix) 												// Skip lower upstream (entry at a time)

	while(res && upstream == targetPrefix &&										// Skip earlier upstream (leaf at a time)
		((walker->upstreamOffsets[targetPrefix]+walker->leafProxy->dataBlock->upstreamOffset) < minEdgePosition))

	while(res && upstream == targetPrefix &&
			((walker->upstreamOffsets[targetPrefix]+entry->width)<minEdgePosition ||
			((walker->upstreamOffsets[targetPrefix]+entry->width)==minEdgePosition && entry->downstream!=targetSuffix))) // Skip earlier? upstream

	while(res && upstream == targetPrefix &&
			(walker->upstreamOffsets[targetPrefix]+entry->width)<=maxEdgePosition && entry->downstream<targetSuffix) // Skip matching upstream with earlier downstream

//	LOG(LOG_INFO,"FwdDone: %i %i %i %i",walker->branchProxy->brindex,walker->branchChildSibdex,walker->leafEntry, res);

//	LOG(LOG_INFO,"OffsetsNow U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);

*/

	/*
	s32 upstreamEdgeOffset=-1;
	s32 downstreamEdgeOffset=-1;

	int res=rttwAdvanceToUpstreamThenOffsetThenDownstream(walker, targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition,
			&upstream, &entry, &upstreamEdgeOffset, &downstreamEdgeOffset);


	//LOG(LOG_CRITICAL,"PackLeaf: rttMergeRoutes_ordered_forwardSingle (leaf skip) TODO: Res %i Upstream %i Entry %p",res,upstream,entry);

	if(!res || upstream > targetPrefix || entry == NULL || (entry->downstream!=targetSuffix && upstreamEdgeOffset>=minEdgePosition))
		{
		int minMargin=upstreamEdgeOffset-minEdgePosition; // Margin between current position and minimum position: Must be zero or positive
		int maxMargin=maxEdgePosition-upstreamEdgeOffset; // Margin between current position and maximum position: Must be zero or positive

		if(minMargin<0 || maxMargin<0)
			{
			LOG(LOG_INFO,"Failed to add forward route for prefix %i suffix %i",targetPrefix,targetSuffix);
			LOG(LOG_INFO,"Current edge offset %i minEdgePosition %i maxEdgePosition %i",upstreamEdgeOffset,minEdgePosition,maxEdgePosition);
			LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets to new entry
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		rttwMergeRoutes_insertEntry(walker, targetPrefix, targetSuffix); // targetPrefix,targetSuffix,1
		}
	else if(upstream==targetPrefix && entry->downstream==targetSuffix)
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

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

		// Map offsets to new entry
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset+minOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset+maxOffset;

		rttwMergeRoutes_widen(walker); // width ++
		}
	else
		{
		int targetEdgePosition=entry->downstream>targetSuffix?minEdgePosition:maxEdgePosition; // Early or late split

		int splitWidth1=targetEdgePosition-upstreamEdgeOffset;
		int splitWidth2=entry->width-splitWidth1;

		if(splitWidth1<=0 || splitWidth2<=0)
			{
			LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i from %i",splitWidth1, splitWidth2, entry->width);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		rttwMergeRoutes_split(walker, targetSuffix, splitWidth1, splitWidth2); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2
		}
*/
}


static void rttMergeRoutes_ordered_reverseSingle(RouteTableTreeWalker *walker, RoutePatch *patch)
{
	s32 targetPrefix=patch->prefixIndex;
	s32 targetSuffix=patch->suffixIndex;
	s32 minEdgePosition=(*(patch->rdiPtr))->minEdgePosition;
	s32 maxEdgePosition=(*(patch->rdiPtr))->maxEdgePosition;

	s32 upstream=-1;
	RouteTableUnpackedEntry *entry=NULL;

//	LOG(LOG_INFO,"Reverse Route Patch: %i %i (%i %i)",targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

	s32 upstreamEdgeOffset=-1;
	s32 downstreamEdgeOffset=-1;

	int res=rttwAdvanceToUpstreamThenOffsetThenDownstream(walker, targetSuffix, targetPrefix, minEdgePosition, maxEdgePosition,
			&upstream, &entry, &upstreamEdgeOffset, &downstreamEdgeOffset);


	if(!res || upstream > targetSuffix || entry == NULL || (entry->downstream!=targetPrefix && upstreamEdgeOffset>=minEdgePosition)) // No suitable existing entry, but can insert/append here
	{
		int minMargin=upstreamEdgeOffset-minEdgePosition; // Margin between current position and minimum position: Must be zero or positive
		int maxMargin=maxEdgePosition-upstreamEdgeOffset; // Margin between current position and maximum position: Must be zero or positive

		if(minMargin<0 || maxMargin<0)
			{
			LOG(LOG_INFO,"Failed to add reverse route for prefix %i suffix %i",targetPrefix,targetSuffix);
			LOG(LOG_INFO,"Current edge offset %i minEdgePosition %i maxEdgePosition %i",upstreamEdgeOffset,minEdgePosition,maxEdgePosition);
			LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets to new entry
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		rttwMergeRoutes_insertEntry(walker, targetSuffix, targetPrefix);
	}

	else if(upstream==targetSuffix && entry->downstream==targetPrefix) // Existing entry suitable, widen
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

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

		// Map offsets to new entry
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset+minOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset+maxOffset;

		rttwMergeRoutes_widen(walker); // width ++
		}
	else // Existing entry unsuitable, split and insert
		{
		int targetEdgePosition=entry->downstream>targetPrefix?minEdgePosition:maxEdgePosition; // Early or late split

		int splitWidth1=targetEdgePosition-upstreamEdgeOffset;
		int splitWidth2=entry->width-splitWidth1;

		if(splitWidth1<=0 || splitWidth2<=0)
			{
			LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i from %i",splitWidth1, splitWidth2, entry->width);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		rttwMergeRoutes_split(walker, targetPrefix, splitWidth1, splitWidth2); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2

		}


}




void rttMergeRoutes(RouteTableTreeBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 prefixCount, s32 suffixCount, RoutingReadData **orderedDispatches, MemDispenser *disp)
{
	rttUpdateRouteTableTreeBuilderTailCounts(builder, prefixCount, suffixCount);

	// Forward Routes

	//LOG(LOG_INFO,"rttMergeRoutes: %i %i",forwardRoutePatchCount, reverseRoutePatchCount);

	if(forwardRoutePatchCount>0)
		{
/*
		RouteTableTreeWalker *walker=&(builder->forwardWalker);

		rttwInitOffsetArrays(walker, prefixCount, suffixCount);

		RoutePatch *patchPtr=forwardRoutePatches;
		RoutePatch *patchEnd=patchPtr+forwardRoutePatchCount;

		while(patchPtr<patchEnd)
			{
			rttwSeekStart(walker);
			rttMergeRoutes_ordered_forwardSingle(builder, walker, patchPtr);
			rttwResetOffsetArrays(walker);

			*(orderedDispatches++)=*(patchPtr->rdiPtr);
			patchPtr++;
			}
			*/

		RoutePatch *patchPtr=forwardRoutePatches;

		rttMergeRoutes_ordered_forwardMulti(builder, patchPtr, patchPtr+forwardRoutePatchCount, prefixCount, suffixCount);

		for(int i=0;i<forwardRoutePatchCount;i++)
			{
			*(orderedDispatches++)=*(patchPtr->rdiPtr);
			patchPtr++;
			}
		}

	// Reverse Routes

	if(reverseRoutePatchCount>0)
		{
		RouteTableTreeWalker *walker=&(builder->reverseWalker);

		rttwInitOffsetArrays(walker, suffixCount, prefixCount);

		RoutePatch *patchPtr=reverseRoutePatches;
		RoutePatch *patchEnd=patchPtr+reverseRoutePatchCount;

		while(patchPtr<patchEnd)
			{
			rttwSeekStart(walker);
			rttMergeRoutes_ordered_reverseSingle(walker, patchPtr);
			rttwResetOffsetArrays(walker);

			*(orderedDispatches++)=*(patchPtr->rdiPtr);
			patchPtr++;
			}

/*		RoutePatch *patchPeekPtr=NULL;

		while(patchPtr<patchEnd)
			{
			int targetUpstream=patchPtr->suffixIndex;
			patchPeekPtr=patchPtr+1;

			if(patchPeekPtr<patchEnd && patchPeekPtr->suffixIndex==targetUpstream)
				{
				while(patchPtr<patchEnd && patchPtr->suffixIndex==targetUpstream)
					{
					walkerSeekStart(walker);
					rttMergeRoutes_ordered_reverseSingle(walker, patchPtr);
					walkerResetOffsetArrays(walker);

					*(orderedDispatches++)=*(patchPtr->rdiPtr);
					patchPtr++;
					}
				}
			else
				{
				walkerSeekStart(walker);
				rttMergeRoutes_ordered_reverseSingle(walker, patchPtr);
				walkerResetOffsetArrays(walker);

				*(orderedDispatches++)=*(patchPtr->rdiPtr);
				patchPtr++;
				}
			}
*/


		}


}


void rttUnpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp)
{
	//u32 prefixBits=0, suffixBits=0, widthBits=0, forwardEntryCount=0, reverseEntryCount=0;

	if(data!=NULL)
		{
		LOG(LOG_CRITICAL,"Not implemented: rttUnpackRouteTableForSmerLinked");
		/*
		data+=decodeHeader(data, &prefixBits, &suffixBits, &widthBits, &forwardEntryCount, &reverseEntryCount);

		smerLinked->forwardRouteEntries=dAlloc(disp, sizeof(RouteTableEntry)*forwardEntryCount);
		smerLinked->reverseRouteEntries=dAlloc(disp, sizeof(RouteTableEntry)*reverseEntryCount);

		smerLinked->forwardRouteCount=forwardEntryCount;
		smerLinked->reverseRouteCount=reverseEntryCount;

		rtaUnpackRoutes(data, prefixBits, suffixBits, widthBits,
				smerLinked->forwardRouteEntries, smerLinked->reverseRouteEntries, forwardEntryCount, reverseEntryCount,
				NULL,NULL,NULL);
				*/
		}
	else
		{
		smerLinked->forwardRouteEntries=NULL;
		smerLinked->reverseRouteEntries=NULL;

		smerLinked->forwardRouteCount=0;
		smerLinked->reverseRouteCount=0;
		}
}



void rttGetStats(RouteTableTreeBuilder *builder,
		s64 *routeTableForwardRouteEntriesPtr, s64 *routeTableForwardRoutesPtr, s64 *routeTableReverseRouteEntriesPtr, s64 *routeTableReverseRoutesPtr,
		s64 *routeTableTreeTopBytesPtr, s64 *routeTableTreeArrayBytesPtr,
		s64 *routeTableTreeLeafBytes, s64 *routeTableTreeLeafOffsetBytes, s64 *routeTableTreeLeafEntryBytes,
		s64 *routeTableTreeBranchBytes, s64 *routeTableTreeBranchOffsetBytes, s64 *routeTableTreeBranchChildBytes)
{
	s64 routeEntries[]={0,0};
	s64 routes[]={0,0};

	s64 arrayBytes=0;
	s64 leafBytes=0;
	s64 leafOffsetBytes=0;
	s64 leafEntryBytes=0;
	s64 branchBytes=0;
	s64 branchOffsetBytes=0;
	s64 branchChildBytes=0;

	RouteTableTreeProxy *treeProxies[2];

	treeProxies[0]=&(builder->forwardProxy);
	treeProxies[1]=&(builder->reverseProxy);

	for(int p=0;p<2;p++)
		{
		// Process leaves: Assume Direct

		int arrayAlloc=treeProxies[p]->leafArrayProxy.oldDataAlloc;
		arrayBytes+=sizeof(RouteTableTreeArrayBlock)+arrayAlloc*sizeof(u8 *);

		int leafCount=treeProxies[p]->leafArrayProxy.oldDataCount;
		for(int i=0;i<leafCount;i++)
			{
			LOG(LOG_CRITICAL,"PackLeaf: rttGetStats TODO");

			/*
			RouteTableTreeLeafProxy *leafProxy=getRouteTableTreeLeafProxy(treeProxies[p], i);


			int dataSize=leafProxy->dataBlock->dataSize;
			int offsetSize=(leafProxy->upstreamAlloc+leafProxy->downstreamAlloc)*sizeof(s32);
			int entrySize=dataSize-offsetSize;

			leafBytes+=leafProxy->dataBlock->dataSize;

			//leafOffsetBytes+=((s32)leafProxy->dataBlock->offsetAlloc)*sizeof(RouteTableTreeLeafOffset);
			//leafEntryBytes+=((s32)leafProxy->dataBlock->entryAlloc)*sizeof(RouteTableTreeLeafEntry);

			s32 routesTmp=0;
			int leafElements=leafProxy->entryCount;


			RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(leafProxy->dataBlock);

			for(int j=0;j<leafElements;j++)
				routesTmp+=entryPtr[j].width;

			routeEntries[p]+=leafElements;
			routes[p]+=routesTmp;
			*/
			}

		// Process branches: Assume Direct
		arrayAlloc=treeProxies[p]->branchArrayProxy.oldDataAlloc;
		arrayBytes+=sizeof(RouteTableTreeArrayBlock)+arrayAlloc*sizeof(u8 *);

		int branchCount=treeProxies[p]->branchArrayProxy.oldDataCount;
		for(int i=0;i<branchCount;i++)
			{
			RouteTableTreeBranchProxy *branchProxy=getRouteTableTreeBranchProxy(treeProxies[p], i);

			branchBytes+=getRouteTableTreeBranchSize_Existing(branchProxy->dataBlock);
			//branchOffsetBytes+=;
			branchChildBytes+=((s32)branchProxy->dataBlock->childAlloc)*sizeof(RouteTableTreeBranchChild);
			}

		}

	if(routeTableForwardRouteEntriesPtr!=NULL)
		*routeTableForwardRouteEntriesPtr=routeEntries[0];

	if(routeTableForwardRoutesPtr!=NULL)
		*routeTableForwardRoutesPtr=routes[0];

	if(routeTableReverseRouteEntriesPtr!=NULL)
		*routeTableReverseRouteEntriesPtr=routeEntries[1];

	if(routeTableReverseRoutesPtr!=NULL)
		*routeTableReverseRoutesPtr=routes[1];

	if(routeTableTreeTopBytesPtr!=NULL)
		*routeTableTreeTopBytesPtr=sizeof(RouteTableTreeTopBlock);

	if(routeTableTreeArrayBytesPtr!=NULL)
		*routeTableTreeArrayBytesPtr=arrayBytes;

	if(routeTableTreeLeafBytes!=NULL)
		*routeTableTreeLeafBytes=leafBytes;

	if(routeTableTreeLeafOffsetBytes!=NULL)
			*routeTableTreeLeafOffsetBytes=leafOffsetBytes;

	if(routeTableTreeLeafEntryBytes!=NULL)
			*routeTableTreeLeafEntryBytes=leafEntryBytes;

	if(routeTableTreeBranchBytes!=NULL)
		*routeTableTreeBranchBytes=branchBytes;

	if(routeTableTreeBranchOffsetBytes!=NULL)
		*routeTableTreeBranchOffsetBytes=branchOffsetBytes;

	if(routeTableTreeBranchChildBytes!=NULL)
		*routeTableTreeBranchChildBytes=branchChildBytes;




}




