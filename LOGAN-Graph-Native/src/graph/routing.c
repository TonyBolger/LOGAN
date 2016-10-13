#include "common.h"


/*

Alloc Header:

	0 0 0 0 0 0 0 0 : INVALID: Unused region

    1 0 0 0 0 0 0 0 : CHUNK: Start of chunk header (with 1 byte tag)

	1 x x x x x x x : Live block
	0 x x x x x x x : Dead block

	x 1 x x x x x x : Indirect block formats
	x 0 x x x x x x : Direct block formats

    x 1 x x x x x 0 : 0xC0 Indirect root
    x 1 0 x x x x 1 : 0xC1 Indirect branch (route only)
    x 1 1 x x x x 1 : 0xE1 Indirect leaf (tail or route)

    x 1 x X X x x 1 : 0x81 Indirect 1-4 byte index
    x 1 0 x x X X 1 : 0x81 Indirect branch/leaflevel (1-4 byte sub-index)

	Branches/Leaves marker followed by 1-4 byte index, 1-4 bytes sub-index
	Branches then followed with up to 256 pointers (optionally NULL)
	Leaf is followed by size (u16)

    x 1 0 0 0 0 1 0 / x 1 0 0 0 1 0 0 / x 1 0 0 0 1 1 0: Unused

	x 1 0 0 1 x x 0 : 0xC8 Indirect root with 0-255 gap (with value+1): Exact
	x 1 0 1 0 x x 0 : 0xD0 Indirect root with 256-511 gap (with value+256): Exact
    x 1 0 1 1 x x 0 : 0xD8 Indirect root with 512-1023 gap (with value*2+512): 0-1
	x 1 1 0 0 x x 0 : 0xE0 Indirect root with 1024-2047 gap (with value*4+1024): 0-3
	x 1 1 0 1 x x 0 : 0xE8 Indirect root with 2048-4095 gap (with value*8+2048): 0-7
	x 1 1 1 0 x x 0 : 0xF0 Indirect root with 4096-8191 gap (with value*16+4096): 0-15
    x 1 1 1 1 x x 0 : 0xF8 Indirect root with 8192-16383 gap (with value*32+8192): 0-31

	x x x x x 0 0 0 : Indirect root 4 pointers (2 tail, 1 forward route, 1 reverse route)
	x x x x x 0 1 0 : Indirect root 16 pointers (2 tail, 7 forward route, 7 reverse route)
	x x x x x 1 0 0 : Indirect root 64 pointers (2 tail, 31 forward route, 31 reverse route)
	x x x x x 1 0 0 : Indirect root 256 pointers (2 tail, 127 forward route, 127 reverse route)

	x 0 0 0 1 x x x : 0x88 Direct with 0-255 gap (with value+1): Exact
	x 0 0 1 0 x x x : 0x90 Direct with 256-511 gap (with value+256): Exact
    x 0 0 1 1 x x x : 0x98 Direct with 512-1023 gap (with value*2+512): 0-1
	x 0 1 0 0 x x x : 0xA0 Direct with 1024-2047 gap (with value*4+1024): 0-3
	x 0 1 0 1 x x x : 0xA8 Direct with 2048-4095 gap (with value*8+2048): 0-7
	x 0 1 1 0 x x x : 0xB0 Direct with 4096-8191 gap (with value*16+4096): 0-15
    x 0 1 1 1 x x x : 0xB8 Direct with 8192-16383 gap (with value*32+8192): 0-31


	x 0 x x x x 1 x : Large # prefixes (>255)
	x 0 x x x x 0 x : Small # prefixes

	x 0 x x x x x 1 : Large # suffixes (>255)
	x 0 x x x x x 0 : Small # suffixes


    Indirect root -> 256 * Branch high level
    Branch HL -> 256 * Branch mid level
    Branch ML ->

 */


//	Old Format:
//		#PrefixTails(2B), PrefixTailData(packed), #SuffixTails(2B), SuffixTailData(packed), RouteHeader(2-10B), ForwardRoutes(packed), ReverseRoutes(packed)
//
//
//	Future Formats:
//
//  Compact Format: Tail counts in first byte (<=7)
//		0 0 P P P S S S,  PrefixTailData(packed), SuffixTailData(packed), RouteHeader(2-10B), ForwardRoutes(packed), ReverseRoutes(packed)
//
//
//


//#define ALLOC_HEADER_DEFAULT 0xBF

#define ALLOC_HEADER_LIVE_MASK 0x80

#define ALLOC_HEADER_LIVE_INDIRECT_ROOT_MASK 0xC1
#define ALLOC_HEADER_LIVE_INDIRECT_ROOT_VALUE 0xC0

#define ALLOC_HEADER_LIVE_INDIRECT_NONROOT_MASK 0xC1



#define ALLOC_HEADER_LIVE_DIRECT_MASK 0xC0
#define ALLOC_HEADER_LIVE_DIRECT_VALUE 0x80

#define ALLOC_HEADER_LIVE_DIRECT_SIZE_MASK 0x38
#define ALLOC_HEADER_LIVE_DIRECT_SMALL 0x88







#define ALLOC_HEADER_LIVE_INDIRECT_MASK 0xC0
#define ALLOC_HEADER_LIVE_INDIRECT_VALUE 0xC0





s32 rtItemSizeResolver(u8 *item)
{
	u8 *scanPtr;

	if(item!=NULL)
		{
		scanPtr=item+2;

		scanPtr=scanTails(scanPtr);
		scanPtr=scanTails(scanPtr);
		scanPtr=rtaScanRouteTableArray(scanPtr);

		return scanPtr-item;
		}

	return 0;
}


static void dumpTagData(u8 **tagData, s32 tagDataLength)
{
	for(int i=0;i<tagDataLength;i++)
		{
		LOG(LOG_INFO,"Tag %i %p",i,tagData[i]);
		}
}

static void encodeBlockHeader(s32 tagOffsetDiff, u8 *data)
{
	if(tagOffsetDiff<0)
		{
		LOG(LOG_CRITICAL,"Expected positive tagOffsetDiff %i",tagOffsetDiff);
		}

	if(tagOffsetDiff<256)
		{
		*data++=ALLOC_HEADER_LIVE_DIRECT_SMALL;
		*data=(u8)(tagOffsetDiff);

//		LOG(LOG_INFO,"Wrote small Header: %02x %02x for %i",ALLOC_HEADER_LIVE_DIRECT_SMALL, tagOffsetDiff&0xFF, tagOffsetDiff);

		}
	else
		{
		tagOffsetDiff=MIN(tagOffsetDiff,16383);

		s32Float helper;
		helper.floatVal=tagOffsetDiff;

		u8 exp=((helper.s32Val>>23)&0xF)-6;
		u8 man=(helper.s32Val>>15)&0xFF;

		u8 data1=ALLOC_HEADER_LIVE_DIRECT_SMALL + (exp << 3);

		*data++=data1;
		*data=man;

		//LOG(LOG_INFO,"Wrote other Header: %02x %02x for %i (exp %i)",data1,man,tagOffsetDiff,exp);
		}
}

static s32 decodeBlockHeader(u8 *data)
{
	u8 data1=*data++;

	int exp=((data1&ALLOC_HEADER_LIVE_DIRECT_SIZE_MASK)>>3);

	u32 res=*data;

	if(exp>1)
		{
		res=(256+res)<<(exp-2);

		//LOG(LOG_INFO,"Read other Header: %i",res);
		return res;
		}
	else
		{
		//LOG(LOG_INFO,"Read small Header: %i",res);
		return res;
		}


}
/*
static s32 scanTagDataNoStart(u8 **tagData, s32 tagDataLength,u8 *wanted)
{
	for(int i=0;i<tagDataLength;i++)
		{
		if(tagData[i]==wanted)
			return i;
		}

	return -1;
}
*/


static s32 scanTagData(u8 **tagData, s32 tagDataLength, s32 startIndex, u8 *wanted)
{
//	int scanCount=0;
	for(int i=startIndex;i<tagDataLength;i++)
		{
		if(tagData[i]==wanted)
			{
//			if(scanCount>10)
//				LOG(LOG_INFO,"Scan count of %i from %i",scanCount,startIndex);

			return i;
			}
//		scanCount++;
		}
/*
	for(int i=0;i<startIndex;i++)
		{
		if(tagData[i]==wanted)
			return i;
		}
*/
	return -1;
}


static MemCircHeapChunkIndex *allocOrExpandIndexer(MemCircHeapChunkIndex *oldIndex, MemDispenser *disp)
{
	if(oldIndex==NULL)
		{
		MemCircHeapChunkIndex *index=dAlloc(disp, sizeof(MemCircHeapChunkIndex)+sizeof(MemCircHeapChunkIndexEntry)*CIRCHEAP_INDEXER_ENTRYALLOC);

		index->entryAlloc=CIRCHEAP_INDEXER_ENTRYALLOC;
		index->entryCount=0;

		return index;
		}
	else
		{
		s32 oldSize=sizeof(MemCircHeapChunkIndex)+sizeof(MemCircHeapChunkIndexEntry)*oldIndex->entryAlloc;
		s32 newSize=oldSize+sizeof(MemCircHeapChunkIndexEntry)*CIRCHEAP_INDEXER_ENTRYALLOC;

		MemCircHeapChunkIndex *index=dAlloc(disp, newSize);
		memcpy(index,oldIndex,oldSize);

		index->entryAlloc+=CIRCHEAP_INDEXER_ENTRYALLOC;
		return index;
		}
}


MemCircHeapChunkIndex *rtReclaimIndexer(u8 *heapDataPtr, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, s32 tagSearchOffset, MemDispenser *disp)
{
	//LOG(LOG_INFO,"Reclaim Index: %p %li %x %p",data,targetAmount,tag,disp);

	MemCircHeapChunkIndex *index=allocOrExpandIndexer(NULL, disp);

	u8 *startOfData=heapDataPtr;
	u8 *endOfData=heapDataPtr+targetAmount;

	s64 liveSize=0;
	s64 deadSize=0;

	s32 currentIndex=tagSearchOffset;
	s32 firstTagOffset=-1;

	int entry=0;

	//LOG(LOG_INFO,"Begin reclaimIndexing from %i",tagSearchOffset);

	while(heapDataPtr<endOfData)
		{
		u8 header=*heapDataPtr;

		if(header==CH_HEADER_CHUNK || header==CH_HEADER_INVALID)
			{
		//	LOG(LOG_INFO,"Header: %p %2x %i",data,header);
			break;
			}

		s32 chunkTagOffsetDiff=decodeBlockHeader(heapDataPtr);

		//LOG(LOG_INFO,"Got offset diff %i",chunkTagOffsetDiff);

		currentIndex+=chunkTagOffsetDiff;
		u8 *scanPtr=heapDataPtr+2;

		scanPtr=scanTails(scanPtr);
		scanPtr=scanTails(scanPtr);
		scanPtr=rtaScanRouteTableArray(scanPtr);

		s32 size=scanPtr-heapDataPtr;

		if(header & ALLOC_HEADER_LIVE_MASK)
			{
			/*
			if(currentIndex==-1)
				currentIndex=scanTagDataNoStart(tagData, tagDataLength, heapDataPtr);
			else
			*/
			currentIndex=scanTagData(tagData, tagDataLength, currentIndex, heapDataPtr);

			if(currentIndex==-1)
				{
				LOG(LOG_INFO,"Failed to find expected heap pointer %p in tag data",heapDataPtr);
				dumpTagData(tagData,tagDataLength);

				return NULL;
				}

			if(firstTagOffset==-1)
				firstTagOffset=currentIndex;

			if(entry==index->entryAlloc)
				index=allocOrExpandIndexer(index, disp);

			index->entries[entry].index=currentIndex;
			index->entries[entry].size=size;
			entry++;

			index->lastLiveTagOffset=currentIndex;

			//LOG(LOG_INFO,"Live Item: %p %2x %i",heapDataPtr,header,size, currentIndex);
			liveSize+=size;
			}
		else
			{
			deadSize+=size;
			//LOG(LOG_INFO,"Dead Item: %p %2x %i",heapDataPtr,header,size);
			}

		heapDataPtr=scanPtr;
		}

	index->entryCount=entry;
	index->firstLiveTagOffset=firstTagOffset;
	index->lastScannedTagOffset=currentIndex;

	index->heapStartPtr=startOfData;
	index->heapEndPtr=heapDataPtr;
	index->reclaimed=heapDataPtr-startOfData;
	index->sizeDead=deadSize;
	index->sizeLive=liveSize;

	return index;
}

void rtRelocater(MemCircHeapChunkIndex *index, u8 tag, u8 **tagData, s32 tagDataLength)
{
	u8 *newChunk=index->newChunk;

	s32 prevOffset=index->newChunkOldTagOffset;

	for(int i=0;i<index->entryCount;i++)
		{
		s32 offset=index->entries[i].index;

		u8 **ptr=&tagData[offset];
		s32 size=index->entries[i].size;

		if(size<2)
			{
			LOG(LOG_CRITICAL,"Undersize block %i, referenced by %p", size, *ptr);
			}

		s32 diff=offset-prevOffset;
		encodeBlockHeader(diff, newChunk);
/*
		s32 decodeDiff=decodeBlockHeader(newChunk);
		if(decodeDiff+10<diff)
			{
			LOG(LOG_INFO,"Failed to encode %i - got %i",diff,decodeDiff);
			}
*/
		//encodeBlockHeader(offset-prevOffset, newChunk);
		prevOffset=offset;

//		LOG(LOG_INFO,"Relocated for %i from %p to %p, referenced by %p", size, *ptr, newChunk, ptr);

		memmove(newChunk+2,(*ptr)+2,size-2);

		*ptr=newChunk;
		newChunk+=size;
		}


//	LOG(LOG_INFO,"Relocated for %p with data %p",index,index->newChunk);
}




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






static u8 *initRouteTableArrayBuilder(RouteTableBuilder *builder, u8 *data, MemDispenser *disp)
{
	builder->disp=disp;

	RouteTableArrayBuilder *arrayBuilder=dAlloc(disp, sizeof(RouteTableArrayBuilder));
	builder->arrayBuilder=arrayBuilder;
	builder->treeBuilder=NULL;

	return rtaInitRouteTableArrayBuilder(arrayBuilder,data,disp);
}


static u8 *initRouteTableTreeBuilder(RouteTableBuilder *builder, u8 *data, MemDispenser *disp)
{
	builder->disp=disp;

	RouteTableTreeBuilder *treeBuilder=dAlloc(disp, sizeof(RouteTableTreeBuilder));
	builder->arrayBuilder=NULL;
	builder->treeBuilder=treeBuilder;

	return rttInitRouteTableTreeBuilder(treeBuilder,data,disp);
}






static void createBuildersFromNullData(s32 *headerSize,
		SeqTailBuilder *prefixBuilder, s32 *prefixDataSize,
		SeqTailBuilder *suffixBuilder, s32 *suffixDataSize,
		RouteTableBuilder *routeTableBuilder, s32 *routeTableDataSize,
		MemDispenser *disp)
{
	initSeqTailBuilder(prefixBuilder, NULL, disp);
	initSeqTailBuilder(suffixBuilder, NULL, disp);
	initRouteTableArrayBuilder(routeTableBuilder, NULL, disp);

	*headerSize=0;
	*prefixDataSize=0;
	*suffixDataSize=0;
	*routeTableDataSize=0;
}





/*
static void dumpRoutingTable(RouteTableBuilder *builder)
{
	rtaDumpRoutingTable(builder->arrayBuilder);
}
*/


static void createBuildersFromDirectData(u8 *data, s32 *headerSize,
			SeqTailBuilder *prefixBuilder, s32 *prefixDataSize,
			SeqTailBuilder *suffixBuilder, s32 *suffixDataSize,
			RouteTableBuilder *routeTableBuilder, s32 *routeTableDataSize,
			MemDispenser *disp)
{
	data+=2;
	*headerSize=2;

	u8 *tmp;

	tmp=data;
	data=initSeqTailBuilder(prefixBuilder, data, disp);
	*prefixDataSize=data-tmp;

	tmp=data;
	data=initSeqTailBuilder(suffixBuilder, data, disp);
	*suffixDataSize=data-tmp;

	tmp=data;
	data=initRouteTableArrayBuilder(routeTableBuilder, data, disp);
	*routeTableDataSize=data-tmp;
}


//static
void createBuildersFromIndirectData(u8 *data, s32 *headerSize,
			SeqTailBuilder *prefixBuilder, s32 *prefixDataSize,
			SeqTailBuilder *suffixBuilder, s32 *suffixDataSize,
			RouteTableBuilder *routeTableBuilder, s32 *routeTableDataSize,
			MemDispenser *disp)
{
	data+=2;
	*headerSize=2;

	data=initRouteTableTreeBuilder(routeTableBuilder, data, disp);

	/*


	u8 *tmp;

	tmp=data;
	data=initSeqTailBuilder(prefixBuilder, data, disp);
	*prefixDataSize=data-tmp;

	tmp=data;
	data=initSeqTailBuilder(suffixBuilder, data, disp);
	*suffixDataSize=data-tmp;

	tmp=data;
	data=initRouteTableArrayBuilder(routeTableBuilder, data, disp);
	*routeTableDataSize=data-tmp;

	*/

}


static void writeBuildersAsDirectData(u8 **smerDataPtr, s8 sliceTag, s32 sliceIndex,
		SeqTailBuilder *prefixBuilder, s32 oldPrefixDataSize,
		SeqTailBuilder *suffixBuilder, s32 oldSuffixDataSize,
		RouteTableBuilder *routeTableBuilder, s32 oldRouteTableDataSize,
		MemCircHeap *circHeap)
{
	int oldTotalSize=2+oldPrefixDataSize+oldSuffixDataSize+oldRouteTableDataSize;

	int prefixPackedSize=getSeqTailBuilderPackedSize(prefixBuilder);
	int suffixPackedSize=getSeqTailBuilderPackedSize(suffixBuilder);
	int routeTablePackedSize=rtaGetRouteTableArrayBuilderPackedSize(routeTableBuilder->arrayBuilder);

	int totalSize=2+prefixPackedSize+suffixPackedSize+routeTablePackedSize;

	if(totalSize>16083)
		{
		int oldShifted=oldTotalSize>>14;
		int newShifted=totalSize>>14;

		if(oldShifted!=newShifted)
			{
			LOG(LOG_INFO,"LARGE ALLOC: %i",totalSize);
			}

		}

	u8 *oldData=*smerDataPtr;
	u8 *newData;

	// Mark old block as dead
	if(oldData!=NULL)
		*oldData&=~ALLOC_HEADER_LIVE_MASK;

	s32 oldTagOffset=0;
	newData=circAlloc(circHeap, totalSize, sliceTag, sliceIndex, &oldTagOffset);

	//LOG(LOG_INFO,"Offset Diff: %i for %i",offsetDiff,sliceIndex);

	if(newData==NULL)
		{
		LOG(LOG_CRITICAL,"Failed at alloc after compact: Wanted %i",totalSize);
		}

	s32 diff=sliceIndex-oldTagOffset;

	*smerDataPtr=newData;
	encodeBlockHeader(diff, newData);

	newData+=2;
	newData=writeSeqTailBuilderPackedData(prefixBuilder, newData);
	newData=writeSeqTailBuilderPackedData(suffixBuilder, newData);
	newData=rtaWriteRouteTableArrayBuilderPackedData(routeTableBuilder->arrayBuilder, newData);

}



static void createRoutePatches(RoutingIndexedReadReferenceBlock *rdi, int entryCount,
		SeqTailBuilder *prefixBuilder, SeqTailBuilder *suffixBuilder,
		RoutePatch *forwardPatches, RoutePatch *reversePatches,
		int *forwardCountPtr, int *reverseCountPtr, int *maxNewPrefixPtr, int *maxNewSuffixPtr)
{
	int forwardCount=0, reverseCount=0;
	int maxNewPrefix=0, maxNewSuffix=0;


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

				maxNewPrefix=MAX(maxNewPrefix,forwardPatches[forwardCount].prefixIndex);
				maxNewSuffix=MAX(maxNewSuffix,forwardPatches[forwardCount].suffixIndex);


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

				maxNewPrefix=MAX(maxNewPrefix,reversePatches[reverseCount].prefixIndex);
				maxNewSuffix=MAX(maxNewSuffix,reversePatches[reverseCount].suffixIndex);

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
	*maxNewPrefixPtr=maxNewPrefix;
	*maxNewSuffixPtr=maxNewSuffix;

}





int rtRouteReadsForSmer(RoutingIndexedReadReferenceBlock *rdi, SmerArraySlice *slice,
		RoutingReadData **orderedDispatches, MemDispenser *disp, MemCircHeap *circHeap, u8 sliceTag)
{
	u32 sliceIndex=rdi->sliceIndex;

	u8 *smerDataOrig=slice->smerData[sliceIndex];
	u8 *smerData=smerDataOrig;

	//if(smerData!=NULL)
//		LOG(LOG_INFO, "Index: %i of %i Entries %i Data: %p",sliceIndex,slice->smerCount, rdi->entryCount, smerData);

	SeqTailBuilder prefixBuilder, suffixBuilder;
	RouteTableBuilder routeTableBuilder;

	s32 oldHeaderSize=0, oldPrefixDataSize=0, oldSuffixDataSize=0, oldRouteTableDataSize=0;

	if(smerData!=NULL)
		{
		u8 header=*smerData;

		if((header&ALLOC_HEADER_LIVE_DIRECT_MASK)==ALLOC_HEADER_LIVE_DIRECT_VALUE)
			{
			createBuildersFromDirectData(smerData, &oldHeaderSize,
					&prefixBuilder, &oldPrefixDataSize, &suffixBuilder, &oldSuffixDataSize,
					&routeTableBuilder, &oldRouteTableDataSize, disp);

			}
		else if((header&ALLOC_HEADER_LIVE_INDIRECT_ROOT_MASK)==ALLOC_HEADER_LIVE_INDIRECT_ROOT_VALUE)
			{
			routeTableBuilder.arrayBuilder=NULL;
			LOG(LOG_CRITICAL,"Alloc indirect header %2x at %p",header,smerData);
			}
		else
			{
			routeTableBuilder.arrayBuilder=NULL;
			LOG(LOG_CRITICAL,"Alloc header invalid %2x at %p",header,smerData);
			}
		}
	else
		{
		createBuildersFromNullData(&oldHeaderSize,
				&prefixBuilder, &oldPrefixDataSize, &suffixBuilder, &oldSuffixDataSize,
				&routeTableBuilder, &oldRouteTableDataSize, disp);
		}

	int entryCount=rdi->entryCount;

	int forwardCount=0;
	int reverseCount=0;

	RoutePatch *forwardPatches=dAlloc(disp,sizeof(RoutePatch)*entryCount);
	RoutePatch *reversePatches=dAlloc(disp,sizeof(RoutePatch)*entryCount);

	s32 maxNewPrefix=0;
	s32 maxNewSuffix=0;

	createRoutePatches(rdi, entryCount, &prefixBuilder, &suffixBuilder, forwardPatches, reversePatches,
			&forwardCount, &reverseCount, &maxNewPrefix, &maxNewSuffix);

	rtaMergeRoutes(routeTableBuilder.arrayBuilder, forwardPatches, reversePatches, forwardCount, reverseCount, maxNewPrefix, maxNewSuffix, orderedDispatches, disp);

	if(getSeqTailBuilderDirty(&prefixBuilder) || getSeqTailBuilderDirty(&suffixBuilder) || rtaGetRouteTableArrayBuilderDirty(routeTableBuilder.arrayBuilder))
		{
		writeBuildersAsDirectData(slice->smerData+sliceIndex, sliceTag, sliceIndex,
				&prefixBuilder, oldPrefixDataSize,
				&suffixBuilder, oldSuffixDataSize,
				&routeTableBuilder, oldRouteTableDataSize,
				circHeap);
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






/*
static RoutePatchMergeWideReadset *buildReadsetForRead(RoutePatch *routePatch, s32 minEdgeOffset, s32 maxEdgeOffset, MemDispenser *disp)
{
	RoutePatchMergeWideReadset *readSet=dAlloc(disp, sizeof(RoutePatchMergeWideReadset));
	readSet->firstRoutePatch=routePatch;
	readSet->minEdgeOffset=minEdgeOffset;
	readSet->maxEdgeOffset=maxEdgeOffset;

	return readSet;
}


static RoutePatchMergePositionOrderedReadtree *buildPositionOrderedReadtreeForReadset(RoutePatchMergeWideReadset *readset,
		s32 minEdgePosition, s32 maxEdgePosition, MemDispenser *disp)
{
	RoutePatchMergePositionOrderedReadtree *firstOrderedReadtree=dAlloc(disp, sizeof(RoutePatchMergePositionOrderedReadtree));
	firstOrderedReadtree->next=NULL;
	firstOrderedReadtree->firstWideReadset=readset;

	firstOrderedReadtree->minEdgePosition=minEdgePosition;
	firstOrderedReadtree->maxEdgePosition=maxEdgePosition;

	return firstOrderedReadtree;

}


static RoutePatchMergePositionOrderedReadtree *mergeRoute_buildForwardMergeTree_mergeRead(RoutePatchMergePositionOrderedReadtree *readTree,
		RoutePatch *routePatch, MemDispenser *disp)
{
	// Scan through existing merge tree, find best location, insert/append as appropriate

	RoutePatchMergePositionOrderedReadtree *treeScan=readTree;
	RoutePatchMergeWideReadset *readsetScan=readTree->firstWideReadset;

	s32 newMinEdgePosition=*(routePatch->rdiPtr)->minEdgePosition;
	s32 newMaxEdgePosition=*(routePatch->rdiPtr)->minEdgePosition;

	s32 oldMinEdgePosition=treeScan->minEdgePosition+readsetScan->minEdgeOffset;
	s32 oldMaxEdgePosition=treeScan->minEdgePosition+readsetScan->maxEdgeOffset;

	if(newMaxEdgePosition<oldMinEdgePosition) // ScenarioGroup 1: "Isolated before"
		{
		RoutePatchMergeWideReadset *newReadset=buildReadsetForRead(routePatch, 0, newMaxEdgePosition-newMinEdgePosition, disp);
		RoutePatchMergePositionOrderedReadtree *newReadTree=buildPositionOrderedReadtreeForReadset(newReadset, newMinEdgePosition, newMaxEdgePosition, disp);

		newReadTree->next=readTree;
		return newReadTree;
		}

	RoutePatchMergePositionOrderedReadtree *treePrevious=treeScan;
	RoutePatchMergeWideReadset *readsetPrevious=readsetScan;

	while(newMinEdgePosition>(oldMaxEdgePosition+1)) // Old too early, scan through
		{

		}

	if(readsetScan==NULL) // ScenarioGroup 4: "Isolated after"
		{

		return readTree;
		}

	if(newMinEdgePosition==(oldMaxEdgePosition+1)) // ScenarioGroup 3: "Linked After"
		{

		return readTree;
		}
													// ScenarioGroup 2: "Linked Before/Other"


	return readTree;
}

static RoutePatchMergePositionOrderedReadtree *mergeRoutes_buildForwardMergeTree(RoutePatch **patchScanPtr, RoutePatch *patchEnd, MemDispenser *disp)
{
	RoutePatch *scanPtr=*patchScanPtr;
	int targetUpstream=scanPtr->prefixIndex;

	s32 minEdgePosition=*(scanPtr->rdiPtr)->minEdgePosition;
	s32 maxEdgePosition=*(scanPtr->rdiPtr)->minEdgePosition;

	RoutePatchMergeWideReadset *readset=buildReadsetForRead(scanPtr, 0, maxEdgePosition-minEdgePosition, disp);
	RoutePatchMergePositionOrderedReadtree *readTree=buildPositionOrderedReadtreeForReadset(readset, minEdgePosition, maxEdgePosition, disp);

	scanPtr++;

	while(scanPtr<patchEnd && scanPtr->prefixIndex==targetUpstream)
		readTree=mergeRoute_buildForwardMergeTree_mergeRead(readTree, scanPtr, disp);

	*patchScanPtr=scanPtr;
	return readTree;
}

*/
/*
static RoutePatchMergePositionOrderedReadset *mergeRoutes_buildForwardMergeTree(RoutePatch **patchScanPtr, RoutePatch *patchEnd, MemDispenser *disp)
{
	RoutePatch *scanPtr=*patchScanPtr;
	int targetUpstream=scanPtr->prefixIndex;

	int minEdgePosition=(*(scanPtr->rdiPtr))->minEdgePosition;
	int maxEdgePosition=(*(scanPtr->rdiPtr))->maxEdgePosition;

	RoutePatchMergeWideReadset *readSet=dAlloc(disp, sizeof(RoutePatchMergeWideReadset));
	readSet->firstRoutePatch=scanPtr;
	readSet->minEdgeOffset=minEdgePosition;
	readSet->maxEdgeOffset=maxEdgePosition-minEdgePosition;

	RoutePatchMergePositionOrderedReadset *firstOrderedReadset=dAlloc(disp, sizeof(RoutePatchMergePositionOrderedReadset));
	firstOrderedReadset->next=NULL;
	firstOrderedReadset->firstWideReadset=readSet;

	firstOrderedReadset->minEdgePosition=minEdgePosition;
	firstOrderedReadset->maxEdgePosition=maxEdgePosition;

	//RoutePatchMergePositionOrderedReadset **appendOrderedReadset=&(firstOrderedReadset->next);

	scanPtr++;


	while(scanPtr<patchEnd && scanPtr->prefixIndex==targetUpstream)
		{
		minEdgePosition=(*(scanPtr->rdiPtr))->minEdgePosition;
		maxEdgePosition=(*(scanPtr->rdiPtr))->maxEdgePosition;

		RoutePatchMergePositionOrderedReadset **scanOrderedReadset=&firstOrderedReadset;

		while(*scanOrderedReadset!=NULL && (*scanOrderedReadset)->maxEdgePosition < minEdgePosition) // Skip all non-overlapping lists before target
			scanOrderedReadset=&((*scanOrderedReadset)->next);

		if((*scanOrderedReadset)->minEdgePosition >= maxEdgePosition) // New Ordered list needed, with top-level renumber
			{
			RoutePatchMergePositionOrderedReadset *newOrderedReadset=dAlloc(disp, sizeof(RoutePatchMergePositionOrderedReadset));
			newOrderedReadset->next=(*scanOrderedReadset)->next;
			*scanOrderedReadset=newOrderedReadset;

			newOrderedReadset->minEdgePosition=minEdgePosition;
			newOrderedReadset->maxEdgePosition=maxEdgePosition;

			readSet=dAlloc(disp, sizeof(RoutePatchMergeWideReadset));
			readSet->firstRoutePatch=scanPtr;
			readSet->minEdgeOffset=minEdgePosition;
			readSet->maxEdgeOffset=maxEdgePosition-minEdgePosition;

			newOrderedReadset->firstWideReadset=readSet;
			}
		else // Existing ordered list found, need parent widen and two level renumber
			{

			}




		scanPtr++;
		}


	*patchScanPtr=scanPtr;
	return firstOrderedReadset;
}

*/








