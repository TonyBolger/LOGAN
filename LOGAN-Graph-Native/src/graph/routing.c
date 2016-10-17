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
		LOG(LOG_CRITICAL,"Expected positive tagOffsetDiff direct %i",tagOffsetDiff);
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
		LOG(LOG_CRITICAL,"Expected positive tagOffsetDiff indirect: %i",tagOffsetDiff);
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
	//LOG(LOG_INFO,"Encoding Leaf: %i Index: %i %i Subindex: %i %i", isLeaf, indexSize, index, subindexSize, subindex);

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
//				LOG(LOG_INFO,"Indexed indirect root at %p",heapDataPtr);

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

//					LOG(LOG_INFO,"Indexed leaf at %p",heapDataPtr);

					decodeNonRootBlockHeader(heapDataPtr, &sindexSize, &sindex, &subindexSize, &subindex);

//					LOG(LOG_INFO,"Found indirect leaf %p in reclaimIndexer with %i %i %i %i",heapDataPtr, sindexSize,sindex,subindexSize,subindex);

					s32 headerSize=getNonRootBlockHeaderSize(sindexSize, subindexSize);
					u8 *scanPtr=heapDataPtr+headerSize;

					if(subindex==0 || subindex==1)
						{
//						LOG(LOG_INFO,"Tail scan");
						scanPtr=scanTails(scanPtr);
						}
					else
						{
//						LOG(LOG_INFO,"Route scan");
						scanPtr=rtaScanRouteTableArray(scanPtr);
						}

					s32 size=scanPtr-heapDataPtr;

//					LOG(LOG_INFO,"Leaf size %i at %p",size, heapDataPtr);

					if(header & ALLOC_HEADER_LIVE_MASK)
						{
						if(entry==index->entryAlloc)
							index=allocOrExpandIndexer(index, disp);

//						LOG(LOG_INFO,"Live entry with %i %i %i",sindex,subindex,size);

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
		s32 sindex=index->entries[i].index;

		u8 **primaryPtr=&tagData[sindex];

		if((*primaryPtr)==NULL)
			{
			LOG(LOG_CRITICAL,"Attempt to relocate NULL data");
			}

		s32 size=index->entries[i].size;
		s32 subindex=index->entries[i].subindex;

		if(subindex==-1) // Direct or Indirect root (tag offset needs updating)
			{
			u8 *headerPtr=*primaryPtr;
			int blockHeaderSize=0;

			s32 diff=sindex-prevOffset;

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

				//u8 *dataPtr=headerPtr+blockHeaderSize;
				//RouteTableSmallRoot *rootPtr=(RouteTableSmallRoot *)dataPtr;

				encodeIndirectRootBlockHeader(diff, ptrBlock, newChunk);
//				LOG(LOG_INFO,"Moving indirect root to %p",newChunk);
				}

			if(size<blockHeaderSize)
				{
				LOG(LOG_CRITICAL,"Undersize block %i, referenced by %p", size, *primaryPtr);
				}

			prevOffset=sindex;
			memmove(newChunk+blockHeaderSize,(*primaryPtr)+blockHeaderSize,size-blockHeaderSize);

			*primaryPtr=newChunk;
			newChunk+=size;
			}
		else // Branch or Leaf, reachable
			{
			//LOG(LOG_INFO,"Leaf/Branch %i %i %i",size,sindex,subindex);

			RouteTableSmallRoot *rootPtr=(RouteTableSmallRoot *)((*primaryPtr)+getIndirectRootBlockHeaderSize());

			u8 *dataPtr=NULL;

			if(subindex==0)
				{
				dataPtr=rootPtr->prefixData;
				rootPtr->prefixData=newChunk;
				}
			else if(subindex==1)
				{
				dataPtr=rootPtr->suffixData;
				rootPtr->suffixData=newChunk;
				}
			else
				{
				dataPtr=rootPtr->forwardRouteData[0];
				rootPtr->forwardRouteData[0]=newChunk;
				}

//			LOG(LOG_INFO,"Moving indirect leaf to %p, ref by %p",newChunk,*primaryPtr);

//			LOG(LOG_INFO,"Moving %i bytes from %p to %p",size,dataPtr,newChunk);

			memmove(newChunk,dataPtr,size);
			newChunk+=size;
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






static void createBuildersFromNullData(RoutingComboBuilder *builder)
{
	builder->combinedDataBlock.subindexSize=0;
	builder->combinedDataBlock.subindex=-1;
	builder->combinedDataBlock.headerSize=0;
	builder->combinedDataBlock.dataSize=0;
	builder->combinedDataBlock.blockPtr=NULL;

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

	s32 headerSize=getDirectBlockHeaderSize();
	u8 *afterHeader=data+headerSize;

	builder->combinedDataBlock.subindexSize=0;
	builder->combinedDataBlock.subindex=-1;
	builder->combinedDataBlock.headerSize=headerSize;
	builder->combinedDataBlock.blockPtr=data;

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
	//LOG(LOG_INFO,"Creating from indirect data");

	u8 *data=*(builder->rootPtr);

	s32 headerSize=getIndirectRootBlockHeaderSize();

	builder->topDataBlock.subindexSize=0;
	builder->topDataBlock.subindex=-1;
	builder->topDataBlock.headerSize=headerSize;
	builder->topDataBlock.blockPtr=data;

	builder->arrayBuilder=NULL;
	builder->treeBuilder=dAlloc(builder->disp, sizeof(RouteTableTreeBuilder));

	RouteTableSmallRoot *root=(RouteTableSmallRoot *)(data+headerSize);
	builder->treeBuilder->rootPtr=root;

//	LOG(LOG_INFO,"Data is %p, root is %p, p/s %p %p",data,root,root->prefixData, root->suffixData);


	if((*(root->prefixData)&ALLOC_HEADER_LIVE_MASK)==0x0)
		{
		LOG(LOG_CRITICAL,"Prefix references dead block");
		}

	if((*(root->suffixData)&ALLOC_HEADER_LIVE_MASK)==0x0)
		{
		LOG(LOG_CRITICAL,"Suffix references dead block");
		}

	if((*(root->forwardRouteData[0])&ALLOC_HEADER_LIVE_MASK)==0x0)
		{
		LOG(LOG_CRITICAL,"Route Table references dead block");
		}

	s32 indexSize=0, subindexSize=0;

	// Extract Prefix Info
	decodeNonRootBlockHeader(root->prefixData, &indexSize, NULL, &subindexSize, &builder->prefixDataBlock.subindex);
	headerSize=getNonRootBlockHeaderSize(indexSize, subindexSize);

	builder->prefixDataBlock.subindexSize=subindexSize;
	builder->prefixDataBlock.headerSize=headerSize;
	builder->prefixDataBlock.blockPtr=root->prefixData;

	u8 *prefixData=root->prefixData+getNonRootBlockHeaderSize(indexSize, subindexSize);
	initSeqTailBuilder(&(builder->prefixBuilder), prefixData, builder->disp);


	// Extract Suffix Info
	decodeNonRootBlockHeader(root->suffixData, &indexSize, NULL, &subindexSize, &builder->suffixDataBlock.subindex);
	headerSize=getNonRootBlockHeaderSize(indexSize, subindexSize);

	builder->suffixDataBlock.subindexSize=subindexSize;
	builder->suffixDataBlock.headerSize=headerSize;
	builder->suffixDataBlock.blockPtr=root->suffixData;

	u8 *suffixData=root->suffixData+getNonRootBlockHeaderSize(indexSize, subindexSize);
	initSeqTailBuilder(&(builder->suffixBuilder), suffixData, builder->disp);


	// Extract RouteTable Info

	decodeNonRootBlockHeader(root->forwardRouteData[0], &indexSize, NULL, &subindexSize, &builder->routeTableDataBlock.subindex);
	headerSize=getNonRootBlockHeaderSize(indexSize, subindexSize);

	builder->suffixDataBlock.subindexSize=subindexSize;
	builder->suffixDataBlock.headerSize=headerSize;
	builder->suffixDataBlock.blockPtr=root->forwardRouteData[0];

	//LOG(LOG_INFO,"Sizes: %i %i",indexSize, subindexSize);
	u8 *routeData=root->forwardRouteData[0]+getNonRootBlockHeaderSize(indexSize, subindexSize);
	rttInitRouteTableTreeBuilder(builder->treeBuilder, routeData, builder->disp);

	builder->upgradedToTree=0;
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

	int totalSize=getDirectBlockHeaderSize()+prefixPackedSize+suffixPackedSize+routeTablePackedSize;

	if(totalSize>16083)
		{
		int oldShifted=oldTotalSize>>14;
		int newShifted=totalSize>>14;

		if(oldShifted!=newShifted)
			{
			LOG(LOG_INFO,"LARGE ALLOC: %i",totalSize);
			}

		}

	u8 *oldData=builder->combinedDataBlock.blockPtr;
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

	*(builder->rootPtr)=newData;

	encodeDirectBlockHeader(diff, newData);

	newData+=getDirectBlockHeaderSize();
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


static void upgradeToTree(RoutingComboBuilder *builder)
{
	RouteTableTreeBuilder *treeBuilder=dAlloc(builder->disp, sizeof(RouteTableTreeBuilder));
	builder->treeBuilder=treeBuilder;

	rttUpgradeToRouteTableTreeBuilder(builder->arrayBuilder,  builder->treeBuilder, builder->disp);
	builder->upgradedToTree=1;
}


static void writeBuildersAsIndirectData(RoutingComboBuilder *routingBuilder, s8 sliceTag, s32 sliceIndex, MemCircHeap *circHeap)
{
	int sliceIndexSize=varipackLength(sliceIndex);

	RouteTableSmallRoot *root=routingBuilder->treeBuilder->rootPtr;

	if(routingBuilder->upgradedToTree || root==NULL)
		{
		u8 *oldData=*(routingBuilder->rootPtr);

		if(oldData!=NULL)
			{
			//LOG(LOG_INFO,"Deleting root %p",oldData);
			*oldData&=~ALLOC_HEADER_LIVE_MASK;
			}

		int rootHeaderSize=getIndirectRootBlockHeaderSize();
		int rootTotalSize=rootHeaderSize+sizeof(RouteTableSmallRoot);

		s32 oldTagOffset=0;
		u8 *newData=circAlloc(circHeap, rootTotalSize, sliceTag, sliceIndex, &oldTagOffset);

//		LOG(LOG_INFO,"Writing indirect root to %p",newData);

		s32 diff=sliceIndex-oldTagOffset;
		*(routingBuilder->rootPtr)=newData;

		encodeIndirectRootBlockHeader(diff, 0, newData);

		root=(RouteTableSmallRoot *)(newData+getIndirectRootBlockHeaderSize());
		routingBuilder->treeBuilder->rootPtr=root;

		root->prefixData=NULL;
		root->suffixData=NULL;
		root->forwardRouteData[0]=NULL;
		root->reverseRouteData[0]=NULL;
		}

	// Consider re-using?
	if(routingBuilder->upgradedToTree || getSeqTailBuilderDirty(&(routingBuilder->prefixBuilder)))
		{
		int subindex=0;
		int subindexSize=varipackLength(subindex);

		int prefixHeaderSize=getNonRootBlockHeaderSize(sliceIndexSize, subindexSize);
		int prefixTotalSize=getSeqTailBuilderPackedSize(&(routingBuilder->prefixBuilder))+prefixHeaderSize;

		s32 oldTagOffset=0;
		u8 *newData=circAlloc(circHeap, prefixTotalSize, sliceTag, sliceIndex, &oldTagOffset);
//		LOG(LOG_INFO,"Writing prefix to %p",newData);

		if(newData==NULL)
			{
			LOG(LOG_CRITICAL,"Failed at alloc after compact: Wanted %i",prefixTotalSize);
			}

		encodeNonRootBlockHeader(1, sliceIndexSize, sliceIndex, subindexSize, subindex, newData);

		u8 *newPrefixData=newData;
		newData+=prefixHeaderSize;

		writeSeqTailBuilderPackedData(&(routingBuilder->prefixBuilder), newData);

		// Hacky due to moving data during alloc

		u8 *rootData=(*(routingBuilder->rootPtr))+getIndirectRootBlockHeaderSize();
		RouteTableSmallRoot *newRoot=(RouteTableSmallRoot *)rootData;

		u8 *oldData=newRoot->prefixData;

		if(oldData!=NULL)
			*oldData&=~ALLOC_HEADER_LIVE_MASK;

		newRoot->prefixData=newPrefixData;
		}

	// Consider re-using?
	if(routingBuilder->upgradedToTree || getSeqTailBuilderDirty(&(routingBuilder->suffixBuilder)))
		{
		int subindex=1;
		int subindexSize=varipackLength(subindex);

		int suffixHeaderSize=getNonRootBlockHeaderSize(sliceIndexSize, subindexSize);
		int suffixTotalSize=getSeqTailBuilderPackedSize(&(routingBuilder->suffixBuilder))+suffixHeaderSize;

		s32 oldTagOffset=0;
		u8 *newData=circAlloc(circHeap, suffixTotalSize, sliceTag, sliceIndex, &oldTagOffset);
//		LOG(LOG_INFO,"Writing suffix to %p",newData);

		if(newData==NULL)
			{
			LOG(LOG_CRITICAL,"Failed at alloc after compact: Wanted %i",suffixTotalSize);
			}

		encodeNonRootBlockHeader(1, sliceIndexSize, sliceIndex, subindexSize, subindex, newData);

		u8 *newSuffixData=newData;
		newData+=suffixHeaderSize;

		writeSeqTailBuilderPackedData(&(routingBuilder->suffixBuilder), newData);

		// Hacky due to moving data during alloc

		u8 *rootData=(*(routingBuilder->rootPtr))+getIndirectRootBlockHeaderSize();
		RouteTableSmallRoot *newRoot=(RouteTableSmallRoot *)rootData;

		u8 *oldData=newRoot->suffixData;

		if(oldData!=NULL)
			*oldData&=~ALLOC_HEADER_LIVE_MASK;

		newRoot->suffixData=newSuffixData;
		}
	//else
//		root->suffixData=suffixBuilder->oldData;

	// Consider re-using?
	if(routingBuilder->upgradedToTree || rttGetRouteTableTreeBuilderDirty(routingBuilder->treeBuilder))
		{
		int subindex=2;
		int subindexSize=varipackLength(subindex);

		int routeTableHeaderSize=getNonRootBlockHeaderSize(sliceIndexSize, subindexSize);
		int routeTableTotalSize=rtaGetRouteTableArrayBuilderPackedSize(routingBuilder->treeBuilder->nestedBuilder)+routeTableHeaderSize;

		s32 oldTagOffset=0;
		u8 *newData=circAlloc(circHeap, routeTableTotalSize, sliceTag, sliceIndex, &oldTagOffset);
//		LOG(LOG_INFO,"Writing routes to %p",newData);

		if(newData==NULL)
			{
			LOG(LOG_CRITICAL,"Failed at alloc after compact: Wanted %i",routeTableTotalSize);
			}

		encodeNonRootBlockHeader(1, sliceIndexSize, sliceIndex, subindexSize, subindex, newData);

		u8 *newRouteData=newData;
		newData+=routeTableHeaderSize;

		rttWriteRouteTableTreeBuilderPackedData(routingBuilder->treeBuilder, newData);

		// Hacky due to moving data during alloc

		u8 *rootData=(*(routingBuilder->rootPtr))+getIndirectRootBlockHeaderSize();
		RouteTableSmallRoot *newRoot=(RouteTableSmallRoot *)rootData;

		u8 *oldData=newRoot->forwardRouteData[0];

		if(oldData!=NULL)
			{
			*oldData&=~ALLOC_HEADER_LIVE_MASK;
			}

		newRoot->forwardRouteData[0]=newRouteData;
		}

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

	RoutingComboBuilder routingBuilder;

	routingBuilder.disp=disp;
	routingBuilder.rootPtr=slice->smerData+sliceIndex;

//	s32 oldHeaderSize=0, oldPrefixDataSize=0, oldSuffixDataSize=0, oldRouteTableDataSize=0;

	if(smerData!=NULL)
		{
		u8 header=*smerData;

		if((header&ALLOC_HEADER_LIVE_DIRECT_MASK)==ALLOC_HEADER_LIVE_DIRECT_VALUE)
			{
			createBuildersFromDirectData(&routingBuilder);
			}
		// ALLOC_HEADER_LIVE_INDIRECT_ROOT_VALUE

		else if((header&ALLOC_HEADER_LIVE_INDIRECT_MASK)==ALLOC_HEADER_LIVE_INDIRECT_ROOT_VALUE)
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

	s32 maxNewPrefix=0;
	s32 maxNewSuffix=0;

	createRoutePatches(rdi, entryCount,
			&(routingBuilder.prefixBuilder), &(routingBuilder.suffixBuilder), forwardPatches, reversePatches,
			&forwardCount, &reverseCount, &maxNewPrefix, &maxNewSuffix);

	if(considerUpgradingToTree(&routingBuilder, forwardCount, reverseCount))
		upgradeToTree(&routingBuilder);

	if(routingBuilder.treeBuilder!=NULL)
		{
		rttMergeRoutes(routingBuilder.treeBuilder, forwardPatches, reversePatches, forwardCount, reverseCount, maxNewPrefix, maxNewSuffix, orderedDispatches, disp);

		writeBuildersAsIndirectData(&routingBuilder, sliceTag, sliceIndex,circHeap);
		}
	else
		{
		rtaMergeRoutes(routingBuilder.arrayBuilder, forwardPatches, reversePatches, forwardCount, reverseCount, maxNewPrefix, maxNewSuffix, orderedDispatches, disp);

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








