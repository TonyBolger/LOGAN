#include "common.h"



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







static void createBuildersFromNullData(RoutingComboBuilder *builder)
{
	initHeapDataBlock(&(builder->combinedDataBlock), builder->sliceIndex);

	builder->combinedDataPtr=NULL;

	initSeqTailBuilder(&(builder->prefixBuilder), NULL, builder->disp);
	initSeqTailBuilder(&(builder->suffixBuilder), NULL, builder->disp);

	builder->arrayBuilder=dAlloc(builder->disp, sizeof(RouteTableArrayBuilder));
	builder->treeBuilder=NULL;

	rtaInitRouteTableArrayBuilder(builder->arrayBuilder, NULL, builder->disp);

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

	initHeapDataBlock(&(builder->combinedDataBlock), builder->sliceIndex);

	builder->combinedDataBlock.headerSize=headerSize;
	builder->combinedDataPtr=data;

	u8 *afterData=afterHeader;
	afterData=initSeqTailBuilder(&(builder->prefixBuilder), afterData, builder->disp);
	afterData=initSeqTailBuilder(&(builder->suffixBuilder), afterData, builder->disp);

	builder->arrayBuilder=dAlloc(builder->disp, sizeof(RouteTableArrayBuilder));
	builder->treeBuilder=NULL;

	afterData=rtaInitRouteTableArrayBuilder(builder->arrayBuilder, afterData, builder->disp);

	u32 size=afterData-afterHeader;
	builder->combinedDataBlock.dataSize=size;

	builder->upgradedToTree=0;
}


//static
void createBuildersFromIndirectData(RoutingComboBuilder *builder)
{

//	LOG(LOG_INFO,"createBuildersFromIndirectData from top: %p",*(builder->rootPtr));

	u8 *data=*(builder->rootPtr);

	s32 topHeaderSize=rtGetGapBlockHeaderSize();

	builder->arrayBuilder=NULL;
	initHeapDataBlock(&(builder->combinedDataBlock), builder->sliceIndex);

	builder->combinedDataPtr=NULL;

	builder->treeBuilder=dAlloc(builder->disp, sizeof(RouteTableTreeBuilder));
	initHeapDataBlock(&(builder->topDataBlock), builder->sliceIndex);

	builder->topDataBlock.headerSize=topHeaderSize;
	builder->topDataBlock.dataSize=sizeof(RouteTableTreeTopBlock);

	builder->topDataPtr=data;

	builder->upgradedToTree=0;
	//HeapDataBlock dataBlocks[8];

	RouteTableTreeBuilder *treeBuilder=builder->treeBuilder;

	treeBuilder->disp=builder->disp;
	treeBuilder->newEntryCount=0;

	//treeBuilder->topDataBlock->dataSize=data;

	RouteTableTreeTopBlock *top=(RouteTableTreeTopBlock *)(data+topHeaderSize);

	for(int i=0;i<ROUTE_TOPINDEX_MAX;i++)
		{
		initHeapDataBlock(treeBuilder->dataBlocks+i, builder->sliceIndex);

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
		u8 *prefixDataEnd=initSeqTailBuilder(&(builder->prefixBuilder), prefixData, builder->disp);
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
		u8 *suffixDataEnd=initSeqTailBuilder(&(builder->suffixBuilder), suffixData, builder->disp);
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


static void writeBuildersAsDirectData(RoutingComboBuilder *builder, s8 sliceTag, s32 sliceIndex, MemCircHeap *circHeap)
{
	if(!(getSeqTailBuilderDirty(&(builder->prefixBuilder)) ||
		 getSeqTailBuilderDirty(&(builder->suffixBuilder)) ||
		 rtaGetRouteTableArrayBuilderDirty(builder->arrayBuilder)))
		{
		LOG(LOG_INFO,"Nothing to write");
		return;
		}

	int oldTotalSize=builder->combinedDataBlock.headerSize+builder->combinedDataBlock.dataSize;

	int prefixPackedSize=getSeqTailBuilderPackedSize(&(builder->prefixBuilder));
	int suffixPackedSize=getSeqTailBuilderPackedSize(&(builder->suffixBuilder));
	int routeTablePackedSize=rtaGetRouteTableArrayBuilderPackedSize(builder->arrayBuilder);

	int totalSize=rtGetGapBlockHeaderSize()+prefixPackedSize+suffixPackedSize+routeTablePackedSize;

	if(totalSize>16083)
		{
		int oldShifted=oldTotalSize>>14;
		int newShifted=totalSize>>14;

		if(oldShifted!=newShifted)
			{
			LOG(LOG_INFO,"LARGE ALLOC: %i",totalSize);
			}

		}

	u8 *oldData=builder->combinedDataPtr;
	u8 *newData;

	// Mark old block as dead
	rtHeaderMarkDead(oldData);

	s32 oldTagOffset=0;
//	LOG(LOG_INFO,"CircAlloc %i",totalSize);
	newData=circAlloc(circHeap, totalSize, sliceTag, sliceIndex, &oldTagOffset);

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

	newData=writeSeqTailBuilderPackedData(&(builder->prefixBuilder), newData);
	newData=writeSeqTailBuilderPackedData(&(builder->suffixBuilder), newData);
	newData=rtaWriteRouteTableArrayBuilderPackedData(builder->arrayBuilder, newData);

}

static int considerUpgradingToTree(RoutingComboBuilder *builder, int newForwardRoutes, int newReverseRoutes)
{
	if(builder->arrayBuilder==NULL)
		return 0;

	if(builder->treeBuilder!=NULL)
		return 0;

	int existingRoutes=(builder->arrayBuilder->oldForwardEntryCount)+(builder->arrayBuilder->oldReverseEntryCount);
	int totalRoutes=existingRoutes+newForwardRoutes+newReverseRoutes;

	return totalRoutes>ROUTING_TREE_THRESHOLD;
}


static void upgradeToTree(RoutingComboBuilder *builder, s32 prefixCount, s32 suffixCount)
{
	//LOG(LOG_INFO,"upgradeToTree");

	RouteTableTreeBuilder *treeBuilder=dAlloc(builder->disp, sizeof(RouteTableTreeBuilder));
	builder->treeBuilder=treeBuilder;

	builder->topDataPtr=NULL;
	initHeapDataBlock(&(builder->topDataBlock), builder->sliceIndex);

	rttUpgradeToRouteTableTreeBuilder(builder->arrayBuilder,  builder->treeBuilder, builder->sliceIndex, prefixCount, suffixCount, builder->disp);

	builder->upgradedToTree=1;
}



static RouteTableTreeTopBlock *writeBuildersAsIndirectData_writeTop(RoutingComboBuilder *routingBuilder, s8 sliceTag, s32 sliceIndex, MemCircHeap *circHeap)
{
	if(routingBuilder->topDataPtr==NULL)
		{
		routingBuilder->topDataBlock.dataSize=sizeof(RouteTableTreeTopBlock);
		routingBuilder->topDataBlock.headerSize=rtGetGapBlockHeaderSize();

		s32 totalSize=routingBuilder->topDataBlock.dataSize+routingBuilder->topDataBlock.headerSize;

		s32 oldTagOffset=0;
		u8 *newTopData=circAlloc(circHeap, totalSize, sliceTag, sliceIndex, &oldTagOffset);
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


static int writeBuildersAsIndirectData_calcTailAndArraySpace(RoutingComboBuilder *routingBuilder, s16 indexSize, HeapDataBlock *neededBlocks)
{
	neededBlocks[ROUTE_TOPINDEX_PREFIX].headerSize=rtGetTailBlockHeaderSize(indexSize);
	neededBlocks[ROUTE_TOPINDEX_PREFIX].dataSize=getSeqTailBuilderPackedSize(&(routingBuilder->prefixBuilder));

	neededBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize=rtGetTailBlockHeaderSize(indexSize);
	neededBlocks[ROUTE_TOPINDEX_SUFFIX].dataSize=getSeqTailBuilderPackedSize(&(routingBuilder->suffixBuilder));

	RouteTableTreeBuilder *treeBuilder=routingBuilder->treeBuilder;

	for(int i=ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0; i<=ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0;i++)
		{
		neededBlocks[i].headerSize=rtGetIndexedBlockHeaderSize_0(indexSize);

		RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);
		//LOG(LOG_INFO,"Array %i had %i, now has %i",i,arrayProxy->dataCount,arrayProxy->newDataCount);

		s32 entries=rttGetArrayEntries(arrayProxy);

		if(entries>ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES) // Change to indirect mode
			{
			LOG(LOG_INFO,"Indirect mode due to %i entries",entries);

			neededBlocks[i].variant=1;
			neededBlocks[i].midHeaderSize=rtGetIndexedBlockHeaderSize_1(indexSize);
			neededBlocks[i].completeMidCount=rttCalcFirstLevelArrayCompleteEntries(entries);

			neededBlocks[i].completeMidDataSize=rttCalcArraySize(ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES);

			int additionalEntries=rttCalcFirstLevelArrayAdditionalEntries(entries);
			neededBlocks[i].additionalMidDataSize=(additionalEntries>0) ? rttCalcArraySize(additionalEntries) : 0;

			LOG(LOG_INFO,"Additional entries of %i require %i",additionalEntries, neededBlocks[i].additionalMidDataSize);
			}

		neededBlocks[i].dataSize=rttGetTopArraySize(arrayProxy);

		if(neededBlocks[i].variant)
			LOG(LOG_INFO,"Array %i - Entries %i %i - Sizes %i %i",i, arrayProxy->dataAlloc, arrayProxy->newDataAlloc, neededBlocks[i].headerSize, neededBlocks[i].dataSize);
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
			if((existingSize!=neededSize) || (treeBuilder->dataBlocks[i].midHeaderSize==0)) // Always new ptr block if upgrading
				totalNeededSize+=neededSize;

			int additionalCompleteBlocks=neededBlocks[i].completeMidCount-treeBuilder->dataBlocks[i].completeMidCount;
			totalNeededSize+=(neededBlocks[i].midHeaderSize+neededBlocks[i].completeMidDataSize)*additionalCompleteBlocks;

			LOG(LOG_INFO,"Indirect array: adding %i (%i to %i) complete blocks of %i %i",
					additionalCompleteBlocks, treeBuilder->dataBlocks[i].completeMidCount, neededBlocks[i].completeMidCount,
					neededBlocks[i].midHeaderSize, neededBlocks[i].completeMidDataSize);

			if(neededBlocks[i].additionalMidDataSize>0)
				{
				int existingAdditionalSize=treeBuilder->dataBlocks[i].midHeaderSize+treeBuilder->dataBlocks[i].additionalMidDataSize;
				int neededAdditionalSize=neededBlocks[i].midHeaderSize+neededBlocks[i].additionalMidDataSize;

				if(existingAdditionalSize!=neededAdditionalSize)
					{
					LOG(LOG_INFO,"Indirect array: adding incomplete block of %i %i with %i entries", neededBlocks[i].midHeaderSize, neededBlocks[i].additionalMidDataSize);

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
		HeapDataBlock *neededBlocks, RouteTableTreeTopBlock *topPtr, u8 *newArrayData)
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

		rtHeaderMarkDead(oldData);
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

		rtHeaderMarkDead(oldData);
		}
	else
		{
		if(existingSize>neededSize)
			LOG(LOG_CRITICAL,"Unexpected reduction in size %i to %i",existingSize,neededSize);

//			LOG(LOG_INFO,"Suffix Rewrite in place %p",topPtr->data[ROUTE_TOPINDEX_SUFFIX]);
		}


	return newArrayData;
}


static u8 *writeBuildersAsIndirectData_bindArrays(RouteTableTreeBuilder *treeBuilder, s32 sliceIndex, s16 indexSize,
		HeapDataBlock *neededBlocks, RouteTableTreeTopBlock *topPtr, u8 *newArrayData)
{
	s32 existingSize=0;
	s32 neededSize=0;

	for(int i=ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0; i<=ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0;i++)
		{
		if(neededBlocks[i].variant>0) // Indirect mode
			{
			if(treeBuilder->dataBlocks[i].variant==0) // Upgrading from direct
				{
				LOG(LOG_INFO,"Before Upgrade: START");
				RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);
				dumpBlockArrayProxy(arrayProxy);

				LOG(LOG_INFO,"Before Upgrade: END");

				LOG(LOG_INFO,"Indirect raw block at %p",newArrayData);

				rtEncodeArrayBlockHeader_Leaf0(i&1, 1, indexSize, sliceIndex, newArrayData);

				u8 *oldData=topPtr->data[i];

				topPtr->data[i]=newArrayData;
				newArrayData+=neededBlocks[i].headerSize;
				memset(newArrayData, 0, neededBlocks[i].dataSize);

				LOG(LOG_INFO,"Indirect data block at %p",newArrayData);

				RouteTableTreeArrayBlock *shallowPtrBlock=(RouteTableTreeArrayBlock *)newArrayData;
				newArrayData+=neededBlocks[i].dataSize;

				int oldDataOffset=0;

				for(int j=0;j<neededBlocks[i].completeMidCount;j++) // Build 'full' deep arrays (part or fully init from old data)
					{
					shallowPtrBlock->data[j]=newArrayData;
					newArrayData+=rtEncodeArrayBlockHeader_Leaf1(i&0x1, indexSize, sliceIndex, j, newArrayData);

					LOG(LOG_INFO,"Encoded as %02x",*(shallowPtrBlock->data[j]));

					RouteTableTreeArrayBlock *arrayDataBlock=(RouteTableTreeArrayBlock *)newArrayData;

					int oldDataToCopy=MIN(arrayProxy->dataCount-oldDataOffset, ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES);
					if(oldDataToCopy>0)
						{
						LOG(LOG_INFO,"Transferring %i entries with offset %i",oldDataToCopy, oldDataOffset);

						memcpy(arrayDataBlock->data, arrayProxy->dataBlock->data+oldDataOffset, oldDataToCopy*sizeof(u8 *));
						oldDataOffset+=oldDataToCopy;
						}

					if(oldDataToCopy<ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES)
						memset(arrayDataBlock->data+oldDataToCopy, 0, (ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES-oldDataToCopy)*sizeof(u8 *));

					arrayDataBlock->dataAlloc=ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES;
					newArrayData+=neededBlocks[i].completeMidDataSize;
					}

				shallowPtrBlock->dataAlloc=neededBlocks[i].completeMidCount;

				if(neededBlocks[i].additionalMidDataSize) // Build optional 'partial' deep array (part or fully init from old data)
					{
					shallowPtrBlock->dataAlloc++;
					int additionalIndex=neededBlocks[i].completeMidCount;

					shallowPtrBlock->data[additionalIndex]=newArrayData;
					newArrayData+=rtEncodeArrayBlockHeader_Leaf1(i&0x1, indexSize, sliceIndex, additionalIndex, newArrayData);

					RouteTableTreeArrayBlock *arrayDataBlock=(RouteTableTreeArrayBlock *)newArrayData;

					int additionalEntries=rttCalcFirstLevelArrayAdditionalEntries(rttGetArrayEntries(arrayProxy));
					int oldDataToCopy=MIN(arrayProxy->dataCount-oldDataOffset, additionalEntries);

					if(oldDataToCopy>0)
						{
						LOG(LOG_INFO,"Tranferring %i entries with offset %i",oldDataToCopy, oldDataOffset);

						memcpy(arrayDataBlock->data, arrayProxy->dataBlock->data+oldDataOffset, oldDataToCopy*sizeof(u8 *));
						oldDataOffset+=oldDataToCopy;
						}

					if(oldDataToCopy<additionalEntries)
						memset(arrayDataBlock->data+oldDataToCopy, 0, (additionalEntries-oldDataToCopy)*sizeof(u8 *));

					arrayDataBlock->dataAlloc=additionalEntries;
					newArrayData+=neededBlocks[i].additionalMidDataSize;
					}

				rttBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, neededBlocks[i].variant); // Updates ptrBlock and dataBlock

				arrayProxy->ptrAlloc=shallowPtrBlock->dataAlloc;
				arrayProxy->ptrCount=shallowPtrBlock->dataAlloc;

				arrayProxy->dataAlloc=arrayProxy->newDataAlloc;

				if(oldData!=NULL)
					rtHeaderMarkDead(oldData);
				//LOG(LOG_CRITICAL,"Upgradey - has %i", arrayProxy->dataAlloc);

				LOG(LOG_INFO,"After Upgrade: START");
				dumpBlockArrayProxy(arrayProxy);

				LOG(LOG_INFO,"After Upgrade: END");
				}
			else
				{
				LOG(LOG_CRITICAL,"TODO: Non-upgrade indirect");

/*
				existingSize=treeBuilder->dataBlocks[i].headerSize+treeBuilder->dataBlocks[i].dataSize;
				neededSize=neededBlocks[i].headerSize+neededBlocks[i].dataSize;

				s32 sizeDiff=neededSize-existingSize;

				if(sizeDiff>0)
					{
					rtEncodeArrayBlockHeader(i,ARRAY_TYPE_SHALLOW_PTR, indexSize, sliceIndex, 0, newArrayData);
					u8 *oldData=topPtr->data[i];

					topPtr->data[i]=newArrayData;
					newArrayData+=neededBlocks[i].headerSize;

					if(oldData!=NULL) // Clear extended region only
						{
						memcpy(newArrayData, oldData+treeBuilder->dataBlocks[i].headerSize, treeBuilder->dataBlocks[i].dataSize);
						memset(newArrayData+treeBuilder->dataBlocks[i].dataSize, 0, neededBlocks[i].dataSize-treeBuilder->dataBlocks[i].dataSize);

						rtHeaderMarkDead(oldData);
						}
					else // Clear full region (not sure how this can happen)
						{
						memset(newArrayData, 0, neededBlocks[i].dataSize);
						}
					}
*/

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

					rtHeaderMarkDead(oldData);
					}
				else // Clear full region
					{
					memset(newArrayData, 0, neededBlocks[i].dataSize);
					}

				RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);
				rttBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, 0); // In direct mode, so no problem
				if(arrayProxy->newDataAlloc!=0)
					{
					RouteTableTreeArrayBlock *arrayBlock=(RouteTableTreeArrayBlock *)newArrayData;
					arrayProxy->dataAlloc=arrayProxy->newDataAlloc;
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

		u8 *oldBranchRawData=getBlockArrayDataEntryRaw(branchArrayProxy, subindex);
		if(oldBranchRawData!=NULL)
			{
			RouteTableTreeBranchBlock *oldBranchData=(RouteTableTreeBranchBlock *)(oldBranchRawData+headerSize);
			oldBranchChildAlloc=oldBranchData->childAlloc;
			}

		RouteTableTreeBranchProxy *newBranchProxy=(RouteTableTreeBranchProxy *)(branchArrayProxy->newEntries[i].proxy);
		RouteTableTreeBranchBlock *newBranchData=newBranchProxy->dataBlock;

		if(newBranchData->childAlloc!=oldBranchChildAlloc)
			totalSize+=headerSize+getRouteTableTreeBranchSize_Existing(newBranchData);

		}

	return totalSize;
}


void writeBuildersAsIndirectData_mergeTopArrayUpdates_branch(RouteTableTreeArrayProxy *branchArrayProxy, int arrayNum, int indexSize, int sliceIndex, u8 *newData, s32 newDataSize)
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

		u8 *oldBranchRawData=getBlockArrayDataEntryRaw(branchArrayProxy, subindex);

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

			setBlockArrayDataEntryRaw(branchArrayProxy, subindex, newData);

			s32 newHeaderSize=rtEncodeEntityBlockHeader_Branch1(arrayNum==ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0,
					indexSize, sliceIndex, subindex>>ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_SHIFT, newData);

			if(newHeaderSize!=headerSize)
				{
				LOG(LOG_CRITICAL,"Header size mismatch %i vs %i",headerSize, newHeaderSize);
				}

			newData+=headerSize;

			s32 dataSize=getRouteTableTreeBranchSize_Existing(newBranchData);
			memcpy(newData, newBranchData, dataSize);

				//LOG(LOG_INFO,"Copying %i bytes of branch data from %p to %p",dataSize,newBranchData,newData);

			newData+=dataSize;

			rtHeaderMarkDead(oldBranchRawData);
			}
		else
			{
//				LOG(LOG_INFO,"Branch rewrite to %p (%i %i)",arrayProxy->dataBlock->data[i], newBranchData->childAlloc,oldBranchChildAlloc);

			s32 dataSize=getRouteTableTreeBranchSize_Existing(newBranchData);
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

	//LOG(LOG_INFO,"New data Alloc %i",leafArrayProxy->newDataAlloc);

	for(int i=0;i<leafArrayProxy->newEntriesCount;i++)
		{
		int oldLeafSize=0;

		int subindex=leafArrayProxy->newEntries[i].index;
		int headerSize=(subindex<ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)?rtGetIndexedBlockHeaderSize_1(indexSize):rtGetIndexedBlockHeaderSize_2(indexSize);

		u8 *oldLeafRawData=getBlockArrayDataEntryRaw(leafArrayProxy, subindex);

		if(oldLeafRawData!=NULL)
			{
			RouteTableTreeLeafBlock *oldLeafData=(RouteTableTreeLeafBlock *)(oldLeafRawData+headerSize);
			oldLeafSize=getRouteTableTreeLeafSize_Existing(oldLeafData);
			}

		RouteTableTreeLeafProxy *newLeafProxy=(RouteTableTreeLeafProxy *)(leafArrayProxy->newEntries[i].proxy);
		RouteTableTreeLeafBlock *newLeafData=newLeafProxy->dataBlock;

		int newLeafSize=getRouteTableTreeLeafSize_Existing(newLeafData);

		if(newLeafSize!=oldLeafSize)
			totalSize+=headerSize+newLeafSize;
		}

	return totalSize;
}


void writeBuildersAsIndirectData_mergeTopArrayUpdates_leaf(RouteTableTreeArrayProxy *leafArrayProxy, int arrayNum, int indexSize, int sliceIndex, u8 *newData, s32 newDataSize)
{

	if(leafArrayProxy->newEntries==NULL)
		return;

	u8 *endNewData=newData+newDataSize;

	//if(arrayProxy->dataBlock->dataAlloc==0)
		//{
		//LOG(LOG_INFO,"New Leaf Array - setting array length to %i",arrayProxy->newDataAlloc);

//	if(arrayProxy->newData!=NULL)
//		arrayProxy->dataBlock->dataAlloc=arrayProxy->newDataAlloc;

		//}

	for(int i=0;i<leafArrayProxy->newEntriesCount;i++)
		{
		RouteTableTreeLeafBlock *oldLeafData=NULL;
		int oldLeafSize=0;

		int subindex=leafArrayProxy->newEntries[i].index;
		int headerSize=(subindex<ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)?rtGetIndexedBlockHeaderSize_1(indexSize):rtGetIndexedBlockHeaderSize_2(indexSize);

		u8 *oldLeafRawData=getBlockArrayDataEntryRaw(leafArrayProxy, subindex);

		if(oldLeafRawData!=NULL)
			{
			oldLeafData=(RouteTableTreeLeafBlock *)(oldLeafRawData+headerSize);
			oldLeafSize=getRouteTableTreeLeafSize_Existing(oldLeafData);
			}

		RouteTableTreeLeafProxy *newLeafProxy=(RouteTableTreeLeafProxy *)(leafArrayProxy->newEntries[i].proxy);
		RouteTableTreeLeafBlock *newLeafData=newLeafProxy->dataBlock;

		int newLeafSize=getRouteTableTreeLeafSize_Existing(newLeafData);

		if(newLeafSize!=oldLeafSize)
			{
//			LOG(LOG_INFO,"Leaf Move/Expand write to %p from %p (%i %i)",newData, oldLeafRawData, newLeafSize, oldLeafSize);

			setBlockArrayDataEntryRaw(leafArrayProxy, subindex, newData);

			s32 newHeaderSize=(subindex<ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)?
					rtEncodeEntityBlockHeader_Leaf1(arrayNum==ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0, indexSize, sliceIndex, subindex>>ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_SHIFT, newData):
					rtEncodeEntityBlockHeader_Leaf2(arrayNum==ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0, indexSize, sliceIndex, subindex>>ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_SHIFT, newData);


			if(newHeaderSize!=headerSize)
				{
				LOG(LOG_CRITICAL,"Header size mismatch %i vs %i",headerSize, newHeaderSize);
				}

			newData+=headerSize;

			s32 dataSize=getRouteTableTreeLeafSize_Existing(newLeafData);
			memcpy(newData, newLeafData, dataSize);
			newData+=dataSize;

			rtHeaderMarkDead(oldLeafRawData);
			}
		else
			{
//			LOG(LOG_INFO,"Leaf rewrite to %p (%i %i)",leafArrayProxy->dataBlock->data[index],  newLeafSize, oldLeafSize);

			memcpy(leafArrayProxy->dataBlock->data[subindex]+headerSize, newLeafData, newLeafSize);
			}
		}

	if(endNewData!=newData)
		LOG(LOG_CRITICAL,"New Leaf Data doesn't match expected: New: %p vs Expected: %p",newData,endNewData);

}




static void writeBuildersAsIndirectData(RoutingComboBuilder *routingBuilder, s8 sliceTag, s32 sliceIndex, MemCircHeap *circHeap)
{
	// May need to create:
	//		top level block
	// 		prefix/suffix blocks
	// 		direct array blocks
	//      indirect array blocks
	// 		branch blocks
	//		leaf blocks
	//		offset blocks

//	LOG(LOG_INFO,"writeBuildersAsIndirectData");

//	rttDumpRoutingTable(routingBuilder->treeBuilder);

	if(routingBuilder->upgradedToTree)
		{
		u8 *oldData=*(routingBuilder->rootPtr);

		rtHeaderMarkDead(oldData);
		}

	s16 indexSize=varipackLength(sliceIndex);
	RouteTableTreeBuilder *treeBuilder=routingBuilder->treeBuilder;

	RouteTableTreeTopBlock *topPtr=writeBuildersAsIndirectData_writeTop(routingBuilder, sliceTag, sliceIndex, circHeap);

	HeapDataBlock neededBlocks[ROUTE_TOPINDEX_MAX];
	for(int i=0;i<ROUTE_TOPINDEX_MAX;i++)
		initHeapDataBlock(neededBlocks+i, sliceIndex);

	int totalNeededSize=writeBuildersAsIndirectData_calcTailAndArraySpace(routingBuilder, indexSize, neededBlocks);

	if(totalNeededSize>0)
		{
		u8 *newArrayData=circAlloc(circHeap, totalNeededSize, sliceTag, 0, NULL);

		memset(newArrayData,0,totalNeededSize);
		u8 *endArrayData=newArrayData+totalNeededSize;

		topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root after alloc

		// Bind tails to newly allocated blocks
		newArrayData=writeBuildersAsIndirectData_bindTails(treeBuilder, sliceIndex, indexSize, neededBlocks, topPtr, newArrayData);

		// Bind direct/indirect arrays to newly allocated blocks
		newArrayData=writeBuildersAsIndirectData_bindArrays(treeBuilder, sliceIndex, indexSize, neededBlocks, topPtr, newArrayData);

		if(endArrayData!=newArrayData)
			LOG(LOG_CRITICAL,"Array end did not match expected: Found: %p vs Expected: %p",newArrayData, endArrayData);
		}


	topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root after alloc

//	LOG(LOG_INFO,"Tails");

	if((routingBuilder->upgradedToTree) || getSeqTailBuilderDirty(&(routingBuilder->prefixBuilder)))
		{
//		LOG(LOG_INFO,"Write prefix to %p",topPtr->data[ROUTE_TOPINDEX_PREFIX]);
		writeSeqTailBuilderPackedData(&(routingBuilder->prefixBuilder), topPtr->data[ROUTE_TOPINDEX_PREFIX]+neededBlocks[ROUTE_TOPINDEX_PREFIX].headerSize);
		}

	if((routingBuilder->upgradedToTree) || getSeqTailBuilderDirty(&(routingBuilder->suffixBuilder)))
		{
//		LOG(LOG_INFO,"Write suffix to %p",topPtr->data[ROUTE_TOPINDEX_SUFFIX]);
		writeSeqTailBuilderPackedData(&(routingBuilder->suffixBuilder), topPtr->data[ROUTE_TOPINDEX_SUFFIX]+neededBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize);
		}

	for(int i=ROUTE_TOPINDEX_FORWARD_LEAF_ARRAY_0; i<=ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0;i++)
		{
		RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);

		if((routingBuilder->upgradedToTree) || rttGetArrayDirty(arrayProxy))
			{
//			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize);

//			LOG(LOG_INFO,"Write leaf array to %p",topPtr->data[i]);

			rttBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, neededBlocks[i].variant);	// Rebind array after alloc (root already done)

			s32 size=writeBuildersAsIndirectData_mergeTopArrayUpdates_leaf_accumulateSize(arrayProxy, indexSize);
			u8 *newLeafData=circAlloc(circHeap, size, sliceTag, 0, NULL);

			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root & array after alloc
			rttBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, neededBlocks[i].variant);

			writeBuildersAsIndirectData_mergeTopArrayUpdates_leaf(arrayProxy, i, indexSize, sliceIndex, newLeafData, size);
			}
		}

	for(int i=ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0; i<=ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0;i++)
		{
		RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);

		if((routingBuilder->upgradedToTree)|| rttGetArrayDirty(arrayProxy))
			{
//			LOG(LOG_INFO,"Write branch array to %p",topPtr->data[i]);

//			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize);
			rttBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, neededBlocks[i].variant);	// Rebind array after alloc (root already done)

			s32 size=writeBuildersAsIndirectData_mergeTopArrayUpdates_branch_accumulateSize(arrayProxy, indexSize);
			u8 *newBranchData=circAlloc(circHeap, size, sliceTag, 0, NULL);

			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root & array after alloc
			rttBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize, neededBlocks[i].variant);

			writeBuildersAsIndirectData_mergeTopArrayUpdates_branch(arrayProxy, i, indexSize, sliceIndex, newBranchData, size);
			}
		}

}




static void createRoutePatches(RoutingIndexedReadReferenceBlock *rdi, int entryCount,
		SeqTailBuilder *prefixBuilder, SeqTailBuilder *suffixBuilder,
		RoutePatch *forwardPatches, RoutePatch *reversePatches,
		int *forwardCountPtr, int *reverseCountPtr)
{
	int forwardCount=0, reverseCount=0;

	for(int i=0;i<entryCount;i++)
	{
		RoutingReadData *rdd=rdi->entries[i];

		int index=rdd->indexCount;

		if(0)
		{
			SmerId currSmer=rdd->fsmers[index];
			SmerId prevSmer=rdd->fsmers[index+1];
			SmerId nextSmer=rdd->fsmers[index-1];

			int upstreamLength=rdd->readIndexes[index]-rdd->readIndexes[index+1];
			int downstreamLength=rdd->readIndexes[index-1]-rdd->readIndexes[index];

			char bufferP[SMER_BASES+1]={0};
			char bufferC[SMER_BASES+1]={0};
			char bufferN[SMER_BASES+1]={0};

			unpackSmer(prevSmer, bufferP);
			unpackSmer(currSmer, bufferC);
			unpackSmer(nextSmer, bufferN);

			LOG(LOG_INFO,"Read Orientation: %s (%i) %s %s (%i)",bufferP, upstreamLength, bufferC, bufferN, downstreamLength);
		}

		SmerId currFmer=rdd->fsmers[index];
		SmerId currRmer=rdd->rsmers[index];

		if(currFmer<=currRmer) // Canonical Read Orientation
			{
//				smerId=currFmer;

				/*
				if(smerId==49511689627288L)
					{
					LOG(LOG_INFO,"Adding forward route to %li",currFmer);
					LOG(LOG_INFO,"Existing Prefixes: %i Suffixes: %i", prefixBuilder.oldTailCount, suffixBuilder.oldTailCount);

					dump=1;
					}
*/

				SmerId prefixSmer=rdd->rsmers[index+1]; // Previous smer in read, reversed
				SmerId suffixSmer=rdd->fsmers[index-1]; // Next smer in read

				int prefixLength=rdd->readIndexes[index]-rdd->readIndexes[index+1];
				int suffixLength=rdd->readIndexes[index-1]-rdd->readIndexes[index];

				forwardPatches[forwardCount].next=NULL;
				forwardPatches[forwardCount].rdiPtr=rdi->entries+i;
//				forwardPatches[forwardCount].rdiIndex=i;

				forwardPatches[forwardCount].prefixIndex=findOrCreateSeqTail(prefixBuilder, prefixSmer, prefixLength);
				forwardPatches[forwardCount].suffixIndex=findOrCreateSeqTail(suffixBuilder, suffixSmer, suffixLength);


				if(0)
					{
					char bufferP[SMER_BASES+1]={0};
					char bufferN[SMER_BASES+1]={0};
					char bufferS[SMER_BASES+1]={0};

					unpackSmer(prefixSmer, bufferP);
					unpackSmer(currFmer, bufferN);
					unpackSmer(suffixSmer, bufferS);

					LOG(LOG_INFO,"Node Orientation: %s (%i) @ %i %s %s (%i) @ %i",
							bufferP, prefixLength, forwardPatches[forwardCount].prefixIndex,  bufferN, bufferS, suffixLength, forwardPatches[forwardCount].suffixIndex);
					}

				forwardCount++;
			}
		else	// Reverse-complement Read Orientation
			{
//				smerId=currRmer;

				/*
				if(smerId==49511689627288L)
					{
					LOG(LOG_INFO,"Adding reverse route to %li",currRmer);
					LOG(LOG_INFO,"Existing Prefixes: %i Suffixes: %i", prefixBuilder.oldTailCount, suffixBuilder.oldTailCount);

					dump=1;
					}
*/

				SmerId prefixSmer=rdd->fsmers[index-1]; // Next smer in read
				SmerId suffixSmer=rdd->rsmers[index+1]; // Previous smer in read, reversed


				int prefixLength=rdd->readIndexes[index-1]-rdd->readIndexes[index];
				int suffixLength=rdd->readIndexes[index]-rdd->readIndexes[index+1];

				reversePatches[reverseCount].next=NULL;
				reversePatches[reverseCount].rdiPtr=rdi->entries+i;
//				reversePatches[reverseCount].rdiIndex=i;

				reversePatches[reverseCount].prefixIndex=findOrCreateSeqTail(prefixBuilder, prefixSmer, prefixLength);
				reversePatches[reverseCount].suffixIndex=findOrCreateSeqTail(suffixBuilder, suffixSmer, suffixLength);

				if(0)
					{
					char bufferP[SMER_BASES+1]={0};
					char bufferN[SMER_BASES+1]={0};
					char bufferS[SMER_BASES+1]={0};

					unpackSmer(prefixSmer, bufferP);
					unpackSmer(currRmer, bufferN);
					unpackSmer(suffixSmer, bufferS);

					LOG(LOG_INFO,"Node Orientation: %s (%i) @ %i %s %s (%i) @ %i",
							bufferP, prefixLength, reversePatches[forwardCount].prefixIndex,  bufferN, bufferS, suffixLength, reversePatches[forwardCount].suffixIndex);
					}

				reverseCount++;
			}
	}

	// Then sort new forward and reverse routes, if more than one

	if(forwardCount>1)
		qsort(forwardPatches, forwardCount, sizeof(RoutePatch), forwardPrefixSorter);

	if(reverseCount>1)
		qsort(reversePatches, reverseCount, sizeof(RoutePatch), reverseSuffixSorter);

	*forwardCountPtr=forwardCount;
	*reverseCountPtr=reverseCount;

}




int rtRouteReadsForSmer(RoutingIndexedReadReferenceBlock *rdi, SmerArraySlice *slice,
		RoutingReadData **orderedDispatches, MemDispenser *disp, MemCircHeap *circHeap, u8 sliceTag)
{
	s32 sliceIndex=rdi->sliceIndex;

	u8 *smerDataOrig=slice->smerData[sliceIndex];
	u8 *smerData=smerDataOrig;

	//if(smerData!=NULL)
//		LOG(LOG_INFO, "Index: %i of %i Entries %i Data: %p",sliceIndex,slice->smerCount, rdi->entryCount, smerData);

	RoutingComboBuilder routingBuilder;

	routingBuilder.disp=disp;
	routingBuilder.rootPtr=slice->smerData+sliceIndex;
	routingBuilder.sliceIndex=sliceIndex;

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

	int entryCount=rdi->entryCount;

	int forwardCount=0;
	int reverseCount=0;

	RoutePatch *forwardPatches=dAlloc(disp,sizeof(RoutePatch)*entryCount);
	RoutePatch *reversePatches=dAlloc(disp,sizeof(RoutePatch)*entryCount);

	createRoutePatches(rdi, entryCount, &(routingBuilder.prefixBuilder), &(routingBuilder.suffixBuilder),
			forwardPatches, reversePatches, &forwardCount, &reverseCount);

	s32 prefixCount=getSeqTailTotalTailCount(&(routingBuilder.prefixBuilder))+1;
	s32 suffixCount=getSeqTailTotalTailCount(&(routingBuilder.suffixBuilder))+1;

	if(considerUpgradingToTree(&routingBuilder, forwardCount, reverseCount))
		{
		//LOG(LOG_INFO,"Prefix Old %i New %i",routingBuilder.prefixBuilder.oldTailCount,routingBuilder.prefixBuilder.newTailCount);
		//LOG(LOG_INFO,"Suffix Old %i New %i",routingBuilder.suffixBuilder.oldTailCount,routingBuilder.suffixBuilder.newTailCount);

		upgradeToTree(&routingBuilder, prefixCount, suffixCount);
		}

	if(routingBuilder.treeBuilder!=NULL)
		{
		rttMergeRoutes(routingBuilder.treeBuilder, forwardPatches, reversePatches, forwardCount, reverseCount, prefixCount, suffixCount, orderedDispatches, disp);

		writeBuildersAsIndirectData(&routingBuilder, sliceTag, sliceIndex,circHeap);
		}
	else
		{
		rtaMergeRoutes(routingBuilder.arrayBuilder, forwardPatches, reversePatches, forwardCount, reverseCount, prefixCount, suffixCount, orderedDispatches, disp);

		writeBuildersAsDirectData(&routingBuilder, sliceTag, sliceIndex, circHeap);

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

		data=unpackPrefixesForSmerLinked(smerLinked, data, disp);

		data=unpackSuffixesForSmerLinked(smerLinked, data, disp);

		rtaUnpackRouteTableArrayForSmerLinked(smerLinked, data, disp);

		for(int i=0;i<smerLinked->prefixCount;i++)
			if(saFindSmer(smerArray, smerLinked->prefixSmers[i])<0)
				smerLinked->prefixSmerExists[i]=0;

		for(int i=0;i<smerLinked->suffixCount;i++)
			if(saFindSmer(smerArray, smerLinked->suffixSmers[i])<0)
				smerLinked->suffixSmerExists[i]=0;

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








