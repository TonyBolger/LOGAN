
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



static void rttMergeRoutes_insertEntry(RouteTableTreeWalker *walker, s32 upstream, s32 downstream)
{
	// New entry, with given up/down and width=1. May require leaf split if full or new leaf if not matching

	/*
	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;
	s16 leafUpstream=-1;

	if(leafProxy==NULL)
		{
//		LOG(LOG_INFO,"Entry Insert: No current leaf - need new leaf append");

		leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->downstreamOffsetCount, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);
		treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, leafProxy, &walker->branchProxy, &walker->branchChildSibdex);

		initRouteTableTreeLeafOffsets(leafProxy, upstream);

		leafProxy->entryCount=1;

		walker->leafProxy=leafProxy;
		walker->leafEntry=0;
		}
	else
		{
		leafUpstream=leafProxy->dataBlock->upstream;

		//LOG(LOG_INFO,"InsertEntry: Leaf: %i Entry: %i",walker->leafProxy->lindex,walker->leafEntry);

		if(upstream<leafUpstream)
			{
//			LOG(LOG_INFO,"Entry Insert: Lower Unmatched upstream %i %i - need new leaf insert",upstream,leafUpstream);

			leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->downstreamOffsetCount, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);
			treeProxyInsertLeafChild(walker->treeProxy, walker->branchProxy, leafProxy,
					walker->branchChildSibdex, &walker->branchProxy, &walker->branchChildSibdex);

			initRouteTableTreeLeafOffsets(leafProxy, upstream);

			leafProxy->entryCount=1;

			walker->leafProxy=leafProxy;
			walker->leafEntry=0;

			//LOG(LOG_INFO,"Entry Insert: Need new leaf %i added to %i",walker->leafProxy->lindex, walker->branchProxy->brindex);
			}
		else if(upstream>leafUpstream)
			{
//			LOG(LOG_INFO,"Entry Insert: Higher Unmatched upstream %i %i - need new leaf append",upstream,leafUpstream);

			leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->downstreamOffsetCount, ROUTE_TABLE_TREE_LEAF_ENTRIES_CHUNK);
			treeProxyInsertLeafChild(walker->treeProxy, walker->branchProxy, leafProxy,
					walker->branchChildSibdex+1, &walker->branchProxy, &walker->branchChildSibdex);

			initRouteTableTreeLeafOffsets(leafProxy, upstream);

			leafProxy->entryCount=1;

			walker->leafProxy=leafProxy;
			walker->leafEntry=0;

			}

		else if(leafProxy->entryCount>=ROUTE_TABLE_TREE_LEAF_ENTRIES)
			{
			//s16 leafEntry=walker->leafEntry;
			//LOG(LOG_INFO,"Entry Insert: Leaf size at maximum - need to split - pos %i",walker->leafEntry);

			RouteTableTreeBranchProxy *targetBranchProxy=NULL;
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
			}
		else
			{
			//LOG(LOG_INFO,"Entry Insert: Leaf full - pos %i",walker->leafEntry);

			if(leafProxy->entryCount>=leafProxy->entryAlloc)
				{
				//LOG(LOG_INFO,"Entry Insert: Expand");
				expandRouteTableTreeLeafProxy(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);
				}
			else
				//ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);
				ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, downstream+1);

			leafMakeEntryInsertSpace(leafProxy, walker->leafEntry, 1);
			}
		}

	if(downstream<0)
		LOG(LOG_CRITICAL,"Insert invalid downstream %i",downstream);

	// Space already made - set entry data

	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(leafProxy->dataBlock);

	entryPtr[walker->leafEntry].downstream=downstream;
	entryPtr[walker->leafEntry].width=1;

	leafProxy->dataBlock->upstreamOffset++;
	getRouteTableTreeLeaf_OffsetPtr(leafProxy->dataBlock)[downstream]++;

	//LOG(LOG_INFO,"Entry Insert: Entry: %i D: %i Width %i (U %i) with %i used of %i",walker->leafEntry, downstream, 1, walker->leafProxy->dataBlock->upstream,walker->leafProxy->entryCount,walker->leafProxy->entryAlloc);

*/
	//dumpLeafProxy(leafProxy);

	LOG(LOG_CRITICAL,"PackLeaf: rttMergeRoutes_insertEntry TODO");
}

static void rttMergeRoutes_widen(RouteTableTreeWalker *walker)
{
	// Add one to the current entry width. Hopefully width doesn't wrap
/*

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	if(leafProxy==NULL)
		{
		LOG(LOG_CRITICAL,"Entry Widen: Null leaf, should never happen");
		}

	if(walker->leafEntry<0)
		{
		LOG(LOG_CRITICAL,"Entry Widen: Invalid entry index, should never happen");
		}


	//ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);

	RouteTableTreeLeafEntry *entryPtr=getRouteTableTreeLeaf_EntryPtr(leafProxy->dataBlock);

	if(entryPtr[walker->leafEntry].downstream<0)
		{
		LOG(LOG_CRITICAL,"Entry Widen: Invalid downstream, should never happen");
		}

	if(entryPtr[walker->leafEntry].width>2000000000)
		{
		LOG(LOG_CRITICAL,"Entry Widen: About to wrap width %i",entryPtr[walker->leafEntry].width);
		}


	entryPtr[walker->leafEntry].width++;
	s32 downstream=entryPtr[walker->leafEntry].downstream;

	//ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);
	ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, downstream+1);

	leafProxy->dataBlock->upstreamOffset++;
	getRouteTableTreeLeaf_OffsetPtr(leafProxy->dataBlock)[downstream]++;

	//LOG(LOG_INFO,"Widened %i %i %i (%i %i) to %i",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry,
			//leafProxy->dataBlock->upstream, leafProxy->dataBlock->entries[walker->leafEntry].downstream, leafProxy->dataBlock->entries[walker->leafEntry].width);

	*/
	LOG(LOG_CRITICAL,"PackLeaf: rttMergeRoutes_widen TODO");

}

static void rttMergeRoutes_split(RouteTableTreeWalker *walker, s32 downstream, s32 width1, s32 width2)
{
	// Add split existing route into two, and insert a new route between
/*
	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	s16 newEntryCount=leafProxy->entryCount+2;

	if(newEntryCount>ROUTE_TABLE_TREE_LEAF_ENTRIES)
		{
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

	LOG(LOG_CRITICAL,"PackLeaf: rttMergeRoutes_split TODO");
}



static void rttMergeRoutes_ordered_forwardSingle(RouteTableTreeBuilder *builder, RouteTableTreeWalker *walker, RoutePatch *patch)
{
	s32 targetPrefix=patch->prefixIndex;
	s32 targetSuffix=patch->suffixIndex;
	int minEdgePosition=(*(patch->rdiPtr))->minEdgePosition;
	int maxEdgePosition=(*(patch->rdiPtr))->maxEdgePosition;

	s16 upstream=-1;
	RouteTableUnpackedEntry *entry=NULL;

//	LOG(LOG_INFO,"rttMergeRoutes_ordered_forwardSingle %i %i with %i %i",targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

//	dumpWalker(walker);

	int res=walkerGetCurrentEntry(walker, &upstream, &entry);

	while(res && upstream < targetPrefix) 												// Skip lower upstream (leaf at a time)
		{
		walkerAccumulateLeafOffsets(walker);
		res=walkerNextLeaf(walker, &upstream, &entry);
//		dumpWalker(walker);
		}

/*
	while(res && upstream < targetPrefix) 												// Skip lower upstream (entry at a time)
		{
		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[upstream]+=entry->width;

		//LOG(LOG_INFO,"EntryLoop1 %i %i %i", walker->leafProxy->dataBlock->upstream, entry->downstream, entry->width);
		//LOG(LOG_INFO,"OffsetsLoop1 U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);

		res=walkerNextEntry(walker, &upstream, &entry, 0);
		}
*/

//	int upstreamEdgeOffset=walker->upstreamOffsets[targetPrefix];
	//int downstreamEdgeOffset=0;


//	LOG(LOG_INFO,"OffsetsNow U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);

//	rttDumpRoutingTable(builder);

/*
	if(entry!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i down: %i Width: %i",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream, entry->downstream, entry->width);
	else if(walker->leafProxy!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i - NULL ENTRY",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream);
	else
		LOG(LOG_INFO,"Empty branch");
*/

	LOG(LOG_CRITICAL,"PackLeaf: rttMergeRoutes_ordered_forwardSingle (leaf skip) TODO");

	/*
	while(res && upstream == targetPrefix &&										// Skip earlier upstream (leaf at a time)
		((walker->upstreamOffsets[targetPrefix]+walker->leafProxy->dataBlock->upstreamOffset) < minEdgePosition))
		{
		walkerAccumulateLeafOffsets(walker);
		res=walkerNextLeaf(walker, &upstream, &entry);
		}

	*/



	while(res && upstream == targetPrefix &&
			((walker->upstreamOffsets[targetPrefix]+entry->width)<minEdgePosition ||
			((walker->upstreamOffsets[targetPrefix]+entry->width)==minEdgePosition && entry->downstream!=targetSuffix))) // Skip earlier? upstream
		{
		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[upstream]+=entry->width;

		//LOG(LOG_INFO,"EntryLoop2 %i %i %i", walker->leafProxy->dataBlock->upstream, entry->downstream, entry->width);
		//LOG(LOG_INFO,"OffsetsLoop2 U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);

		res=walkerNextEntry(walker, &upstream, &entry, 1);
		}
/*
	LOG(LOG_INFO,"OffsetsNow U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);

	if(entry!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i down: %i Width: %i",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream, entry->downstream, entry->width);
	else if(walker->leafProxy!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i - NULL ENTRY",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream);
	else
		LOG(LOG_INFO,"Empty branch");
*/
	while(res && upstream == targetPrefix &&
			(walker->upstreamOffsets[targetPrefix]+entry->width)<=maxEdgePosition && entry->downstream<targetSuffix) // Skip matching upstream with earlier downstream
		{
		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[upstream]+=entry->width;

		//LOG(LOG_INFO,"EntryLoop3 %i %i %i", walker->leafProxy->dataBlock->upstream, entry->downstream, entry->width);
		//LOG(LOG_INFO,"OffsetsLoop3 U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);

		res=walkerNextEntry(walker, &upstream, &entry, 1);
		}

//	LOG(LOG_INFO,"FwdDone: %i %i %i %i",walker->branchProxy->brindex,walker->branchChildSibdex,walker->leafEntry, res);

//	LOG(LOG_INFO,"OffsetsNow U: %i D: %i",walker->upstreamOffsets[targetPrefix], walker->downstreamOffsets[targetSuffix]);
/*
	if(entry!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i down: %i Width: %i",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream, entry->downstream, entry->width);
	else if(walker->leafProxy!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i - NULL ENTRY",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream);
	else
		LOG(LOG_INFO,"Empty branch");
*/
	s32 upstreamEdgeOffset=walker->upstreamOffsets[targetPrefix];
	s32 downstreamEdgeOffset=walker->downstreamOffsets[targetSuffix];


	if(!res || upstream > targetPrefix || entry == NULL || (entry->downstream!=targetSuffix && upstreamEdgeOffset>=minEdgePosition)) // No suitable existing entry, but can insert/append here
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

		rttMergeRoutes_insertEntry(walker, targetPrefix, targetSuffix); // targetPrefix,targetSuffix,1
		}
	else if(upstream==targetPrefix && entry->downstream==targetSuffix) // Existing entry suitable, widen
		{
//		LOG(LOG_INFO,"Widen");
//		LOG(LOG_INFO,"Min %i Max %i",minEdgePosition, maxEdgePosition);
//		LOG(LOG_INFO,"U %i, D %i",upstreamEdgeOffset,downstreamEdgeOffset);

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

		rttMergeRoutes_widen(walker); // width ++
		}
	else // Existing entry unsuitable, split and insert
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

		rttMergeRoutes_split(walker, targetSuffix, splitWidth1, splitWidth2); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2

		//LOG(LOG_CRITICAL,"Entry Split"); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2
		}

//	LOG(LOG_INFO,"*****************");
//	LOG(LOG_INFO,"mergeRoutes_ordered_forwardSingle done");
//	LOG(LOG_INFO,"*****************");

}


static void rttMergeRoutes_ordered_reverseSingle(RouteTableTreeWalker *walker, RoutePatch *patch)
{
	s32 targetPrefix=patch->prefixIndex;
	s32 targetSuffix=patch->suffixIndex;
	int minEdgePosition=(*(patch->rdiPtr))->minEdgePosition;
	int maxEdgePosition=(*(patch->rdiPtr))->maxEdgePosition;

	s16 upstream=-1;
	RouteTableUnpackedEntry *entry=NULL;

//	LOG(LOG_INFO,"rttMergeRoutes_ordered_reverseSingle %i %i with %i %i",targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

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

//	LOG(LOG_INFO,"*****************");
//	LOG(LOG_INFO,"Reverse: Looking for P %i S %i Min %i Max %i", targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);
//	LOG(LOG_INFO,"*****************");


	int res=walkerGetCurrentEntry(walker, &upstream, &entry);

	while(res && upstream < targetSuffix) 												// Skip lower upstream (leaf at a time)
		{
		walkerAccumulateLeafOffsets(walker);
		res=walkerNextLeaf(walker, &upstream, &entry);
//		dumpWalker(walker);
		}

	/*
	while(res && upstream < targetSuffix) 												// Skip lower upstream
		{
		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[upstream]+=entry->width;

		//LOG(LOG_INFO,"EntryLoop1 %i %i %i", walker->leafProxy->dataBlock->upstream, entry->downstream, entry->width);
		//LOG(LOG_INFO,"OffsetsLoop1 U: %i D: %i",walker->upstreamOffsets[targetSuffix], walker->downstreamOffsets[targetPrefix]);

		res=walkerNextEntry(walker, &upstream, &entry, 0);
		}
*/

//	int upstreamEdgeOffset=walker->upstreamOffsets[targetPrefix];
	//int downstreamEdgeOffset=0;
/*
	LOG(LOG_INFO,"OffsetsNow U: %i D: %i",walker->upstreamOffsets[targetSuffix], walker->downstreamOffsets[targetPrefix]);

	if(entry!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i down: %i Width: %i",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream, entry->downstream, entry->width);
	else if(walker->leafProxy!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i - NULL ENTRY",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream);
	else
		LOG(LOG_INFO,"Empty branch");

	LOG(LOG_INFO,"Proxy is %p, Block is %p",walker->leafProxy, walker->leafProxy->dataBlock);
*/

	LOG(LOG_CRITICAL,"PackLeaf: rttMergeRoutes_ordered_reverseSingle (leaf skip) TODO");

	/*
	while(res && upstream == targetSuffix &&										// Skip earlier upstream (leaf at a time)
		((walker->upstreamOffsets[targetSuffix]+walker->leafProxy->dataBlock->upstreamOffset) < minEdgePosition))
		{
		walkerAccumulateLeafOffsets(walker);
		res=walkerNextLeaf(walker, &upstream, &entry);
		}
*/


	while(res && upstream == targetSuffix &&
			((walker->upstreamOffsets[targetSuffix]+entry->width)<minEdgePosition ||
			((walker->upstreamOffsets[targetSuffix]+entry->width)==minEdgePosition && entry->downstream!=targetPrefix))) // Skip earlier? upstream
		{
		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[upstream]+=entry->width;

		//LOG(LOG_INFO,"EntryLoop2 %i %i %i", walker->leafProxy->dataBlock->upstream, entry->downstream, entry->width);
		//LOG(LOG_INFO,"OffsetsLoop2 U: %i D: %i",walker->upstreamOffsets[targetSuffix], walker->downstreamOffsets[targetPrefix]);

		res=walkerNextEntry(walker, &upstream, &entry, 1);
		}
/*
	LOG(LOG_INFO,"OffsetsNow U: %i D: %i",walker->upstreamOffsets[targetSuffix], walker->downstreamOffsets[targetPrefix]);

	if(entry!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i down: %i Width: %i",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream, entry->downstream, entry->width);
	else if(walker->leafProxy!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i - NULL ENTRY",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream);
	else
		LOG(LOG_INFO,"Empty branch");
*/
	while(res && upstream == targetSuffix &&
			(walker->upstreamOffsets[targetSuffix]+entry->width)<=maxEdgePosition && entry->downstream<targetPrefix) // Skip matching upstream with earlier downstream
		{
		walker->downstreamOffsets[entry->downstream]+=entry->width;
		walker->upstreamOffsets[upstream]+=entry->width;

		//LOG(LOG_INFO,"EntryLoop3 %i %i %i", walker->leafProxy->dataBlock->upstream, entry->downstream, entry->width);
		//LOG(LOG_INFO,"OffsetsLoop3 U: %i D: %i",walker->upstreamOffsets[targetSuffix], walker->downstreamOffsets[targetPrefix]);

		res=walkerNextEntry(walker, &upstream, &entry, 1);
		}
/*
	LOG(LOG_INFO,"FwdDone: %i %i %i %i",walker->branchProxy->brindex,walker->branchChildSibdex,walker->leafEntry, res);

	LOG(LOG_INFO,"OffsetsNow U: %i D: %i",walker->upstreamOffsets[targetSuffix], walker->downstreamOffsets[targetPrefix]);

	if(entry!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i down: %i Width: %i",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream, entry->downstream, entry->width);
	else if(walker->leafProxy!=NULL)
		LOG(LOG_INFO,"Currently at Branch: %i Leaf: %i Entry: %i - Up: %i - NULL ENTRY",walker->branchProxy->brindex, walker->leafProxy->lindex, walker->leafEntry, upstream);
	else
		LOG(LOG_INFO,"Empty branch");
*/

	s32 upstreamEdgeOffset=walker->upstreamOffsets[targetSuffix];
	s32 downstreamEdgeOffset=walker->downstreamOffsets[targetPrefix];

	if(!res || upstream > targetSuffix || entry == NULL || (entry->downstream!=targetPrefix && upstreamEdgeOffset>=minEdgePosition)) // No suitable existing entry, but can insert/append here
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

		rttMergeRoutes_insertEntry(walker, targetSuffix, targetPrefix); // targetPrefix,targetSuffix,1
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

		rttMergeRoutes_widen(walker); // width ++
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

		rttMergeRoutes_split(walker, targetPrefix, splitWidth1, splitWidth2); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2

		//LOG(LOG_CRITICAL,"Entry Split"); // splitWidth1, (targetPrefix, targetSuffix, 1), splitWidth2
		}

//	LOG(LOG_INFO,"*****************");
//	LOG(LOG_INFO,"mergeRoutes_ordered_reverseSingle done");
//	LOG(LOG_INFO,"*****************");
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




