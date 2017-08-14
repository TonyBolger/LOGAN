
#include "common.h"






void rttInitRouteTableTreeBuilder(RouteTableTreeBuilder *treeBuilder, RouteTableTreeTopBlock *top, s32 prefixCount, s32 suffixCount)
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
			prefixCount, suffixCount, treeBuilder->disp);

	//LOG(LOG_INFO,"rttInitRouteTableTreeBuilder: Reverse %p %p %p",top->data[ROUTE_TOPINDEX_REVERSE_LEAF],top->data[ROUTE_TOPINDEX_REVERSE_LEAF],top->data[ROUTE_TOPINDEX_REVERSE_OFFSET]);
	initTreeProxy(&(treeBuilder->reverseProxy),
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0]),top->data[ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0],
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0]),top->data[ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0],
			suffixCount, prefixCount, treeBuilder->disp);




	//LOG(LOG_INFO,"rttInitRouteTableTreeBuilder");
	initTreeWalker(&(treeBuilder->forwardWalker), &(treeBuilder->forwardProxy));
	//LOG(LOG_INFO,"rttInitRouteTableTreeBuilder");
	initTreeWalker(&(treeBuilder->reverseWalker), &(treeBuilder->reverseProxy));

//	LOG(LOG_INFO,"Forward: %i Leaves, %i Branches", treeBuilder->forwardProxy.leafArrayProxy.dataCount, treeBuilder->forwardProxy.branchArrayProxy.dataCount);
	//LOG(LOG_INFO,"Reverse: %i Leaves, %i Branches", treeBuilder->reverseProxy.leafArrayProxy.dataCount, treeBuilder->reverseProxy.branchArrayProxy.dataCount);



}

void rttUpgradeToRouteTableTreeBuilder(RouteTableArrayBuilder *arrayBuilder,  RouteTableTreeBuilder *treeBuilder, s32 sliceIndex, s32 prefixCount, s32 suffixCount, MemDispenser *disp)
{
	LOG(LOG_INFO,"Upgrade to Tree");

	treeBuilder->disp=arrayBuilder->disp;

	for(int i=0;i<ROUTE_TOPINDEX_MAX;i++)
		initHeapDataBlock(treeBuilder->dataBlocks+i, sliceIndex);

	initTreeProxy(&(treeBuilder->forwardProxy),
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_FORWARD_LEAF_ARRAY_0]),NULL,
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0]),NULL,
			prefixCount, suffixCount, disp);

	initTreeProxy(&(treeBuilder->reverseProxy),
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0]),NULL,
			&(treeBuilder->dataBlocks[ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0]),NULL,
			suffixCount, prefixCount, disp);

	initTreeWalker(&(treeBuilder->forwardWalker), &(treeBuilder->forwardProxy));
	walkerInitOffsetArrays(&(treeBuilder->forwardWalker), prefixCount, suffixCount);

	initTreeWalker(&(treeBuilder->reverseWalker), &(treeBuilder->reverseProxy));
	walkerInitOffsetArrays(&(treeBuilder->reverseWalker), suffixCount, prefixCount);

	//LOG(LOG_INFO,"Upgrading tree with %i forward entries and %i reverse entries to tree",arrayBuilder->oldForwardEntryCount, arrayBuilder->oldReverseEntryCount);

//	LOG(LOG_INFO,"Adding %i forward entries to tree",arrayBuilder->oldForwardEntryCount);
	walkerAppendPreorderedEntries(&(treeBuilder->forwardWalker), arrayBuilder->oldForwardEntries, arrayBuilder->oldForwardEntryCount, ROUTING_TABLE_FORWARD);

//	LOG(LOG_INFO,"Adding %i reverse entries to tree",arrayBuilder->oldReverseEntryCount);
	walkerAppendPreorderedEntries(&(treeBuilder->reverseWalker), arrayBuilder->oldReverseEntries, arrayBuilder->oldReverseEntryCount, ROUTING_TABLE_REVERSE);

//	LOG(LOG_INFO,"Upgrade completed");

	treeBuilder->newEntryCount=arrayBuilder->oldForwardEntryCount+arrayBuilder->oldReverseEntryCount;

	//rttDumpRoutingTable(treeBuilder);
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




static void rttMergeRoutes_ordered_forwardSingle(RouteTableTreeBuilder *builder, RouteTableTreeWalker *walker, RoutePatch *patch)
{
	s32 targetPrefix=patch->prefixIndex;
	s32 targetSuffix=patch->suffixIndex;
	s32 minEdgePosition=(*(patch->rdiPtr))->minEdgePosition;
	s32 maxEdgePosition=(*(patch->rdiPtr))->maxEdgePosition;

	s32 upstream=-1;
	RouteTableUnpackedEntry *entry=NULL;

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
	s32 upstreamEdgeOffset=-1;
	s32 downstreamEdgeOffset=-1;

	int res=walkerAdvanceToUpstreamThenOffsetThenDownstream(walker, targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition,
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

		walkerMergeRoutes_insertEntry(walker, targetPrefix, targetSuffix); // targetPrefix,targetSuffix,1
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

		walkerMergeRoutes_widen(walker); // width ++
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

		walkerMergeRoutes_split(walker, targetSuffix, splitWidth1, splitWidth2); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2
		}



}


static void rttMergeRoutes_ordered_reverseSingle(RouteTableTreeWalker *walker, RoutePatch *patch)
{
	s32 targetPrefix=patch->prefixIndex;
	s32 targetSuffix=patch->suffixIndex;
	s32 minEdgePosition=(*(patch->rdiPtr))->minEdgePosition;
	s32 maxEdgePosition=(*(patch->rdiPtr))->maxEdgePosition;

	s32 upstream=-1;
	RouteTableUnpackedEntry *entry=NULL;

	s32 upstreamEdgeOffset=-1;
	s32 downstreamEdgeOffset=-1;

	int res=walkerAdvanceToUpstreamThenOffsetThenDownstream(walker, targetSuffix, targetPrefix, minEdgePosition, maxEdgePosition,
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

		walkerMergeRoutes_insertEntry(walker, targetSuffix, targetPrefix);
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

		walkerMergeRoutes_widen(walker); // width ++
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

		walkerMergeRoutes_split(walker, targetPrefix, splitWidth1, splitWidth2); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2

		}


}




void rttMergeRoutes(RouteTableTreeBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 prefixCount, s32 suffixCount, RoutingReadData **orderedDispatches, MemDispenser *disp)
{

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
					rttMergeRoutes_ordered_forwardSingle(builder, walker, patchPtr);
					walkerResetOffsetArrays(walker);

					*(orderedDispatches++)=*(patchPtr->rdiPtr);
					patchPtr++;
					}
				}
			else
				{
				walkerSeekStart(walker);
				rttMergeRoutes_ordered_forwardSingle(builder, walker, patchPtr);
				walkerResetOffsetArrays(walker);

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




