
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

	LOG(LOG_INFO,"Adding %i forward entries to tree",arrayBuilder->oldForwardEntryCount);
	rttwAppendPreorderedEntries(&(treeBuilder->forwardWalker), arrayBuilder->oldForwardEntries, arrayBuilder->oldForwardEntryCount, ROUTING_TABLE_FORWARD);

	LOG(LOG_INFO,"Adding %i reverse entries to tree",arrayBuilder->oldReverseEntryCount);
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

		rttbDumpBranchBlock(branchBlock);
		}


	for(int i=0;i<leafCount;i++)
		{
		RouteTableTreeLeafBlock *leafBlock=getRouteTableTreeLeafBlock(treeProxy, i);

		rttlDumpLeafBlock(leafBlock);
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
	int entryCount=rttaGetArrayEntries(arrayProxy);

	if(entryCount>ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)
		{
		entryCount=rttaCalcFirstLevelArrayEntries(entryCount);
		}

	return rttaCalcArraySize(entryCount);
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

/*
static void dumpBufNewEntryArrays(RouteTableTreeEntryBuffer *buf)
{
	int arrayCount=buf->newEntryArraysPtr-buf->newEntryArraysBlockPtr;

	for(int i=0;i<arrayCount;i++)
		{
		RouteTableUnpackedEntryArray *array=buf->newEntryArraysBlockPtr[i];
		LOG(LOG_INFO,"  EntryCount %i Upstream %i",array->entryCount, array->upstream);

		for(int j=0;j<array->entryCount;j++)
			{
			LOG(LOG_INFO,"    Downstream %i Width %i",array->entries[j].downstream, array->entries[j].width);
			}
		}

	LOG(LOG_INFO,"Upstream Offsets %i",buf->upstreamOffsetCount);
	for(int i=0;i<buf->upstreamOffsetCount;i++)
		LOG(LOG_INFO,"  %i %i",i,buf->newLeafUpstreamOffsets[i]);

	LOG(LOG_INFO,"Downstream Offsets %i",buf->downstreamOffsetCount);
	for(int i=0;i<buf->downstreamOffsetCount;i++)
		LOG(LOG_INFO,"  %i %i",i,buf->newLeafDownstreamOffsets[i]);

}
*/

static void appendStarterLeaf(RouteTableTreeEntryBuffer *buf)
{
	if(buf->oldBranchProxy==NULL)
		LOG(LOG_CRITICAL,"Not on a valid branch");

	buf->oldLeafProxy=rttlAllocRouteTableTreeLeafProxy(buf->treeProxy, buf->upstreamOffsetCount, buf->downstreamOffsetCount, ROUTEPACKING_ENTRYARRAYS_CHUNK);
	treeProxyAppendLeafChild(buf->treeProxy, buf->oldBranchProxy, buf->oldLeafProxy, &buf->oldBranchProxy, &buf->oldBranchChildSibdex);
}







static void treeEntryBufferInit(RouteTableTreeEntryBuffer *buf, RouteTableTreeProxy *treeProxy, int upstreamOffsetCount, int downstreamOffsetCount)
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

	buf->oldUpstream=0;
	buf->oldDownstream=0;
	buf->oldWidth=0;

	buf->accumUpstreamOffsets=dAlloc(treeProxy->disp, sizeof(s32)*upstreamOffsetCount);
	buf->accumDownstreamOffsets=dAlloc(treeProxy->disp, sizeof(s32)*downstreamOffsetCount);
	memset(buf->accumUpstreamOffsets,0,sizeof(s32)*upstreamOffsetCount);
	memset(buf->accumDownstreamOffsets,0,sizeof(s32)*downstreamOffsetCount);

	buf->newBranchProxy=NULL;
	buf->newLeafProxy=NULL;
	buf->newBranchChildSibdex=-1;

	buf->newEntryArraysBlockPtr=NULL;
	buf->newEntryArraysPtr=NULL;
	buf->newEntryArraysPtrEnd=NULL;

	buf->newEntryCurrentArrayPtr=NULL;
	buf->newEntryPtr=NULL;
	buf->newEntryPtrEnd=NULL;

	buf->newUpstream=0;
	buf->newDownstream=0;
	buf->newWidth=0;

	buf->newEntriesTotal=0;

	/*
	buf->newLeafUpstreamOffsets=dAlloc(treeProxy->disp, sizeof(s32)*upstreamOffsetCount);
	buf->newLeafDownstreamOffsets=dAlloc(treeProxy->disp, sizeof(s32)*downstreamOffsetCount);
	memset(buf->newLeafUpstreamOffsets,0,sizeof(s32)*upstreamOffsetCount);
	memset(buf->newLeafDownstreamOffsets,0,sizeof(s32)*downstreamOffsetCount);
*/

	buf->newLeafUpstreamOffsets=NULL;
	buf->newLeafDownstreamOffsets=NULL;

	buf->maxWidth=0;
}


static RouteTableUnpackedEntryArray *treeEntryBufferNewOutputEntryArray(RouteTableTreeEntryBuffer *buf, s32 upstream)
{
	if(buf->newLeafProxy!=buf->oldLeafProxy)
		LOG(LOG_CRITICAL,"Proxy mismatch");

	if(buf->newEntryArraysPtr==buf->newEntryArraysPtrEnd || buf->newEntriesTotal > ROUTEPACKING_TOTALENTRYS_SPLIT_THRESHOLD)
		{
		int arrayCount=buf->newEntryArraysPtr-buf->newEntryArraysBlockPtr;

//		LOG(LOG_INFO,"Splitting Output Arrays: %i with %i",arrayCount, buf->newEntriesTotal);

//		if(arrayCount==ROUTEPACKING_ENTRYARRAYS_MAX)
//			dumpBufNewEntryArrays(buf);

		//LOG(LOG_INFO,"SPLIT NEEDED");

		//void treeProxyInsertLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy, s16 childPosition,
				//RouteTableTreeBranchProxy **updatedParentBranchProxyPtr, s16 *updatedChildPositionPtr);

		int arraysToMove=(arrayCount+1)/2;
		int arraysToKeep=arrayCount-arraysToMove;

		RouteTableTreeLeafProxy *freshLeafProxy=rttlAllocRouteTableTreeLeafProxy(buf->treeProxy, buf->upstreamOffsetCount, buf->downstreamOffsetCount, arraysToMove);
		rttlMarkDirty(buf->treeProxy, freshLeafProxy);

		RouteTableTreeBranchProxy *updatedBranchProxy;
		s16 updatedExistingLeafPosition;
		s16 updatedFreshLeafPosition;

//		treeProxyInsertLeafChild(buf->treeProxy, buf->newBranchProxy, newLeafProxy, buf->newBranchChildSibdex, &updatedParentBranchProxy, &updatedChildPosition);

		treeProxyInsertLeafChildBefore(buf->treeProxy, buf->newBranchProxy, buf->newLeafProxy, buf->newBranchChildSibdex, freshLeafProxy,
				&updatedBranchProxy, &updatedExistingLeafPosition, &updatedFreshLeafPosition);

		if(updatedBranchProxy != buf->newBranchProxy)
			buf->newBranchProxy=buf->oldBranchProxy=updatedBranchProxy;

		//LOG(LOG_INFO,"Requested %i Got %i", buf->newBranchChildSibdex, updatedChildPosition);

		buf->oldBranchChildSibdex=updatedExistingLeafPosition;
		buf->newBranchChildSibdex=updatedExistingLeafPosition;

		RouteTableUnpackedSingleBlock *newBlock=freshLeafProxy->unpackedBlock;

//		memcpy(newBlock->upstreamLeafOffsets, buf->newLeafUpstreamOffsets, sizeof(s32)*buf->upstreamOffsetCount);
//		memcpy(newBlock->downstreamLeafOffsets, buf->newLeafDownstreamOffsets, sizeof(s32)*buf->downstreamOffsetCount);

		memcpy(newBlock->entryArrays, buf->newEntryArraysBlockPtr, sizeof(RouteTableUnpackedEntryArray *)*arraysToMove);
		newBlock->entryArrayCount=arraysToMove;
		rtpRecalculateUnpackedBlockOffsets(newBlock);

		memset(buf->newLeafUpstreamOffsets,0,sizeof(s32)*buf->upstreamOffsetCount);
		memset(buf->newLeafDownstreamOffsets,0,sizeof(s32)*buf->downstreamOffsetCount);

		memmove(buf->newEntryArraysBlockPtr, buf->newEntryArraysBlockPtr+arraysToMove, sizeof(RouteTableUnpackedEntryArray *)*arraysToKeep);
		memset(buf->newEntryArraysBlockPtr+arraysToKeep,0,sizeof(RouteTableUnpackedEntryArray *)*arraysToMove);

//		buf->newEntryArraysBlockPtr=dAlloc(buf->treeProxy->disp, sizeof(RouteTableUnpackedEntryArray *)*ROUTEPACKING_ENTRYARRAYS_MAX);
//		memset(buf->newEntryArraysBlockPtr,0,sizeof(RouteTableUnpackedEntryArray *)*ROUTEPACKING_ENTRYARRAYS_MAX);

		buf->newEntryArraysPtr=buf->newEntryArraysBlockPtr+arraysToKeep;
		//buf->newEntryArraysPtrEnd=buf->newEntryArraysBlockPtr+ROUTEPACKING_ENTRYARRAYS_MAX; // Removed since no new alloc

		buf->newEntriesTotal=rtpRecalculateEntryArrayOffsets(buf->newEntryArraysBlockPtr, arraysToKeep, buf->newLeafUpstreamOffsets, buf->newLeafDownstreamOffsets);

//		LOG(LOG_INFO,"After Output Split: %i with %i", arraysToKeep, buf->newEntriesTotal);

		}

	RouteTableUnpackedEntryArray *array=dAlloc(buf->treeProxy->disp, sizeof(RouteTableUnpackedEntryArray)+sizeof(RouteTableUnpackedEntry)*ROUTEPACKING_ENTRYS_MAX);

	array->upstream=upstream;
	array->entryAlloc=ROUTEPACKING_ENTRYS_MAX;
	array->entryCount=0;

	buf->newEntryPtr=array->entries;
	buf->newEntryPtrEnd=array->entries+ROUTEPACKING_ENTRYS_MAX;

	*(buf->newEntryArraysPtr)=array;
	buf->newEntryCurrentArrayPtr=array;

	buf->newEntryArraysPtr++;

	return array;
}


static void treeEntryBufferFlushOutputArrays(RouteTableTreeEntryBuffer *buf)
{
	if(buf->oldLeafProxy != buf->newLeafProxy)
		LOG(LOG_CRITICAL,"Leaf Mismatch");

	// Update last array size
	if(buf->newEntryCurrentArrayPtr!=NULL)
		{
		buf->newEntryCurrentArrayPtr->entryCount=buf->newEntryPtr-buf->newEntryCurrentArrayPtr->entries;
		buf->newEntriesTotal+=buf->newEntryCurrentArrayPtr->entryCount;
		}

	int arrayCount=buf->newEntryArraysPtr-buf->newEntryArraysBlockPtr;

//	LOG(LOG_INFO,"Flushing Output Arrays: %i with %i",arrayCount, buf->newEntriesTotal);

	RouteTableTreeLeafProxy *leafProxy=buf->newLeafProxy;

	rttlMarkDirty(buf->treeProxy, leafProxy);

	rtpSetUnpackedData(buf->newLeafProxy->unpackedBlock,
			buf->newLeafUpstreamOffsets,buf->upstreamOffsetCount,
			buf->newLeafDownstreamOffsets,buf->downstreamOffsetCount,
			buf->newEntryArraysBlockPtr, arrayCount);

}


static void treeEntryBufferFlushOutputEntry(RouteTableTreeEntryBuffer *buf)
{
	if(buf->newWidth>0)
		{
		RouteTableUnpackedEntryArray *array=buf->newEntryCurrentArrayPtr;

		if(array!=NULL && (array->upstream!=buf->newUpstream || buf->newEntryPtr==buf->newEntryPtrEnd))
			{
			array->entryCount=buf->newEntryPtr-array->entries;

			buf->newEntriesTotal+=array->entryCount;
			array=NULL;
			}

		if(array==NULL)
			treeEntryBufferNewOutputEntryArray(buf, buf->newUpstream);


		buf->newLeafUpstreamOffsets[buf->newUpstream]+=buf->newWidth;
		buf->newLeafDownstreamOffsets[buf->newDownstream]+=buf->newWidth;

		buf->newEntryPtr->downstream=buf->newDownstream;
		buf->newEntryPtr->width=buf->newWidth;

		buf->maxWidth=MAX(buf->maxWidth,buf->newWidth);

		buf->newEntryPtr++;
		buf->newWidth=0;
		}
}




static s32 treeEntryBufferPollInput(RouteTableTreeEntryBuffer *buf)
{
	RouteTableUnpackedEntry *oldEntryPtr=buf->oldEntryPtr;

	if(oldEntryPtr==buf->oldEntryPtrEnd)
		{
		RouteTableUnpackedEntryArray **oldArrayPtr=buf->oldEntryArraysPtr;

		if(oldArrayPtr==buf->oldEntryArraysPtrEnd)
			{
			if(!hasMoreSiblings(buf->treeProxy, buf->oldBranchProxy, buf->oldBranchChildSibdex, buf->oldLeafProxy))
			{
				buf->oldUpstream=0;
				buf->oldDownstream=0;
				buf->oldWidth=0;

				return 0;
				}

			// First flush the output buffers - may restructure tree
			treeEntryBufferFlushOutputEntry(buf);
			treeEntryBufferFlushOutputArrays(buf);

			RouteTableTreeBranchProxy *branchProxy=buf->oldBranchProxy;
			RouteTableTreeLeafProxy *leafProxy=buf->oldLeafProxy;
			s16 branchChildSibdex=buf->oldBranchChildSibdex;

			if(!getNextLeafSibling(buf->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy)) // No more leaves
				LOG(LOG_CRITICAL,"treeEntryBufferPollInput: No more leaves");

			// More leaves
			//LOG(LOG_INFO,"treeEntryBufferPollInput: Moved leaf - now %i",branchChildSibdex);

			// Move input side then bind

			buf->oldBranchProxy=branchProxy;
			buf->oldLeafProxy=leafProxy;
			buf->oldBranchChildSibdex=branchChildSibdex;

			rttlEnsureFullyUnpacked(buf->treeProxy, buf->oldLeafProxy);

			RouteTableUnpackedSingleBlock *block=buf->oldLeafProxy->unpackedBlock;

			buf->oldEntryArraysPtr=block->entryArrays;
			buf->oldEntryArraysPtrEnd=block->entryArrays+block->entryArrayCount;

			// Move output side, alloc arrays and bind

			buf->newBranchProxy=branchProxy;
			buf->newLeafProxy=leafProxy;
			buf->newBranchChildSibdex=branchChildSibdex;

			buf->newEntryArraysBlockPtr=dAlloc(buf->treeProxy->disp, sizeof(RouteTableUnpackedEntryArray *)*ROUTEPACKING_ENTRYARRAYS_MAX);
			memset(buf->newEntryArraysBlockPtr,0,sizeof(RouteTableUnpackedEntryArray *)*ROUTEPACKING_ENTRYARRAYS_MAX);

			buf->newEntryArraysPtr=buf->newEntryArraysBlockPtr;
			buf->newEntryArraysPtrEnd=buf->newEntryArraysBlockPtr+ROUTEPACKING_ENTRYARRAYS_MAX;

			buf->newEntryCurrentArrayPtr=NULL;
			buf->newEntryPtr=NULL;
			buf->newEntryPtrEnd=NULL;

			buf->newEntriesTotal=0;

			buf->newLeafUpstreamOffsets=dAlloc(buf->treeProxy->disp, sizeof(s32)*buf->upstreamOffsetCount);
			buf->newLeafDownstreamOffsets=dAlloc(buf->treeProxy->disp, sizeof(s32)*buf->downstreamOffsetCount);
			memset(buf->newLeafUpstreamOffsets,0,sizeof(s32)*buf->upstreamOffsetCount);
			memset(buf->newLeafDownstreamOffsets,0,sizeof(s32)*buf->downstreamOffsetCount);

			}

		RouteTableUnpackedEntryArray *array=*(buf->oldEntryArraysPtr);

		buf->oldEntryPtr=array->entries;
		buf->oldEntryPtrEnd=array->entries+array->entryCount;
		buf->oldUpstream=array->upstream;

		buf->oldEntryArraysPtr++;

		oldEntryPtr=buf->oldEntryPtr;
		}

	if(oldEntryPtr==buf->oldEntryPtrEnd)
		LOG(LOG_CRITICAL,"Empty array - %p %p",oldEntryPtr, buf->oldEntryPtrEnd);

	buf->oldDownstream=oldEntryPtr->downstream;
	buf->oldWidth=oldEntryPtr->width;

	if(buf->oldWidth==0)
		LOG(LOG_CRITICAL,"Loaded zero-width entry");

	buf->oldEntryPtr++;

	return buf->oldWidth;
}

//static
s32 treeEntryBufferPollInput_sameLeaf(RouteTableTreeEntryBuffer *buf)
{
	RouteTableUnpackedEntry *oldEntryPtr=buf->oldEntryPtr;

	if(oldEntryPtr==buf->oldEntryPtrEnd)
		{
		RouteTableUnpackedEntryArray **oldArrayPtr=buf->oldEntryArraysPtr;

		if(oldArrayPtr==buf->oldEntryArraysPtrEnd)
			{
			buf->oldUpstream=0;
			buf->oldDownstream=0;
			buf->oldWidth=0;

			return 0;
			}

		RouteTableUnpackedEntryArray *array=*(buf->oldEntryArraysPtr);

		buf->oldEntryPtr=array->entries;
		buf->oldEntryPtrEnd=array->entries+array->entryCount;
		buf->oldUpstream=array->upstream;

		buf->oldEntryArraysPtr++;

		oldEntryPtr=buf->oldEntryPtr;
		}

	if(oldEntryPtr==buf->oldEntryPtrEnd)
		LOG(LOG_CRITICAL,"Empty array - %p %p",oldEntryPtr, buf->oldEntryPtrEnd);

	buf->oldDownstream=oldEntryPtr->downstream;
	buf->oldWidth=oldEntryPtr->width;

	if(buf->oldWidth==0)
		LOG(LOG_CRITICAL,"Loaded zero-width entry");

	buf->oldEntryPtr++;

	return buf->oldWidth;
}


static void treeEntryBufferAccumulateOldLeafOffsets(RouteTableTreeEntryBuffer *buf)
{
	RouteTableUnpackedSingleBlock *block=buf->oldLeafProxy->unpackedBlock;

	int upstreamValidOffsets=MIN(buf->upstreamOffsetCount, block->upstreamOffsetAlloc);

	for(int i=0;i<upstreamValidOffsets;i++)
		buf->accumUpstreamOffsets[i]+=block->upstreamLeafOffsets[i];

	int downstreamValidOffsets=MIN(buf->downstreamOffsetCount, block->downstreamOffsetAlloc);

	for(int i=0;i<downstreamValidOffsets;i++)
		buf->accumDownstreamOffsets[i]+=block->downstreamLeafOffsets[i];
}


static void treeEntryBufferBindEntries(RouteTableTreeEntryBuffer *buf)
{
	// Old Entries:

	rttlEnsureFullyUnpacked(buf->treeProxy, buf->oldLeafProxy);

	RouteTableUnpackedSingleBlock *block=buf->oldLeafProxy->unpackedBlock;

	buf->oldEntryArraysPtr=block->entryArrays;
	buf->oldEntryArraysPtrEnd=block->entryArrays+block->entryArrayCount;

//	LOG(LOG_INFO,"Arrays %i",block->entryArrayCount);

	if(block->entryArrayCount>0)
		{
		RouteTableUnpackedEntryArray *array=*(buf->oldEntryArraysPtr);

		buf->oldEntryPtr=array->entries;
		buf->oldEntryPtrEnd=array->entries+array->entryCount;

		buf->oldUpstream=array->upstream;

		buf->oldEntryArraysPtr++;
/*
		LOG(LOG_INFO,"Entries %i",array->entryCount);

		if(array->entryCount>0)
			LOG(LOG_INFO,"Entry %i %i %i",array->upstream, array->entries[0].downstream, array->entries[0].width);
			*/
		}
	else
		{
		buf->oldEntryPtr=buf->oldEntryPtrEnd=NULL;
		}

	// New Entries:

	buf->newBranchProxy=buf->oldBranchProxy;
	buf->newBranchChildSibdex=buf->oldBranchChildSibdex;
	buf->newLeafProxy=buf->oldLeafProxy;

	buf->newEntryArraysBlockPtr=dAlloc(buf->treeProxy->disp, sizeof(RouteTableUnpackedEntry *)*ROUTEPACKING_ENTRYARRAYS_MAX);
	memset(buf->newEntryArraysBlockPtr,0,sizeof(RouteTableUnpackedEntry *)*ROUTEPACKING_ENTRYARRAYS_MAX);

	buf->newEntryArraysPtr=buf->newEntryArraysBlockPtr;
	buf->newEntryArraysPtrEnd=buf->newEntryArraysBlockPtr+ROUTEPACKING_ENTRYARRAYS_MAX;

	buf->newEntriesTotal=0;

	buf->newLeafUpstreamOffsets=dAlloc(buf->treeProxy->disp, sizeof(s32)*buf->upstreamOffsetCount);
	buf->newLeafDownstreamOffsets=dAlloc(buf->treeProxy->disp, sizeof(s32)*buf->downstreamOffsetCount);
	memset(buf->newLeafUpstreamOffsets,0,sizeof(s32)*buf->upstreamOffsetCount);
	memset(buf->newLeafDownstreamOffsets,0,sizeof(s32)*buf->downstreamOffsetCount);
}

//static
void treeEntryBufferUnbindEntries(RouteTableTreeEntryBuffer *buf)
{
	buf->oldEntryArraysPtr=buf->oldEntryArraysPtrEnd=NULL;
	buf->oldEntryPtr=buf->oldEntryPtrEnd=NULL;

	buf->newEntryArraysBlockPtr=buf->newEntryArraysPtr=buf->newEntryArraysPtrEnd=NULL;
	buf->newEntryCurrentArrayPtr=NULL;
	buf->newEntryPtr=buf->newEntryPtrEnd=NULL;

	buf->newEntriesTotal=0;

	buf->newLeafUpstreamOffsets=buf->newLeafDownstreamOffsets=NULL;
}


static s32 snoopLastUpstream(RouteTableTreeLeafProxy *leafProxy)
{
	RouteTableUnpackedSingleBlock *block=leafProxy->unpackedBlock;

	for(int i=block->upstreamOffsetAlloc;i>0;--i)
		{
		if(block->upstreamLeafOffsets[i]>0)
			return i;
		}
	return -1;
}


static s32 treeEntryBufferSkipOldToNextLeaf_targetUpstream(RouteTableTreeEntryBuffer *buf, s32 targetUpstream)
{
	if(buf->oldLeafProxy==NULL)
		return 0;

	if(buf->oldEntryArraysPtr!=NULL) // Current bound - oops
		{
		LOG(LOG_CRITICAL,"TODO: advance over bound leaf");
		return 0;
		}

	RouteTableTreeBranchProxy *branchProxy=buf->oldBranchProxy;
	RouteTableTreeLeafProxy *leafProxy=buf->oldLeafProxy;
	s16 branchChildSibdex=buf->oldBranchChildSibdex;

	if(!getNextLeafSibling(buf->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy)) // No more leaves
		{
//		LOG(LOG_INFO,"treeEntryBufferAdvanceOldToNextLeaf_targetUpstream: No more leaves");
		return 0;
		}

/*	int snoopUpstream=snoopLastUpstream(leafProxy);

	if(snoopUpstream<targetUpstream)
		{
		treeEntryBufferAccumulateOldLeafOffsets(buf);

		buf->oldBranchProxy=branchProxy;
		buf->oldLeafProxy=leafProxy;
		buf->oldBranchChildSibdex=branchChildSibdex;

		//LOG(LOG_INFO,"treeEntryBufferAdvanceOldToNextLeaf_targetUpstream: Next leaf");
		return 1;
		}

	//LOG(LOG_INFO,"treeEntryBufferAdvanceOldToNextLeaf_targetUpstream: Next leaf is too far");

	return 0;
	*/

	treeEntryBufferAccumulateOldLeafOffsets(buf);

	buf->oldBranchProxy=branchProxy;
	buf->oldLeafProxy=leafProxy;
	buf->oldBranchChildSibdex=branchChildSibdex;

	return 1;
}



static int treeEntryBufferSkipOldLeaf_targetUpstreamAndOffset(RouteTableTreeEntryBuffer *buf, s32 targetUpstream, s32 targetUpsteamOffset)
{
//	LOG(LOG_INFO,"Advancing to U: %i [%i]",targetUpstream,targetUpsteamOffset);

	int total=0;

	while(buf->oldLeafProxy!=NULL)
		{
		int snoopUpstream=snoopLastUpstream(buf->oldLeafProxy);

		if(snoopUpstream>targetUpstream)
			{
//			if(total)
//				LOG(LOG_INFO,"A: Post move %i offset is %i", total, buf->accumUpstreamOffsets[targetUpstream]);

			return 1;
			}

		if(snoopUpstream==targetUpstream)
			{
			RouteTableUnpackedSingleBlock *block=buf->oldLeafProxy->unpackedBlock;
			int snoopOffset=buf->accumUpstreamOffsets[targetUpstream]+block->upstreamLeafOffsets[targetUpstream];

			if(snoopOffset>=targetUpsteamOffset)
				{
//				if(total)
//					LOG(LOG_INFO,"B: Post move %i offset is %i", total, buf->accumUpstreamOffsets[targetUpstream]);
				return 1;
				}
			}

//		LOG(LOG_INFO,"Pre move offset is %i", buf->accumUpstreamOffsets[targetUpstream]);

		int res=treeEntryBufferSkipOldToNextLeaf_targetUpstream(buf, targetUpstream);

		if(!res)
			{
//			if(total)
//				LOG(LOG_INFO,"C: Post move %i offset is %i", total, buf->accumUpstreamOffsets[targetUpstream]);

			return 1;
			}

		total++;

		}

//	if(total)
//		LOG(LOG_INFO,"D: Post move %i offset is %i", total, buf->accumUpstreamOffsets[targetUpstream]);

	return 0;
}






static s32 treeEntryBufferTransfer(RouteTableTreeEntryBuffer *buf)
{
	//LOG(LOG_INFO,"treeEntryBufferTransfer");

	s32 widthToTransfer=buf->oldWidth;
	buf->oldWidth=0;

	if(buf->newWidth>0)
		{
		if(buf->newUpstream!=buf->oldUpstream || buf->newDownstream!=buf->oldDownstream)
			treeEntryBufferFlushOutputEntry(buf);
		else
			{
			buf->newWidth+=widthToTransfer;
			treeEntryBufferPollInput(buf);
			return widthToTransfer;
			}
		}

	buf->newUpstream=buf->oldUpstream;
	buf->newDownstream=buf->oldDownstream;
	buf->newWidth=widthToTransfer;

	treeEntryBufferPollInput(buf);
	return widthToTransfer;
}

static s32 treeEntryBufferTransfer_sameLeaf(RouteTableTreeEntryBuffer *buf)
{
	//LOG(LOG_INFO,"treeEntryBufferTransfer");

	s32 widthToTransfer=buf->oldWidth;
	buf->oldWidth=0;

	if(buf->newWidth>0)
		{
		if(buf->newUpstream!=buf->oldUpstream || buf->newDownstream!=buf->oldDownstream)
			treeEntryBufferFlushOutputEntry(buf);
		else
			{
			buf->newWidth+=widthToTransfer;
			treeEntryBufferPollInput_sameLeaf(buf);
			return widthToTransfer;
			}
		}

	buf->newUpstream=buf->oldUpstream;
	buf->newDownstream=buf->oldDownstream;
	buf->newWidth=widthToTransfer;

	treeEntryBufferPollInput_sameLeaf(buf);
	return widthToTransfer;
}




static void treeEntryBufferTransferLeafRemainder(RouteTableTreeEntryBuffer *buf)
{
	//LOG(LOG_INFO,"treeEntryBufferTransfer");
	s32 widthToTransfer=buf->oldWidth;

	while(widthToTransfer)
		{
		buf->oldWidth=0;

		if(buf->newWidth>0)
			{
			if(buf->newUpstream!=buf->oldUpstream || buf->newDownstream!=buf->oldDownstream)
				{
				treeEntryBufferFlushOutputEntry(buf);
				buf->newUpstream=buf->oldUpstream;
				buf->newDownstream=buf->oldDownstream;
				buf->newWidth=widthToTransfer;
				}
			else
				buf->newWidth+=widthToTransfer;
			}
		else
			{
			buf->newUpstream=buf->oldUpstream;
			buf->newDownstream=buf->oldDownstream;
			buf->newWidth=widthToTransfer;
			}

		treeEntryBufferPollInput_sameLeaf(buf);
		widthToTransfer=buf->oldWidth;
		}


}



//static
s32 treeEntryBufferPartialTransfer(RouteTableTreeEntryBuffer *buf, s32 requestedTransferWidth)
{
//	LOG(LOG_INFO,"treeEntryBufferPartialTransfer");

	s32 widthToTransfer=MIN(requestedTransferWidth, buf->oldWidth);
	buf->oldWidth-=widthToTransfer;

	if(buf->newWidth>0)
		{
		if(buf->newUpstream!=buf->oldUpstream || buf->newDownstream!=buf->oldDownstream)
			treeEntryBufferFlushOutputEntry(buf);
		else
			{
			buf->newWidth+=widthToTransfer;
			if(buf->oldWidth==0)
				treeEntryBufferPollInput(buf);

			return widthToTransfer;
			}
		}

	buf->newUpstream=buf->oldUpstream;
	buf->newDownstream=buf->oldDownstream;
	buf->newWidth=widthToTransfer;

	if(buf->oldWidth==0)
		treeEntryBufferPollInput(buf);

	return widthToTransfer;
}


static void treeEntryBufferPushOutput(RouteTableTreeEntryBuffer *buf, s32 upstream, s32 downstream, s32 width)
{
//	LOG(LOG_INFO,"treeEntryBufferPushOutput - %i %i %i",upstream,downstream,width);

	if(buf->newWidth>0)
		{
		if(buf->newUpstream!=upstream || buf->newDownstream!=downstream)
			treeEntryBufferFlushOutputEntry(buf);
		else
			{
			buf->newWidth+=width;
			return;
			}
		}

	buf->newUpstream=upstream;
	buf->newDownstream=downstream;
	buf->newWidth=width;
}




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
		s32 prefixCount, s32 suffixCount)// int *maxWidth)
{
	RouteTableTreeEntryBuffer buf;
	treeEntryBufferInit(&buf, &(builder->forwardProxy), prefixCount, suffixCount);

//	LOG(LOG_INFO,"rttMergeRoutes_ordered_forwardMulti: Patch count %i",(patchPtrEnd-patchPtr));

	while(patchPtr<patchPtrEnd)
	{
		s32 targetUpstream=patchPtr->prefixIndex;
		s32 targetDownstream=patchPtr->suffixIndex;
		s32 minEdgePosition=patchPtr->rdiPtr->minEdgePosition;
		s32 maxEdgePosition=patchPtr->rdiPtr->maxEdgePosition;

//		LOG(LOG_INFO,"Patch: P: %i S: %i [%i %i]",targetUpstream, targetDownstream, minEdgePosition, maxEdgePosition);

		int expectedMaxEdgePosition=maxEdgePosition+1;
		RoutePatch *patchGroupPtr=patchPtr+1;  // Make groups of compatible inserts for combined processing

		if(minEdgePosition!=TR_INIT_MINEDGEPOSITION || maxEdgePosition!=TR_INIT_MAXEDGEPOSITION)
			{
			while(patchGroupPtr<patchPtrEnd && patchGroupPtr->prefixIndex==targetUpstream && patchGroupPtr->suffixIndex==targetDownstream &&
					patchGroupPtr->rdiPtr->minEdgePosition == minEdgePosition && patchGroupPtr->rdiPtr->maxEdgePosition == expectedMaxEdgePosition)
				{
				patchGroupPtr++;
				expectedMaxEdgePosition++;
				}
			}
		else
			{
			while(patchGroupPtr<patchPtrEnd && patchGroupPtr->prefixIndex==targetUpstream && patchGroupPtr->suffixIndex==targetDownstream &&
					patchGroupPtr->rdiPtr->minEdgePosition == TR_INIT_MINEDGEPOSITION && patchGroupPtr->rdiPtr->maxEdgePosition == TR_INIT_MAXEDGEPOSITION)
				patchGroupPtr++;
			}
/*
		int groupLen=patchGroupPtr-patchPtr;
		if(groupLen>1)
			LOG(LOG_INFO,"GROUPED %i",groupLen);
*/

		if(buf.oldEntryArraysPtr!=NULL) // Currently bound, but can perhaps unbind after current leaf
			{
			while(buf.oldWidth && buf.oldUpstream<targetUpstream) // Skip lower upstream
				{
	//			LOG(LOG_INFO,"Loop 1");

				s32 width=buf.oldWidth;
				buf.accumUpstreamOffsets[buf.oldUpstream]+=width;
				buf.accumDownstreamOffsets[buf.oldDownstream]+=width;

				treeEntryBufferTransfer_sameLeaf(&buf);

	//			LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);
				}

			while(buf.oldWidth && buf.oldUpstream==targetUpstream &&
				((buf.accumUpstreamOffsets[targetUpstream]+buf.oldWidth)<minEdgePosition ||
				((buf.accumUpstreamOffsets[targetUpstream]+buf.oldWidth)==minEdgePosition && buf.oldDownstream!=targetDownstream))) // Skip earlier upstream
				{
	//			LOG(LOG_INFO,"Loop 2");

				s32 width=buf.oldWidth;
				buf.accumUpstreamOffsets[buf.oldUpstream]+=width;
				buf.accumDownstreamOffsets[buf.oldDownstream]+=width;

				treeEntryBufferTransfer_sameLeaf(&buf);
				}

			if(!buf.oldWidth && hasMoreSiblings(buf.treeProxy, buf.oldBranchProxy, buf.oldBranchChildSibdex, buf.oldLeafProxy))
				{
				treeEntryBufferFlushOutputEntry(&buf);
				treeEntryBufferFlushOutputArrays(&buf);

				treeEntryBufferUnbindEntries(&buf);

				if(!getNextLeafSibling(buf.treeProxy, &buf.oldBranchProxy, &buf.oldBranchChildSibdex, &buf.oldLeafProxy))
					LOG(LOG_CRITICAL,"No more leaves");

//				treeEntryBufferPollInput(&buf);

				//LOG(LOG_INFO,"At end of leaf, unbound");
				}
			}


		if(buf.oldEntryArraysPtr==NULL)
			{
			if(hasMoreSiblings(buf.treeProxy, buf.oldBranchProxy, buf.oldBranchChildSibdex, buf.oldLeafProxy))
				{
				//LOG(LOG_INFO,"Skipping");
				treeEntryBufferSkipOldLeaf_targetUpstreamAndOffset(&buf, targetUpstream, minEdgePosition); // Skip leaves if possible (which may be still semi-packed)
				}

			treeEntryBufferBindEntries(&buf);
			treeEntryBufferPollInput(&buf);
			}


//		LOG(LOG_INFO,"PostSeek Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);

		//int upstreamEdgeOffset=buf.accumUpstreamOffsets[targetUpstream];

		while(buf.oldWidth && buf.oldUpstream<targetUpstream) // Skip lower upstream
			{
//			LOG(LOG_INFO,"Loop 1");

			s32 width=buf.oldWidth;
			buf.accumUpstreamOffsets[buf.oldUpstream]+=width;
			buf.accumDownstreamOffsets[buf.oldDownstream]+=width;

			treeEntryBufferTransfer(&buf);

//			LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);
			}

//		LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);

		while(buf.oldWidth && buf.oldUpstream==targetUpstream &&
			((buf.accumUpstreamOffsets[targetUpstream]+buf.oldWidth)<minEdgePosition ||
			((buf.accumUpstreamOffsets[targetUpstream]+buf.oldWidth)==minEdgePosition && buf.oldDownstream!=targetDownstream))) // Skip earlier upstream
			{
//			LOG(LOG_INFO,"Loop 2");

			s32 width=buf.oldWidth;
			buf.accumUpstreamOffsets[buf.oldUpstream]+=width;
			buf.accumDownstreamOffsets[buf.oldDownstream]+=width;

			treeEntryBufferTransfer(&buf);

//			LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);
			}

//      LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);

		while(buf.oldWidth && buf.oldUpstream==targetUpstream &&
			(buf.accumUpstreamOffsets[buf.oldUpstream]+buf.oldWidth)<=maxEdgePosition && buf.oldDownstream<targetDownstream) // Skip matching upstream with earlier downstream
			{
//			LOG(LOG_INFO,"Loop 3");

			s32 width=buf.oldWidth;
			buf.accumUpstreamOffsets[buf.oldUpstream]+=width;
			buf.accumDownstreamOffsets[buf.oldDownstream]+=width;

			treeEntryBufferTransfer(&buf);

//			LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);
			}

//		LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);

		s32 upstreamEdgeOffset=buf.accumUpstreamOffsets[targetUpstream];
		s32 downstreamEdgeOffset=buf.accumDownstreamOffsets[targetDownstream];

//		LOG(LOG_INFO,"Patch: P: %i S: %i [%i %i]",targetUpstream, targetDownstream, minEdgePosition, maxEdgePosition);

//		LOG(LOG_INFO,"PreMod Entry: %i %i %i [%i] [%i]",buf.oldUpstream, buf.oldDownstream, buf.oldWidth, upstreamEdgeOffset, downstreamEdgeOffset);


		if(buf.oldWidth==0 || buf.oldUpstream>targetUpstream || (buf.oldDownstream!=targetDownstream && upstreamEdgeOffset>=minEdgePosition))
			{
			int minMargin=upstreamEdgeOffset-minEdgePosition; // Margin between current position and minimum position: Must be zero or positive
			int maxMargin=maxEdgePosition-upstreamEdgeOffset; // Margin between current position and maximum position: Must be zero or positive

			if(minMargin<0 || maxMargin<0)
				{
				LOG(LOG_INFO,"Invalid margins for insert");

				rttDumpRoutingTable(builder);

				LOG(LOG_INFO,"Failed to add forward route for prefix %i suffix %i",targetUpstream,targetDownstream);
				LOG(LOG_INFO,"Current edge offset %i minEdgePosition %i maxEdgePosition %i",upstreamEdgeOffset,minEdgePosition,maxEdgePosition);
				LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
				}

			// LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

			// Map offsets to new entry
			patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
			patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset;

			patchPtr++;
			int width=1;

			while(patchPtr<patchGroupPtr) // Remaining entries in group gain width
				{
				patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
				patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+width;

				patchPtr++;
				width++;
				}

			treeEntryBufferPushOutput(&buf, targetUpstream, targetDownstream, width);

			buf.accumUpstreamOffsets[targetUpstream]+=width;
			buf.accumDownstreamOffsets[targetDownstream]+=width;
			}
		else if(buf.oldUpstream==targetUpstream && buf.oldDownstream==targetDownstream) // Existing entry suitable, widen
			{
//			LOG(LOG_INFO,"Widening");

			int upstreamEdgeOffsetEnd=upstreamEdgeOffset+buf.oldWidth;

			// Adjust offsets
			if(minEdgePosition<upstreamEdgeOffset) // Trim upstream range to entry
				minEdgePosition=upstreamEdgeOffset;

			if(maxEdgePosition>upstreamEdgeOffsetEnd) // Trim upstream range to entry
				maxEdgePosition=upstreamEdgeOffsetEnd;

			int minOffset=minEdgePosition-upstreamEdgeOffset; // Offset of minimum position: zero or positive
			int maxOffset=maxEdgePosition-upstreamEdgeOffset; // Offset of maximum position: zero or positive

			if(minOffset<0 || maxOffset<0 || minOffset>maxOffset)
				{
				LOG(LOG_INFO,"Invalid margins for widen");

				LOG(LOG_INFO,"OldEntry Offset Range: %i %i", upstreamEdgeOffset, upstreamEdgeOffsetEnd);
				LOG(LOG_INFO,"Min / Max position: %i %i", minEdgePosition, maxEdgePosition);

				LOG(LOG_CRITICAL,"Invalid offsets or gap detected in route insert - Min: %i Max: %i",minOffset,maxOffset);
				}

	//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

			// Map offsets to new entry
			patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset+minOffset;
			patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+maxOffset;

//			LOG(LOG_INFO,"Handoff %i %i to RDI: %p",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset, (*(patchPtr->rdiPtr)));

			patchPtr++;
			int width=1;

			while(patchPtr<patchGroupPtr) // Remaining entries in group gain width
				{
				patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset+minOffset;
				//(*(patchPtr->rdiPtr))->maxEdgePosition=downstreamEdgeOffset+width;
				patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+maxOffset+width;

//				LOG(LOG_INFO,"Handoff %i %i to RDI: %p",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

				patchPtr++;
				width++;
				}

			treeEntryBufferPushOutput(&buf,  targetUpstream, targetDownstream, width);
			s32 trans=treeEntryBufferPartialTransfer(&buf, maxOffset);
			if(trans!=maxOffset)
				LOG(LOG_CRITICAL,"Widening transfer size mismatch %i %i",maxOffset, trans);

//			LOG(LOG_INFO,"Post transfer: Min %i Max %i",minOffset, maxOffset);

			buf.accumUpstreamOffsets[targetUpstream]+=maxOffset+width;
			buf.accumDownstreamOffsets[targetDownstream]+=maxOffset+width;
			}
		else // Existing entry unsuitable, split and insert
			{
			int targetEdgePosition=buf.oldDownstream>targetDownstream?minEdgePosition:maxEdgePosition; // Early or late split
			int splitWidth1=targetEdgePosition-upstreamEdgeOffset;
			int splitWidth2=buf.oldWidth-splitWidth1;

			if(splitWidth1<=0 || splitWidth2<=0)
				{
				LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i from %i",splitWidth1, splitWidth2);
				}

			//LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

			// Map offsets
			patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
			patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset;

			patchPtr++;
			int width=1;

			while(patchPtr<patchGroupPtr) // Remaining entries in group gain width
				{
				patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
				patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+width;

				patchPtr++;
				width++;
				}

			buf.accumUpstreamOffsets[buf.oldUpstream]+=splitWidth1;
			buf.accumDownstreamOffsets[buf.oldDownstream]+=splitWidth1;

			s32 transWidth1=treeEntryBufferPartialTransfer(&buf, splitWidth1);

			if(transWidth1!=splitWidth1)
				LOG(LOG_CRITICAL,"Split transfer size mismatch %i %i",splitWidth1, transWidth1);

			treeEntryBufferPushOutput(&buf, targetUpstream, targetDownstream, width);

			buf.accumUpstreamOffsets[targetUpstream]+=width;
			buf.accumDownstreamOffsets[targetDownstream]+=width;
			}

		}


//	LOG(LOG_INFO,"Transfer remainder");

//	while(buf.oldWidth) // Copy remaining old entries
		//treeEntryBufferTransfer(&buf);

	treeEntryBufferTransferLeafRemainder(&buf); // Copy remaining old entries in this leaf
	treeEntryBufferFlushOutputEntry(&buf);
	treeEntryBufferFlushOutputArrays(&buf);

//	*maxWidth=MAX(*maxWidth,buf.maxWidth);



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
	}
*/


/* No buffer version

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

//	LOG(LOG_INFO,"Block has %i",walker->leafProxy->unpackedBlock->entryArrayCount);
//	LOG(LOG_INFO,"Array %i Entry %i", walker->leafArrayIndex, walker->leafEntryIndex);


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

*/


static void rttMergeRoutes_ordered_reverseMulti(RouteTableTreeBuilder *builder, RoutePatch *patchPtr, RoutePatch *patchPtrEnd,
		s32 prefixCount, s32 suffixCount)// int *maxWidth)
{
	RouteTableTreeEntryBuffer buf;
	treeEntryBufferInit(&buf, &(builder->reverseProxy), suffixCount, prefixCount);

//	LOG(LOG_INFO,"rttMergeRoutes_ordered_forwardMulti: Patch count %i",(patchPtrEnd-patchPtr));

	while(patchPtr<patchPtrEnd)
	{
		s32 targetUpstream=patchPtr->suffixIndex;
		s32 targetDownstream=patchPtr->prefixIndex;
		s32 minEdgePosition=patchPtr->rdiPtr->minEdgePosition;
		s32 maxEdgePosition=patchPtr->rdiPtr->maxEdgePosition;

//		LOG(LOG_INFO,"Patch: P: %i S: %i [%i %i]",targetUpstream, targetDownstream, minEdgePosition, maxEdgePosition);

		int expectedMaxEdgePosition=maxEdgePosition+1;
		RoutePatch *patchGroupPtr=patchPtr+1;  // Make groups of compatible inserts for combined processing

		if(minEdgePosition!=TR_INIT_MINEDGEPOSITION || maxEdgePosition!=TR_INIT_MAXEDGEPOSITION)
			{
			while(patchGroupPtr<patchPtrEnd && patchGroupPtr->suffixIndex==targetUpstream && patchGroupPtr->prefixIndex==targetDownstream &&
					patchGroupPtr->rdiPtr->minEdgePosition == minEdgePosition && patchGroupPtr->rdiPtr->maxEdgePosition == expectedMaxEdgePosition)
				{
				patchGroupPtr++;
				expectedMaxEdgePosition++;
				}
			}
		else
			{
			while(patchGroupPtr<patchPtrEnd && patchGroupPtr->suffixIndex==targetUpstream && patchGroupPtr->prefixIndex==targetDownstream &&
					patchGroupPtr->rdiPtr->minEdgePosition == TR_INIT_MINEDGEPOSITION && patchGroupPtr->rdiPtr->maxEdgePosition == TR_INIT_MAXEDGEPOSITION)
				patchGroupPtr++;
			}
/*
		int groupLen=patchGroupPtr-patchPtr;
		if(groupLen>1)
			LOG(LOG_INFO,"GROUPED %i",groupLen);
*/

		/*
		// Handle already bound case here
		if(buf.oldEntryArraysPtr!=NULL)
			{
			LOG(LOG_CRITICAL,"Leaf already bound before seek");
			}

		treeEntryBufferAdvanceOldLeafToUpstream(&buf, targetUpstream, minEdgePosition); // Skip leaves if possible (which may be still partly packed)

		treeEntryBufferBindEntries(&buf);
		treeEntryBufferPollInput(&buf);
*/

/*
		if(buf.oldEntryArraysPtr==NULL)
			{
			if(hasMoreSiblings(buf.treeProxy, buf.oldBranchProxy, buf.oldBranchChildSibdex, buf.oldLeafProxy))
				treeEntryBufferSkipOldLeaf_targetUpstreamAndOffset(&buf, targetUpstream, minEdgePosition); // Skip leaves if possible (which may be still semi-packed)

			treeEntryBufferBindEntries(&buf);
			treeEntryBufferPollInput(&buf);
			}
*/


		if(buf.oldEntryArraysPtr!=NULL) // Currently bound, but can perhaps unbind after current leaf
			{
			while(buf.oldWidth && buf.oldUpstream<targetUpstream) // Skip lower upstream
				{
	//			LOG(LOG_INFO,"Loop 1");

				s32 width=buf.oldWidth;
				buf.accumUpstreamOffsets[buf.oldUpstream]+=width;
				buf.accumDownstreamOffsets[buf.oldDownstream]+=width;

				treeEntryBufferTransfer_sameLeaf(&buf);

	//			LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);
				}

			while(buf.oldWidth && buf.oldUpstream==targetUpstream &&
				((buf.accumUpstreamOffsets[targetUpstream]+buf.oldWidth)<minEdgePosition ||
				((buf.accumUpstreamOffsets[targetUpstream]+buf.oldWidth)==minEdgePosition && buf.oldDownstream!=targetDownstream))) // Skip earlier upstream
				{
	//			LOG(LOG_INFO,"Loop 2");

				s32 width=buf.oldWidth;
				buf.accumUpstreamOffsets[buf.oldUpstream]+=width;
				buf.accumDownstreamOffsets[buf.oldDownstream]+=width;

				treeEntryBufferTransfer_sameLeaf(&buf);
				}

			if(!buf.oldWidth && hasMoreSiblings(buf.treeProxy, buf.oldBranchProxy, buf.oldBranchChildSibdex, buf.oldLeafProxy))
				{
				treeEntryBufferFlushOutputEntry(&buf);
				treeEntryBufferFlushOutputArrays(&buf);

				treeEntryBufferUnbindEntries(&buf);

				if(!getNextLeafSibling(buf.treeProxy, &buf.oldBranchProxy, &buf.oldBranchChildSibdex, &buf.oldLeafProxy))
					LOG(LOG_CRITICAL,"No more leaves");

//				treeEntryBufferPollInput(&buf);

				//LOG(LOG_INFO,"At end of leaf, unbound");
				}
			}

		if(buf.oldEntryArraysPtr==NULL)
			{
			if(hasMoreSiblings(buf.treeProxy, buf.oldBranchProxy, buf.oldBranchChildSibdex, buf.oldLeafProxy))
				{
				//LOG(LOG_INFO,"Skipping");
				treeEntryBufferSkipOldLeaf_targetUpstreamAndOffset(&buf, targetUpstream, minEdgePosition); // Skip leaves if possible (which may be still semi-packed)
				}

			treeEntryBufferBindEntries(&buf);
			treeEntryBufferPollInput(&buf);
			}


//		LOG(LOG_INFO,"PostSeek Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);

		//int upstreamEdgeOffset=buf.accumUpstreamOffsets[targetUpstream];

		while(buf.oldWidth && buf.oldUpstream<targetUpstream) // Skip lower upstream
			{
//			LOG(LOG_INFO,"Loop 1");

			s32 width=buf.oldWidth;
			buf.accumUpstreamOffsets[buf.oldUpstream]+=width;
			buf.accumDownstreamOffsets[buf.oldDownstream]+=width;

			treeEntryBufferTransfer(&buf);

//			LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);
			}

//		LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);

		while(buf.oldWidth && buf.oldUpstream==targetUpstream &&
			((buf.accumUpstreamOffsets[targetUpstream]+buf.oldWidth)<minEdgePosition ||
			((buf.accumUpstreamOffsets[targetUpstream]+buf.oldWidth)==minEdgePosition && buf.oldDownstream!=targetDownstream))) // Skip earlier upstream
			{
//			LOG(LOG_INFO,"Loop 2");

			s32 width=buf.oldWidth;
			buf.accumUpstreamOffsets[buf.oldUpstream]+=width;
			buf.accumDownstreamOffsets[buf.oldDownstream]+=width;

			treeEntryBufferTransfer(&buf);

//			LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);
			}

//      LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);

		while(buf.oldWidth && buf.oldUpstream==targetUpstream &&
			(buf.accumUpstreamOffsets[buf.oldUpstream]+buf.oldWidth)<=maxEdgePosition && buf.oldDownstream<targetDownstream) // Skip matching upstream with earlier downstream
			{
//			LOG(LOG_INFO,"Loop 3");

			s32 width=buf.oldWidth;
			buf.accumUpstreamOffsets[buf.oldUpstream]+=width;
			buf.accumDownstreamOffsets[buf.oldDownstream]+=width;

			treeEntryBufferTransfer(&buf);

//			LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);
			}

//		LOG(LOG_INFO,"Entry: %i %i %i",buf.oldUpstream, buf.oldDownstream, buf.oldWidth);

		s32 upstreamEdgeOffset=buf.accumUpstreamOffsets[targetUpstream];
		s32 downstreamEdgeOffset=buf.accumDownstreamOffsets[targetDownstream];

//		LOG(LOG_INFO,"Patch: P: %i S: %i [%i %i]",targetUpstream, targetDownstream, minEdgePosition, maxEdgePosition);

//		LOG(LOG_INFO,"PreMod Entry: %i %i %i [%i] [%i]",buf.oldUpstream, buf.oldDownstream, buf.oldWidth, upstreamEdgeOffset, downstreamEdgeOffset);


		if(buf.oldWidth==0 || buf.oldUpstream>targetUpstream || (buf.oldDownstream!=targetDownstream && upstreamEdgeOffset>=minEdgePosition))
			{
			int minMargin=upstreamEdgeOffset-minEdgePosition; // Margin between current position and minimum position: Must be zero or positive
			int maxMargin=maxEdgePosition-upstreamEdgeOffset; // Margin between current position and maximum position: Must be zero or positive

			if(minMargin<0 || maxMargin<0)
				{
				LOG(LOG_INFO,"Invalid margins for insert");

				rttDumpRoutingTable(builder);

				LOG(LOG_INFO,"Failed to add forward route for prefix %i suffix %i",targetUpstream,targetDownstream);
				LOG(LOG_INFO,"Current edge offset %i minEdgePosition %i maxEdgePosition %i",upstreamEdgeOffset,minEdgePosition,maxEdgePosition);
				LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
				}

			// LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

			// Map offsets to new entry
			patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
			patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset;

			patchPtr++;
			int width=1;

			while(patchPtr<patchGroupPtr) // Remaining entries in group gain width
				{
				patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
				patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+width;

				patchPtr++;
				width++;
				}

			treeEntryBufferPushOutput(&buf, targetUpstream, targetDownstream, width);

			buf.accumUpstreamOffsets[targetUpstream]+=width;
			buf.accumDownstreamOffsets[targetDownstream]+=width;
			}
		else if(buf.oldUpstream==targetUpstream && buf.oldDownstream==targetDownstream) // Existing entry suitable, widen
			{
//			LOG(LOG_INFO,"Widening");

			int upstreamEdgeOffsetEnd=upstreamEdgeOffset+buf.oldWidth;

			// Adjust offsets
			if(minEdgePosition<upstreamEdgeOffset) // Trim upstream range to entry
				minEdgePosition=upstreamEdgeOffset;

			if(maxEdgePosition>upstreamEdgeOffsetEnd) // Trim upstream range to entry
				maxEdgePosition=upstreamEdgeOffsetEnd;

			int minOffset=minEdgePosition-upstreamEdgeOffset; // Offset of minimum position: zero or positive
			int maxOffset=maxEdgePosition-upstreamEdgeOffset; // Offset of maximum position: zero or positive

			if(minOffset<0 || maxOffset<0 || minOffset>maxOffset)
				{
				LOG(LOG_INFO,"Invalid margins for widen");

				LOG(LOG_INFO,"OldEntry Offset Range: %i %i", upstreamEdgeOffset, upstreamEdgeOffsetEnd);
				LOG(LOG_INFO,"Min / Max position: %i %i", minEdgePosition, maxEdgePosition);

				LOG(LOG_CRITICAL,"Invalid offsets or gap detected in route insert - Min: %i Max: %i",minOffset,maxOffset);
				}

	//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

			// Map offsets to new entry
			patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset+minOffset;
			patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+maxOffset;

//			LOG(LOG_INFO,"Handoff %i %i to RDI: %p",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset, (*(patchPtr->rdiPtr)));

			patchPtr++;
			int width=1;

			while(patchPtr<patchGroupPtr) // Remaining entries in group gain width
				{
				patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset+minOffset;
				//(*(patchPtr->rdiPtr))->maxEdgePosition=downstreamEdgeOffset+width;
				patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+maxOffset+width;

//				LOG(LOG_INFO,"Handoff %i %i to RDI: %p",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

				patchPtr++;
				width++;
				}

			treeEntryBufferPushOutput(&buf,  targetUpstream, targetDownstream, width);
			s32 trans=treeEntryBufferPartialTransfer(&buf, maxOffset);
			if(trans!=maxOffset)
				LOG(LOG_CRITICAL,"Widening transfer size mismatch %i %i",maxOffset, trans);

//			LOG(LOG_INFO,"Post transfer: Min %i Max %i",minOffset, maxOffset);

			buf.accumUpstreamOffsets[targetUpstream]+=maxOffset+width;
			buf.accumDownstreamOffsets[targetDownstream]+=maxOffset+width;
			}
		else // Existing entry unsuitable, split and insert
			{
			int targetEdgePosition=buf.oldDownstream>targetDownstream?minEdgePosition:maxEdgePosition; // Early or late split
			int splitWidth1=targetEdgePosition-upstreamEdgeOffset;
			int splitWidth2=buf.oldWidth-splitWidth1;

			if(splitWidth1<=0 || splitWidth2<=0)
				{
				LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i from %i",splitWidth1, splitWidth2);
				}

			//LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

			// Map offsets
			patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
			patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset;

			patchPtr++;
			int width=1;

			while(patchPtr<patchGroupPtr) // Remaining entries in group gain width
				{
				patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
				patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+width;

				patchPtr++;
				width++;
				}

			buf.accumUpstreamOffsets[buf.oldUpstream]+=splitWidth1;
			buf.accumDownstreamOffsets[buf.oldDownstream]+=splitWidth1;

			s32 transWidth1=treeEntryBufferPartialTransfer(&buf, splitWidth1);

			if(transWidth1!=splitWidth1)
				LOG(LOG_CRITICAL,"Split transfer size mismatch %i %i",splitWidth1, transWidth1);

			treeEntryBufferPushOutput(&buf, targetUpstream, targetDownstream, width);

			buf.accumUpstreamOffsets[targetUpstream]+=width;
			buf.accumDownstreamOffsets[targetDownstream]+=width;
			}

		}


//	LOG(LOG_INFO,"Transfer remainder");

//	while(buf.oldWidth) // Copy remaining old entries
//		treeEntryBufferTransfer(&buf);

	treeEntryBufferTransferLeafRemainder(&buf); // Copy remaining old entries in this leaf
	treeEntryBufferFlushOutputEntry(&buf);
	treeEntryBufferFlushOutputArrays(&buf);

//	*maxWidth=MAX(*maxWidth,buf.maxWidth);



}




void rttMergeRoutes(RouteTableTreeBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 prefixCount, s32 suffixCount, u32 *orderedDispatches, MemDispenser *disp)
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
			*(orderedDispatches++)=patchPtr->dispatchLinkIndex;
			patchPtr++;
			}
		}

	// Reverse Routes

	if(reverseRoutePatchCount>0)
		{
		RoutePatch *patchPtr=reverseRoutePatches;

		rttMergeRoutes_ordered_reverseMulti(builder, patchPtr, patchPtr+reverseRoutePatchCount, prefixCount, suffixCount);

		for(int i=0;i<reverseRoutePatchCount;i++)
			{
			*(orderedDispatches++)=patchPtr->dispatchLinkIndex;
			patchPtr++;
			}

		/*
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
			*/

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


void rttUnpackRouteTableForSmerLinked(SmerLinked *smerLinked, RouteTableTreeWalker *forwardWalker, RouteTableTreeWalker *reverseWalker, s64 routeLimit, MemDispenser *disp)
{
	RouteTableUnpackedEntry *unpackedEntry;
	s16 upstream;
	int totalEntryCount;
	RouteTableEntry *routeEntries;

	s64 totalRoutes=0;

	rttwSeekStart(forwardWalker, 1);

	if(forwardWalker->leafEntryArray!=NULL && routeLimit>=0)
		{
		totalEntryCount=forwardWalker->leafEntryArray->entryCount;

		while(rttwAdvanceToNextArray(forwardWalker))
			totalEntryCount+=forwardWalker->leafEntryArray->entryCount;

		routeEntries=dAlloc(disp, sizeof(RouteTableEntry)*totalEntryCount);
		smerLinked->forwardRouteEntries=routeEntries;
		smerLinked->forwardRouteEntryCount=totalEntryCount;

		rttwSeekStart(forwardWalker,1);

		if(rttwGetCurrentEntry(forwardWalker, &upstream, &unpackedEntry))
			{
			do
				{
				routeEntries->prefix=upstream;
				routeEntries->suffix=unpackedEntry->downstream;
				routeEntries->width=unpackedEntry->width;

				totalRoutes+=unpackedEntry->width;

				routeEntries++;
				}
			while(rttwNextEntry(forwardWalker, &upstream, &unpackedEntry));
			}

		if(smerLinked->forwardRouteEntries+smerLinked->forwardRouteEntryCount != routeEntries)
			LOG(LOG_CRITICAL,"Route dump mismatch");
		}
	else
		{
		smerLinked->forwardRouteEntries=NULL;
		smerLinked->forwardRouteEntryCount=0;
		}

	rttwSeekStart(reverseWalker, 1);

	if(reverseWalker->leafEntryArray!=NULL && routeLimit>=0)
		{
		totalEntryCount=reverseWalker->leafEntryArray->entryCount;

		while(rttwAdvanceToNextArray(reverseWalker))
			{
			totalEntryCount+=reverseWalker->leafEntryArray->entryCount;
			}

		routeEntries=dAlloc(disp, sizeof(RouteTableEntry)*totalEntryCount);
		smerLinked->reverseRouteEntries=routeEntries;
		smerLinked->reverseRouteEntryCount=totalEntryCount;

		rttwSeekStart(reverseWalker, 1);

		if(rttwGetCurrentEntry(reverseWalker, &upstream, &unpackedEntry))
			{
			do
				{
				routeEntries->prefix=unpackedEntry->downstream;
				routeEntries->suffix=upstream;
				routeEntries->width=unpackedEntry->width;

				totalRoutes+=unpackedEntry->width;

				routeEntries++;
				}
			while(rttwNextEntry(reverseWalker, &upstream, &unpackedEntry));

			if(smerLinked->reverseRouteEntries+smerLinked->reverseRouteEntryCount != routeEntries)
				LOG(LOG_CRITICAL,"Route dump mismatch");
			}
		}
	else
		{
		smerLinked->reverseRouteEntries=NULL;
		smerLinked->reverseRouteEntryCount=0;
		}

	smerLinked->totalRoutes=totalRoutes;
}


void rttGetStats(RouteTableTreeBuilder *builder,
		s64 *routeTableForwardRouteEntriesPtr, s64 *routeTableForwardRoutesPtr,
		s64 *routeTableReverseRouteEntriesPtr, s64 *routeTableReverseRoutesPtr,
		s64 *routeTableTreeTopBytesPtr,
		s64 *routeTableTreeArrayBytesPtr,
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
			RouteTableTreeLeafProxy *leafProxy=getRouteTableTreeLeafProxy(treeProxies[p], i);
			rttlEnsureFullyUnpacked(treeProxies[p],leafProxy);

			int packedSize=leafProxy->unpackedBlock->packingInfo.oldPackedSize;

			RouteTablePackedSingleBlock *packedBlock=(RouteTablePackedSingleBlock *)(leafProxy->leafBlock->packedBlockData);
			u16 blockHeader=packedBlock->blockHeader;


			int coreSize=2+																// BlockHeader
					leafProxy->unpackedBlock->unpackingInfo.sizePayloadSize+1+ 		// Payload Size
					leafProxy->unpackedBlock->unpackingInfo.sizeUpstreamRange*2+2+ 	// Upstream Range
					leafProxy->unpackedBlock->unpackingInfo.sizeDownstreamRange*2+2+ 	// Downstream Range
					1;																	// Array Count

			s32 sizeOffset=((blockHeader&RTP_PACKEDHEADER_OFFSETSIZE_MASK)>>RTP_PACKEDHEADER_OFFSETSIZE_SHIFT)+1;
			int offsetSize=(leafProxy->unpackedBlock->upstreamOffsetAlloc+leafProxy->unpackedBlock->downstreamOffsetAlloc)*sizeOffset;

			int entrySize=packedSize-coreSize-offsetSize;

			leafBytes+=packedSize;
			leafOffsetBytes+=offsetSize;
			leafEntryBytes+=entrySize;

			s32 leafRoutes=0, leafEntries=0;

			int arrayCount=leafProxy->unpackedBlock->entryArrayCount;

			for(int j=0;j<arrayCount;j++)
				{
				RouteTableUnpackedEntryArray *array=leafProxy->unpackedBlock->entryArrays[j];

				int entryCount=array->entryCount;

				for(int k=0;k<entryCount;k++)
					leafRoutes+=array->entries[k].width;

				leafEntries+=entryCount;
				}

			routeEntries[p]+=leafEntries;
			routes[p]+=leafRoutes;

			}

		// Process branches: Assume Direct
		arrayAlloc=treeProxies[p]->branchArrayProxy.oldDataAlloc;
		arrayBytes+=sizeof(RouteTableTreeArrayBlock)+arrayAlloc*sizeof(u8 *);

		int branchCount=treeProxies[p]->branchArrayProxy.oldDataCount;
		for(int i=0;i<branchCount;i++)
			{
			RouteTableTreeBranchProxy *branchProxy=getRouteTableTreeBranchProxy(treeProxies[p], i);

			branchBytes+=rttbGetRouteTableTreeBranchSize_Existing(branchProxy->dataBlock);
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




