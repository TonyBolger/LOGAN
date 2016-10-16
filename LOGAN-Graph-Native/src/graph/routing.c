#include "common.h"


/*

Alloc Header:

	0 0 0 0 0 0 0 0 : INVALID: Unused region

    1 0 0 0 0 0 0 0 : CHUNK: Start of chunk header (with 1 byte tag)

	1 x x x x x x x : Live block
	0 x x x x x x x : Dead block

	. 0 x x x x x x : Direct block
	. 1 x x x x x x : Indirect block formats

	. 0 g g g x x x : Direct, g=gap exponent(1+), x=reserved

	. 0 0 0 0 0 1 0 : Unused
	. 0 0 0 0 1 0 0 : Unused
	. 0 0 0 0 1 1 0 : Unused

	. 1 g g g p p 0 : Indirect root, g=gap exponent(1+), p=pointer block size

    . 1 0 i i s s 1 : Indirect route branch, i=index size, s=subindex size
    . 1 1 i i s s 1 : Indirect route leaf, i=index size, s=subindex size

	Branches/Leaves marker followed by 1-4 byte index, 1-4 bytes sub-index
	Branches then followed with up to 256 pointers (optionally NULL)
	Leaf is followed by size (u16)

	Gap exponent 1: 0-255 gap (with value): Exact
	Gap exponent 2: 256-511 gap (with value+256): Exact
    Gap exponent 3: 0xD8 Indirect root with 512-1023 gap (with value*2+512): 0-1
	Gap exponent 4: 0xE0 Indirect root with 1024-2047 gap (with value*4+1024): 0-3
	Gap exponent 5: 0xE8 Indirect root with 2048-4095 gap (with value*8+2048): 0-7
	Gap exponent 6: 0xF0 Indirect root with 4096-8191 gap (with value*16+4096): 0-15
    Gap exponent 7: 0xF8 Indirect root with 8192-16383 gap (with value*32+8192): 0-31

	Pointer block 1: Indirect root 4 pointers (2 tail, 1 forward route, 1 reverse route)
	Pointer block 2: Indirect root 16 pointers (2 tail, 7 forward route, 7 reverse route)
	Pointer block 3: Indirect root 64 pointers (2 tail, 31 forward route, 31 reverse route)
	Pointer block 4: Indirect root 256 pointers (2 tail, 127 forward route, 127 reverse route)


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

// Check Live

#define ALLOC_HEADER_LIVE_MASK 0x80							// ? - - -  - - - -

// Check Direct

#define ALLOC_HEADER_DIRECT_MASK 0x40						// - ? - -  - - - -
#define ALLOC_HEADER_DIRECT_VALUE 0x00						// - 0 - -  - - - -
#define ALLOC_HEADER_INDIRECT_VALUE 0x40					// - 1 - -  - - - -

#define ALLOC_HEADER_LIVE_DIRECT_MASK 0xC0					// ? ? - -  - - - -
#define ALLOC_HEADER_LIVE_DIRECT_VALUE 0x80					// 1 0 - -  - - - -

#define ALLOC_HEADER_DIRECT_GAPSIZE_MASK 0x38				// - - ? ?  ? - - -
#define ALLOC_HEADER_LIVE_DIRECT_GAPSMALL 0x88              // 1 0 0 0  1 0 0 0

// Check type of Indirect

#define ALLOC_HEADER_INDIRECT_MASK 0x41     				// ? ? - -  - - - ?
#define ALLOC_HEADER_INDIRECT_ROOT_VALUE 0x40    			// 1 1 - -  - - - 0
#define ALLOC_HEADER_INDIRECT_NONROOT_VALUE 0x41	 		// 1 1 - -  - - - 1

#define ALLOC_HEADER_LIVE_INDIRECT_MASK 0xC1     			// ? ? - -  - - - ?
#define ALLOC_HEADER_LIVE_INDIRECT_ROOT_VALUE 0xC0    		// 1 1 - -  - - - 0
#define ALLOC_HEADER_LIVE_INDIRECT_NONROOT_VALUE 0xC1		// 1 1 - -  - - - 1


#define ALLOC_HEADER_INDIRECT_ROOT_GAPSIZE_MASK 0x38  		// - - ? ?  ? - - -
#define ALLOC_HEADER_INDIRECT_ROOT_PTRSIZE_MASK 0x06   		// - - - -  - ? ? -

#define ALLOC_HEADER_LIVE_INDIRECT_ROOT_GAPSMALL 0xC8   	// 1 1 0 0  1 0 0 0

// Check type of NonRoot

#define ALLOC_HEADER_NONROOT_MASK 0x61						// - ? ? -  - - - ?
#define ALLOC_HEADER_NONROOT_BRANCH 0x41					// - 1 0 -  - - - 1
#define ALLOC_HEADER_NONROOT_LEAF 0x61						// - 1 1 -  - - - 1

#define ALLOC_HEADER_LIVE_NONROOT_MASK 0xE1					// ? ? ? -  - - - ?
#define ALLOC_HEADER_LIVE_NONROOT_BRANCH 0xC1				// 1 1 0 -  - - - 1
#define ALLOC_HEADER_LIVE_NONROOT_LEAF 0xE1					// 1 1 1 -  - - - 1








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

static void encodeDirectBlockHeader(s32 tagOffsetDiff, u8 *data)
{
	if(tagOffsetDiff<0)
		{
		LOG(LOG_CRITICAL,"Expected positive tagOffsetDiff %i",tagOffsetDiff);
		}

	if(tagOffsetDiff<256)
		{
		*data++=ALLOC_HEADER_LIVE_DIRECT_GAPSMALL;
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

		u8 data1=ALLOC_HEADER_LIVE_DIRECT_GAPSMALL + (exp << 3);

		*data++=data1;
		*data=man;

		//LOG(LOG_INFO,"Wrote other Header: %02x %02x for %i (exp %i)",data1,man,tagOffsetDiff,exp);
		}
}

static void decodeDirectBlockHeader(u8 *data, s32 *tagOffsetDiffPtr)
{
	u8 header=*data++;

	if(tagOffsetDiffPtr!=NULL)
		{
		int exp=((header&ALLOC_HEADER_DIRECT_GAPSIZE_MASK)>>3);

		u32 res=*data;

		if(exp>1)
			res=(256+res)<<(exp-2);

		*tagOffsetDiffPtr=res;
		}

}

static s32 getDirectBlockHeaderSize()
{
	return 2;
}




//static
void encodeIndirectRootBlockHeader(s32 tagOffsetDiff, s32 ptrBlockSize, u8 *data)
{
	if(tagOffsetDiff<0)
		{
		LOG(LOG_CRITICAL,"Expected positive tagOffsetDiff %i",tagOffsetDiff);
		}

	ptrBlockSize<<=1;

	if(tagOffsetDiff<256)
		{
		*data++=ALLOC_HEADER_LIVE_INDIRECT_ROOT_GAPSMALL+ptrBlockSize;
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

		u8 data1=ALLOC_HEADER_LIVE_INDIRECT_ROOT_GAPSMALL+(exp << 3)+ptrBlockSize;

		*data++=data1;
		*data=man;

		//LOG(LOG_INFO,"Wrote other Header: %02x %02x for %i (exp %i)",data1,man,tagOffsetDiff,exp);
		}
}

//static
void decodeIndirectRootBlockHeader(u8 *data, s32 *tagOffsetDiffPtr, s32 *ptrBlockSizePtr)
{
	u8 header=*data++;

	if(tagOffsetDiffPtr!=NULL)
		{
		int exp=((header&ALLOC_HEADER_INDIRECT_ROOT_GAPSIZE_MASK)>>3);

		u32 res=*data;

		if(exp>1)
			res=(256+res)<<(exp-2);

		*tagOffsetDiffPtr=res;
		}

	if(ptrBlockSizePtr!=NULL)
		*ptrBlockSizePtr=(header&ALLOC_HEADER_INDIRECT_ROOT_PTRSIZE_MASK)>>1;

}

//static
s32 getIndirectRootBlockHeaderSize()
{
	return 2;
}


void encodeNonRootBlockHeader(u32 isLeaf, u32 indexSize, u32 index, u32 subindexSize, u32 subindex, u8 *data)
{
	LOG(LOG_INFO,"Encoding Leaf: %i Index: %i %i Subindex: %i %i", isLeaf, indexSize, index, subindexSize, subindex);

	if(isLeaf)
		*data++=ALLOC_HEADER_LIVE_NONROOT_LEAF+((indexSize-1)<<3)+((subindexSize-1)<<1);
	else
		*data++=ALLOC_HEADER_LIVE_NONROOT_BRANCH+((indexSize-1)<<3)+((subindexSize-1)<<1);

	varipackEncode(index, data);
	data+=indexSize;
	varipackEncode(subindex, data);

}

//static
void decodeNonRootBlockHeader(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexSizePtr, s32 *subindexPtr)
{
	u8 header=*data++;

//	if(isLeafPtr!=NULL)
//		*isLeafPtr=(header>>6)&0x1;

	s32 indexSize=1+((header>>3)&0x3);
	s32 subindexSize=1+((header>>1)&0x1);

	if(indexSizePtr!=NULL)
		*indexSizePtr=indexSize;

	if(subindexSizePtr!=NULL)
		*subindexSizePtr=subindexSize;

	if(indexPtr!=NULL)
		*indexPtr=varipackDecode(indexSize, data);

	data+=indexSize;

	if(subindexPtr!=NULL)
		*subindexPtr=varipackDecode(subindexSize, data);

}

//static
s32 getNonRootBlockHeaderSize(int indexSize, int subIndexSize)
{
	return 1+indexSize+subIndexSize;
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

		if((header & ALLOC_HEADER_DIRECT_MASK) == ALLOC_HEADER_DIRECT_VALUE)
			{
			s32 chunkTagOffsetDiff=0;
			decodeDirectBlockHeader(heapDataPtr, &chunkTagOffsetDiff);

			currentIndex+=chunkTagOffsetDiff;
			u8 *scanPtr=heapDataPtr+2;

			scanPtr=scanTails(scanPtr);
			scanPtr=scanTails(scanPtr);
			scanPtr=rtaScanRouteTableArray(scanPtr);

			s32 size=scanPtr-heapDataPtr;

			if(header & ALLOC_HEADER_LIVE_MASK)
				{
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
				index->entries[entry].subindex=-1;
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
		else // Some kind of indirect
			{
			if((header & ALLOC_HEADER_INDIRECT_MASK)==ALLOC_HEADER_INDIRECT_ROOT_VALUE) // Indirect root
				{
				s32 chunkTagOffsetDiff=0, ptrBlockSize=0;
				decodeIndirectRootBlockHeader(heapDataPtr, &chunkTagOffsetDiff, &ptrBlockSize);
				currentIndex+=chunkTagOffsetDiff;

				s32 size=0;

				if(ptrBlockSize==0)
					size=getIndirectRootBlockHeaderSize()+sizeof(RouteTableSmallRoot);
				else
					LOG(LOG_CRITICAL,"Found indirect non-small root block in reclaimIndexer");

				if(header & ALLOC_HEADER_LIVE_MASK)
					{
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
					index->entries[entry].subindex=-1;
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

				heapDataPtr+=size;
				}
			else
				{
				if((header & ALLOC_HEADER_NONROOT_MASK)== ALLOC_HEADER_NONROOT_BRANCH) // Indirect branch
					{
					LOG(LOG_CRITICAL,"Found indirect branch in reclaimIndexer");
					}
				else																  // Indirect Leaf
					{
					s32 sindexSize=0, sindex=0;
					s32 subindexSize=0, subindex=0;

					decodeNonRootBlockHeader(heapDataPtr, &sindexSize, &sindex, &subindexSize, &subindex);

					LOG(LOG_INFO,"Found indirect leaf %p in reclaimIndexer with %i %i %i %i",heapDataPtr, sindexSize,sindex,subindexSize,subindex);

					s32 headerSize=getNonRootBlockHeaderSize(sindexSize, subindexSize);
					u8 *scanPtr=heapDataPtr+headerSize;

					if(subindex==0 || subindex==1)
						{
						scanPtr=scanTails(scanPtr);
						}
					else
						{
						scanPtr=rtaScanRouteTableArray(scanPtr);
						}

					s32 size=scanPtr-heapDataPtr;

					LOG(LOG_INFO,"Leaf size %i",size);

					if(header & ALLOC_HEADER_LIVE_MASK)
						{
						if(entry==index->entryAlloc)
							index=allocOrExpandIndexer(index, disp);

						index->entries[entry].index=sindex;
						index->entries[entry].subindex=subindex;
						index->entries[entry].size=size;
						entry++;

						liveSize+=size;
						}
					else
						{
						deadSize+=size;
						}

					heapDataPtr+=size;
					}
				}
			}
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

		if(index->entries[i].subindex==-1) // Direct or Indirect root
			{
			u8 *headerPtr=*ptr;
			int blockHeaderSize=0;

			s32 diff=offset-prevOffset;

			if(((*headerPtr)&ALLOC_HEADER_DIRECT_MASK) == ALLOC_HEADER_DIRECT_VALUE)
				{
				blockHeaderSize=getDirectBlockHeaderSize();
				encodeDirectBlockHeader(diff, newChunk);
				}
			else
				{
				s32 ptrBlock;
				decodeIndirectRootBlockHeader(headerPtr, NULL, &ptrBlock);

				blockHeaderSize=getIndirectRootBlockHeaderSize();
				encodeIndirectRootBlockHeader(diff, ptrBlock, newChunk);
				}

			if(size<blockHeaderSize)
				{
				LOG(LOG_CRITICAL,"Undersize block %i, referenced by %p", size, *ptr);
				}

			prevOffset=offset;
			memmove(newChunk+blockHeaderSize,(*ptr)+blockHeaderSize,size-blockHeaderSize);

			*ptr=newChunk;
			newChunk+=size;
			}
		else
			{
			LOG(LOG_CRITICAL,"Not implemented");
			}
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
	builder->upgradedToTree=0;

	return rtaInitRouteTableArrayBuilder(arrayBuilder,data,disp);
}


//static
u8 *initRouteTableTreeBuilder(RouteTableBuilder *builder, u8 *data, MemDispenser *disp)
{
	builder->disp=disp;

	RouteTableTreeBuilder *treeBuilder=dAlloc(disp, sizeof(RouteTableTreeBuilder));
	builder->arrayBuilder=NULL;
	builder->treeBuilder=treeBuilder;
	builder->upgradedToTree=0;

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

	prefixBuilder->oldData=NULL;
	suffixBuilder->oldData=NULL;
}


//static
void createBuildersFromIndirectData(u8 *data, s32 *headerSize,
			SeqTailBuilder *prefixBuilder, s32 *prefixDataSize,
			SeqTailBuilder *suffixBuilder, s32 *suffixDataSize,
			RouteTableBuilder *routeTableBuilder, s32 *routeTableDataSize,
			MemDispenser *disp)
{
	LOG(LOG_CRITICAL,"Not implemented");

	/*

	data+=2;
	*headerSize=2;

	data=initRouteTableTreeBuilder(routeTableBuilder, data, disp);
*/

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
	if(!(getSeqTailBuilderDirty(prefixBuilder) || getSeqTailBuilderDirty(suffixBuilder) || rtaGetRouteTableArrayBuilderDirty(routeTableBuilder->arrayBuilder)))
		{
		LOG(LOG_INFO,"Nothing to write");
		return;
		}

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

	//LOG(LOG_INFO,"Direct write to %p %i",newData,totalSize);

	s32 diff=sliceIndex-oldTagOffset;

	*smerDataPtr=newData;
	encodeDirectBlockHeader(diff, newData);

	newData+=2;
	newData=writeSeqTailBuilderPackedData(prefixBuilder, newData);
	newData=writeSeqTailBuilderPackedData(suffixBuilder, newData);
	newData=rtaWriteRouteTableArrayBuilderPackedData(routeTableBuilder->arrayBuilder, newData);

}

static int considerUpgradingToTree(RouteTableBuilder *routeTableBuilder, int newForwardRoutes, int newReverseRoutes)
{
	if(routeTableBuilder->arrayBuilder==NULL)
		return 0;

	if(routeTableBuilder->treeBuilder!=NULL)
		{
		LOG(LOG_CRITICAL,"Routes already a Tree");
		return 0;
		}

	int existingRoutes=(routeTableBuilder->arrayBuilder->oldForwardEntryCount)+(routeTableBuilder->arrayBuilder->oldReverseEntryCount);
	int totalRoutes=existingRoutes+newForwardRoutes+newReverseRoutes;

	return totalRoutes>ROUTING_TREE_THRESHOLD;
}


static void upgradeToTree(RouteTableBuilder *routeTableBuilder,	SeqTailBuilder *prefixBuilder, SeqTailBuilder *suffixBuilder)
{
	RouteTableTreeBuilder *treeBuilder=dAlloc(routeTableBuilder->disp, sizeof(RouteTableTreeBuilder));
	routeTableBuilder->treeBuilder=treeBuilder;

	routeTableBuilder->upgradedToTree=1;

	rttUpgradeToRouteTableTreeBuilder(treeBuilder, prefixBuilder, suffixBuilder, routeTableBuilder->arrayBuilder, routeTableBuilder->disp);
}


static void writeBuildersAsIndirectData(u8 **smerDataPtr, s8 sliceTag, s32 sliceIndex,
		SeqTailBuilder *prefixBuilder, SeqTailBuilder *suffixBuilder, RouteTableBuilder *routeTableBuilder,
		MemCircHeap *circHeap)
{
	int sliceIndexSize=varipackLength(sliceIndex);

	RouteTableSmallRoot *root=routeTableBuilder->treeBuilder->rootPtr;

	if(routeTableBuilder->upgradedToTree || root==NULL)
		{
		u8 *oldData=*smerDataPtr;

		if(oldData!=NULL)
			{
			LOG(LOG_INFO,"Deleting %p",oldData);
			*oldData&=~ALLOC_HEADER_LIVE_MASK;
			}

		int rootHeaderSize=getIndirectRootBlockHeaderSize();
		int rootTotalSize=rootHeaderSize+sizeof(RouteTableSmallRoot);

		s32 oldTagOffset=0;
		u8 *newData=circAlloc(circHeap, rootTotalSize, sliceTag, sliceIndex, &oldTagOffset);

		LOG(LOG_INFO,"Writing indirect root to %p",newData);

		s32 diff=sliceIndex-oldTagOffset;
		*smerDataPtr=newData;

		encodeIndirectRootBlockHeader(diff, 0, newData);

		root=(RouteTableSmallRoot *)(newData+getIndirectRootBlockHeaderSize());
		routeTableBuilder->treeBuilder->rootPtr=root;
		}

	if(routeTableBuilder->upgradedToTree || getSeqTailBuilderDirty(prefixBuilder))
		{
		int subindex=0;
		int subindexSize=varipackLength(subindex);

		int prefixHeaderSize=getNonRootBlockHeaderSize(sliceIndexSize, subindexSize);

		int prefixTotalSize=getSeqTailBuilderPackedSize(prefixBuilder)+prefixHeaderSize;
		u8 *oldData=prefixBuilder->oldData;

		// Mark old block as dead

		if(oldData!=NULL)
			{
			LOG(LOG_INFO,"Deleting %p",*oldData);
			*oldData&=~ALLOC_HEADER_LIVE_MASK;
			}

		s32 oldTagOffset=0;
		u8 *newData=circAlloc(circHeap, prefixTotalSize, sliceTag, sliceIndex, &oldTagOffset);

		LOG(LOG_INFO,"Writing indirect prefix to %p",newData);

		//LOG(LOG_INFO,"Offset Diff: %i for %i",offsetDiff,sliceIndex);

		if(newData==NULL)
			{
			LOG(LOG_CRITICAL,"Failed at alloc after compact: Wanted %i",prefixTotalSize);
			}

		encodeNonRootBlockHeader(1, sliceIndexSize, sliceIndex, subindexSize, subindex, newData);

		root->prefixData=newData;
		newData+=prefixHeaderSize;

		writeSeqTailBuilderPackedData(prefixBuilder, newData);
		}
	else
		root->prefixData=prefixBuilder->oldData;


	if(routeTableBuilder->upgradedToTree || getSeqTailBuilderDirty(suffixBuilder))
		{
		int subindex=1;
		int subindexSize=varipackLength(subindex);

		int suffixHeaderSize=getNonRootBlockHeaderSize(sliceIndexSize, subindexSize);

		int suffixTotalSize=getSeqTailBuilderPackedSize(suffixBuilder)+suffixHeaderSize;
		u8 *oldData=suffixBuilder->oldData;

		// Mark old block as dead

		if(oldData!=NULL)
			{
			LOG(LOG_INFO,"Deleting %p",*oldData);
			*oldData&=~ALLOC_HEADER_LIVE_MASK;
			}

		s32 oldTagOffset=0;
		u8 *newData=circAlloc(circHeap, suffixTotalSize, sliceTag, sliceIndex, &oldTagOffset);

		LOG(LOG_INFO,"Writing indirect suffix to %p",newData);

		//LOG(LOG_INFO,"Offset Diff: %i for %i",offsetDiff,sliceIndex);

		if(newData==NULL)
			{
			LOG(LOG_CRITICAL,"Failed at alloc after compact: Wanted %i",suffixTotalSize);
			}

		encodeNonRootBlockHeader(1, sliceIndexSize, sliceIndex, subindexSize, subindex, newData);

		root->suffixData=newData;
		newData+=suffixHeaderSize;

		writeSeqTailBuilderPackedData(suffixBuilder, newData);
		}
	else
		root->suffixData=suffixBuilder->oldData;


	if(routeTableBuilder->upgradedToTree || rttGetRouteTableTreeBuilderDirty(routeTableBuilder->treeBuilder))
		{
		int subindex=2;
		int subindexSize=varipackLength(subindex);

		int routeTableHeaderSize=getNonRootBlockHeaderSize(sliceIndexSize, subindexSize);

		int routeTableTotalSize=rtaGetRouteTableArrayBuilderPackedSize(routeTableBuilder->treeBuilder->nestedBuilder)+routeTableHeaderSize;
//		u8 *oldData=routeTableBuilder->treeBuilder->nestedBuilder->oldData;

		// Mark old block as dead
		/*
		if(oldData!=NULL)
			{
			LOG(LOG_INFO,"Deleting %p",*oldData);
			*oldData&=~ALLOC_HEADER_LIVE_MASK;
			}
*/

		s32 oldTagOffset=0;
		u8 *newData=circAlloc(circHeap, routeTableTotalSize, sliceTag, sliceIndex, &oldTagOffset);

		LOG(LOG_INFO,"Writing indirect rootTable to %p %i",newData, routeTableTotalSize);

		//LOG(LOG_INFO,"Offset Diff: %i for %i",offsetDiff,sliceIndex);

		if(newData==NULL)
			{
			LOG(LOG_CRITICAL,"Failed at alloc after compact: Wanted %i",routeTableTotalSize);
			}

		encodeNonRootBlockHeader(1, sliceIndexSize, sliceIndex, subindexSize, subindex, newData);

		root->forwardRouteData[0]=newData;
		newData+=routeTableHeaderSize;

		newData=rttWriteRouteTableTreeBuilderPackedData(routeTableBuilder->treeBuilder, newData);
		}
	else
		{

		}

	root->reverseRouteData[0]=NULL;

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
		// ALLOC_HEADER_LIVE_INDIRECT_ROOT_VALUE

		else if((header&ALLOC_HEADER_LIVE_INDIRECT_MASK)==ALLOC_HEADER_LIVE_INDIRECT_ROOT_VALUE)
			{
			routeTableBuilder.arrayBuilder=NULL;
			LOG(LOG_CRITICAL,"Alloc indirect root %2x at %p",header,smerData);
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

	if(considerUpgradingToTree(&routeTableBuilder, forwardCount, reverseCount))
		upgradeToTree(&routeTableBuilder, &prefixBuilder, &suffixBuilder);

	if(routeTableBuilder.treeBuilder!=NULL)
		{
		rttMergeRoutes(routeTableBuilder.treeBuilder, forwardPatches, reversePatches, forwardCount, reverseCount, maxNewPrefix, maxNewSuffix, orderedDispatches, disp);

		writeBuildersAsIndirectData(slice->smerData+sliceIndex, sliceTag, sliceIndex,
				&prefixBuilder, &suffixBuilder, &routeTableBuilder, circHeap);
		}
	else
		{
		rtaMergeRoutes(routeTableBuilder.arrayBuilder, forwardPatches, reversePatches, forwardCount, reverseCount, maxNewPrefix, maxNewSuffix, orderedDispatches, disp);

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








