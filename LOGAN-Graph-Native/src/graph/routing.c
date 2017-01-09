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
	//builder->combinedDataBlock.subindexSize=0;
	//builder->combinedDataBlock.subindex=-1;
	builder->combinedDataBlock.headerSize=0;
	builder->combinedDataBlock.dataSize=0;
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

//	builder->combinedDataBlock.subindexSize=0;
//	builder->combinedDataBlock.subindex=-1;
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
	//LOG(LOG_INFO,"Begin parse indirect from top: %p",*(builder->rootPtr));

	u8 *data=*(builder->rootPtr);

	s32 topHeaderSize=rtGetGapBlockHeaderSize();

	builder->arrayBuilder=NULL;
	builder->combinedDataBlock.headerSize=0;
	builder->combinedDataBlock.dataSize=0;
	builder->combinedDataPtr=NULL;

	builder->treeBuilder=dAlloc(builder->disp, sizeof(RouteTableTreeBuilder));
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
	else
		{
//		LOG(LOG_INFO,"Null prefix");
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
	else
		{
//		LOG(LOG_INFO,"Null suffix");
		}


	//treeBuilder->forwardProxy

//	LOG(LOG_INFO,"Parse Indirect: Arrays");

	for(int i=ROUTE_TOPINDEX_FORWARD_LEAF; i<ROUTE_TOPINDEX_FORWARD_OFFSET;i++)
		{
		u8 *arrayBlockData=top->data[i];

//		LOG(LOG_INFO,"Begin parse indirect array %i from %p",i,arrayBlockData);

		if(arrayBlockData!=NULL)
			{
			treeBuilder->dataBlocks[i].headerSize=rtDecodeArrayBlockHeader(arrayBlockData, NULL, NULL, NULL, NULL, NULL, NULL);
			}
		else
			{
			//LOG(LOG_INFO,"Null array");
			}
		}

	treeBuilder->dataBlocks[ROUTE_TOPINDEX_FORWARD_OFFSET].headerSize=0;
	treeBuilder->dataBlocks[ROUTE_TOPINDEX_FORWARD_OFFSET].dataSize=0;
	treeBuilder->dataBlocks[ROUTE_TOPINDEX_REVERSE_OFFSET].headerSize=0;
	treeBuilder->dataBlocks[ROUTE_TOPINDEX_REVERSE_OFFSET].dataSize=0;

//	LOG(LOG_INFO,"Parse Indirect: Building");

	rttInitRouteTableTreeBuilder(treeBuilder, top);

/*
	LOG(LOG_INFO,"Parse Indirect: Building complete %i %i %i %i",
			treeBuilder->forwardProxy.leafArrayProxy.dataAlloc,
			treeBuilder->forwardProxy.branchArrayProxy.dataAlloc,
			treeBuilder->reverseProxy.leafArrayProxy.dataAlloc,
			treeBuilder->reverseProxy.branchArrayProxy.dataAlloc);
*/

	//for(int i=0;i<treeBuilder->forwardProxy.leafArrayProxy.dataCount;i++)
//		LOG(LOG_INFO,"Forward Leaf: DataPtr %p",treeBuilder->forwardProxy.leafArrayProxy.dataBlock->data[i]);



	//rttDumpRoutingTable(treeBuilder);


	/*
	s32 indexSize=0, subindexSize=0;

	// Extract Prefix Info
	rtDecodeNonRootBlockHeader(prefixBlockData, NULL, &indexSize, NULL, &subindexSize, &builder->prefixDataBlock.subindex);
	headerSize=rtGetNonRootBlockHeaderSize(indexSize, subindexSize);

	builder->prefixDataBlock.subindexSize=subindexSize;
	builder->prefixDataBlock.headerSize=headerSize;
	builder->prefixDataBlock.blockPtr=prefixBlockData;

	u8 *prefixData=prefixBlockData+headerSize;
	initSeqTailBuilder(&(builder->prefixBuilder), prefixData, builder->disp);


	// Extract Suffix Info
	rtDecodeNonRootBlockHeader(suffixBlockData, NULL, &indexSize, NULL, &subindexSize, &builder->suffixDataBlock.subindex);
	headerSize=rtGetNonRootBlockHeaderSize(indexSize, subindexSize);

	builder->suffixDataBlock.subindexSize=subindexSize;
	builder->suffixDataBlock.headerSize=headerSize;
	builder->suffixDataBlock.blockPtr=suffixBlockData;

	u8 *suffixData=suffixBlockData+headerSize;
	initSeqTailBuilder(&(builder->suffixBuilder), suffixData, builder->disp);

	// Extract RouteTable Info

	rttInitRouteTableTreeBuilder(builder->treeBuilder, &(builder->topDataBlock), builder->disp);

	builder->upgradedToTree=0;
	*/

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

	//LOG(LOG_INFO,"Write Direct Prefix: %p",newData);
	newData=writeSeqTailBuilderPackedData(&(builder->prefixBuilder), newData);

	//LOG(LOG_INFO,"Write Direct Suffix: %p",newData);
	newData=writeSeqTailBuilderPackedData(&(builder->suffixBuilder), newData);

	//LOG(LOG_INFO,"Write Direct Routes: %p",newData);
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


static void upgradeToTree(RoutingComboBuilder *builder)
{
	RouteTableTreeBuilder *treeBuilder=dAlloc(builder->disp, sizeof(RouteTableTreeBuilder));
	builder->treeBuilder=treeBuilder;

	builder->topDataPtr=NULL;
	builder->topDataBlock.headerSize=0;
	builder->topDataBlock.dataSize=0;

	rttUpgradeToRouteTableTreeBuilder(builder->arrayBuilder,  builder->treeBuilder, builder->disp);

	builder->upgradedToTree=1;
}


s32 mergeTopArrayUpdates_branch_accumulateSize(RouteTableTreeArrayProxy *arrayProxy, int indexSize)
{
	if(arrayProxy->newData==NULL)
		{
		return 0;
		}

	s32 totalSize=0;

	for(int i=0;i<arrayProxy->newDataAlloc;i++)
		{
		int oldBranchChildAlloc=0;

		if(arrayProxy->dataBlock!=NULL && i<arrayProxy->dataBlock->dataAlloc && arrayProxy->dataBlock->data[i]!=NULL) //FIXME (allow for header)
			{
			u8 *oldBranchRawData=arrayProxy->dataBlock->data[i];
			RouteTableTreeBranchBlock *oldBranchData=(RouteTableTreeBranchBlock *)(oldBranchRawData+rtGetArrayBlockHeaderSize(indexSize,1));
			oldBranchChildAlloc=oldBranchData->childAlloc;
			}

		if(arrayProxy->newData[i]!=NULL)
			{
			RouteTableTreeBranchBlock *newBranchData=(RouteTableTreeBranchBlock *)(arrayProxy->newData[i]);

			if(newBranchData->childAlloc!=oldBranchChildAlloc)
				totalSize+=rtGetArrayBlockHeaderSize(indexSize,1)+sizeof(RouteTableTreeBranchBlock)+sizeof(s16)*((s32)(newBranchData->childAlloc));
			}
		}

	return totalSize;
}


void mergeTopArrayUpdates_branch(RouteTableTreeArrayProxy *arrayProxy, int arrayNum, int indexSize, int index, u8 *newData, s32 newDataSize)
{
	if(arrayProxy->newData==NULL)
		return ;

	u8 *endNewData=newData+newDataSize;

//	if(arrayProxy->newData!=NULL)
//		arrayProxy->dataBlock->dataAlloc=arrayProxy->newDataAlloc;

	for(int i=0;i<arrayProxy->newDataAlloc;i++)
		{
		u8 *oldBranchRawData=NULL;
		RouteTableTreeBranchBlock *oldBranchData=NULL;
		int oldBranchChildAlloc=0;

		if(arrayProxy->dataBlock!=NULL && i<arrayProxy->dataBlock->dataAlloc && arrayProxy->dataBlock->data[i]!=NULL)
			{
			oldBranchRawData=arrayProxy->dataBlock->data[i];
			oldBranchData=(RouteTableTreeBranchBlock *)(oldBranchRawData+rtGetArrayBlockHeaderSize(indexSize,1));
			oldBranchChildAlloc=oldBranchData->childAlloc;
			}

		if(arrayProxy->newData[i]!=NULL)
			{
			RouteTableTreeBranchBlock *newBranchData=(RouteTableTreeBranchBlock *)(arrayProxy->newData[i]);

			if(newBranchData->childAlloc!=oldBranchChildAlloc)
				{
//				LOG(LOG_INFO,"Branch Move/Expand write to %p (%i %i)",newData, newBranchData->childAlloc,oldBranchChildAlloc);

				arrayProxy->dataBlock->data[i]=newData;

				s32 headerSize=rtEncodeArrayBlockHeader(arrayNum, ARRAY_TYPE_SHALLOW_DATA, indexSize, index, i, newData);
				newData+=headerSize;

				s32 dataSize=sizeof(RouteTableTreeBranchBlock)+sizeof(s16)*(newBranchData->childAlloc);
				memcpy(newData, newBranchData, dataSize);

				//LOG(LOG_INFO,"Copying %i bytes of branch data from %p to %p",dataSize,newBranchData,newData);

				newData+=dataSize;

				rtHeaderMarkDead(oldBranchRawData);
				}
			else
				{
//				LOG(LOG_INFO,"Branch rewrite to %p (%i %i)",arrayProxy->dataBlock->data[i], newBranchData->childAlloc,oldBranchChildAlloc);

				s32 headerSize=rtGetArrayBlockHeaderSize(indexSize,1);
				s32 dataSize=sizeof(RouteTableTreeBranchBlock)+sizeof(s16)*(newBranchData->childAlloc);
				memcpy(arrayProxy->dataBlock->data[i]+headerSize, newBranchData, dataSize);

				}
			}
		}

	if(endNewData!=newData)
		LOG(LOG_CRITICAL,"New Branch Data doesn't match expected: New: %p vs Expected: %p",newData,endNewData);

}




s32 mergeTopArrayUpdates_leaf_accumulateSize(RouteTableTreeArrayProxy *arrayProxy, int indexSize)
{
	if(arrayProxy->newData==NULL)
		{
		return 0;
		}

	s32 totalSize=0;

	//LOG(LOG_INFO,"New data Alloc %i",arrayProxy->newDataAlloc);

	for(int i=0;i<arrayProxy->newDataAlloc;i++)
		{
		int oldLeafEntryAlloc=0;

		if(arrayProxy->dataBlock!=NULL && i<arrayProxy->dataBlock->dataAlloc && arrayProxy->dataBlock->data[i]!=NULL)
			{
			u8 *oldLeafRawData=arrayProxy->dataBlock->data[i];
			RouteTableTreeLeafBlock *oldLeafData=(RouteTableTreeLeafBlock *)(oldLeafRawData+rtGetArrayBlockHeaderSize(indexSize,1));
			oldLeafEntryAlloc=oldLeafData->entryAlloc;
			}

		if(arrayProxy->newData[i]!=NULL)
			{
			RouteTableTreeLeafBlock *newLeafData=(RouteTableTreeLeafBlock *)(arrayProxy->newData[i]);

			if(newLeafData->entryAlloc!=oldLeafEntryAlloc)
				totalSize+=rtGetArrayBlockHeaderSize(indexSize,1)+sizeof(RouteTableTreeLeafBlock)+sizeof(RouteTableTreeLeafEntry)*((s32)(newLeafData->entryAlloc));
			}
		}

	return totalSize;
}


void mergeTopArrayUpdates_leaf(RouteTableTreeArrayProxy *arrayProxy, int arrayNum, int indexSize, int index, u8 *newData, s32 newDataSize)
{
	if(arrayProxy->newData==NULL)
		return ;

	u8 *endNewData=newData+newDataSize;

	//if(arrayProxy->dataBlock->dataAlloc==0)
		//{
		//LOG(LOG_INFO,"New Leaf Array - setting array length to %i",arrayProxy->newDataAlloc);

//	if(arrayProxy->newData!=NULL)
//		arrayProxy->dataBlock->dataAlloc=arrayProxy->newDataAlloc;

		//}

	for(int i=0;i<arrayProxy->newDataAlloc;i++)
		{
		u8 *oldLeafRawData=NULL;
		RouteTableTreeLeafBlock *oldLeafData=NULL;
		int oldLeafEntryAlloc=0;

		if(arrayProxy->dataBlock!=NULL && i<arrayProxy->dataBlock->dataAlloc && arrayProxy->dataBlock->data[i]!=NULL)
			{
			oldLeafRawData=(arrayProxy->dataBlock->data[i]);
			oldLeafData=(RouteTableTreeLeafBlock *)(oldLeafRawData+rtGetArrayBlockHeaderSize(indexSize,1));
			oldLeafEntryAlloc=oldLeafData->entryAlloc;
			}

		if(arrayProxy->newData[i]!=NULL)
			{
			RouteTableTreeLeafBlock *newLeafData=(RouteTableTreeLeafBlock *)(arrayProxy->newData[i]);

			if(newLeafData->entryAlloc!=oldLeafEntryAlloc)
				{
//				LOG(LOG_INFO,"Leaf Move/Expand write to %p (%i %i)",newData, newLeafData->entryAlloc,oldLeafEntryAlloc);
				arrayProxy->dataBlock->data[i]=newData;

				s32 headerSize=rtEncodeArrayBlockHeader(arrayNum, ARRAY_TYPE_SHALLOW_DATA, indexSize, index, i, newData);
				newData+=headerSize;

				s32 dataSize=sizeof(RouteTableTreeLeafBlock)+sizeof(RouteTableTreeLeafEntry)*(newLeafData->entryAlloc);
				memcpy(newData, newLeafData, dataSize);
				newData+=dataSize;

				rtHeaderMarkDead(oldLeafRawData);
				}
			else
				{
//				LOG(LOG_INFO,"Leaf rewrite to %p (%i %i)",arrayProxy->dataBlock->data[i], newLeafData->entryAlloc,oldLeafEntryAlloc);

				s32 headerSize=rtGetArrayBlockHeaderSize(indexSize,1);
				s32 dataSize=sizeof(RouteTableTreeLeafBlock)+sizeof(RouteTableTreeLeafEntry)*(newLeafData->entryAlloc);
				memcpy(arrayProxy->dataBlock->data[i]+headerSize, newLeafData, dataSize);
				}
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
	// 		array blocks
	// 		branch blocks
	//		leaf blocks
	//		offset blocks

//	rttDumpRoutingTable(routingBuilder->treeBuilder);

	if(routingBuilder->upgradedToTree)
		{
		u8 *oldData=*(routingBuilder->rootPtr);

		rtHeaderMarkDead(oldData);
		}

	s16 indexSize=varipackLength(sliceIndex);
	RouteTableTreeBuilder *treeBuilder=routingBuilder->treeBuilder;

//	LOG(LOG_INFO,"Writing as indirect data (index size is %i)",indexSize);

	RouteTableTreeTopBlock *topPtr=NULL;

	if(routingBuilder->topDataPtr==NULL)
		{
		routingBuilder->topDataBlock.dataSize=sizeof(RouteTableTreeTopBlock);
		routingBuilder->topDataBlock.headerSize=rtGetGapBlockHeaderSize();

		s32 totalSize=routingBuilder->topDataBlock.dataSize+routingBuilder->topDataBlock.headerSize;

		s32 oldTagOffset=0;
//		LOG(LOG_INFO,"CircAlloc %i",totalSize);
		u8 *newTopData=circAlloc(circHeap, totalSize, sliceTag, sliceIndex, &oldTagOffset);
//		LOG(LOG_INFO,"Alloc top level at %p",newTopData);

		s32 diff=sliceIndex-oldTagOffset;

		rtEncodeGapTopBlockHeader(diff, newTopData);

		u8 *newTop=newTopData+routingBuilder->topDataBlock.headerSize;
		memset(newTop,0,sizeof(RouteTableTreeTopBlock));

		topPtr=(RouteTableTreeTopBlock *)newTop;

		*(routingBuilder->rootPtr)=newTopData;
		}
	else
		topPtr=(RouteTableTreeTopBlock *)(routingBuilder->topDataPtr+routingBuilder->topDataBlock.headerSize);

	HeapDataBlock neededBlocks[ROUTE_TOPINDEX_MAX];
	memset(neededBlocks,0,sizeof(HeapDataBlock)*ROUTE_TOPINDEX_MAX);

	neededBlocks[ROUTE_TOPINDEX_PREFIX].headerSize=rtGetTailBlockHeaderSize(indexSize);
	neededBlocks[ROUTE_TOPINDEX_PREFIX].dataSize=getSeqTailBuilderPackedSize(&(routingBuilder->prefixBuilder));

	neededBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize=rtGetTailBlockHeaderSize(indexSize);
	neededBlocks[ROUTE_TOPINDEX_SUFFIX].dataSize=getSeqTailBuilderPackedSize(&(routingBuilder->suffixBuilder));

	for(int i=ROUTE_TOPINDEX_FORWARD_LEAF;i<ROUTE_TOPINDEX_FORWARD_OFFSET;i++)
		{
		neededBlocks[i].headerSize=rtGetArrayBlockHeaderSize(indexSize, 0);

		RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);
		//LOG(LOG_INFO,"Array %i had %i, now has %i",i,arrayProxy->dataCount,arrayProxy->newDataCount);

		neededBlocks[i].dataSize=rttGetTopArraySize(arrayProxy);

		if(arrayProxy->newDataCount>255)
			{
			LOG(LOG_INFO,"Array %i oversize",i);
			rttDumpRoutingTable(routingBuilder->treeBuilder);
			LOG(LOG_CRITICAL,"Bailing out");
			}
		}

	int totalNeededSize=0;

	for(int i=0;i<ROUTE_TOPINDEX_MAX;i++)
		{
		s32 existingSize=treeBuilder->dataBlocks[i].headerSize+treeBuilder->dataBlocks[i].dataSize;
		s32 neededSize=neededBlocks[i].headerSize+neededBlocks[i].dataSize;

		if(existingSize!=neededSize)
			totalNeededSize+=neededSize;

//		LOG(LOG_INFO,"Array %i has %i %i, needs %i %i",i,
//				treeBuilder->dataBlocks[i].headerSize,treeBuilder->dataBlocks[i].dataSize, neededBlocks[i].headerSize,neededBlocks[i].dataSize);
		}

	if(totalNeededSize>0)
		{
//		LOG(LOG_INFO,"CircAlloc %i",totalNeededSize);
		u8 *newArrayData=circAlloc(circHeap, totalNeededSize, sliceTag, sliceIndex, NULL);
//		LOG(LOG_INFO,"Alloc array level at %p",newArrayData);

		memset(newArrayData,0,totalNeededSize);

		u8 *endArrayData=newArrayData+totalNeededSize;

		topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root after alloc

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

		// Handle Leaf/Branch Arrays

		for(int i=ROUTE_TOPINDEX_FORWARD_LEAF;i<ROUTE_TOPINDEX_FORWARD_OFFSET;i++)
			{
			existingSize=treeBuilder->dataBlocks[i].headerSize+treeBuilder->dataBlocks[i].dataSize;
			neededSize=neededBlocks[i].headerSize+neededBlocks[i].dataSize;

			s32 sizeDiff=neededSize-existingSize;

			if(sizeDiff>0)
				{
//				u8 *endPtr=newArrayData+neededBlocks[i].headerSize+neededBlocks[i].dataSize;
//				LOG(LOG_INFO,"Next array (if any) at %p with %02x",endPtr, *endPtr);

				rtEncodeArrayBlockHeader(i,ARRAY_TYPE_SHALLOW_PTR, indexSize, sliceIndex, 0, newArrayData);
				u8 *oldData=topPtr->data[i];

//				LOG(LOG_INFO,"Array %i Move/Expand %p to %p",i,oldData,newArrayData);

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
					//LOG(LOG_INFO,"Setting %i from %p to zero",neededBlocks[i].dataSize, newArrayData);
					memset(newArrayData, 0, neededBlocks[i].dataSize);
					}

				RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);
				rttBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize);
				if(arrayProxy->newDataAlloc!=0)
					{
					RouteTableTreeArrayBlock *arrayBlock=(RouteTableTreeArrayBlock *)newArrayData;
					arrayBlock->dataAlloc=arrayProxy->newDataAlloc;

		//			LOG(LOG_INFO,"Set arrayBlock %p dataAlloc to %i",arrayBlock,arrayBlock->dataAlloc);
					}

				newArrayData+=neededBlocks[i].dataSize;
//				LOG(LOG_INFO,"Next array (if any) at %p with %02x",newArrayData, *newArrayData);

				}
			else
				{
				if(existingSize>neededSize)
					LOG(LOG_CRITICAL,"Unexpected reduction in size %i to %i",existingSize,neededSize);

//				LOG(LOG_INFO,"Array %i Rewrite in place %p",i, topPtr->data[i]);

				}
			}

		if(endArrayData!=newArrayData)
			LOG(LOG_CRITICAL,"Array end did not match expected: %p vs %p",newArrayData, endArrayData);
		}


	topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root after alloc

//	LOG(LOG_INFO,"Tails");

	if((routingBuilder->upgradedToTree) || getSeqTailBuilderDirty(&(routingBuilder->prefixBuilder)))
		{
//		LOG(LOG_INFO,"Write Indirect Prefix %p %i",topPtr->data[ROUTE_TOPINDEX_PREFIX],neededBlocks[ROUTE_TOPINDEX_PREFIX].headerSize);
		writeSeqTailBuilderPackedData(&(routingBuilder->prefixBuilder), topPtr->data[ROUTE_TOPINDEX_PREFIX]+neededBlocks[ROUTE_TOPINDEX_PREFIX].headerSize);
		}

	if((routingBuilder->upgradedToTree) || getSeqTailBuilderDirty(&(routingBuilder->suffixBuilder)))
		{
//		LOG(LOG_INFO,"Write Indirect Suffix %p %i",topPtr->data[ROUTE_TOPINDEX_SUFFIX],neededBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize);
		writeSeqTailBuilderPackedData(&(routingBuilder->suffixBuilder), topPtr->data[ROUTE_TOPINDEX_SUFFIX]+neededBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize);
		}

//	LOG(LOG_INFO,"Leaves");

	for(int i=ROUTE_TOPINDEX_FORWARD_LEAF; i<ROUTE_TOPINDEX_FORWARD_BRANCH;i++)
		{
		RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);

		if((routingBuilder->upgradedToTree) || rttGetTopArrayDirty(arrayProxy))
			{
//			LOG(LOG_INFO,"Calc space");

			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root after alloc
			rttBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize);

			s32 size=mergeTopArrayUpdates_leaf_accumulateSize(arrayProxy, indexSize);
//			LOG(LOG_INFO,"Need to allocate %i to write leaf array %i",size, i);

//			LOG(LOG_INFO,"CircAlloc %i",size);
			u8 *newLeafData=circAlloc(circHeap, size, sliceTag, sliceIndex, NULL);
//			LOG(LOG_INFO,"Alloc leaf level at %p",newLeafData);

//			LOG(LOG_INFO,"Allocating");

//			LOG(LOG_INFO,"Binding to %p",topPtr->data[i]+neededBlocks[i].headerSize);

			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root after alloc
			rttBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize);

			//LOG(LOG_INFO,"Merging - alloc %i",arrayProxy->dataBlock->dataAlloc);


			//void mergeTopArrayUpdates_leaf(RouteTableTreeArrayProxy *arrayProxy, int arrayNum, int indexSize, int index, u8 *newData, s32 newDataSize)

			mergeTopArrayUpdates_leaf(arrayProxy, i, indexSize, sliceIndex, newLeafData, size);

			}
		}

	//LOG(LOG_INFO,"Branches");

	for(int i=ROUTE_TOPINDEX_FORWARD_BRANCH; i<ROUTE_TOPINDEX_FORWARD_OFFSET;i++)
		{
		RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);

		if((routingBuilder->upgradedToTree)|| rttGetTopArrayDirty(arrayProxy))
			{
		//	LOG(LOG_INFO,"Calc space");

			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root after alloc
			rttBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize);

			s32 size=mergeTopArrayUpdates_branch_accumulateSize(arrayProxy, indexSize);
			//LOG(LOG_INFO,"Need to allocate %i to write branch array %i",size, i);

//			LOG(LOG_INFO,"CircAlloc %i",size);
			u8 *newBranchData=circAlloc(circHeap, size, sliceTag, sliceIndex, NULL);
//			LOG(LOG_INFO,"Alloc branch level at %p",newBranchData);

			//LOG(LOG_INFO,"Allocating");

//			LOG(LOG_INFO,"Binding to %p",topPtr->data[i]+neededBlocks[i].headerSize);

			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+routingBuilder->topDataBlock.headerSize); // Rebind root after alloc
			rttBindBlockArrayProxy(arrayProxy, topPtr->data[i], neededBlocks[i].headerSize);

			//LOG(LOG_INFO,"Merging - alloc %i",arrayProxy->dataBlock->dataAlloc);


			//void mergeTopArrayUpdates_leaf(RouteTableTreeArrayProxy *arrayProxy, int arrayNum, int indexSize, int index, u8 *newData, s32 newDataSize)

			mergeTopArrayUpdates_branch(arrayProxy, i, indexSize, sliceIndex, newBranchData, size);

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
	u32 sliceIndex=rdi->sliceIndex;

	u8 *smerDataOrig=slice->smerData[sliceIndex];
	u8 *smerData=smerDataOrig;

	//if(smerData!=NULL)
//		LOG(LOG_INFO, "Index: %i of %i Entries %i Data: %p",sliceIndex,slice->smerCount, rdi->entryCount, smerData);

	RoutingComboBuilder routingBuilder;

	routingBuilder.disp=disp;
	routingBuilder.rootPtr=slice->smerData+sliceIndex;

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

	s32 prefixCount=getSeqTailTotalTailCount(&(routingBuilder.prefixBuilder));
	s32 suffixCount=getSeqTailTotalTailCount(&(routingBuilder.suffixBuilder));

	if(considerUpgradingToTree(&routingBuilder, forwardCount, reverseCount))
		{
		//LOG(LOG_INFO,"Prefix Old %i New %i",routingBuilder.prefixBuilder.oldTailCount,routingBuilder.prefixBuilder.newTailCount);
		//LOG(LOG_INFO,"Suffix Old %i New %i",routingBuilder.suffixBuilder.oldTailCount,routingBuilder.suffixBuilder.newTailCount);

		upgradeToTree(&routingBuilder);
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
						&(stats[i].routeTableTreeLeafBytes), &(stats[i].routeTableTreeBranchBytes));
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








