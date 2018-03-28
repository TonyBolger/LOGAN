#include "common.h"








static void createBuildersFromNullData(RoutingComboBuilder *builder)
{
	rtInitHeapDataBlock(&(builder->combinedDataBlock), builder->sliceIndex);

	builder->combinedDataPtr=NULL;

	stInitSeqTailBuilder(&(builder->prefixBuilder), NULL, builder->disp);
	stInitSeqTailBuilder(&(builder->suffixBuilder), NULL, builder->disp);

	builder->arrayBuilder=dAlloc(builder->disp, sizeof(RouteTableArrayBuilder));
	builder->tagBuilder=dAlloc(builder->disp, sizeof(RouteTableTagBuilder));

	builder->treeBuilder=NULL;

	rtaInitRouteTableArrayBuilder(builder->arrayBuilder, NULL, builder->disp);
	rtgInitRouteTableTagBuilder(builder->tagBuilder, NULL, builder->disp);

	builder->upgradedToTree=0;
}





/*
static void dumpRoutingTable(RouteTableBuilder *builder)
{
	rtaDumpRoutingTable(builder->arrayBuilder);
}
*/


static void createBuildersFromDirectData(RoutingComboBuilder *builder)
{
	u8 *data=*(builder->rootPtr);

	s32 headerSize=rtGetGapBlockHeaderSize();
	u8 *afterHeader=data+headerSize;

	rtInitHeapDataBlock(&(builder->combinedDataBlock), builder->sliceIndex);

	builder->combinedDataBlock.headerSize=headerSize;
	builder->combinedDataPtr=data;

	u8 *afterData=afterHeader;
	afterData=stInitSeqTailBuilder(&(builder->prefixBuilder), afterData, builder->disp);
	afterData=stInitSeqTailBuilder(&(builder->suffixBuilder), afterData, builder->disp);

	builder->arrayBuilder=dAlloc(builder->disp, sizeof(RouteTableArrayBuilder));
	builder->tagBuilder=dAlloc(builder->disp, sizeof(RouteTableTagBuilder));

	builder->treeBuilder=NULL;

	afterData=rtaInitRouteTableArrayBuilder(builder->arrayBuilder, afterData, builder->disp);
	afterData=rtgInitRouteTableTagBuilder(builder->tagBuilder, afterData, builder->disp);

	u32 size=afterData-afterHeader;
	builder->combinedDataBlock.dataSize=size;

	builder->upgradedToTree=0;
}


//static
void createBuildersFromIndirectData(RoutingComboBuilder *builder)
{

	//LOG(LOG_INFO,"createBuildersFromIndirectData from top: %p for %i %i",*(builder->rootPtr), builder->sliceTag, builder->sliceIndex);

	u8 *data=*(builder->rootPtr);

	s32 topHeaderSize=rtGetGapBlockHeaderSize();

	builder->arrayBuilder=NULL;
	builder->tagBuilder=NULL;

	rtInitHeapDataBlock(&(builder->combinedDataBlock), builder->sliceIndex);

	builder->combinedDataPtr=NULL;

	builder->treeBuilder=dAlloc(builder->disp, sizeof(RouteTableTreeBuilder));
	rtInitHeapDataBlock(&(builder->topDataBlock), builder->sliceIndex);

	builder->topDataBlock.headerSize=topHeaderSize;
	builder->topDataBlock.dataSize=sizeof(RouteTableTreeTopBlock);

	builder->topDataPtr=data;

	builder->upgradedToTree=0;

	RouteTableTreeBuilder *treeBuilder=builder->treeBuilder;

	treeBuilder->disp=builder->disp;
	treeBuilder->newEntryCount=0;

	RouteTableTreeTopBlock *top=(RouteTableTreeTopBlock *)(data+topHeaderSize);

	for(int i=0;i<ROUTE_TOPINDEX_MAX;i++)
		{
		rtInitHeapDataBlock(treeBuilder->dataBlocks+i, builder->sliceIndex);

		if(top->data[i]!=NULL && (!rtHeaderIsLive(*(top->data[i]))))
			{
			LOG(LOG_CRITICAL,"Entry references dead block");
			}
		}

//	LOG(LOG_INFO,"Parse Indirect: Prefix");
	u8 *prefixBlockData=top->data[ROUTE_TOPINDEX_PREFIX];

	if(prefixBlockData!=NULL)
		{
//		LOG(LOG_INFO,"Begin parse Indirect prefix from %p",prefixBlockData);

		s32 tailHeaderSize=rtDecodeTailBlockHeader(prefixBlockData, NULL, NULL, NULL);
		treeBuilder->dataBlocks[ROUTE_TOPINDEX_PREFIX].headerSize=tailHeaderSize;
		u8 *prefixData=prefixBlockData+tailHeaderSize;
		u8 *prefixDataEnd=stInitSeqTailBuilder(&(builder->prefixBuilder), prefixData, builder->disp);
		treeBuilder->dataBlocks[ROUTE_TOPINDEX_PREFIX].dataSize=prefixDataEnd-prefixData;
		}

//	LOG(LOG_INFO,"Parse Indirect: Suffix");
	u8 *suffixBlockData=top->data[ROUTE_TOPINDEX_SUFFIX];

	if(suffixBlockData!=NULL)
		{
//		LOG(LOG_INFO,"Begin parse Indirect suffix from %p",suffixBlockData);

		s32 tailHeaderSize=rtDecodeTailBlockHeader(suffixBlockData, NULL, NULL, NULL);
		treeBuilder->dataBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize=tailHeaderSize;
		u8 *suffixData=suffixBlockData+tailHeaderSize;
		u8 *suffixDataEnd=stInitSeqTailBuilder(&(builder->suffixBuilder), suffixData, builder->disp);
		treeBuilder->dataBlocks[ROUTE_TOPINDEX_SUFFIX].dataSize=suffixDataEnd-suffixData;
		}

//	LOG(LOG_INFO,"Parse Indirect: Arrays");

	for(int i=ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0; i<=ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0;i++)
		{
		u8 *arrayBlockData=top->data[i];

		if(arrayBlockData!=NULL)
			{
			treeBuilder->dataBlocks[i].headerSize=rtDecodeIndexedBlockHeader_0(arrayBlockData, &(treeBuilder->dataBlocks[i].variant), NULL, NULL);
			}
		}

//	LOG(LOG_INFO,"Parse Indirect: Building");

	rttInitRouteTableTreeBuilder(treeBuilder, top);

//	LOG(LOG_INFO,"Parse Indirect: Done");
}


static void writeBuildersAsDirectData(RoutingComboBuilder *builder, s8 sliceTag, s32 sliceIndex, MemHeap *heap)
{
	if(!(stGetSeqTailBuilderDirty(&(builder->prefixBuilder)) ||
		 stGetSeqTailBuilderDirty(&(builder->suffixBuilder)) ||
		 rtaGetRouteTableArrayBuilderDirty(builder->arrayBuilder) ||
		 rtgGetRouteTableTagBuilderDirty(builder->tagBuilder) ))
		{
		LOG(LOG_INFO,"Nothing to write");
		return;
		}

	int oldTotalSize=builder->combinedDataBlock.headerSize+builder->combinedDataBlock.dataSize;

	int prefixPackedSize=stGetSeqTailBuilderPackedSize(&(builder->prefixBuilder));
	int suffixPackedSize=stGetSeqTailBuilderPackedSize(&(builder->suffixBuilder));
	int routeTablePackedSize=rtaGetRouteTableArrayBuilderPackedSize(builder->arrayBuilder);
	int routeTableTagPackedSize=rtgGetRouteTableTagBuilderPackedSize(builder->tagBuilder);

	int totalSize=rtGetGapBlockHeaderSize()+prefixPackedSize+suffixPackedSize+routeTablePackedSize+routeTableTagPackedSize;

	if(totalSize>65535)
		{
		int oldShifted=oldTotalSize>>16;
		int newShifted=totalSize>>16;

		if(oldShifted!=newShifted)
			{
			LOG(LOG_INFO,"LARGE ALLOC: %i",totalSize);
			}

		}

	u8 *oldData=builder->combinedDataPtr;
	u8 *newData;

	// Mark old block as dead
	mhFree(heap, oldData, oldTotalSize);

	s32 oldTagOffset=0;

	if(totalSize>1000000)
		LOG(LOG_INFO,"writeDirect: CircAlloc %i",totalSize);

	newData=mhAlloc(heap, totalSize, sliceTag, sliceIndex, &oldTagOffset);

	//LOG(LOG_INFO,"Offset Diff: %i for %i",offsetDiff,sliceIndex);

	if(newData==NULL)
		{
		LOG(LOG_CRITICAL,"Failed at alloc after compact: Wanted %i",totalSize);
		}

	//LOG(LOG_INFO,"Direct write to %p %i",newData,totalSize);

	s32 diff=sliceIndex-oldTagOffset;

	*(builder->rootPtr)=newData;

	rtEncodeGapDirectBlockHeader(diff, newData);

	newData+=rtGetGapBlockHeaderSize();

	newData=stWriteSeqTailBuilderPackedData(&(builder->prefixBuilder), newData);
	newData=stWriteSeqTailBuilderPackedData(&(builder->suffixBuilder), newData);
	newData=rtaWriteRouteTableArrayBuilderPackedData(builder->arrayBuilder, newData);
	newData=rtgWriteRouteTableTagBuilderPackedData(builder->tagBuilder, newData);

}

static int considerUpgradingToTree(RoutingComboBuilder *builder, int newForwardRoutes, int newReverseRoutes)
{
	if(builder->arrayBuilder==NULL)
		return 0;

	if(builder->treeBuilder!=NULL)
		return 0;

	int existingRoutes=(builder->arrayBuilder->oldForwardEntryCount)+(builder->arrayBuilder->oldReverseEntryCount);
	int totalRoutes=existingRoutes;//+newForwardRoutes+newReverseRoutes; // Can't tell if new routes are separate entries

	return totalRoutes>ROUTING_TREE_THRESHOLD;
}


static void upgradeToTree(RoutingComboBuilder *builder, s32 prefixCount, s32 suffixCount)
{
	//LOG(LOG_INFO,"upgradeToTree");

	RouteTableTreeBuilder *treeBuilder=dAlloc(builder->disp, sizeof(RouteTableTreeBuilder));
	builder->treeBuilder=treeBuilder;

	builder->topDataPtr=NULL;
	rtInitHeapDataBlock(&(builder->topDataBlock), builder->sliceIndex);

	rttUpgradeToRouteTableTreeBuilder(builder->arrayBuilder,  builder->treeBuilder, builder->sliceIndex, prefixCount, suffixCount, builder->disp);

	builder->upgradedToTree=1;
}



static RouteTableTreeTopBlock *writeBuildersAsIndirectData_writeTop(RoutingComboBuilder *routingBuilder, s8 sliceTag, s32 sliceIndex, MemHeap *heap)
{
	if(routingBuilder->topDataPtr==NULL)
		{
		routingBuilder->topDataBlock.dataSize=sizeof(RouteTableTreeTopBlock);
		routingBuilder->topDataBlock.headerSize=rtGetGapBlockHeaderSize();

		s32 totalSize=routingBuilder->topDataBlock.dataSize+routingBuilder->topDataBlock.headerSize;

		s32 oldTagOffset=0;

		if(totalSize>1000000)
			LOG(LOG_INFO,"writeIndirectTop: CircAlloc %i",totalSize);

		u8 *newTopData=mhcAlloc(&(heap->circ), totalSize, sliceTag, sliceIndex, &oldTagOffset);
		s32 diff=sliceIndex-oldTagOffset;

		rtEncodeGapTopBlockHeader(diff, newTopData);

		u8 *newTop=newTopData+routingBuilder->topDataBlock.headerSize;
		memset(newTop,0,sizeof(RouteTableTreeTopBlock));

		*(routingBuilder->rootPtr)=newTopData;

		return (RouteTableTreeTopBlock *)newTop;
		}
	else
		return (RouteTableTreeTopBlock *)(routingBuilder->topDataPtr+routingBuilder->topDataBlock.headerSize);
}

static void writeBuildersAsIndirectData_calcIndirectArraySpace(HeapDataBlock *heapDataBlock, s16 indexSize, s32 entries)
{

	heapDataBlock->midHeaderSize=rtGetIndexedBlockHeaderSize_1(indexSize);
	heapDataBlock->completeMidCount=rttaCalcFirstLevelArrayCompleteEntries(entries);

	heapDataBlock->completeMidDataSize=rttaCalcArraySize(ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES);

	int partialEntries=rttaCalcFirstLevelArrayAdditionalEntries(entries);
	heapDataBlock->partialMidDataSize=(partialEntries>0) ? rttaCalcArraySize(partialEntries) : 0;

	//LOG(LOG_INFO,"Partial entries of %i require %i",partialEntries, heapDataBlock->partialMidDataSize);
}


static int writeBuildersAsIndirectData_calcTailAndArraySpace(RoutingComboBuilder *routingBuilder, s16 indexSize, HeapDataBlock *neededBlocks)
{
//	LOG(LOG_INFO,"writeBuildersAsIndirectData_calcTailAndArraySpace");

	neededBlocks[ROUTE_TOPINDEX_PREFIX].headerSize=rtGetTailBlockHeaderSize(indexSize);
	neededBlocks[ROUTE_TOPINDEX_PREFIX].dataSize=stGetSeqTailBuilderPackedSize(&(routingBuilder->prefixBuilder));

	neededBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize=rtGetTailBlockHeaderSize(indexSize);
	neededBlocks[ROUTE_TOPINDEX_SUFFIX].dataSize=stGetSeqTailBuilderPackedSize(&(routingBuilder->suffixBuilder));

	RouteTableTreeBuilder *treeBuilder=routingBuilder->treeBuilder;

	for(int i=ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0; i<=ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0;i++)
		{
		neededBlocks[i].headerSize=rtGetIndexedBlockHeaderSize_0(indexSize);

		RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);
	//	LOG(LOG_INFO,"Previous indirect mode %i vs %i for array %i",arrayProxy->heapDataBlock->variant,treeBuilder->dataBlocks[i].variant, i);

		//LOG(LOG_INFO,"Array %i had %i, now has %i",i,arrayProxy->dataCount,arrayProxy->newDataCount);

		s32 entries=rttaGetArrayEntries(arrayProxy);

		if(entries>ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES) // Using indirect mode
			{
			if(i<ROUTE_TOPINDEX_FORWARD_LEAF_ARRAY_0)
				{
				LOG(LOG_CRITICAL,"Needed to use indirect mode for branches: %i",entries);
				}

//			LOG(LOG_INFO,"Indirect mode due to %i entries",entries);

			neededBlocks[i].variant=1;
			writeBuildersAsIndirectData_calcIndirectArraySpace(neededBlocks+i, indexSize, entries);

			if(arrayProxy->heapDataBlock->variant)
				{
//				LOG(LOG_INFO,"Previous indirect mode %i",arrayProxy->heapDataBlock->variant);
				writeBuildersAsIndirectData_calcIndirectArraySpace(arrayProxy->heapDataBlock, indexSize, arrayProxy->oldDataAlloc);
				}

			}

		neededBlocks[i].dataSize=rttGetTopArraySize(arrayProxy);

//		if(neededBlocks[i].variant)
//			LOG(LOG_INFO,"Array %i - Entries %i %i - Sizes %i %i",i, arrayProxy->oldDataAlloc, arrayProxy->newDataAlloc, neededBlocks[i].headerSize, neededBlocks[i].dataSize);
		}

	int totalNeededSize=0;

	for(int i=0;i<ROUTE_TOPINDEX_MAX;i++)
		{
		s32 existingSize=treeBuilder->dataBlocks[i].headerSize+treeBuilder->dataBlocks[i].dataSize;
		s32 neededSize=neededBlocks[i].headerSize+neededBlocks[i].dataSize;

		if(neededBlocks[i].variant==0) // Direct
			{
			if(existingSize!=neededSize)
				totalNeededSize+=neededSize;

			if(treeBuilder->dataBlocks[i].variant!=0)
				LOG(LOG_CRITICAL,"Cannot downgrade from indirect to direct arrays");
			}
		else // Indirect blocks in play
			{
			if((existingSize!=neededSize) || (treeBuilder->dataBlocks[i].variant==0)) // Always new ptr block if upgrading
				totalNeededSize+=neededSize;

			int additionalCompleteBlocks=neededBlocks[i].completeMidCount-treeBuilder->dataBlocks[i].completeMidCount;
			totalNeededSize+=(neededBlocks[i].midHeaderSize+neededBlocks[i].completeMidDataSize)*additionalCompleteBlocks;

//			LOG(LOG_INFO,"Indirect array: adding %i (%i to %i) complete blocks of %i %i",
//					additionalCompleteBlocks, treeBuilder->dataBlocks[i].completeMidCount, neededBlocks[i].completeMidCount,
//					neededBlocks[i].midHeaderSize, neededBlocks[i].completeMidDataSize);

			if(neededBlocks[i].partialMidDataSize>0)
				{
				int existingAdditionalSize=treeBuilder->dataBlocks[i].midHeaderSize+treeBuilder->dataBlocks[i].partialMidDataSize;
				int neededAdditionalSize=neededBlocks[i].midHeaderSize+neededBlocks[i].partialMidDataSize;

				if(existingAdditionalSize!=neededAdditionalSize || additionalCompleteBlocks)
					{
//					LOG(LOG_INFO,"Indirect array: adding incomplete block of %i %i with %i entries", neededBlocks[i].midHeaderSize, neededBlocks[i].partialMidDataSize);

					totalNeededSize+=neededAdditionalSize;
					}
				}
			}

//		LOG(LOG_INFO,"Array %i has %i %i, needs %i %i",i,
//				treeBuilder->dataBlocks[i].headerSize,treeBuilder->dataBlocks[i].dataSize, neededBlocks[i].headerSize,neededBlocks[i].dataSize);
		}

	return totalNeededSize;
}

static u8 *writeBuildersAsIndirectData_bindTails(RouteTableTreeBuilder *treeBuilder, s32 sliceIndex, s16 indexSize,
		HeapDataBlock *neededBlocks, RouteTableTreeTopBlock *topPtr, u8 *newArrayData, MemHeap *heap)
{
	s32 existingSize=0;
	s32 neededSize=0;

	// Handle Prefix Resize

	existingSize=treeBuilder->dataBlocks[ROUTE_TOPINDEX_PREFIX].headerSize+treeBuilder->dataBlocks[ROUTE_TOPINDEX_PREFIX].dataSize;
	neededSize=neededBlocks[ROUTE_TOPINDEX_PREFIX].headerSize+neededBlocks[ROUTE_TOPINDEX_PREFIX].dataSize;

	if(existingSize<neededSize)
		{
		rtEncodeTailBlockHeader(0, indexSize, sliceIndex, newArrayData);
		u8 *oldData=topPtr->data[ROUTE_TOPINDEX_PREFIX];

		topPtr->data[ROUTE_TOPINDEX_PREFIX]=newArrayData;

//			LOG(LOG_INFO,"Prefix Move/Expand %p to %p",oldData,newArrayData);
		newArrayData+=neededBlocks[ROUTE_TOPINDEX_PREFIX].headerSize+neededBlocks[ROUTE_TOPINDEX_PREFIX].dataSize;

		mhcFree(&(heap->circ), oldData, existingSize);
		}
	else
		{
		if(existingSize>neededSize)
			LOG(LOG_CRITICAL,"Unexpected reduction in size %i to %i",existingSize,neededSize);

//			LOG(LOG_INFO,"Prefix Rewrite in place %p",topPtr->data[ROUTE_TOPINDEX_PREFIX]);
		}

	// Handle Suffix Resize

	existingSize=treeBuilder->dataBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize+treeBuilder->dataBlocks[ROUTE_TOPINDEX_SUFFIX].dataSize;
	neededSize=neededBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize+neededBlocks[ROUTE_TOPINDEX_SUFFIX].dataSize;

	if(existingSize<neededSize)
		{
		rtEncodeTailBlockHeader(1, indexSize, sliceIndex, newArrayData);
		u8 *oldData=topPtr->data[ROUTE_TOPINDEX_SUFFIX];

//			LOG(LOG_INFO,"Suffix Move/Expand %p to %p",oldData,newArrayData);
		topPtr->data[ROUTE_TOPINDEX_SUFFIX]=newArrayData;
		newArrayData+=neededBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize+neededBlocks[ROUTE_TOPINDEX_SUFFIX].dataSize;

		mhcFree(&(heap->circ), oldData, existingSize);
		}
	else
		{
		if(existingSize>neededSize)
			LOG(LOG_CRITICAL,"Unexpected reduction in size %i to %i",existingSize,neededSize);

//			LOG(LOG_INFO,"Suffix Rewrite in place %p",topPtr->data[ROUTE_TOPINDEX_SUFFIX]);
		}


	return newArrayData;
}


static RouteTableTreeArrayBlock *writeBuildersAsIndirectData_createOrExpandLeafArray0(u8 **heapData, s32 totalEntries,
		s32 topIndex, s32 indexSize, s32 sliceIndex, u8 *oldArrayDataRaw, MemHeap *heap)
{
	u8 *newArrayData=*heapData;

	s32 headerSize=rtEncodeArrayBlockHeader_Leaf0(topIndex&1, 1, indexSize, sliceIndex, newArrayData);

	newArrayData+=headerSize;

	RouteTableTreeArrayBlock *arrayDataBlock=(RouteTableTreeArrayBlock *)newArrayData;

	int oldEntriesToCopy=0;
	if(oldArrayDataRaw!=NULL)
		{
		RouteTableTreeArrayBlock *oldArrayDataBlock=(RouteTableTreeArrayBlock *)(oldArrayDataRaw+headerSize);
		oldEntriesToCopy=oldArrayDataBlock->dataAlloc;

		memcpy(arrayDataBlock->data, oldArrayDataBlock->data, oldEntriesToCopy*sizeof(u8 *));

		mhcFree(&(heap->circ), oldArrayDataRaw, headerSize+oldEntriesToCopy);
		}

	int newEntries=(totalEntries-oldEntriesToCopy);

	if(oldEntriesToCopy<totalEntries)
		memset(arrayDataBlock->data+oldEntriesToCopy, 0, newEntries*sizeof(u8 *));

	arrayDataBlock->dataAlloc=totalEntries;

	newArrayData+=rttaCalcArraySize(totalEntries);
	*heapData=newArrayData;

	return arrayDataBlock;
}


static s32 writeBuildersAsIndirectData_createAndCopyPartialDirectLeafArray(u8 **heapData, s32 totalEntries,
		s32 topIndex, s32 indexSize, s32 sliceIndex, s32 subIndex,
		u8 *oldDataArray[], s32 oldDataOffset, s32 oldDataCount)
{
	u8 *newArrayData=*heapData;

	s32 headerSize=rtEncodeArrayBlockHeader_Leaf1(topIndex&0x1, indexSize, sliceIndex, subIndex, newArrayData);
	newArrayData+=headerSize;

	RouteTableTreeArrayBlock *arrayDataBlock=(RouteTableTreeArrayBlock *)newArrayData;

	int oldDataToCopy=MIN(oldDataCount-oldDataOffset, totalEntries);
	if(oldDataToCopy>0)
		memcpy(arrayDataBlock->data, oldDataArray+oldDataOffset, oldDataToCopy*sizeof(u8 *));

	if(oldDataToCopy<totalEntries)
		memset(arrayDataBlock->data+oldDataToCopy, 0, (totalEntries-oldDataToCopy)*sizeof(u8 *));

	arrayDataBlock->dataAlloc=totalEntries;

	newArrayData+=rttaCalcArraySize(totalEntries);
	*heapData=newArrayData;

	return oldDataToCopy;
}


static void writeBuildersAsIndirectData_expandAndCopyLeafArray1(u8 **heapData, s32 totalEntries,
		s32 topIndex, s32 indexSize, s32 sliceIndex, s32 subIndex, u8 *oldArrayDataRaw, MemHeap *heap)
{
	u8 *newArrayData=*heapData;

	newArrayData+=rtEncodeArrayBlockHeader_Leaf1(topIndex&0x1, indexSize, sliceIndex, subIndex, newArrayData);

	RouteTableTreeArrayBlock *arrayDataBlock=(RouteTableTreeArrayBlock *)newArrayData;

	int oldEntriesToCopy=0;
	if(oldArrayDataRaw!=NULL)
		{
		int headerSize=rtGetIndexedBlockHeaderSize_1(indexSize);
		RouteTableTreeArrayBlock *oldArrayDataBlock=(RouteTableTreeArrayBlock *)(oldArrayDataRaw+headerSize);
		oldEntriesToCopy=oldArrayDataBlock->dataAlloc;

		if(oldEntriesToCopy>totalEntries)
			LOG(LOG_CRITICAL,"Unexpected array shrinkage %i to %i", oldEntriesToCopy, totalEntries);

		memcpy(arrayDataBlock->data, oldArrayDataBlock->data, oldEntriesToCopy*sizeof(u8 *));

		mhcFree(&(heap->circ), oldArrayDataRaw, headerSize+oldEntriesToCopy);
		}

	int newEntries=(totalEntries-oldEntriesToCopy);

	if(oldEntriesToCopy<totalEntries)
		memset(arrayDataBlock->data+oldEntriesToCopy, 0, newEntries*sizeof(u8 *));

	arrayDataBlock->dataAlloc=totalEntries;

	newArrayData+=rttaCalcArraySize(totalEntries);

	*heapData=newArrayData;
}



static u8 *writeBuildersAsIndirectData_bindArrays(RouteTableTreeBuilder *treeBuilder, s32 sliceIndex, s16 indexSize,
		HeapDataBlock *neededBlocks, RouteTableTreeTopBlock *topPtr, u8 *newArrayData, MemHeap *heap)
{
	s32 existingSize=0;
	s32 neededSize=0;

	for(int i=ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0; i<=ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0;i++)
		{
		if(neededBlocks[i].variant>0) // Indirect mode (should not happen on branches, for now)
			{
			if(i<ROUTE_TOPINDEX_FORWARD_LEAF_ARRAY_0)
				LOG(LOG_CRITICAL,"Unexpected variant encoding of branches");

			if(treeBuilder->dataBlocks[i].variant==0) // Upgrading from direct
				{
				RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);
				rttaBindBlockArrayProxy(arrayProxy, topPtr->data[i], arrayProxy->heapDataBlock->headerSize, arrayProxy->heapDataBlock->variant);

				LOG(LOG_INFO,"Upgrading Leaf Array to 2 level variant");

//				LOG(LOG_INFO,"Before Upgrade: START");
//				dumpBlockArrayProxy(arrayProxy);
//				LOG(LOG_INFO,"Before Upgrade: END");
//				LOG(LOG_INFO,"Indirect raw block at %p",newArrayData);

				int totalEntries=rttaCalcFirstLevelArrayEntries(arrayProxy->newDataAlloc);

				u8 *oldData=topPtr->data[i];
				topPtr->data[i]=newArrayData;

				RouteTableTreeArrayBlock *shallowPtrBlock=writeBuildersAsIndirectData_createOrExpandLeafArray0(&newArrayData, totalEntries,
						i, indexSize, sliceIndex, NULL, heap);

				int oldDataOffset=0;

				for(int j=0;j<neededBlocks[i].completeMidCount;j++) // Build 'full' deep arrays (part or fully init from old data)
					{
					shallowPtrBlock->data[j]=newArrayData;

					oldDataOffset+=writeBuildersAsIndirectData_createAndCopyPartialDirectLeafArray(&newArrayData, ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES,
							i, indexSize, sliceIndex, j, arrayProxy->dataBlock->data, oldDataOffset, arrayProxy->oldDataCount);
					}

				shallowPtrBlock->dataAlloc=neededBlocks[i].completeMidCount;

				if(neededBlocks[i].partialMidDataSize) // Build optional 'partial' deep array (part or fully init from old data)
					{
					shallowPtrBlock->dataAlloc++;
					int additionalIndex=neededBlocks[i].completeMidCount;

					shallowPtrBlock->data[additionalIndex]=newArrayData;

					int additionalEntries=rttaCalcFirstLevelArrayAdditionalEntries(rttaGetArrayEntries(arrayProxy));
					oldDataOffset+=writeBuildersAsIndirectData_createAndCopyPartialDirectLeafArray(&newArrayData, additionalEntries,
							i, indexSize, sliceIndex, additionalIndex, arrayProxy->dataBlock->data, oldDataOffset, arrayProxy->oldDataCount);
					}

				rttaBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, neededBlocks[i].variant); // Updates ptrBlock and dataBlock

				arrayProxy->ptrAlloc=shallowPtrBlock->dataAlloc;
				arrayProxy->ptrCount=shallowPtrBlock->dataAlloc;

				arrayProxy->oldDataAlloc=arrayProxy->newDataAlloc;

				if(oldData!=NULL)
					mhcFree(&(heap->circ), oldData, treeBuilder->dataBlocks[i].headerSize+treeBuilder->dataBlocks[i].dataSize);
				//LOG(LOG_CRITICAL,"Upgradey - has %i", arrayProxy->dataAlloc);

//				LOG(LOG_INFO,"After Upgrade: START");
//				dumpBlockArrayProxy(arrayProxy);
//				LOG(LOG_INFO,"After Upgrade: END");

				}
			else
				{
				RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);
				rttaBindBlockArrayProxy(arrayProxy, topPtr->data[i], arrayProxy->heapDataBlock->headerSize, arrayProxy->heapDataBlock->variant);

//				LOG(LOG_INFO,"Previous Upgrade: START");
//				dumpBlockArrayProxy(arrayProxy);
//				LOG(LOG_INFO,"Previous Upgrade: END");

				existingSize=treeBuilder->dataBlocks[i].headerSize+treeBuilder->dataBlocks[i].dataSize;
				neededSize=neededBlocks[i].headerSize+neededBlocks[i].dataSize;

				if(neededSize>existingSize)
					{
					int totalEntries=rttaCalcFirstLevelArrayEntries(arrayProxy->newDataAlloc);

//					LOG(LOG_INFO,"Non-upgrade indirect (expand top array to %i)",totalEntries);

					u8 *oldDataRaw=topPtr->data[i];
					topPtr->data[i]=newArrayData;

					RouteTableTreeArrayBlock *shallowPtrBlock=writeBuildersAsIndirectData_createOrExpandLeafArray0(&newArrayData, totalEntries,
							i, indexSize, sliceIndex, oldDataRaw, heap);

					shallowPtrBlock->dataAlloc=totalEntries;

					rttaBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, neededBlocks[i].variant);

					arrayProxy->ptrAlloc=totalEntries;
					arrayProxy->ptrCount=totalEntries;
					}
				else if(neededSize<existingSize)
					{
					LOG(LOG_CRITICAL,"Unexpected size reduction, indirect (top array)");
					}

				RouteTableTreeArrayBlock *shallowPtrBlock=arrayProxy->ptrBlock;

				if(neededBlocks[i].completeMidCount>treeBuilder->dataBlocks[i].completeMidCount)
					{
					int incompleteIndex=treeBuilder->dataBlocks[i].completeMidCount;

					if(treeBuilder->dataBlocks[i].partialMidDataSize>0) // First, old incomplete to complete
						{
//						LOG(LOG_INFO,"Non-upgrade indirect (embiggen old partial to complete)");
//						LOG(LOG_INFO,"Indirect data block at %p",newArrayData);

						u8 *oldArrayDataRaw=shallowPtrBlock->data[incompleteIndex];
						shallowPtrBlock->data[incompleteIndex]=newArrayData;

						writeBuildersAsIndirectData_expandAndCopyLeafArray1(&newArrayData, ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES,
								i, indexSize, sliceIndex, incompleteIndex, oldArrayDataRaw, heap);

						incompleteIndex++;

						}

					while(incompleteIndex<neededBlocks[i].completeMidCount)
						{
//						LOG(LOG_INFO,"Non-upgrade indirect (new empty complete)");

						shallowPtrBlock->data[incompleteIndex]=newArrayData;

						writeBuildersAsIndirectData_expandAndCopyLeafArray1(&newArrayData, ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES,
								i, indexSize, sliceIndex, incompleteIndex, NULL, heap);

						incompleteIndex++;
						}

					if(neededBlocks[i].partialMidDataSize>0)
						{
//						LOG(LOG_INFO,"Non-upgrade indirect (new partial)");
//						LOG(LOG_INFO,"Indirect data block at %p",newArrayData);

						int partialEntries=rttaCalcFirstLevelArrayAdditionalEntries(arrayProxy->newDataAlloc);

						shallowPtrBlock->data[incompleteIndex]=newArrayData;
						writeBuildersAsIndirectData_expandAndCopyLeafArray1(&newArrayData, partialEntries,
								i, indexSize, sliceIndex, incompleteIndex, NULL, heap);

						}

//					LOG(LOG_INFO,"Updating alloc from %i to %i", arrayProxy->oldDataAlloc, arrayProxy->newDataAlloc);
					arrayProxy->oldDataAlloc=arrayProxy->newDataAlloc;

//					LOG(LOG_INFO,"Expanded1: START");
//					dumpBlockArrayProxy(arrayProxy);
//					LOG(LOG_INFO,"Expanded1: END");
					}
				else if(neededBlocks[i].completeMidCount<treeBuilder->dataBlocks[i].completeMidCount)
					{
					LOG(LOG_CRITICAL,"Unexpected size reduction, indirect (completeMidCount)");
					}
				else if(neededBlocks[i].partialMidDataSize>treeBuilder->dataBlocks[i].partialMidDataSize)
					{
//					LOG(LOG_INFO,"TODO: Non-upgrade indirect (embiggen old partial)");
//					LOG(LOG_INFO,"Indirect data block at %p",newArrayData);

					int incompleteIndex=treeBuilder->dataBlocks[i].completeMidCount;
					int partialEntries=rttaCalcFirstLevelArrayAdditionalEntries(arrayProxy->newDataAlloc);

					u8 *oldArrayDataRaw=shallowPtrBlock->data[incompleteIndex];

					shallowPtrBlock->data[incompleteIndex]=newArrayData;
					writeBuildersAsIndirectData_expandAndCopyLeafArray1(&newArrayData, partialEntries,
							i, indexSize, sliceIndex, incompleteIndex, oldArrayDataRaw, heap);

//					LOG(LOG_INFO,"Updating alloc from %i to %i", arrayProxy->oldDataAlloc, arrayProxy->newDataAlloc);
					arrayProxy->oldDataAlloc=arrayProxy->newDataAlloc;

//					LOG(LOG_INFO,"Expanded2: START");
//					dumpBlockArrayProxy(arrayProxy);
//					LOG(LOG_INFO,"Expanded2: END");
					}
				else if(neededBlocks[i].partialMidDataSize<treeBuilder->dataBlocks[i].partialMidDataSize)
					{
					LOG(LOG_CRITICAL,"Unexpected size reduction, indirect (partial)");
					}
				}

			}
		else // Direct mode
			{
			existingSize=treeBuilder->dataBlocks[i].headerSize+treeBuilder->dataBlocks[i].dataSize;
			neededSize=neededBlocks[i].headerSize+neededBlocks[i].dataSize;

			s32 sizeDiff=neededSize-existingSize;

			if(sizeDiff>0)
				{
				switch(i)
					{
					case ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0:
						rtEncodeArrayBlockHeader_Branch0(0,indexSize, sliceIndex, newArrayData);
						break;

					case ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0:
						rtEncodeArrayBlockHeader_Branch0(1,indexSize, sliceIndex, newArrayData);
						break;

					case ROUTE_TOPINDEX_FORWARD_LEAF_ARRAY_0:
						rtEncodeArrayBlockHeader_Leaf0(0, 0, indexSize, sliceIndex, newArrayData);
						break;

					case ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0:
						rtEncodeArrayBlockHeader_Leaf0(1, 0, indexSize, sliceIndex, newArrayData);
						break;

					}

				u8 *oldData=topPtr->data[i];

				topPtr->data[i]=newArrayData;
				newArrayData+=neededBlocks[i].headerSize;

				if(oldData!=NULL) // Clear extended region only
					{
					memcpy(newArrayData, oldData+treeBuilder->dataBlocks[i].headerSize, treeBuilder->dataBlocks[i].dataSize);
					memset(newArrayData+treeBuilder->dataBlocks[i].dataSize, 0, neededBlocks[i].dataSize-treeBuilder->dataBlocks[i].dataSize);

					mhcFree(&(heap->circ), oldData, treeBuilder->dataBlocks[i].headerSize+treeBuilder->dataBlocks[i].dataSize);
					}
				else // Clear full region
					{
					memset(newArrayData, 0, neededBlocks[i].dataSize);
					}

				RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);
				rttaBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, 0); // In direct mode, so no problem
				if(arrayProxy->newDataAlloc!=0)
					{
					RouteTableTreeArrayBlock *arrayBlock=(RouteTableTreeArrayBlock *)newArrayData;
					arrayProxy->oldDataAlloc=arrayProxy->newDataAlloc;
					arrayBlock->dataAlloc=arrayProxy->newDataAlloc;
					}

				newArrayData+=neededBlocks[i].dataSize;
				}
			else
				{
				if(existingSize>neededSize)
					LOG(LOG_CRITICAL,"Unexpected reduction in size %i to %i for %i %p",existingSize,neededSize,i,topPtr->data[i]);
				}
			}
		}

	return newArrayData;
}



s32 writeBuildersAsIndirectData_mergeTopArrayUpdates_branch_accumulateSize(RouteTableTreeArrayProxy *branchArrayProxy, int indexSize)
{
	if(branchArrayProxy->newEntries==NULL)
		{
		return 0;
		}

	s32 totalSize=0;

	for(int i=0;i<branchArrayProxy->newEntriesCount;i++)
		{
		int oldBranchChildAlloc=0;
		int subindex=branchArrayProxy->newEntries[i].index;

		int headerSize=(subindex<ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)?rtGetIndexedBlockHeaderSize_1(indexSize):rtGetIndexedBlockHeaderSize_2(indexSize);

		u8 *oldBranchRawData=rttaGetBlockArrayDataEntryRaw(branchArrayProxy, subindex);
		if(oldBranchRawData!=NULL)
			{
			RouteTableTreeBranchBlock *oldBranchData=(RouteTableTreeBranchBlock *)(oldBranchRawData+headerSize);
			oldBranchChildAlloc=oldBranchData->childAlloc;
			}

		RouteTableTreeBranchProxy *newBranchProxy=(RouteTableTreeBranchProxy *)(branchArrayProxy->newEntries[i].proxy);
		RouteTableTreeBranchBlock *newBranchData=newBranchProxy->dataBlock;

		if(newBranchData->childAlloc!=oldBranchChildAlloc)
			totalSize+=headerSize+rttbGetRouteTableTreeBranchSize_Existing(newBranchData);

		}

	return totalSize;
}


void writeBuildersAsIndirectData_mergeTopArrayUpdates_branch(RouteTableTreeArrayProxy *branchArrayProxy, int arrayNum, int indexSize, int sliceIndex,
		u8 *newData, s32 newDataSize, MemHeap *heap)
{
	if(branchArrayProxy->newEntries==NULL)
		return ;

	u8 *endNewData=newData+newDataSize;

//	if(arrayProxy->newData!=NULL)
//		branchArrayProxy->dataBlock->dataAlloc=branchArrayProxy->newDataAlloc;

	for(int i=0;i<branchArrayProxy->newEntriesCount;i++)
		{
		RouteTableTreeBranchBlock *oldBranchData=NULL;
		int oldBranchChildAlloc=0;

		int subindex=branchArrayProxy->newEntries[i].index;
		int headerSize=(subindex<ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)?rtGetIndexedBlockHeaderSize_1(indexSize):rtGetIndexedBlockHeaderSize_2(indexSize);

		u8 *oldBranchRawData=rttaGetBlockArrayDataEntryRaw(branchArrayProxy, subindex);

		if(oldBranchRawData!=NULL)
			{
			oldBranchData=(RouteTableTreeBranchBlock *)(oldBranchRawData+headerSize);
			oldBranchChildAlloc=oldBranchData->childAlloc;
			}

		RouteTableTreeBranchProxy *newBranchProxy=(RouteTableTreeBranchProxy *)(branchArrayProxy->newEntries[i].proxy);
		RouteTableTreeBranchBlock *newBranchData=newBranchProxy->dataBlock;

		if(newBranchData->childAlloc!=oldBranchChildAlloc)
			{
//			LOG(LOG_INFO,"Branch Move/Expand write to %p (%i %i)",newData, newBranchData->childAlloc,oldBranchChildAlloc);

			rttaSetBlockArrayDataEntryRaw(branchArrayProxy, subindex, newData);

			s32 newHeaderSize=rtEncodeEntityBlockHeader_Branch1(arrayNum==ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0,
					indexSize, sliceIndex, subindex>>ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_SHIFT, newData);

			if(newHeaderSize!=headerSize)
				{
				LOG(LOG_CRITICAL,"Header size mismatch %i vs %i",headerSize, newHeaderSize);
				}

			newData+=headerSize;

			s32 dataSize=rttbGetRouteTableTreeBranchSize_Existing(newBranchData);
			memcpy(newData, newBranchData, dataSize);

				//LOG(LOG_INFO,"Copying %i bytes of branch data from %p to %p",dataSize,newBranchData,newData);

			newData+=dataSize;

			mhcFree(&(heap->circ), oldBranchRawData, headerSize+dataSize);
			}
		else
			{
//				LOG(LOG_INFO,"Branch rewrite to %p (%i %i)",arrayProxy->dataBlock->data[i], newBranchData->childAlloc,oldBranchChildAlloc);

			s32 dataSize=rttbGetRouteTableTreeBranchSize_Existing(newBranchData);
			memcpy(branchArrayProxy->dataBlock->data[subindex]+headerSize, newBranchData, dataSize);
			}
		}

	if(endNewData!=newData)
		LOG(LOG_CRITICAL,"New Branch Data doesn't match expected: New: %p vs Expected: %p",newData,endNewData);

}




s32 writeBuildersAsIndirectData_mergeTopArrayUpdates_leaf_accumulateSize(RouteTableTreeArrayProxy *leafArrayProxy, int indexSize)
{

	if(leafArrayProxy->newEntries==NULL)
		{
		return 0;
		}

	s32 totalSize=0;

	for(int i=0;i<leafArrayProxy->newEntriesCount;i++)
		{
		RouteTableTreeLeafProxy *leafProxy=(RouteTableTreeLeafProxy *)leafArrayProxy->newEntries[i].proxy;

		rtpUpdateUnpackedSingleBlockPackingInfo(leafProxy->unpackedBlock);

		RouteTablePackingInfo *packingInfo=&(leafProxy->unpackedBlock->packingInfo);

		if(packingInfo->packedSize!=packingInfo->oldPackedSize)
			{
			int subindex=leafArrayProxy->newEntries[i].index;
			int headerSize=(subindex<ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)?rtGetIndexedBlockHeaderSize_1(indexSize):rtGetIndexedBlockHeaderSize_2(indexSize);

			totalSize+=headerSize+sizeof(RouteTableTreeLeafBlock)+packingInfo->packedSize;
			}
		}

	return totalSize;
}


void writeBuildersAsIndirectData_mergeTopArrayUpdates_leaf(RouteTableTreeArrayProxy *leafArrayProxy, int arrayNum, int indexSize, int sliceIndex,
		u8 *newData, s32 newDataSize, MemHeap *heap)
{

	if(leafArrayProxy->newEntries==NULL)
		return;

	u8 *endNewData=newData+newDataSize;

	//LOG(LOG_CRITICAL,"PackLeaf: writeBuildersAsIndirectData_mergeTopArrayUpdates_leaf TODO");

	//if(arrayProxy->dataBlock->dataAlloc==0)
		//{
		//LOG(LOG_INFO,"New Leaf Array - setting array length to %i",arrayProxy->newDataAlloc);

//	if(arrayProxy->newData!=NULL)
//		arrayProxy->dataBlock->dataAlloc=arrayProxy->newDataAlloc;

		//}

	for(int i=0;i<leafArrayProxy->newEntriesCount;i++)
		{
		int subindex=leafArrayProxy->newEntries[i].index;
		int headerSize=(subindex<ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)?rtGetIndexedBlockHeaderSize_1(indexSize):rtGetIndexedBlockHeaderSize_2(indexSize);

		RouteTableTreeLeafProxy *newLeafProxy=(RouteTableTreeLeafProxy *)(leafArrayProxy->newEntries[i].proxy);

//		dumpLeafProxy(newLeafProxy);

		RouteTablePackingInfo *packingInfo=&(newLeafProxy->unpackedBlock->packingInfo);
		int oldBlockSize=packingInfo->oldPackedSize+headerSize;
		int newBlockSize=packingInfo->packedSize+headerSize;

		//LOG(LOG_CRITICAL,"PackLeaf: writeBuildersAsIndirectData_mergeTopArrayUpdates_leaf Header: %i Old: %i New: %i",headerSize, oldPackedSize, newPackdeSize);

		u8 *oldLeafRawData=rttaGetBlockArrayDataEntryRaw(leafArrayProxy, subindex);

		if(newBlockSize!=oldBlockSize)
			{
//			LOG(LOG_INFO,"Leaf write to %p (%i %i)",newData,  newBlockSize, oldBlockSize);

//			LOG(LOG_INFO,"Leaf Move/Expand write to %p from %p (%i %i)",newData, oldLeafRawData, newLeafSize, oldLeafSize);

			rttaSetBlockArrayDataEntryRaw(leafArrayProxy, subindex, newData);

			s32 newHeaderSize=(subindex<ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)?
					rtEncodeEntityBlockHeader_Leaf1(arrayNum==ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0, indexSize, sliceIndex, subindex>>ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_SHIFT, newData):
					rtEncodeEntityBlockHeader_Leaf2(arrayNum==ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0, indexSize, sliceIndex, subindex>>ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_SHIFT, newData);

			if(newHeaderSize!=headerSize)
				{
				LOG(LOG_CRITICAL,"Header size mismatch %i vs %i",headerSize, newHeaderSize);
				}

			newData+=headerSize;

			RouteTableTreeLeafBlock *leafBlock=(RouteTableTreeLeafBlock *)newData;
			leafBlock->parentBrindex=newLeafProxy->parentBrindex;

			rtpPackSingleBlock(newLeafProxy->unpackedBlock, (RouteTablePackedSingleBlock *)(&(leafBlock->packedBlockData)));

			newData+=sizeof(RouteTableTreeLeafBlock)+packingInfo->packedSize;

			mhcFree(&(heap->circ), oldLeafRawData, oldBlockSize);
			}
		else
			{
//			LOG(LOG_INFO,"Leaf rewrite to %p (%i %i)",newData,  newBlockSize, oldBlockSize);

			RouteTableTreeLeafBlock *leafBlock=(RouteTableTreeLeafBlock *)(oldLeafRawData+headerSize);

			rtpPackSingleBlock(newLeafProxy->unpackedBlock, (RouteTablePackedSingleBlock *)(&(leafBlock->packedBlockData)));
			}

		}

	if(endNewData!=newData)
		LOG(LOG_CRITICAL,"New Leaf Data doesn't match expected: New: %p vs Expected: %p",newData,endNewData);

}




static void writeBuildersAsIndirectData(RoutingComboBuilder *routingBuilder, u8 sliceTag, s32 sliceIndex, MemHeap *heap)
{
	// May need to create:
	//		top level block
	// 		prefix/suffix blocks
	// 		direct array blocks
	//      indirect array blocks
	// 		branch blocks
	//		leaf blocks
	//		offset blocks

//	LOG(LOG_INFO,"writeBuildersAsIndirectData %i %i",sliceTag, sliceIndex);

//	rttDumpRoutingTable(routingBuilder->treeBuilder);

	if(routingBuilder->upgradedToTree)
		{
		u8 *oldData=*(routingBuilder->rootPtr);
		int oldTotalSize=routingBuilder->combinedDataBlock.headerSize+routingBuilder->combinedDataBlock.dataSize;

		mhFree(heap, oldData, oldTotalSize);
		}

	s16 indexSize=vpExtCalcLength(sliceIndex);
	RouteTableTreeBuilder *treeBuilder=routingBuilder->treeBuilder;

	RouteTableTreeTopBlock *topPtr=writeBuildersAsIndirectData_writeTop(routingBuilder, sliceTag, sliceIndex, heap);

	HeapDataBlock neededBlocks[ROUTE_TOPINDEX_MAX];
	for(int i=0;i<ROUTE_TOPINDEX_MAX;i++)
		rtInitHeapDataBlock(neededBlocks+i, sliceIndex);

	int totalNeededSize=writeBuildersAsIndirectData_calcTailAndArraySpace(routingBuilder, indexSize, neededBlocks);

	if(totalNeededSize>0)
		{
		if(totalNeededSize>1000000)
			LOG(LOG_INFO,"writeIndirectTailAndArray: CircAlloc %i",totalNeededSize);

		u8 *newArrayData=mhAlloc(heap, totalNeededSize, sliceTag, INT_MAX, NULL);

		memset(newArrayData,0,totalNeededSize); // Really needed?
		u8 *endArrayData=newArrayData+totalNeededSize;

		topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root after alloc

		// Bind tails to newly allocated blocks
		newArrayData=writeBuildersAsIndirectData_bindTails(treeBuilder, sliceIndex, indexSize, neededBlocks, topPtr, newArrayData, heap);

		// Bind direct/indirect arrays to newly allocated blocks
		newArrayData=writeBuildersAsIndirectData_bindArrays(treeBuilder, sliceIndex, indexSize, neededBlocks, topPtr, newArrayData, heap);

		if(endArrayData!=newArrayData)
			LOG(LOG_CRITICAL,"Array end did not match expected: Found: %p vs Expected: %p",newArrayData, endArrayData);
		}
	//else
//		LOG(LOG_INFO,"Skip tail/array bind since unchanged");

	topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root after alloc

//	LOG(LOG_INFO,"Tails");

	if((routingBuilder->upgradedToTree) || stGetSeqTailBuilderDirty(&(routingBuilder->prefixBuilder)))
		{
//		LOG(LOG_INFO,"Write prefix to %p",topPtr->data[ROUTE_TOPINDEX_PREFIX]);
		stWriteSeqTailBuilderPackedData(&(routingBuilder->prefixBuilder), topPtr->data[ROUTE_TOPINDEX_PREFIX]+neededBlocks[ROUTE_TOPINDEX_PREFIX].headerSize);
		}

	if((routingBuilder->upgradedToTree) || stGetSeqTailBuilderDirty(&(routingBuilder->suffixBuilder)))
		{
//		LOG(LOG_INFO,"Write suffix to %p",topPtr->data[ROUTE_TOPINDEX_SUFFIX]);
		stWriteSeqTailBuilderPackedData(&(routingBuilder->suffixBuilder), topPtr->data[ROUTE_TOPINDEX_SUFFIX]+neededBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize);
		}

	for(int i=ROUTE_TOPINDEX_FORWARD_LEAF_ARRAY_0; i<=ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0;i++)
		{
		RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);

		if((routingBuilder->upgradedToTree) || rttaGetArrayDirty(arrayProxy))
			{
//			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize);

//			LOG(LOG_INFO,"Write leaf array to %p",topPtr->data[i]);

			rttaBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, neededBlocks[i].variant);	// Rebind array after alloc (root already done)

			s32 size=writeBuildersAsIndirectData_mergeTopArrayUpdates_leaf_accumulateSize(arrayProxy, indexSize);

			if(size>1000000)
				LOG(LOG_INFO,"writeIndirect Leaf: CircAlloc %i",size);

			u8 *newLeafData=mhAlloc(heap, size, sliceTag, INT_MAX, NULL);

			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root & array after alloc
			rttaBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, neededBlocks[i].variant);

			writeBuildersAsIndirectData_mergeTopArrayUpdates_leaf(arrayProxy, i, indexSize, sliceIndex, newLeafData, size, heap);
			}
		}

	for(int i=ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0; i<=ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0;i++)
		{
		RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);

		if((routingBuilder->upgradedToTree)|| rttaGetArrayDirty(arrayProxy))
			{
//			LOG(LOG_INFO,"Write branch array to %p",topPtr->data[i]);

//			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize);
			rttaBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, neededBlocks[i].variant);	// Rebind array after alloc (root already done)

			s32 size=writeBuildersAsIndirectData_mergeTopArrayUpdates_branch_accumulateSize(arrayProxy, indexSize);

			if(size>1000000)
				LOG(LOG_INFO,"writeIndirect Branch: CircAlloc %i",size);

			u8 *newBranchData=mhAlloc(heap, size, sliceTag, INT_MAX, NULL);

			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root & array after alloc
			rttaBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, neededBlocks[i].variant);

			writeBuildersAsIndirectData_mergeTopArrayUpdates_branch(arrayProxy, i, indexSize, sliceIndex, newBranchData, size, heap);
			}
		}

//	LOG(LOG_INFO,"writeBuildersAsIndirectData Completed: %i %i",sliceTag, sliceIndex);
}



void dumpRoutePatches(char *label, RoutePatch *patches, s32 patchCount)
{
	LOG(LOG_INFO,"%s Patches: %i",label, patchCount);

	for(int i=0;i<patchCount;i++)
		LOG(LOG_INFO, "Patch %i (%i): %i <-> %i [%i:%i]",i, patches[i].dispatchLinkIndex, patches[i].prefixIndex, patches[i].suffixIndex,
				patches[i].rdiPtr->minEdgePosition, patches[i].rdiPtr->maxEdgePosition);

}


/*

static int forwardPrefixSorter(const void *a, const void *b)
{
	RoutePatch *pa=(RoutePatch *)a;
	RoutePatch *pb=(RoutePatch *)b;

	int diff=pa->prefixIndex-pb->prefixIndex;
	if(diff)
		return diff;

	return pa->rdiPtr-pb->rdiPtr;
}

static int reverseSuffixSorter(const void *a, const void *b)
{
	RoutePatch *pa=(RoutePatch *)a;
	RoutePatch *pb=(RoutePatch *)b;

	int diff=pa->suffixIndex-pb->suffixIndex;
	if(diff)
		return diff;

	return pa->rdiPtr-pb->rdiPtr;
}

*/



// Case 1: { [10:10] , [5:5] } -> { [5:5], [11:11] }
// Case 2: { [10:10] , [5:5], [11:12] } -> { [5:5], [11:11], [11:12] }
// Case 3: { [10:10] , [5:5], [1:1] } -> { [1:1], [6:6], [12:12] }
// Case 4: { [1:1]A, [1:1]B } -> { [1:1]A, [1:1]B }
// Case 5: { [1:10]A, [1:1]B } -> { [1:1]B, [2:11]B }

/*
static void reorderForwardPatchRange_bubble(RoutePatch *forwardPatches, int firstPosition, int lastPosition)
{
//	LOG(LOG_INFO,"First / Last %i %i",firstPosition, lastPosition);

	for (int j = lastPosition-1; j>firstPosition; --j)
		{
		int swap = 0;
		for (int k = firstPosition; k<j; k++)
			{
//			LOG(LOG_INFO,"Compare %i %i",k, k+1);

			int dsEarlier=forwardPatches[k].suffixIndex;
			int dsLater=forwardPatches[k+1].suffixIndex;

			int minEarlier=(*(forwardPatches[k].rdiPtr))->minEdgePosition;
			int minLater=(*(forwardPatches[k+1].rdiPtr))->minEdgePosition;

			int maxEarlier=(*(forwardPatches[k].rdiPtr))->maxEdgePosition;
			int maxLater=(*(forwardPatches[k+1].rdiPtr))->maxEdgePosition;

			if(maxLater<=minEarlier) // Swap incompatible groups based on position
				{
				swap=1;
				DispatchLink *rdiPtr=(*(forwardPatches[k].rdiPtr));
				rdiPtr->minEdgePosition++;
				rdiPtr->maxEdgePosition++;

				RoutePatch tmpPatch=forwardPatches[k];
				forwardPatches[k]=forwardPatches[k+1];
				forwardPatches[k+1]=tmpPatch;
				}
			else if(minLater==minEarlier && maxLater == maxEarlier+1 && dsLater<dsEarlier) // Swap routed compatible groups based on downstream
				{
				swap=1;
				(*(forwardPatches[k].rdiPtr))->maxEdgePosition++;
				(*(forwardPatches[k+1].rdiPtr))->maxEdgePosition--;

				RoutePatch tmpPatch=forwardPatches[k];
				forwardPatches[k]=forwardPatches[k+1];
				forwardPatches[k+1]=tmpPatch;
				}
			else if(minEarlier==TR_INIT_MINEDGEPOSITION && minLater==TR_INIT_MINEDGEPOSITION &&
					maxEarlier==TR_INIT_MAXEDGEPOSITION && maxLater==TR_INIT_MAXEDGEPOSITION && dsLater<dsEarlier) // Swap unrouted compatible groups based on downstream
				{
				swap=1;

				RoutePatch tmpPatch=forwardPatches[k];
				forwardPatches[k]=forwardPatches[k+1];
				forwardPatches[k+1]=tmpPatch;
				}


			}
		if(!swap)
			break;
		}

}


static RoutePatch *reorderForwardPatches_bubble(RoutePatch *forwardPatches, s32 forwardCount, MemDispenser *disp)
{
	qsort(forwardPatches, forwardCount, sizeof(RoutePatch), forwardPrefixSorter);

	//LOG(LOG_INFO,"Before reorder");
	//dumpRoutePatches("Forward", forwardPatches, forwardCount);

	int firstPrefix=forwardPatches[0].prefixIndex;
	int firstPosition=0;

	for(int i=1; i<forwardCount;i++)
		{
		if(firstPrefix!=forwardPatches[i].prefixIndex) 			// Range from firstPosition to i-1
			{
			if(firstPosition<i-1)
				reorderForwardPatchRange_bubble(forwardPatches, firstPosition, i);

			firstPrefix=forwardPatches[i].prefixIndex;
			firstPosition=i;
			}
		}

	if(firstPosition<forwardCount-1)
		reorderForwardPatchRange_bubble(forwardPatches, firstPosition, forwardCount);

	//LOG(LOG_INFO,"After reorder");
	//dumpRoutePatches("Forward", forwardPatches, forwardCount);

	return forwardPatches;
}


*/






static int reorderForwardPatches_byUpstream(RoutePatch **patchBuffers, s32 patchCount, int pingPong)
{
//	LOG(LOG_INFO,"Tail sorting %i to %i",0, patchCount);

	int inGroupSize=1;

	while(inGroupSize<patchCount)
	{
		int outGroupSize=inGroupSize*2;
		int pongPing=!pingPong;

		RoutePatch *srcBuf=patchBuffers[pingPong];
		RoutePatch *dest=patchBuffers[pongPing];

		int src1=0;
		while(src1<patchCount)
			{
			int src1End=MIN(patchCount, src1+inGroupSize);
			int src2=src1End;
			int src2End=MIN(patchCount, src2+inGroupSize);

//			LOG(LOG_INFO,"Tail sorting %i to %i and %i to %i",src1,src1End,src2,src2End);

			while(src1<src1End && src2<src2End)
				{
				int prefix1=srcBuf[src1].prefixIndex;
				int prefix2=srcBuf[src2].prefixIndex;

//				LOG(LOG_INFO,"TailSort: %i %i",prefix1, prefix2);

				if(prefix1<=prefix2)
					*(dest++)=srcBuf[src1++];
				else
					*(dest++)=srcBuf[src2++];
				}

			while(src1<src1End)
				*(dest++)=srcBuf[src1++];

			while(src2<src2End)
				*(dest++)=srcBuf[src2++];

			src1=src2End;
			}
		pingPong=pongPing;
		inGroupSize=outGroupSize;
	}


	return pingPong;

}

static int reorderForwardPatchRange_byOffset(RoutePatch **patchBuffers, s32 patchStart, s32 patchEnd, int pingPong)
{
//	LOG(LOG_INFO,"Offset sorting %i to %i",patchStart, patchEnd);

	int inGroupSize=1;
	int patchCount=patchEnd-patchStart;

	while(inGroupSize<patchCount)
	{
//		LOG(LOG_INFO,"Offset sorting %i to %i - %i",patchStart, patchEnd, inGroupSize);
//		dumpRoutePatches("forward", patchBuffers[pingPong]+patchStart, patchCount);

		int outGroupSize=inGroupSize*2;
		int pongPing=!pingPong;

		RoutePatch *srcBuf=patchBuffers[pingPong];
		RoutePatch *dest=patchBuffers[pongPing]+patchStart;

		int src1=patchStart;
		while(src1<patchEnd)
			{
			int positionShift=0;

			int src1End=MIN(patchEnd, src1+inGroupSize);
			int src2=src1End;
			int src2End=MIN(patchEnd, src2+inGroupSize);

//			LOG(LOG_INFO,"Offset sorting %i to %i and %i to %i",src1,src1End,src2,src2End);

			while(src1<src1End && src2<src2End)
				{
				int offset1=srcBuf[src1].rdiPtr->minEdgePosition+positionShift;
				int offset2=srcBuf[src2].rdiPtr->maxEdgePosition;

				if(offset1<offset2) // Existing order is ok
					{
					*dest=srcBuf[src1++];
					dest->rdiPtr->minEdgePosition+=positionShift;
					dest->rdiPtr->maxEdgePosition+=positionShift;
					dest++;
					}
				else
					{
					*(dest++)=srcBuf[src2++];
					positionShift++;
					}
				}

			while(src1<src1End)
				{
				*dest=srcBuf[src1++];
				dest->rdiPtr->minEdgePosition+=positionShift;
				dest->rdiPtr->maxEdgePosition+=positionShift;
				dest++;
				}

			while(src2<src2End)
				*(dest++)=srcBuf[src2++];

			src1=src2End;
			}
		pingPong=pongPing;
		inGroupSize=outGroupSize;
	}


	return pingPong;
}

static void reorderForwardPatchRange_byDownstream_unrouted(RoutePatch **patchBuffers, s32 patchStart, s32 patchEnd, int pingPong)
{
//	LOG(LOG_INFO,"Downstream unrouted sorting %i to %i",patchStart, patchEnd);

	int inGroupSize=1;
	int patchCount=patchEnd-patchStart;

	while(inGroupSize<patchCount)
	{
		int outGroupSize=inGroupSize*2;
		int pongPing=!pingPong;

		RoutePatch *srcBuf=patchBuffers[pingPong];
		RoutePatch *dest=patchBuffers[pongPing]+patchStart;

		int src1=patchStart;

		while(src1<patchEnd)
			{
			int src1End=MIN(patchEnd, src1+inGroupSize);
			int src2=src1End;
			int src2End=MIN(patchEnd, src2+inGroupSize);


			while(src1<src1End && src2<src2End)
				{
				int suffix1=srcBuf[src1].suffixIndex;
				int suffix2=srcBuf[src2].suffixIndex;

				if(suffix1<=suffix2) // Existing order is ok
					*(dest++)=srcBuf[src1++];
				else
					*(dest++)=srcBuf[src2++];
				}

			while(src1<src1End)
				*(dest++)=srcBuf[src1++];

			while(src2<src2End)
				*(dest++)=srcBuf[src2++];

			src1=src2End;
			}
		pingPong=pongPing;
		inGroupSize=outGroupSize;
	}


	if(pingPong)
		memcpy(patchBuffers[0]+patchStart, patchBuffers[1]+patchStart, sizeof(RoutePatch)*patchCount);
}


static void reorderForwardPatchRange_byDownstream_routed(RoutePatch **patchBuffers, s32 patchStart, s32 patchEnd, int pingPong)
{
	int inGroupSize=1;
	int patchCount=patchEnd-patchStart;

	//	LOG(LOG_INFO,"Downstream routed sorting %i to %i",patchStart, patchEnd);
//	dumpRoutePatches("forward", patchBuffers[pingPong]+patchStart, patchCount);

	int maxPosition=patchBuffers[pingPong][patchStart].rdiPtr->maxEdgePosition;

	while(inGroupSize<patchCount)
		{
		int outGroupSize=inGroupSize*2;
		int pongPing=!pingPong;

		RoutePatch *srcBuf=patchBuffers[pingPong];
		RoutePatch *dest=patchBuffers[pongPing]+patchStart;

		int src1=patchStart;
		//int maxPosition=(*(srcBuf[patchStart].rdiPtr))->maxEdgePosition;

		while(src1<patchEnd)
			{
			int src1End=MIN(patchEnd, src1+inGroupSize);
			int src2=src1End;
			int src2End=MIN(patchEnd, src2+inGroupSize);

			while(src1<src1End && src2<src2End)
				{
				int suffix1=srcBuf[src1].suffixIndex;
				int suffix2=srcBuf[src2].suffixIndex;

				if(suffix1<=suffix2) // Existing order is ok
					*(dest++)=srcBuf[src1++];
				else
					*(dest++)=srcBuf[src2++];

/*
				if(suffix1<=suffix2) // Existing order is ok
					{
					*dest=srcBuf[src1++];
					(*(dest->rdiPtr))->maxEdgePosition=maxPosition++;
					dest++;
					}
				else
					{
					*dest=srcBuf[src2++];
					(*(dest->rdiPtr))->maxEdgePosition=maxPosition++;
					dest++;
					}*/
				}

			while(src1<src1End)
				*(dest++)=srcBuf[src1++];

			while(src2<src2End)
				*(dest++)=srcBuf[src2++];

			/*
			while(src1<src1End)
				{
				*dest=srcBuf[src1++];
				(*(dest->rdiPtr))->maxEdgePosition=maxPosition++;
				dest++;
				}

			while(src2<src2End)
				{
				*dest=srcBuf[src2++];
				(*(dest->rdiPtr))->maxEdgePosition=maxPosition++;
				dest++;
				}
*/

			src1=src2End;
			}

		pingPong=pongPing;
		inGroupSize=outGroupSize;
		}

	RoutePatch *dest=patchBuffers[pingPong]+patchStart;

	for(int i=patchStart;i<patchEnd;i++)
		{
		dest->rdiPtr->maxEdgePosition=maxPosition++;
		dest++;
		}


	if(pingPong)
		memcpy(patchBuffers[0]+patchStart, patchBuffers[1]+patchStart, sizeof(RoutePatch)*patchCount);

}



static void reorderForwardPatchRange_byDownstream(RoutePatch **patchBuffers, s32 patchStart, s32 patchEnd, int rangePingPong)
{
//	int patchCount=patchEnd-patchStart;
//	LOG(LOG_INFO,"Downstream sorting %i to %i",patchStart, patchEnd);
//	dumpRoutePatches("forward", patchBuffers[rangePingPong]+patchStart, patchCount);

	RoutePatch *groupedPatches=patchBuffers[rangePingPong];

	int firstMinPosition=groupedPatches[patchStart].rdiPtr->minEdgePosition;
	int firstMaxPosition=groupedPatches[patchStart].rdiPtr->maxEdgePosition;
	int firstPosition=patchStart;

	int i=patchStart+1;

	int patchLast=patchEnd-1;

	while(i<patchLast)
		{
		int minPosition=groupedPatches[i].rdiPtr->minEdgePosition;
		int maxPosition=groupedPatches[i].rdiPtr->maxEdgePosition;

		if(firstMinPosition==TR_INIT_MINEDGEPOSITION && firstMaxPosition==TR_INIT_MAXEDGEPOSITION) // Unrouted range
			{
			while(i<patchLast && minPosition==TR_INIT_MINEDGEPOSITION && maxPosition==TR_INIT_MAXEDGEPOSITION)
				{
				i++;
				minPosition=groupedPatches[i].rdiPtr->minEdgePosition;
				maxPosition=groupedPatches[i].rdiPtr->maxEdgePosition;
				}

			reorderForwardPatchRange_byDownstream_unrouted(patchBuffers, firstPosition, i, rangePingPong);
			}
		else																					// Pre-routed range
			{
			int expectedMaxPosition=maxPosition;

			while(i<patchLast && minPosition==firstMinPosition && maxPosition==expectedMaxPosition)
				{
				i++;
				minPosition=groupedPatches[i].rdiPtr->minEdgePosition;
				maxPosition=groupedPatches[i].rdiPtr->maxEdgePosition;

				expectedMaxPosition++;
				}

			reorderForwardPatchRange_byDownstream_routed(patchBuffers, firstPosition, i, rangePingPong);
			}

		firstMinPosition=minPosition;
		firstMaxPosition=maxPosition;
		firstPosition=i;
		}

	int rangeCount=patchEnd-firstPosition;

	if((rangeCount>0) && rangePingPong)
		memcpy(patchBuffers[0]+firstPosition, patchBuffers[1]+firstPosition, sizeof(RoutePatch)*rangeCount);



}


static RoutePatch *reorderForwardPatches(RoutePatch *forwardPatches, s32 patchCount, MemDispenser *disp)
{
//	LOG(LOG_INFO,"Before reorder");
//	dumpRoutePatches("Forward", forwardPatches, patchCount);

//	LOG(LOG_INFO,"Need to sort %i",patchCount);

	RoutePatch *patchBuffers[2];

	patchBuffers[0]=forwardPatches;
	patchBuffers[1]=dAlloc(disp, sizeof(RoutePatch)*patchCount);

	int pingPong=0; // Indicates which patchBuffer is next source

	pingPong=reorderForwardPatches_byUpstream(patchBuffers, patchCount, pingPong);

//	LOG(LOG_INFO,"Upstream Ordered");
//	dumpRoutePatches("Forward", patchBuffers[pingPong], patchCount);

	RoutePatch *groupedPatches=patchBuffers[pingPong];
	int firstPrefix=groupedPatches[0].prefixIndex;
	int firstPosition=0;

	for(int i=1; i<patchCount;i++)
		{
		if(firstPrefix!=groupedPatches[i].prefixIndex) 			// Range from firstPosition to i-1
			{
			int rangeCount=i-firstPosition;

			if(rangeCount>1)
				{
				int rangePingPong=reorderForwardPatchRange_byOffset(patchBuffers, firstPosition, i, pingPong);
				reorderForwardPatchRange_byDownstream(patchBuffers, firstPosition, i, rangePingPong);
				}
			else if(pingPong)
				memcpy(patchBuffers[0]+firstPosition,patchBuffers[1]+firstPosition,sizeof(RoutePatch)*rangeCount);

			firstPrefix=groupedPatches[i].prefixIndex;
			firstPosition=i;
			}
		}

	int rangeCount=patchCount-firstPosition;

	if(rangeCount>1)
		{
		int rangePingPong=reorderForwardPatchRange_byOffset(patchBuffers, firstPosition, patchCount, pingPong);
		reorderForwardPatchRange_byDownstream(patchBuffers, firstPosition, patchCount, rangePingPong);
		}
	else if(pingPong)
		memcpy(patchBuffers[0]+firstPosition,patchBuffers[1]+firstPosition,sizeof(RoutePatch)*rangeCount);


//	LOG(LOG_INFO,"After reorder");
	//dumpRoutePatches("Forward", patchBuffers[pingPong], patchCount);
//	dumpRoutePatches("Forward", forwardPatches, patchCount);

	return forwardPatches;
}

/*

static void reorderReversePatchRange_bubble(RoutePatch *reversePatches, int firstPosition, int lastPosition)
{
	for (int j = lastPosition-1; j>firstPosition; --j)
		{
		int swap = 0;
		for (int k = firstPosition; k<j; k++)
			{
			int dsEarlier=reversePatches[k].prefixIndex;
			int dsLater=reversePatches[k+1].prefixIndex;

			int minEarlier=(*(reversePatches[k].rdiPtr))->minEdgePosition;
			int minLater=(*(reversePatches[k+1].rdiPtr))->minEdgePosition;

			int maxEarlier=(*(reversePatches[k].rdiPtr))->maxEdgePosition;
			int maxLater=(*(reversePatches[k+1].rdiPtr))->maxEdgePosition;

			if(maxLater<=minEarlier) // Swap incompatible groups based on position
				{
				swap=1;
				DispatchLink *rdiPtr=(*(reversePatches[k].rdiPtr));
				rdiPtr->minEdgePosition++;
				rdiPtr->maxEdgePosition++;

				RoutePatch tmpPatch=reversePatches[k];
				reversePatches[k]=reversePatches[k+1];
				reversePatches[k+1]=tmpPatch;
				}
			else if(minLater==minEarlier && maxLater == maxEarlier+1 && dsLater<dsEarlier) // Swap routed compatible groups based on downstream
				{
				swap=1;
				(*(reversePatches[k].rdiPtr))->maxEdgePosition++;
				(*(reversePatches[k+1].rdiPtr))->maxEdgePosition--;

				RoutePatch tmpPatch=reversePatches[k];
				reversePatches[k]=reversePatches[k+1];
				reversePatches[k+1]=tmpPatch;
				}
			else if(minEarlier==TR_INIT_MINEDGEPOSITION && minLater==TR_INIT_MINEDGEPOSITION &&
					maxEarlier==TR_INIT_MAXEDGEPOSITION && maxLater==TR_INIT_MAXEDGEPOSITION && dsLater<dsEarlier) // Swap unrouted compatible groups based on downstream
				{
				swap=1;

				RoutePatch tmpPatch=reversePatches[k];
				reversePatches[k]=reversePatches[k+1];
				reversePatches[k+1]=tmpPatch;
				}
			}
		if(!swap)
			break;
		}

}



static RoutePatch *reorderReversePatches_bubble(RoutePatch *reversePatches, s32 reverseCount, MemDispenser *disp)
{
	//LOG(LOG_INFO,"Before reorder");
	//dumpRoutePatches("Reverse", reversePatches, reverseCount);

	qsort(reversePatches, reverseCount, sizeof(RoutePatch), reverseSuffixSorter);

	int firstSuffix=reversePatches[0].suffixIndex;
	int firstPosition=0;

	for(int i=1; i<reverseCount;i++)
		{
		if(firstSuffix!=reversePatches[i].suffixIndex) 			// Range from firstPosition to i-1
			{
			if(firstPosition<i-1)
				reorderReversePatchRange_bubble(reversePatches, firstPosition, i);

			firstSuffix=reversePatches[i].suffixIndex;
			firstPosition=i;
			}
		}

	if(firstPosition<reverseCount-1)
		reorderReversePatchRange_bubble(reversePatches, firstPosition, reverseCount);

	//LOG(LOG_INFO,"After reorder");
	//dumpRoutePatches("Reverse", reversePatches, reverseCount);

	return reversePatches;
}

*/




static int reorderReversePatches_byUpstream(RoutePatch **patchBuffers, s32 patchCount, int pingPong)
{
//	LOG(LOG_INFO,"Tail sorting %i to %i",0, patchCount);

	int inGroupSize=1;

	while(inGroupSize<patchCount)
	{
		int outGroupSize=inGroupSize*2;
		int pongPing=!pingPong;

		RoutePatch *srcBuf=patchBuffers[pingPong];
		RoutePatch *dest=patchBuffers[pongPing];

		int src1=0;
		while(src1<patchCount)
			{
			int src1End=MIN(patchCount, src1+inGroupSize);
			int src2=src1End;
			int src2End=MIN(patchCount, src2+inGroupSize);

//			LOG(LOG_INFO,"Tail sorting %i to %i and %i to %i",src1,src1End,src2,src2End);

			while(src1<src1End && src2<src2End)
				{
				int suffix1=srcBuf[src1].suffixIndex;
				int suffix2=srcBuf[src2].suffixIndex;

//				LOG(LOG_INFO,"TailSort: %i %i",prefix1, prefix2);

				if(suffix1<=suffix2)
					*(dest++)=srcBuf[src1++];
				else
					*(dest++)=srcBuf[src2++];
				}

			while(src1<src1End)
				*(dest++)=srcBuf[src1++];

			while(src2<src2End)
				*(dest++)=srcBuf[src2++];

			src1=src2End;
			}
		pingPong=pongPing;
		inGroupSize=outGroupSize;
	}


	return pingPong;

}

static int reorderReversePatchRange_byOffset(RoutePatch **patchBuffers, s32 patchStart, s32 patchEnd, int pingPong)
{
//	LOG(LOG_INFO,"Offset sorting %i to %i",patchStart, patchEnd);

	int inGroupSize=1;
	int patchCount=patchEnd-patchStart;

	while(inGroupSize<patchCount)
	{
//		LOG(LOG_INFO,"Offset sorting %i to %i - %i",patchStart, patchEnd, inGroupSize);
//		dumpRoutePatches("forward", patchBuffers[pingPong]+patchStart, patchCount);

		int outGroupSize=inGroupSize*2;
		int pongPing=!pingPong;

		RoutePatch *srcBuf=patchBuffers[pingPong];
		RoutePatch *dest=patchBuffers[pongPing]+patchStart;

		int src1=patchStart;
		while(src1<patchEnd)
			{
			int positionShift=0;

			int src1End=MIN(patchEnd, src1+inGroupSize);
			int src2=src1End;
			int src2End=MIN(patchEnd, src2+inGroupSize);

//			LOG(LOG_INFO,"Offset sorting %i to %i and %i to %i",src1,src1End,src2,src2End);

			while(src1<src1End && src2<src2End)
				{
				int offset1=srcBuf[src1].rdiPtr->minEdgePosition+positionShift;
				int offset2=srcBuf[src2].rdiPtr->maxEdgePosition;

				if(offset1<offset2) // Existing order is ok
					{
					*dest=srcBuf[src1++];
					dest->rdiPtr->minEdgePosition+=positionShift;
					dest->rdiPtr->maxEdgePosition+=positionShift;
					dest++;
					}
				else
					{
					*(dest++)=srcBuf[src2++];
					positionShift++;
					}
				}

			while(src1<src1End)
				{
				*dest=srcBuf[src1++];
				dest->rdiPtr->minEdgePosition+=positionShift;
				dest->rdiPtr->maxEdgePosition+=positionShift;
				dest++;
				}

			while(src2<src2End)
				*(dest++)=srcBuf[src2++];

			src1=src2End;
			}
		pingPong=pongPing;
		inGroupSize=outGroupSize;
	}


	return pingPong;
}

static void reorderReversePatchRange_byDownstream_unrouted(RoutePatch **patchBuffers, s32 patchStart, s32 patchEnd, int pingPong)
{
//	LOG(LOG_INFO,"Downstream unrouted sorting %i to %i",patchStart, patchEnd);

	int inGroupSize=1;
	int patchCount=patchEnd-patchStart;

	while(inGroupSize<patchCount)
	{
		int outGroupSize=inGroupSize*2;
		int pongPing=!pingPong;

		RoutePatch *srcBuf=patchBuffers[pingPong];
		RoutePatch *dest=patchBuffers[pongPing]+patchStart;

		int src1=patchStart;

		while(src1<patchEnd)
			{
			int src1End=MIN(patchEnd, src1+inGroupSize);
			int src2=src1End;
			int src2End=MIN(patchEnd, src2+inGroupSize);


			while(src1<src1End && src2<src2End)
				{
				int prefix1=srcBuf[src1].prefixIndex;
				int prefix2=srcBuf[src2].prefixIndex;

				if(prefix1<=prefix2) // Existing order is ok
					*(dest++)=srcBuf[src1++];
				else
					*(dest++)=srcBuf[src2++];
				}

			while(src1<src1End)
				*(dest++)=srcBuf[src1++];

			while(src2<src2End)
				*(dest++)=srcBuf[src2++];

			src1=src2End;
			}
		pingPong=pongPing;
		inGroupSize=outGroupSize;
	}


	if(pingPong)
		memcpy(patchBuffers[0]+patchStart, patchBuffers[1]+patchStart, sizeof(RoutePatch)*patchCount);
}


static void reorderReversePatchRange_byDownstream_routed(RoutePatch **patchBuffers, s32 patchStart, s32 patchEnd, int pingPong)
{
	int inGroupSize=1;
	int patchCount=patchEnd-patchStart;

	//	LOG(LOG_INFO,"Downstream routed sorting %i to %i",patchStart, patchEnd);
//	dumpRoutePatches("forward", patchBuffers[pingPong]+patchStart, patchCount);

	int maxPosition=patchBuffers[pingPong][patchStart].rdiPtr->maxEdgePosition;

	while(inGroupSize<patchCount)
		{
		int outGroupSize=inGroupSize*2;
		int pongPing=!pingPong;

		RoutePatch *srcBuf=patchBuffers[pingPong];
		RoutePatch *dest=patchBuffers[pongPing]+patchStart;

		int src1=patchStart;
		//int maxPosition=(*(srcBuf[patchStart].rdiPtr))->maxEdgePosition;

		while(src1<patchEnd)
			{
			int src1End=MIN(patchEnd, src1+inGroupSize);
			int src2=src1End;
			int src2End=MIN(patchEnd, src2+inGroupSize);

			while(src1<src1End && src2<src2End)
				{
				int prefix1=srcBuf[src1].prefixIndex;
				int prefix2=srcBuf[src2].prefixIndex;

				if(prefix1<=prefix2) // Existing order is ok
					*(dest++)=srcBuf[src1++];
				else
					*(dest++)=srcBuf[src2++];

/*
				if(suffix1<=suffix2) // Existing order is ok
					{
					*dest=srcBuf[src1++];
					(*(dest->rdiPtr))->maxEdgePosition=maxPosition++;
					dest++;
					}
				else
					{
					*dest=srcBuf[src2++];
					(*(dest->rdiPtr))->maxEdgePosition=maxPosition++;
					dest++;
					}*/
				}

			while(src1<src1End)
				*(dest++)=srcBuf[src1++];

			while(src2<src2End)
				*(dest++)=srcBuf[src2++];

			/*
			while(src1<src1End)
				{
				*dest=srcBuf[src1++];
				(*(dest->rdiPtr))->maxEdgePosition=maxPosition++;
				dest++;
				}

			while(src2<src2End)
				{
				*dest=srcBuf[src2++];
				(*(dest->rdiPtr))->maxEdgePosition=maxPosition++;
				dest++;
				}
*/

			src1=src2End;
			}

		pingPong=pongPing;
		inGroupSize=outGroupSize;
		}

	RoutePatch *dest=patchBuffers[pingPong]+patchStart;

	for(int i=patchStart;i<patchEnd;i++)
		{
		dest->rdiPtr->maxEdgePosition=maxPosition++;
		dest++;
		}


	if(pingPong)
		memcpy(patchBuffers[0]+patchStart, patchBuffers[1]+patchStart, sizeof(RoutePatch)*patchCount);

}



static void reorderReversePatchRange_byDownstream(RoutePatch **patchBuffers, s32 patchStart, s32 patchEnd, int rangePingPong)
{
//	int patchCount=patchEnd-patchStart;
//	LOG(LOG_INFO,"Downstream sorting %i to %i",patchStart, patchEnd);
//	dumpRoutePatches("forward", patchBuffers[rangePingPong]+patchStart, patchCount);

	RoutePatch *groupedPatches=patchBuffers[rangePingPong];

	int firstMinPosition=groupedPatches[patchStart].rdiPtr->minEdgePosition;
	int firstMaxPosition=groupedPatches[patchStart].rdiPtr->maxEdgePosition;
	int firstPosition=patchStart;

	int i=patchStart+1;

	int patchLast=patchEnd-1;

	while(i<patchLast)
		{
		int minPosition=groupedPatches[i].rdiPtr->minEdgePosition;
		int maxPosition=groupedPatches[i].rdiPtr->maxEdgePosition;

		if(firstMinPosition==TR_INIT_MINEDGEPOSITION && firstMaxPosition==TR_INIT_MAXEDGEPOSITION) // Unrouted range
			{
			while(i<patchLast && minPosition==TR_INIT_MINEDGEPOSITION && maxPosition==TR_INIT_MAXEDGEPOSITION)
				{
				i++;
				minPosition=groupedPatches[i].rdiPtr->minEdgePosition;
				maxPosition=groupedPatches[i].rdiPtr->maxEdgePosition;
				}

			reorderReversePatchRange_byDownstream_unrouted(patchBuffers, firstPosition, i, rangePingPong);
			}
		else																					// Pre-routed range
			{
			int expectedMaxPosition=maxPosition;

			while(i<patchLast && minPosition==firstMinPosition && maxPosition==expectedMaxPosition)
				{
				i++;
				minPosition=groupedPatches[i].rdiPtr->minEdgePosition;
				maxPosition=groupedPatches[i].rdiPtr->maxEdgePosition;

				expectedMaxPosition++;
				}

			reorderReversePatchRange_byDownstream_routed(patchBuffers, firstPosition, i, rangePingPong);
			}

		firstMinPosition=minPosition;
		firstMaxPosition=maxPosition;
		firstPosition=i;
		}

	int rangeCount=patchEnd-firstPosition;

	if((rangeCount>0) && rangePingPong)
		memcpy(patchBuffers[0]+firstPosition, patchBuffers[1]+firstPosition, sizeof(RoutePatch)*rangeCount);



}


static RoutePatch *reorderReversePatches(RoutePatch *reversePatches, s32 patchCount, MemDispenser *disp)
{
//	LOG(LOG_INFO,"Before reorder");
//	dumpRoutePatches("Forward", forwardPatches, patchCount);

//	LOG(LOG_INFO,"Need to sort %i",patchCount);

	RoutePatch *patchBuffers[2];

	patchBuffers[0]=reversePatches;
	patchBuffers[1]=dAlloc(disp, sizeof(RoutePatch)*patchCount);

	int pingPong=0; // Indicates which patchBuffer is next source

	pingPong=reorderReversePatches_byUpstream(patchBuffers, patchCount, pingPong);

//	LOG(LOG_INFO,"Upstream Ordered");
//	dumpRoutePatches("Forward", patchBuffers[pingPong], patchCount);

	RoutePatch *groupedPatches=patchBuffers[pingPong];
	int firstSuffix=groupedPatches[0].suffixIndex;
	int firstPosition=0;

	for(int i=1; i<patchCount;i++)
		{
		if(firstSuffix!=groupedPatches[i].suffixIndex) 			// Range from firstPosition to i-1
			{
			int rangeCount=i-firstPosition;

			if(rangeCount>1)
				{
				int rangePingPong=reorderReversePatchRange_byOffset(patchBuffers, firstPosition, i, pingPong);
				reorderReversePatchRange_byDownstream(patchBuffers, firstPosition, i, rangePingPong);
				}
			else if(pingPong)
				memcpy(patchBuffers[0]+firstPosition,patchBuffers[1]+firstPosition,sizeof(RoutePatch)*rangeCount);

			firstSuffix=groupedPatches[i].suffixIndex;
			firstPosition=i;
			}
		}

	int rangeCount=patchCount-firstPosition;

	if(rangeCount>1)
		{
		int rangePingPong=reorderReversePatchRange_byOffset(patchBuffers, firstPosition, patchCount, pingPong);
		reorderReversePatchRange_byDownstream(patchBuffers, firstPosition, patchCount, rangePingPong);
		}
	else if(pingPong)
		memcpy(patchBuffers[0]+firstPosition,patchBuffers[1]+firstPosition,sizeof(RoutePatch)*rangeCount);


//	LOG(LOG_INFO,"After reorder");
	//dumpRoutePatches("Forward", patchBuffers[pingPong], patchCount);
//	dumpRoutePatches("Forward", reversePatches, patchCount);

	return reversePatches;
}

/*
static void dumpTag(u8 *tagData)
{
	if(tagData==NULL)
		{
		LOG(LOG_INFO,"TagData: NULL");
		return;
		}

	int length=*(tagData++);

	LOG(LOG_INFO,"TagData Length: %i", length);

	LOGS(LOG_INFO,"Data:");

	for(int i=0;i<length;i++)
		LOGS(LOG_INFO," %02x", *(tagData++));

	LOGN(LOG_INFO,"");
}
*/

static u8 *extractTagData(RoutingIndexedDispatchLinkIndexBlock *rdi, DispatchLink *dispatchLink, MemDispenser *disp)
{
	// Dispatch* -> Sequence* -> null

	while(dispatchLink->indexType==LINK_INDEXTYPE_DISPATCH && dispatchLink->nextOrSourceIndex!=LINK_INDEX_DUMMY)
		dispatchLink=mbDoubleBrickFindByIndex(rdi->dispatchLinkPile, dispatchLink->nextOrSourceIndex);

	if(dispatchLink->nextOrSourceIndex==LINK_INDEX_DUMMY)
		LOG(LOG_CRITICAL, "Expected sequence link, found end of chain");

	if(dispatchLink->indexType!=LINK_INDEXTYPE_SEQ)
		LOG(LOG_CRITICAL, "Expected sequence type, found %i", dispatchLink->indexType);

	u32 sequenceLinkIndex=dispatchLink->nextOrSourceIndex;
	SequenceLink *sequenceLink=mbSingleBrickFindByIndex(rdi->sequenceLinkPile, sequenceLinkIndex);

	while(sequenceLink->tagLength==0 && sequenceLink->nextIndex!=LINK_INDEX_DUMMY)
		sequenceLink=mbSingleBrickFindByIndex(rdi->sequenceLinkPile, sequenceLink->nextIndex);

	int tagLength=sequenceLink->tagLength;

	if(tagLength==0) // No tag
		return NULL;

	SequenceLink *scanSequenceLink=sequenceLink;

	while(scanSequenceLink->nextIndex!=LINK_INDEX_DUMMY)
		{
		scanSequenceLink=mbSingleBrickFindByIndex(rdi->sequenceLinkPile, scanSequenceLink->nextIndex);
		tagLength+=scanSequenceLink->tagLength;
		}

	if(tagLength>SEQUENCE_LINK_MAX_TAG_LENGTH)
		LOG(LOG_CRITICAL,"Found over-long tag with %i length", tagLength);

	u8 *tagData=dAlloc(disp, tagLength+1);
	*tagData=tagLength;

	u8 *tagPtr=tagData+1;
	int toCopy=sequenceLink->seqLength;

	int offset=(sequenceLink->seqLength+3)>>2;

	memcpy(tagPtr, sequenceLink->packedSequence+offset, toCopy);
	tagPtr+=toCopy;

	while(sequenceLink->nextIndex!=LINK_INDEX_DUMMY)
		{
		sequenceLink=mbSingleBrickFindByIndex(rdi->sequenceLinkPile, sequenceLink->nextIndex);

		toCopy=sequenceLink->seqLength;
		memcpy(tagPtr, sequenceLink->packedSequence, toCopy);
		tagPtr+=toCopy;
		}

	//dumpTag(tagData);

	return tagData;
}


static void createRoutePatches(RoutingIndexedDispatchLinkIndexBlock *rdi, int entryOffset, int entryCount,
		SeqTailBuilder *prefixBuilder, SeqTailBuilder *suffixBuilder, RoutePatch **forwardPatchesPtr, RoutePatch **reversePatchesPtr,
		int *forwardCountPtr, int *reverseCountPtr, int *forwardTagCountPtr, int *reverseTagCountPtr, MemDispenser *disp)
{
	int forwardCount=0, reverseCount=0;
	int forwardTagCount=0, reverseTagCount=0;

	RoutePatch *forwardPatches=dAlloc(disp,sizeof(RoutePatch)*entryCount);
	RoutePatch *reversePatches=dAlloc(disp,sizeof(RoutePatch)*entryCount);

	int last=entryOffset+entryCount;
	for(int i=entryOffset;i<last;i++)
	{
		DispatchLink *rdd=rdi->linkEntries[i];

		int index=rdd->position+1;

		if(0)
		{/*
			SmerId currSmer=rdd->indexedData[index].fsmer;
			SmerId prevSmer=rdd->indexedData[index+1].fsmer;
			SmerId nextSmer=rdd->indexedData[index-1].fsmer;

			int upstreamLength=rdd->indexedData[index].readIndex-rdd->indexedData[index+1].readIndex;
			int downstreamLength=rdd->indexedData[index-1].readIndex-rdd->indexedData[index].readIndex;

			char bufferP[SMER_BASES+1]={0};
			char bufferC[SMER_BASES+1]={0};
			char bufferN[SMER_BASES+1]={0};

			unpackSmer(prevSmer, bufferP);
			unpackSmer(currSmer, bufferC);
			unpackSmer(nextSmer, bufferN);

			LOG(LOG_INFO,"Read Orientation: %s (%i) %s %s (%i)",bufferP, upstreamLength, bufferC, bufferN, downstreamLength);
			*/
		}


		//int smerRc=(rdd->revComp>>index)&0x1;

		int smerRc=(rdd->revComp>>(index-1))&0x7; // index+1 , index, index-1 (lsb)

		//SmerId currRmer=rdd->indexedData[index].rsmer;

		if(!(smerRc&0x2)) // Canonical Read Orientation
			{
//				smerId=currFmer;

				//SmerId prefixSmer=rdd->indexedData[index+1].rsmer; // Previous smer in read, reversed
				//SmerId suffixSmer=rdd->indexedData[index-1].fsmer; // Next smer in read
				SmerId currSmer=rdd->smers[index].smer; // Canonical, matches read orientation

				SmerId prefixSmer=(smerRc&0x1)?(rdd->smers[index-1].smer):complementSmerId(rdd->smers[index-1].smer); // RevComp read orientation
				SmerId suffixSmer=(smerRc&0x4)?complementSmerId(rdd->smers[index+1].smer):(rdd->smers[index+1].smer); // Read orientation

				int prefixLength=rdd->smers[index].seqIndexOffset;
				int suffixLength=rdd->smers[index+1].seqIndexOffset;

				if(prefixLength>23)
					LOG(LOG_CRITICAL,"Overlong prefix: %i (%i) [%i]",prefixLength, index, rdd->smers[0].seqIndexOffset);

				if(suffixLength>23)
					LOG(LOG_CRITICAL,"Overlong suffix: %i (%i) [%i]",suffixLength, index, rdd->smers[0].seqIndexOffset);

				forwardPatches[forwardCount].rdiPtr=rdi->linkEntries[i];
				forwardPatches[forwardCount].dispatchLinkIndex=rdi->linkIndexEntries[i];

				forwardPatches[forwardCount].nodePosition=-1;

				forwardPatches[forwardCount].prefixIndex=stFindOrCreateSeqTail(prefixBuilder, prefixSmer, prefixLength);
				forwardPatches[forwardCount].suffixIndex=stFindOrCreateSeqTail(suffixBuilder, suffixSmer, suffixLength);


				if(0)
					{
					u8 bufferP[SMER_BASES+1]={0};
					u8 bufferN[SMER_BASES+1]={0};
					u8 bufferS[SMER_BASES+1]={0};

					unpackSmer(prefixSmer, bufferP);
					unpackSmer(currSmer, bufferN);
					unpackSmer(suffixSmer, bufferS);

					LOG(LOG_INFO,"(F: %i) Node Orientation: %s (%2i) @ %i %s %s (%2i) @ %i", rdi->linkIndexEntries[i],
							bufferP, prefixLength, forwardPatches[forwardCount].prefixIndex,  bufferN, bufferS, suffixLength, forwardPatches[forwardCount].suffixIndex);
					}

				if(rdd->revComp & DISPATCHLINK_EOS_FLAG)
					{
					forwardPatches[forwardCount].tagData=extractTagData(rdi, rdd, disp);
					if(forwardPatches[forwardCount].tagData!=NULL)
						forwardTagCount++;
					}
				else
					forwardPatches[forwardCount].tagData=NULL;

				forwardCount++;
			}
		else	// (smerRc&0x2) - Reverse-complement Read Orientation
			{
				//SmerId prefixSmer=rdd->indexedData[index-1].fsmer; // Next smer in read
				//SmerId suffixSmer=rdd->indexedData[index+1].rsmer; // Previous smer in read, reversed

				SmerId currSmer=rdd->smers[index].smer; // Node orientation, RevComp read orientation

				SmerId prefixSmer=(smerRc&0x4)?complementSmerId(rdd->smers[index+1].smer):(rdd->smers[index+1].smer); // Read orientation
				SmerId suffixSmer=(smerRc&0x1)?(rdd->smers[index-1].smer):complementSmerId(rdd->smers[index-1].smer); // RevComp read orientation

				int prefixLength=rdd->smers[index+1].seqIndexOffset;
				int suffixLength=rdd->smers[index].seqIndexOffset;

				if(prefixLength>23)
					LOG(LOG_CRITICAL,"Overlong prefix: %i (%i) [%i]",prefixLength, index, rdd->smers[0].seqIndexOffset);

				if(suffixLength>23)
					LOG(LOG_CRITICAL,"Overlong suffix: %i (%i) [%i]",suffixLength, index, rdd->smers[0].seqIndexOffset);

				reversePatches[reverseCount].rdiPtr=rdi->linkEntries[i];
				reversePatches[reverseCount].dispatchLinkIndex=rdi->linkIndexEntries[i];

				reversePatches[reverseCount].nodePosition=-1;

				reversePatches[reverseCount].prefixIndex=stFindOrCreateSeqTail(prefixBuilder, prefixSmer, prefixLength);
				reversePatches[reverseCount].suffixIndex=stFindOrCreateSeqTail(suffixBuilder, suffixSmer, suffixLength);

				if(0)
					{
					u8 bufferP[SMER_BASES+1]={0};
					u8 bufferN[SMER_BASES+1]={0};
					u8 bufferS[SMER_BASES+1]={0};

					unpackSmer(prefixSmer, bufferP);
					unpackSmer(currSmer, bufferN);
					unpackSmer(suffixSmer, bufferS);

					LOG(LOG_INFO,"(R: %i) Node Orientation: %s (%2i) @ %2i %s %s (%2i) @ %2i", rdi->linkIndexEntries[i],
							bufferP, prefixLength, reversePatches[reverseCount].prefixIndex,  bufferN, bufferS, suffixLength, reversePatches[reverseCount].suffixIndex);
					}

				if(rdd->revComp & DISPATCHLINK_EOS_FLAG)
					{
					reversePatches[reverseCount].tagData=extractTagData(rdi, rdd, disp);
					if(reversePatches[reverseCount].tagData!=NULL)
						reverseTagCount++;
					}
				else
					reversePatches[reverseCount].tagData=NULL;

				reverseCount++;
			}
	}

	// Then order new forward and reverse routes, if more than one

	if(forwardCount>1)
		forwardPatches=reorderForwardPatches(forwardPatches, forwardCount, disp);

	if(reverseCount>1)
		reversePatches=reorderReversePatches(reversePatches, reverseCount, disp);

	*forwardPatchesPtr=forwardPatches;
	*reversePatchesPtr=reversePatches;

	*forwardCountPtr=forwardCount;
	*reverseCountPtr=reverseCount;

	*forwardTagCountPtr=forwardTagCount;
	*reverseTagCountPtr=reverseTagCount;
}





int rtRouteReadsForSmer(RoutingIndexedDispatchLinkIndexBlock *rdi, u32 entryOffset, u32 entryCount, SmerArraySlice *slice,
		u32 *orderedDispatches, MemDispenser *disp, MemHeap *heap, u8 sliceTag)
{
	s32 sliceIndex=rdi->sliceIndex;

//	LOG(LOG_INFO,"SliceIndex is %i",sliceIndex);

	u8 *smerDataOrig=slice->smerData[sliceIndex];
	u8 *smerData=smerDataOrig;

	//if(smerData!=NULL)
//		LOG(LOG_INFO, "Index: %i of %i Entries %i Data: %p",sliceIndex,slice->smerCount, rdi->entryCount, smerData);

	RoutingComboBuilder routingBuilder;

	routingBuilder.disp=disp;
	routingBuilder.rootPtr=slice->smerData+sliceIndex;
	routingBuilder.sliceIndex=sliceIndex;
	routingBuilder.sliceTag=sliceTag;

//	s32 oldHeaderSize=0, oldPrefixDataSize=0, oldSuffixDataSize=0, oldRouteTableDataSize=0;

	if(smerData!=NULL)
		{
		u8 header=*smerData;

		if(rtHeaderIsLiveDirect(header))
			{
			createBuildersFromDirectData(&routingBuilder);
			}
		// ALLOC_HEADER_LIVE_INDIRECT_ROOT_VALUE

		else if(rtHeaderIsLiveTop(header))
			{
			createBuildersFromIndirectData(&routingBuilder);
			}
		else
			{
			routingBuilder.arrayBuilder=NULL;
			routingBuilder.treeBuilder=NULL;

			LOG(LOG_CRITICAL,"Alloc header invalid %2x at %p",header,smerData);
			}
		}
	else
		{
		createBuildersFromNullData(&routingBuilder);
		}

	RoutePatch *forwardPatches=NULL;
	RoutePatch *reversePatches=NULL;

	int forwardCount=0;
	int reverseCount=0;

	int forwardTagCount=0;
	int reverseTagCount=0;

	createRoutePatches(rdi, entryOffset, entryCount, &(routingBuilder.prefixBuilder), &(routingBuilder.suffixBuilder),
			&forwardPatches, &reversePatches, &forwardCount, &reverseCount, &forwardTagCount, &reverseTagCount, disp);

	s32 prefixCount=stGetSeqTailTotalTailCount(&(routingBuilder.prefixBuilder))+1;
	s32 suffixCount=stGetSeqTailTotalTailCount(&(routingBuilder.suffixBuilder))+1;

	if(considerUpgradingToTree(&routingBuilder, forwardCount, reverseCount))
		{
//		LOG(LOG_INFO,"Prefix Old %i New %i",routingBuilder.prefixBuilder.oldTailCount,routingBuilder.prefixBuilder.newTailCount);
//		LOG(LOG_INFO,"Suffix Old %i New %i",routingBuilder.suffixBuilder.oldTailCount,routingBuilder.suffixBuilder.newTailCount);

//		LOG(LOG_INFO,"Upgrade: Tail counts are %i %i",prefixCount, suffixCount);

		upgradeToTree(&routingBuilder, prefixCount, suffixCount);
		}

	if(routingBuilder.treeBuilder!=NULL)
		{
/*
		if(entryCount>0)
			{
			DispatchLink *rdd=rdi->linkEntries[0];
			int index=rdd->position+1;

			SmerId currSmer=rdd->smers[index].smer;

			char buffer[SMER_BASES+1]={0};
			unpackSmer(currSmer, buffer);

			//LOG(LOG_INFO,"ROUTEMERGE\t%s\tTREE\tFORWARD\t%i",buffer,forwardCount);
			//LOG(LOG_INFO,"ROUTEMERGE\t%s\tTREE\tREVERSE\t%i",buffer,reverseCount);
			}
*/
		rttMergeRoutes(routingBuilder.treeBuilder, forwardPatches, reversePatches, forwardCount, reverseCount, prefixCount, suffixCount, orderedDispatches, disp);

		//LOG(LOG_INFO,"writeBuildersIndirect");

		writeBuildersAsIndirectData(&routingBuilder, sliceTag, sliceIndex,heap);

		//LOG(LOG_INFO,"writeBuildersIndirect Done");
		}
	else
		{
/*
		if(entryCount>0)
			{
			RoutingReadData *rdd=rdi->entries[0];
			int index=rdd->indexCount;
			SmerId currFmer=rdd->indexedData[index].fsmer;
			SmerId currRmer=rdd->indexedData[index].rsmer;

			SmerId currSmer=currFmer<currRmer?currFmer:currRmer;
			char buffer[SMER_BASES+1]={0};
			unpackSmer(currSmer, buffer);

			LOG(LOG_INFO,"ROUTEMERGE\t%s\tARRAY\tFORWARD\t%i",buffer,forwardCount);
			LOG(LOG_INFO,"ROUTEMERGE\t%s\tARRAY\tREVERSE\t%i",buffer,reverseCount);
			}
*/
		/*
		if(forwardCount>1)
			dumpRoutePatches("Forward", forwardPatches, forwardCount);

		if(reverseCount>1)
			dumpRoutePatches("Reverse", reversePatches, reverseCount);
*/

//		LOG(LOG_INFO,"Before merge route:");
//		rtaDumpRoutingTableArray(routingBuilder.arrayBuilder);

		rtaMergeRoutes(routingBuilder.arrayBuilder, routingBuilder.tagBuilder,
				forwardPatches, reversePatches, forwardCount, reverseCount, forwardTagCount, reverseTagCount,
				prefixCount, suffixCount, orderedDispatches, disp);

//		LOG(LOG_INFO,"After merge route:");
//		rtaDumpRoutingTableArray(routingBuilder.arrayBuilder);

		writeBuildersAsDirectData(&routingBuilder, sliceTag, sliceIndex, heap);
		}

	return entryCount;
}










/*


typedef struct smerLinkedStr
{
	SmerId smerId;
	u32 compFlag;

	u64 *prefixData;
	SmerId *prefixSmers;
	u8 *prefixSmerExists;
	u32 prefixCount;

	u64 *suffixData;
	SmerId *suffixSmers;
	u8 *suffixSmerExists;
	u32 suffixCount;

	RouteTableEntry *forwardRouteEntries;
	RouteTableEntry *reverseRouteEntries;
	u32 forwardRouteCount;
	u32 reverseRouteCount;

} SmerLinked;



 */

SmerLinked *rtGetLinkedSmer(SmerArray *smerArray, SmerId rootSmerId, MemDispenser *disp)
{
	int sliceNum, index;
	u8 *data=saFindSmerAndData(smerArray, rootSmerId, &sliceNum, &index);

	if(index<0)
		{
		//LOG(LOG_INFO,"Linked Smer: Not Found");
		return NULL;
		}

	SmerLinked *smerLinked=dAlloc(disp,sizeof(SmerLinked));
	smerLinked->smerId=rootSmerId;

	if(data==NULL)
		{
		//LOG(LOG_INFO,"Linked Smer: No data");
		memset(smerLinked,0,sizeof(SmerLinked));
		}
	else
		{
		//LOG(LOG_INFO,"Linked Smer: Got data 1");

		if(rtHeaderIsLiveDirect(*data))
			{
			//LOG(LOG_INFO,"Linked Smer: Got data 2");

			data+=rtGetGapBlockHeaderSize();
			data=stUnpackPrefixesForSmerLinked(smerLinked, data, disp);
			data=stUnpackSuffixesForSmerLinked(smerLinked, data, disp);
			data=rtaUnpackRouteTableArrayForSmerLinked(smerLinked, data, disp);
			rtgUnpackRouteTableTagsForSmerLinked(smerLinked, data, disp);

			for(int i=0;i<smerLinked->prefixCount;i++)
				if(saFindSmer(smerArray, smerLinked->prefixSmers[i])<0)
					smerLinked->prefixSmerExists[i]=0;

			for(int i=0;i<smerLinked->suffixCount;i++)
				if(saFindSmer(smerArray, smerLinked->suffixSmers[i])<0)
					smerLinked->suffixSmerExists[i]=0;

			}
		else if(rtHeaderIsLiveTop(*data))
			{
			//LOG(LOG_INFO,"Linked Smer: Got data 3");

			RoutingComboBuilder routingBuilder;

			routingBuilder.disp=disp;
			routingBuilder.rootPtr=&data;
			routingBuilder.sliceIndex=index;
			routingBuilder.sliceTag=sliceNum & SMER_DISPATCH_GROUP_SLICEMASK;

			createBuildersFromIndirectData(&routingBuilder);
			RouteTableTreeBuilder *treeBuilder=routingBuilder.treeBuilder;

			RouteTableTreeTopBlock *top=(RouteTableTreeTopBlock *)(routingBuilder.topDataPtr+routingBuilder.topDataBlock.headerSize);

			u8 *prefixBlockData=top->data[ROUTE_TOPINDEX_PREFIX];
			if(prefixBlockData!=NULL)
				{
				u8 *prefixData=prefixBlockData+treeBuilder->dataBlocks[ROUTE_TOPINDEX_PREFIX].headerSize;
				stUnpackPrefixesForSmerLinked(smerLinked, prefixData, disp);
				}
			else
				{
				smerLinked->prefixCount=0;
				smerLinked->prefixSmerExists=NULL;
				smerLinked->prefixSmers=NULL;
				smerLinked->prefixes=NULL;
				}

			u8 *suffixBlockData=top->data[ROUTE_TOPINDEX_SUFFIX];
			if(suffixBlockData!=NULL)
				{
				u8 *suffixData=suffixBlockData+treeBuilder->dataBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize;
				stUnpackSuffixesForSmerLinked(smerLinked, suffixData, disp);
				}
			else
				{
				smerLinked->suffixCount=0;
				smerLinked->suffixSmerExists=NULL;
				smerLinked->suffixSmers=NULL;
				smerLinked->suffixes=NULL;
				}

			rttUnpackRouteTableForSmerLinked(smerLinked, &(treeBuilder->forwardWalker), &(treeBuilder->reverseWalker), disp);

			// TAGS not supported yet in Tree mode

			smerLinked->forwardRouteTags=NULL;
			smerLinked->forwardRouteTagCount=0;

			smerLinked->reverseRouteTags=NULL;
			smerLinked->reverseRouteTagCount=0;

			for(int i=0;i<smerLinked->prefixCount;i++)
				if(saFindSmer(smerArray, smerLinked->prefixSmers[i])<0)
					smerLinked->prefixSmerExists[i]=0;

			for(int i=0;i<smerLinked->suffixCount;i++)
				if(saFindSmer(smerArray, smerLinked->suffixSmers[i])<0)
					smerLinked->suffixSmerExists[i]=0;

			}

		//LOG(LOG_INFO,"Linked Smer: Got data 4 - %i %i",smerLinked->forwardRouteCount, smerLinked->reverseRouteCount);
		}

	smerLinked->smerId=rootSmerId;

	return smerLinked;
}






SmerRoutingStats *rtGetRoutingStats(SmerArraySlice *smerArraySlice, u32 sliceNum, MemDispenser *disp)
{
	int smerCount=smerArraySlice->smerCount;

	SmerRoutingStats *stats=dAlloc(disp, sizeof(SmerRoutingStats)*smerCount);
	memset(stats,0,sizeof(SmerRoutingStats)*smerCount);

	for(int i=0;i<smerCount;i++)
		{
		SmerEntry entry=smerArraySlice->smerIT[i];

		SmerId smerId=recoverSmerId(sliceNum, entry);
		stats[i].smerId=smerId;

		unpackSmer(smerId, stats[i].smerStr);

		u8 *smerData=smerArraySlice->smerData[i];

		if(smerData!=NULL)
			{
			RoutingComboBuilder routingBuilder;

			routingBuilder.disp=disp;
			routingBuilder.rootPtr=smerArraySlice->smerData+i;
			routingBuilder.sliceIndex=i;
			routingBuilder.sliceTag=sliceNum & SMER_DISPATCH_GROUP_SLICEMASK;

			u8 header=*smerData;

			if(rtHeaderIsLiveDirect(header))
				{
				stats[i].routeTableFormat=1;
				createBuildersFromDirectData(&routingBuilder);

				stats[i].prefixBytes=routingBuilder.prefixBuilder.oldDataSize;
				stats[i].prefixTails=routingBuilder.prefixBuilder.oldTailCount;

				stats[i].suffixBytes=routingBuilder.suffixBuilder.oldDataSize;
				stats[i].suffixTails=routingBuilder.suffixBuilder.oldTailCount;

				RouteTableArrayBuilder *arrayBuilder=routingBuilder.arrayBuilder;
				rtaGetStats(arrayBuilder,
						&(stats[i].routeTableForwardRouteEntries), &(stats[i].routeTableForwardRoutes),
						&(stats[i].routeTableReverseRouteEntries), &(stats[i].routeTableReverseRoutes),
						&(stats[i].routeTableArrayBytes));
				}
			else if(rtHeaderIsLiveTop(header))
				{
				stats[i].routeTableFormat=2;
				createBuildersFromIndirectData(&routingBuilder);

				stats[i].prefixBytes=routingBuilder.prefixBuilder.oldDataSize;
				stats[i].prefixTails=routingBuilder.prefixBuilder.oldTailCount;

				stats[i].suffixBytes=routingBuilder.suffixBuilder.oldDataSize;
				stats[i].suffixTails=routingBuilder.suffixBuilder.oldTailCount;

				RouteTableTreeBuilder *treeBuilder=routingBuilder.treeBuilder;

				rttGetStats(treeBuilder,
						&(stats[i].routeTableForwardRouteEntries), &(stats[i].routeTableForwardRoutes),
						&(stats[i].routeTableReverseRouteEntries), &(stats[i].routeTableReverseRoutes),
						&(stats[i].routeTableTreeTopBytes), &(stats[i].routeTableTreeArrayBytes),
						&(stats[i].routeTableTreeLeafBytes), &(stats[i].routeTableTreeLeafOffsetBytes), &(stats[i].routeTableTreeLeafEntryBytes),
						&(stats[i].routeTableTreeBranchBytes), &(stats[i].routeTableTreeBranchOffsetBytes), &(stats[i].routeTableTreeBranchChildBytes));
				}
			else
				{
				LOG(LOG_INFO,"Null routing table for Smer: %s",stats[i].smerStr);
				}


			stats[i].routeTableTotalBytes=stats[i].routeTableArrayBytes+
					stats[i].routeTableTreeArrayBytes+stats[i].routeTableTreeBranchBytes+
					stats[i].routeTableTreeLeafBytes+stats[i].routeTableTreeTopBytes;

			stats[i].smerTotalBytes=sizeof(SmerEntry)+sizeof(u8 *)+stats[i].prefixBytes+stats[i].suffixBytes+stats[i].routeTableTotalBytes;
			}

		}



	return stats;
}








