#include "common.h"


/*

Alloc Header:

	0 0 0 0 0 0 0 0 : INVALID: Unused region

    1 0 0 0 0 0 0 0 : CHUNK: Start of chunk header (with 1 byte tag)

	1 x x x x x x x : Live block (127 options)
	0 x x x x x x x : Dead block (127 options)

	Summary:
    . 0 0 0 0 g g g: Direct
    . 0 0 1 0 g g g: IndirectTop
	. 0 0 0 1 x i i: Prefix, x=reserved, i=index size
	. 0 0 1 1 x i i: Suffix, x=reserved, i=index size

	. 0 1 0 p s i i: Forward Leaf, p=pointer/data, s=shallow/deep
	. 0 1 1 p s i i: Reverse Leaf, p=pointer/data, s=shallow/deep
	. 1 0 0 p s i i: Forward Branch, p=pointer/data, s=shallow/deep
	. 1 0 1 p s i i: Reverse Branch, p=pointer/data, s=shallow/deep
	. 1 1 0 p s i i: Forward Offset, p=pointer/data, s=shallow/deep
	. 1 1 1 p s i i: Reverse Offset, p=pointer/data, s=shallow/deep

	3 live, 3 dead options

	. 0 0 0 1 0 0 0: Unused
    . 0 0 1 0 0 0 0: Unused
    . 0 0 1 1 0 0 0: Unused

	14 live, 14 dead options

	. 0 0 0 1 g g g: Direct (2 byte header)
    . 0 0 1 1 g g g: Indirect Top ( byte header)				RouteTableTreeTopBlock

	28 live, 28 dead options

    . 0 0 0 0 g g g: Direct
    . 0 0 1 0 g g g: IndirectTop
	. 0 0 0 1 x i i: Prefix, x=reserved, i=index size
	. 0 0 1 1 x i i: Suffix, x=reserved, i=index size

	96 Live, 96 head options
	. 0 1 0 p s i i: Forward Leaf, p=pointer/data, s=shallow/deep
	. 0 1 1 p s i i: Reverse Leaf, p=pointer/data, s=shallow/deep
	. 1 0 0 p s i i: Forward Branch, p=pointer/data, s=shallow/deep
	. 1 0 1 p s i i: Reverse Branch, p=pointer/data, s=shallow/deep
	. 1 1 0 p s i i: Forward Offset, p=pointer/data, s=shallow/deep
	. 1 1 1 p s i i: Reverse Offset, p=pointer/data, s=shallow/deep

	Full Breakdown:

    16 live, 16 dead options

    . 0 0 0 1 0 i i: Prefix Tail block, standard
    . 0 0 0 1 1 i i: Prefix Tail block, big or alternate?
    . 0 0 1 1 0 i i: Suffix Tail block, standard
    . 0 0 1 1 1 i i: Suffix Tail block, big or alternate?

	32 live, 32 dead options


      0 1 0 0 0 i i: Forward Leaf Shallow Ptr					RouteTableTreeArrayBlock
      0 1 0 0 1 i i: Forward Leaf Deep Ptr (1 byte sub)			RouteTableTreeArrayBlock
      0 1 0 1 0 i i: Forward Leaf Shallow Data (1 byte sub)		RouteTableTreeLeafBlock
      0 1 0 1 1 i i: Forward Leaf Deep Data (2 byte sub)		RouteTableTreeLeafBlock

      0 1 1 0 0 i i: Reverse Leaf Shallow Ptr					RouteTableTreeArrayBlock
      0 1 1 0 1 i i: Reverse Leaf Deep Ptr (1 byte sub)			RouteTableTreeArrayBlock
      0 1 1 1 0 i i: Reverse Leaf Shallow Data (1 byte sub)		RouteTableTreeLeafBlock
      0 1 1 1 1 i i: Reverse Leaf Deep Data (2 byte sub)		RouteTableTreeLeafBlock

	32 live, 32 dead options

      1 0 0 0 0 i i: Forward Branch Shallow Ptr					RouteTableTreeArrayBlock
      1 0 0 0 1 i i: Forward Branch Deep Ptr (1 byte sub)		RouteTableTreeArrayBlock
      1 0 0 1 0 i i: Forward Branch Shallow Data (1 byte sub)	RouteTableTreeBranchBlock
      1 0 0 1 1 i i: Forward Branch Deep Data (2 byte sub)		RouteTableTreeBranchBlock

      1 0 1 0 0 i i: Reverse Branch Shallow Ptr					RouteTableTreeArrayBlock
      1 0 1 0 1 i i: Reverse Branch Deep Ptr (1 byte sub)		RouteTableTreeArrayBlock
      1 0 1 1 0 i i: Reverse Branch Shallow Data (1 byte sub)	RouteTableTreeBranchBlock
      1 0 1 1 1 i i: Reverse Branch Deep Data (2 byte sub)		RouteTableTreeBranchBlock

	32 live, 32 dead options

      1 1 0 0 0 i i: Forward Offset Shallow Ptr					RouteTableTreeArrayBlock
      1 1 0 0 1 i i: Forward Offset Deep Ptr (1 byte sub)		RouteTableTreeArrayBlock
      1 1 0 1 0 i i: Forward Offset Shallow Data (1 byte sub)	RouteTableTreeOffsetBlock
      1 1 0 1 1 i i: Forward Offset Deep Data (2 byte sub)		RouteTableTreeOffsetBlock

      1 1 1 0 0 i i: Reverse Offset Shallow Ptr					RouteTableTreeArrayBlock
      1 1 1 0 1 i i: Reverse Offset Deep Ptr (1 byte sub)		RouteTableTreeArrayBlock
      1 1 1 1 0 i i: Reverse Offset Shallow Data (1 byte sub)	RouteTableTreeOffsetBlock
      1 1 1 1 1 i i: Reverse Offset Deep Data (2 byte sub)		RouteTableTreeOffsetBlock


 Tree Related

 	. 0 1 Leaf
	. 1 0 Branch
	. 1 1 Offset
          x 		Fwd/Rev
	        x 	Pointer/Data
	          x   	Shallow/Deep
	            i i



	Direct and Indirect root blocks encode a relative index offset vs the last such block

	Gap exponent 1: 0-255 gap (with value): Exact
	Gap exponent 2: 256-511 gap (with value+256): Exact
    Gap exponent 3: 0xD8 Indirect root with 512-1023 gap (with value*2+512): 0-1
	Gap exponent 4: 0xE0 Indirect root with 1024-2047 gap (with value*4+1024): 0-3
	Gap exponent 5: 0xE8 Indirect root with 2048-4095 gap (with value*8+2048): 0-7
	Gap exponent 6: 0xF0 Indirect root with 4096-8191 gap (with value*16+4096): 0-15
    Gap exponent 7: 0xF8 Indirect root with 8192-16383 gap (with value*32+8192): 0-31


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

// Check Gap-coded											// . 0 0 -  0 g g g: g=gap exponent(1+)
#define ALLOC_HEADER_GAP_MASK 0x68	 						// - ? ? -  ? - - -
#define ALLOC_HEADER_GAP_VALUE 0x00							// - 0 0 -  0 - - -

#define ALLOC_HEADER_GAP_ROOT_MASK 0x78	 					// - ? ? ?  ? - - -
#define ALLOC_HEADER_GAP_ROOT_DIRECT_VALUE 0x00				// - 0 0 0  0 - - -
#define ALLOC_HEADER_GAP_ROOT_TOP_VALUE 0x10				// - 0 0 1  0 - - -

// Check Gap-coded & live
#define ALLOC_HEADER_LIVE_GAP_MASK 0xE8						// ? ? ? -  ? - - -
#define ALLOC_HEADER_LIVE_GAP_ROOT_VALUE 0x80				// 1 0 0 -  0 - - -

#define ALLOC_HEADER_LIVE_GAP_ROOT_MASK 0xF8	 			// ? ? ? ?  ? - - -
#define ALLOC_HEADER_LIVE_GAP_ROOT_DIRECT_VALUE 0x80		// 1 0 0 0  0 - - -
#define ALLOC_HEADER_LIVE_GAP_ROOT_TOP_VALUE 0x90			// 1 0 0 1  0 - - -


// Check Block & live

#define ALLOC_HEADER_BLOCK_MASK	0x60						// - ? ? -  - - - -
#define ALLOC_HEADER_BLOCK_TAIL 0x00						// - 0 0 -  - - - -
#define ALLOC_HEADER_BLOCK_LEAF 0x20						// - 0 1 -  - - - -
#define ALLOC_HEADER_BLOCK_BRANCH 0x40						// - 1 0 -  - - - -
#define ALLOC_HEADER_BLOCK_OFFSET 0x60						// - 1 1 -  - - - -



//#define ALLOC_HEADER_BLOCK_MASK	0x70						// - ? ? ?  - - - -
//#define ALLOC_HEADER_BLOCK_PREFIX 0x00						// - 0 0 0  - - - -
//#define ALLOC_HEADER_BLOCK_SUFFIX 0x10						// - 0 0 1  - - - -
//#define ALLOC_HEADER_BLOCK_FWDLEAF 0x20						// - 0 1 0  - - - -
//#define ALLOC_HEADER_BLOCK_REVLEAF 0x30						// - 0 1 1  - - - -
//#define ALLOC_HEADER_BLOCK_FWDBRANCH 0x40					// - 1 0 0  - - - -
//#define ALLOC_HEADER_BLOCK_REVBRANCH 0x50					// - 1 0 1  - - - -
//#define ALLOC_HEADER_BLOCK_FWDOFFSET 0x60					// - 1 1 0  - - - -
//#define ALLOC_HEADER_BLOCK_REVOFFSET 0x70					// - 1 1 1  - - - -

//#define ALLOC_HEADER_LIVE_BLOCK_MASK 0xF0					// ? ? ? ?  - - - -
//#define ALLOC_HEADER_LIVE_BLOCK_PREFIX 0x80					// 1 0 0 0  - - - -
//#define ALLOC_HEADER_LIVE_BLOCK_SUFFIX 0x90					// 1 0 0 1  - - - -
//#define ALLOC_HEADER_LIVE_BLOCK_FWDLEAF 0xA0				// 1 0 1 0  - - - -
//#define ALLOC_HEADER_LIVE_BLOCK_REVLEAF 0xB0				// 1 0 1 1  - - - -
//#define ALLOC_HEADER_LIVE_BLOCK_FWDBRANCH 0xC0				// 1 1 0 0  - - - -
//#define ALLOC_HEADER_LIVE_BLOCK_REVBRANCH 0xD0				// 1 1 0 1  - - - -
//#define ALLOC_HEADER_LIVE_BLOCK_FWDOFFSET 0xE0				// 1 1 1 0  - - - -
//#define ALLOC_HEADER_LIVE_BLOCK_REVOFFSET 0xF0				// 1 1 1 1  - - - -

// Extaction masks

#define ALLOC_HEADER_GAPSIZE_MASK 0x07				        // - - - -  - ? ? ?
#define ALLOC_HEADER_TAIL_MASK 0x10							// - - - ?  - - - -
#define ALLOC_HEADER_INDEX_MASK 0x03						// - - - -  - - ? ?

#define ALLOC_HEADER_ARRAYFMT_MASK 0x0C				        // - - - -  ? ? - -
#define ALLOC_HEADER_ARRAYFMT_SHALLOWPTR_VALUE 0x00       	// - - - -  0 0 - -
#define ALLOC_HEADER_ARRAYFMT_DEEPPTR_VALUE 0x04			// - - - -  0 1 - -
#define ALLOC_HEADER_ARRAYFMT_SHALLOWDATA_VALUE 0x08		// - - - -  1 0 - -
#define ALLOC_HEADER_ARRAYFMT_DEEPDATA_VALUE 0x0C			// - - - -  1 1 - -

#define ALLOC_HEADER_INDEX_MASK 0x03				        // - - - -  - - ? ?

// Initialization values

#define ALLOC_HEADER_LIVE_DIRECT_GAPSMALL 0x81				// 1 0 0 0  0 0 0 1
#define ALLOC_HEADER_LIVE_TOP_GAPSMALL 0x91					// 1 0 0 1  0 0 0 1

#define ALLOC_HEADER_LIVE_TAIL		   0x88					// 1 0 0 0  1 0 0 0
#define ALLOC_HEADER_LIVE_PREFIX	   0x88					// 1 0 0 0  1 0 0 0
#define ALLOC_HEADER_LIVE_SUFFIX	   0x98					// 1 0 0 1  1 0 0 0

#define ALLOC_HEADER_LIVE_ARRAY		   0x80					// 1 0 0 0  0 0 0 0

//#define ALLOC_HEADER_INTOP_VALUE 0x20			            // - 0 1 -  - - - -

//#define ALLOC_HEADER_LIVE_DIRECT_MASK 0xC0					// ? ? ? -  - - - -
//#define ALLOC_HEADER_LIVE_DIRECT_VALUE 0x80					// 1 0 0 -  - - - -
//#define ALLOC_HEADER_LIVE_INDIRECT_TOP_VALUE 0x80			// 1 0 0 -  - - - -

//#define ALLOC_HEADER_GAPSIZE_MASK 0x07				        // - - - -  - ? ? ?
//#define ALLOC_HEADER_LIVE_DIRECT_GAPSMALL 0x81              // 1 0 0 0  0 0 0 1
//#define ALLOC_HEADER_LIVE_INTOP_GAPSMALL 0x81               // 1 0 1 0  0 0 0 1

// Check type of Indirect Member (not top)

//#define ALLOC_HEADER_INDIRECT_MASK  0x70   			  	    // - ? ? ?  - - - -
//#define ALLOC_HEADER_INDIRECT_TAIL_VALUE 0x70    			// - 1 0 0  - - - -
//#define ALLOC_HEADER_INDIRECT_LEAF_VALUE 0x70    			// - 1 0 1  - - - -
//#define ALLOC_HEADER_INDIRECT_BRANCH_VALUE 0x70    			// - 1 1 0  - - - -
//#define ALLOC_HEADER_INDIRECT_TOP_VALUE 0x70    			// - 1 1 1  - - - -

//. 1 1 1 1 g g g


//#define ALLOC_HEADER_LIVE_INDIRECT_TOP_MASK  0xF0     		// ? ? ? ?  - - - -
//#define ALLOC_HEADER_LIVE_INDIRECT_TOP_VALUE 0xF0    		// 1 1 1 1  - - - -

//#define ALLOC_HEADER_INDIRECT_TOP_GAPSIZE_MASK 0x07  		// - - - -  - ? ? ?
//#define ALLOC_HEADER_LIVE_INDIRECT_ROOT_GAPSMALL 0xC1   	// 1 1 0 0  0 0 0 1

// Check type of NonRoot







//#define ALLOC_HEADER_NONROOT_MASK 0x61						// - ? ? -  - - - ?
//#define ALLOC_HEADER_NONROOT_BRANCH 0x41					// - 1 0 -  - - - 1
//#define ALLOC_HEADER_NONROOT_LEAF 0x61						// - 1 1 -  - - - 1

//#define ALLOC_HEADER_LIVE_NONROOT_MASK 0xE1					// ? ? ? -  - - - ?
//#define ALLOC_HEADER_LIVE_NONROOT_BRANCH 0xC1				// 1 1 0 -  - - - 1
//#define ALLOC_HEADER_LIVE_NONROOT_LEAF 0xE1					// 1 1 1 -  - - - 1


//	. 1 0 0 0 i i x : Prefix Tail block, i=index size
//  . 1 0 0 1 i i x : Suffix Tail block, i=index size
//	. 1 0 1 0 i i s : Forward Branch block, i=index size, s=subindexed block (2nd level indirect array)
//  . 1 0 1 0 i i s : Reverse Branch block, i=index size, s=subindexed block (2nd level indirect array)
//  . 1 1 0 1 i i s : Forward Leaf block, i=index size, s=subindexed block (2nd level indirect array)
//  . 1 1 0 1 i i s : Reverse Leaf block, i=index size, s=subindexed block (2nd level indirect array)



static void dumpTagData(u8 **tagData, s32 tagDataLength)
{
	for(int i=0;i<tagDataLength;i++)
		{
		LOG(LOG_INFO,"Tag %i %p",i,tagData[i]);
		}
}

//static
int encodeGapDirectBlockHeader(s32 tagOffsetDiff, u8 *data)
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

		u8 data1=ALLOC_HEADER_LIVE_DIRECT_GAPSMALL + (exp);

		*data++=data1;
		*data=man;

		//LOG(LOG_INFO,"Wrote other Header: %02x %02x for %i (exp %i)",data1,man,tagOffsetDiff,exp);
		}

	return 2;
}


//static
int encodeGapTopBlockHeader(s32 tagOffsetDiff, u8 *data)
{
	if(tagOffsetDiff<0)
		{
		LOG(LOG_CRITICAL,"Expected positive tagOffsetDiff indirect: %i",tagOffsetDiff);
		}

	if(tagOffsetDiff<256)
		{
		*data++=ALLOC_HEADER_LIVE_TOP_GAPSMALL;
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

		u8 data1=ALLOC_HEADER_LIVE_TOP_GAPSMALL+exp;

		*data++=data1;
		*data=man;

		//LOG(LOG_INFO,"Wrote other Header: %02x %02x for %i (exp %i)",data1,man,tagOffsetDiff,exp);
		}

	return 2;
}

//static
int decodeGapBlockHeader(u8 *data, s32 *tagOffsetDiffPtr)
{
	u8 header=*data++;

	if(tagOffsetDiffPtr!=NULL)
		{
		int exp=(header&ALLOC_HEADER_GAPSIZE_MASK);

		u32 res=*data;

		if(exp>1)
			res=(256+res)<<(exp-2);

		*tagOffsetDiffPtr=res;
		}

	return 2;
}

//static
s32 getGapBlockHeaderSize()
{
	return 2;
}



s32 rtEncodeTailBlockHeader(u32 prefixSuffix, u32 indexSize, u32 index, u8 *data)
{
	LOG(LOG_INFO,"rtEncodeTailBlockHeader: %i Index: %i %i -> %p",prefixSuffix, indexSize, index,data);

	//ALLOC_HEADER_LIVE_TAIL

	*data++=(prefixSuffix?ALLOC_HEADER_LIVE_SUFFIX:ALLOC_HEADER_LIVE_PREFIX)+(indexSize-1);
	varipackEncode(index, data);

	return 1+indexSize;
}

s32 rtDecodeTailBlockHeader(u8 *data, u32 *prefixSuffixPtr, u32 *indexSizePtr, u32 *indexPtr)
{
	u8 header=*data++;

	u32 prefixSuffix=(header&ALLOC_HEADER_TAIL_MASK)!=0;

	if(prefixSuffixPtr!=NULL)
		*prefixSuffixPtr=prefixSuffix;

	u32 indexSize=1+(header&0x3);

	if(indexSizePtr!=NULL)
		*indexSizePtr=indexSize;

	u32 index=varipackDecode(indexSize, data);
	if(indexPtr!=NULL)
		*indexPtr=index;

	LOG(LOG_INFO,"rtDecodeTailBlockHeader: %i Index: %i %i <- %p (%02x)",prefixSuffix, indexSize, index, (data-1), header);

	return 1+indexSize;
}

s32 rtGetTailBlockHeaderSize(int indexSize)
{
	LOG(LOG_INFO,"rtGetTailBlockHeader IndexSize: %i",indexSize);

	return 1+indexSize;
}


s32 rtEncodeArrayBlockHeader(u32 arrayNum, u32 arrayType, u32 indexSize, u32 index, u32 subindex, u8 *data)
{
	if(subindex==256)
		LOG(LOG_CRITICAL,"rtEncodeArrayBlockHeader with subindex 256");

	//LOG(LOG_INFO,"Encoding Leaf: %i Index: %i %i Subindex: %i %i", isLeaf, indexSize, index, subindexSize, subindex);

	*data++=(ALLOC_HEADER_LIVE_ARRAY+(arrayNum<<4)+(arrayType<<2)+(indexSize-1));
	data+=varipackEncode(index, data);

	switch(arrayType)
	{
		case 1:
		case 2:
			*data=subindex;
			return 2+indexSize;

		case 3:
			*((u16 *)data)=subindex;
			return 3+indexSize;
	}

	return 1+indexSize;
}


// u32 arrayNum, u32 arrayType, u32 indexSize, u32 index, u32 subindex
s32 rtDecodeArrayBlockHeader(u8 *data, u32 *arrayNumPtr, u32 *arrayTypePtr, u32 *indexSizePtr, u32 *indexPtr, u32 *subindexSizePtr, u32 *subindexPtr)
{
	u8 header=(*data++)&(~ALLOC_HEADER_LIVE_ARRAY);

	u32 indexSize=1+(header&0x3);
	u32 arrayType=(header>>2)&0x3;
	u32 arrayNum=(header>>4)&0x7;

	if(arrayNumPtr!=NULL)
		*arrayNumPtr=arrayNum;

	if(arrayTypePtr!=NULL)
		*arrayTypePtr=arrayType;

	if(indexSizePtr!=NULL)
		*indexSizePtr=indexSize;

	u32 index=varipackDecode(indexSize, data);
	if(indexPtr!=NULL)
		*indexPtr=index;

	data+=indexSize;

	u32 subindexSize=0, subindex=-1;

	switch(arrayType)
		{
		case 1:
		case 2:
			subindexSize=1;
			subindex=*data;
			break;

		case 3:
			subindexSize=2;
			subindex=*((u16 *)data);
		}

	if(subindexSizePtr!=NULL)
		*subindexSizePtr=subindexSize;

	if(subindexPtr!=NULL)
		*subindexPtr=subindex;

	return 1+indexSize+subindexSize;
}


s32 rtGetArrayBlockHeaderSize(int indexSize, int subindexSize)
{
	return 1+indexSize+subindexSize;
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
	LOG(LOG_INFO,"Reclaim Index: %p %li %x %p",heapDataPtr,targetAmount,tag,disp);

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
		LOG(LOG_INFO,"rtReclaimIndexer %p %02x",heapDataPtr,header);

		if(header==CH_HEADER_CHUNK || header==CH_HEADER_INVALID)
			{
		//	LOG(LOG_INFO,"Header: %p %2x %i",data,header);
			break;
			}

		if((header & ALLOC_HEADER_GAP_MASK) == ALLOC_HEADER_GAP_VALUE) // Gap encoded: Direct or top
			{
			if((header & ALLOC_HEADER_GAP_ROOT_MASK) == ALLOC_HEADER_GAP_ROOT_DIRECT_VALUE) // Direct
				{
				s32 chunkTagOffsetDiff=0;
				decodeGapBlockHeader(heapDataPtr, &chunkTagOffsetDiff);

				currentIndex+=chunkTagOffsetDiff;
				u8 *scanPtr=heapDataPtr+getGapBlockHeaderSize();

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
					index->entries[entry].topindex=ROUTE_TOPINDEX_DIRECT;
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
			else // Top
				{
				s32 chunkTagOffsetDiff=0;
				decodeGapBlockHeader(heapDataPtr, &chunkTagOffsetDiff);

				currentIndex+=chunkTagOffsetDiff;
				s32 size=getGapBlockHeaderSize()+sizeof(RouteTableTreeTopBlock);

				u8 *scanPtr=heapDataPtr+size;

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
					index->entries[entry].topindex=ROUTE_TOPINDEX_TOP;
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
			}
		else // Indirect: Tail or Array
			{

			if((header & ALLOC_HEADER_BLOCK_MASK)==ALLOC_HEADER_BLOCK_TAIL) // Tail: Prefix or Suffix
				{
				u32 prefixSuffix=0;
				u32 smerIndex=0;

				s32 headerSize=rtDecodeTailBlockHeader(heapDataPtr, &prefixSuffix, NULL, &smerIndex);
				u8 *scanPtr=heapDataPtr+headerSize;

				scanPtr=scanTails(scanPtr);
				u32 size=scanPtr-heapDataPtr;

				if(header & ALLOC_HEADER_LIVE_MASK)
					{
					if(entry==index->entryAlloc)
						index=allocOrExpandIndexer(index, disp);

					index->entries[entry].index=smerIndex;
					index->entries[entry].topindex=prefixSuffix;
					index->entries[entry].subindex=-1;
					index->entries[entry].size=size;
					entry++;

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
			else // Array or Array Entry: Leaf, Branch or Offset
				{
				u32 arrayNum=0,arrayType=0;
				u32 smerIndex=0;
				u32 subindex=-1;

				s32 headerSize=rtDecodeArrayBlockHeader(heapDataPtr, &arrayNum, &arrayType, NULL, &smerIndex, NULL, &subindex);
				u8 *scanPtr=heapDataPtr+headerSize;

				LOG(LOG_INFO,"Decoded Array: %i %i %i %i",arrayNum,arrayType,smerIndex,subindex);

				u32 dataSize=0;

				switch(arrayType)
					{
					case ARRAY_TYPE_SHALLOW_PTR: // Shallow Ptr (no subindex)
						dataSize=sizeof(RouteTableTreeArrayBlock)+sizeof(u8 *)*((RouteTableTreeArrayBlock *)scanPtr)->dataAlloc;

						LOG(LOG_INFO,"Shallow Array %p %i",heapDataPtr, dataSize);
						break;

					case ARRAY_TYPE_DEEP_PTR: // Deep Ptr (1-byte subindex)
						LOG(LOG_CRITICAL,"Deep Array: Not implemented");
						break;

					case ARRAY_TYPE_SHALLOW_DATA: // Shallow Data Ptr (1-byte subindex)
						switch(arrayNum)
							{
							case ROUTE_TOPINDEX_FORWARD_LEAF:
							case ROUTE_TOPINDEX_REVERSE_LEAF:
								dataSize=sizeof(RouteTableTreeLeafBlock)+sizeof(RouteTableTreeLeafEntry)*((RouteTableTreeLeafBlock *)scanPtr)->entryAlloc;
								LOG(LOG_INFO,"Shallow Leaf Data %p %i",heapDataPtr, dataSize);
								break;

							case ROUTE_TOPINDEX_FORWARD_BRANCH:
							case ROUTE_TOPINDEX_REVERSE_BRANCH:
								dataSize=sizeof(RouteTableTreeBranchBlock)+sizeof(s16)*((RouteTableTreeBranchBlock *)scanPtr)->childAlloc;
								LOG(LOG_INFO,"Shallow Branch Data %p %i",heapDataPtr, dataSize);
								break;

							case ROUTE_TOPINDEX_FORWARD_OFFSET:
							case ROUTE_TOPINDEX_REVERSE_OFFSET:
							default:
								LOG(LOG_CRITICAL,"ShallowData with ArrayNum %i: Not implemented",arrayNum);
							}
						break;

					case ARRAY_TYPE_DEEP_DATA: // Deep Ptr (2-byte subindex)
						LOG(LOG_CRITICAL,"Deep Data: Not implemented");
						break;
					}

				scanPtr+=dataSize;
				u32 size=scanPtr-heapDataPtr;

				if(header & ALLOC_HEADER_LIVE_MASK)
					{
					if(entry==index->entryAlloc)
						index=allocOrExpandIndexer(index, disp);

					index->entries[entry].index=smerIndex;
					index->entries[entry].topindex=arrayNum;
					index->entries[entry].subindex=subindex;
					index->entries[entry].size=size;
					entry++;

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
		s32 topindex=index->entries[i].topindex;

		if(topindex<0) // Gap encoded: tag offset needs updating
			{
			int blockHeaderSize=0;

			s32 diff=sindex-prevOffset;
			blockHeaderSize=getGapBlockHeaderSize();

			if(topindex==ROUTE_TOPINDEX_DIRECT) // Direct
				encodeGapDirectBlockHeader(diff, newChunk);
			else	// Top
				encodeGapTopBlockHeader(diff, newChunk);

			if(size<blockHeaderSize)
				{
				LOG(LOG_CRITICAL,"Undersize block %i, referenced by %p", size, *primaryPtr);
				}

			prevOffset=sindex;
			memmove(newChunk+blockHeaderSize,(*primaryPtr)+blockHeaderSize,size-blockHeaderSize);

			*primaryPtr=newChunk;
			newChunk+=size;
			}
		else // Tail, Leaf, Branch or Offset
			{
			//LOG(LOG_INFO,"Leaf/Branch %i %i %i",size,sindex,subindex);

			//int headerSize=de

			RouteTableTreeTopBlock *topPtr=(RouteTableTreeTopBlock *)((*primaryPtr)+getGapBlockHeaderSize());

			//u8 *dataPtr=NULL;
			s32 subindex=index->entries[i].subindex;

			if(subindex==-1)
				{
				LOG(LOG_INFO,"Transfer %i bytes from %p to %p",size,topPtr->data[topindex],newChunk);

				memmove(newChunk,topPtr->data[topindex],size);
				topPtr->data[topindex]=newChunk;
				newChunk+=size;
				}
			else
				{
				u8 *arrayBlockPtr=topPtr->data[topindex];
				s32 headerSize=rtDecodeArrayBlockHeader(arrayBlockPtr,NULL,NULL,NULL,NULL,NULL,NULL);
				RouteTableTreeArrayBlock *array=(RouteTableTreeArrayBlock *)(arrayBlockPtr+headerSize);

				LOG(LOG_INFO,"Transfer %i bytes from %p to %p",size,array->data[subindex],newChunk);

				memmove(newChunk,array->data[subindex],size);
				array->data[subindex]=newChunk;
				newChunk+=size;
				}

//			LOG(LOG_INFO,"Moving indirect leaf to %p, ref by %p",newChunk,*primaryPtr);

//			LOG(LOG_INFO,"Moving %i bytes from %p to %p",size,dataPtr,newChunk);

//			memmove(newChunk,dataPtr,size);
//			newChunk+=size;
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

	s32 headerSize=getGapBlockHeaderSize();
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
	LOG(LOG_INFO,"Begin parse indirect");

	u8 *data=*(builder->rootPtr);

	s32 headerSize=getGapBlockHeaderSize();

	builder->arrayBuilder=NULL;
	builder->combinedDataBlock.headerSize=0;
	builder->combinedDataBlock.dataSize=0;
	builder->combinedDataPtr=NULL;

	builder->treeBuilder=dAlloc(builder->disp, sizeof(RouteTableTreeBuilder));
	builder->topDataBlock.headerSize=headerSize;
	builder->topDataBlock.dataSize=sizeof(RouteTableTreeTopBlock);
	builder->topDataPtr=data;

	builder->upgradedToTree=0;
	//HeapDataBlock dataBlocks[8];

	RouteTableTreeBuilder *treeBuilder=builder->treeBuilder;

	treeBuilder->disp=builder->disp;
	treeBuilder->newEntryCount=0;

	//treeBuilder->topDataBlock->dataSize=data;

	RouteTableTreeTopBlock *top=(RouteTableTreeTopBlock *)(data+headerSize);

	for(int i=0;i<ROUTE_TOPINDEX_MAX;i++)
		{
		if(top->data[i]!=NULL && ((*(top->data[i]))&ALLOC_HEADER_LIVE_MASK)==0x0)
			{
			LOG(LOG_CRITICAL,"Entry references dead block");
			}
		}

	LOG(LOG_INFO,"Parse Indirect: Prefix");
	u8 *prefixBlockData=top->data[ROUTE_TOPINDEX_PREFIX];

	if(prefixBlockData!=NULL)
		{
		treeBuilder->dataBlocks[ROUTE_TOPINDEX_PREFIX].headerSize=rtDecodeTailBlockHeader(prefixBlockData, NULL, NULL, NULL);
		u8 *prefixData=prefixBlockData+headerSize;
		u8 *prefixDataEnd=initSeqTailBuilder(&(builder->prefixBuilder), prefixData, builder->disp);
		treeBuilder->dataBlocks[ROUTE_TOPINDEX_PREFIX].dataSize=prefixDataEnd-prefixData;
		}
	else
		{
		LOG(LOG_INFO,"Null prefix");
		}

	LOG(LOG_INFO,"Parse Indirect: Suffix");
	u8 *suffixBlockData=top->data[ROUTE_TOPINDEX_SUFFIX];

	if(suffixBlockData!=NULL)
		{
		treeBuilder->dataBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize=rtDecodeTailBlockHeader(suffixBlockData, NULL, NULL, NULL);
		u8 *suffixData=suffixBlockData+headerSize;
		u8 *suffixDataEnd=initSeqTailBuilder(&(builder->suffixBuilder), suffixData, builder->disp);
		treeBuilder->dataBlocks[ROUTE_TOPINDEX_SUFFIX].dataSize=suffixDataEnd-suffixData;
		}
	else
		{
		LOG(LOG_INFO,"Null suffix");
		}


	//treeBuilder->forwardProxy

	LOG(LOG_INFO,"Parse Indirect: Arrays");

	for(int i=ROUTE_TOPINDEX_FORWARD_LEAF; i<ROUTE_TOPINDEX_FORWARD_OFFSET;i++)
		{
		u8 *arrayBlockData=top->data[i];

		if(arrayBlockData!=NULL)
			{
			treeBuilder->dataBlocks[i].headerSize=rtDecodeArrayBlockHeader(arrayBlockData, NULL, NULL, NULL, NULL, NULL, NULL);
			}
		else
			{
			LOG(LOG_INFO,"Null array");
			}
		}

	treeBuilder->dataBlocks[ROUTE_TOPINDEX_FORWARD_OFFSET].headerSize=0;
	treeBuilder->dataBlocks[ROUTE_TOPINDEX_FORWARD_OFFSET].dataSize=0;
	treeBuilder->dataBlocks[ROUTE_TOPINDEX_REVERSE_OFFSET].headerSize=0;
	treeBuilder->dataBlocks[ROUTE_TOPINDEX_REVERSE_OFFSET].dataSize=0;

	LOG(LOG_INFO,"Parse Indirect: Building");

	rttInitRouteTableTreeBuilder(treeBuilder, top);


	LOG(LOG_INFO,"Parse Indirect: Building complete %i %i %i %i",
			treeBuilder->forwardProxy.leafArrayProxy.dataAlloc,
			treeBuilder->forwardProxy.branchArrayProxy.dataAlloc,
			treeBuilder->reverseProxy.leafArrayProxy.dataAlloc,
			treeBuilder->reverseProxy.branchArrayProxy.dataAlloc);


	//for(int i=0;i<treeBuilder->forwardProxy.leafArrayProxy.dataCount;i++)
//		LOG(LOG_INFO,"Forward Leaf: DataPtr %p",treeBuilder->forwardProxy.leafArrayProxy.dataBlock->data[i]);



	rttDumpRoutingTable(treeBuilder);


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

	int totalSize=getGapBlockHeaderSize()+prefixPackedSize+suffixPackedSize+routeTablePackedSize;

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

	encodeGapDirectBlockHeader(diff, newData);

	newData+=getGapBlockHeaderSize();
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
			RouteTableTreeBranchBlock *oldBranchData=(RouteTableTreeBranchBlock *)(arrayProxy->dataBlock->data[i]);
			oldBranchChildAlloc=oldBranchData->childAlloc;
			}

		if(arrayProxy->newData[i]!=NULL)
			{
			RouteTableTreeBranchBlock *newBranchData=(RouteTableTreeBranchBlock *)(arrayProxy->newData[i]);

			if(newBranchData->childAlloc!=oldBranchChildAlloc)
				totalSize+=rtGetArrayBlockHeaderSize(indexSize,1)+sizeof(RouteTableTreeBranchBlock)+sizeof(s16)*newBranchData->childAlloc;
			}
		}

	return totalSize;
}


void mergeTopArrayUpdates_branch(RouteTableTreeArrayProxy *arrayProxy, int arrayNum, int indexSize, int index, u8 *newData, s32 newDataSize)
{
	if(arrayProxy->newData==NULL)
		return ;

//	if(arrayProxy->dataBlock->dataAlloc==0)
//		{
//		LOG(LOG_INFO,"New Branch Array - setting array length to %i",arrayProxy->newDataAlloc);
		arrayProxy->dataBlock->dataAlloc=arrayProxy->newDataAlloc;
//		}




	for(int i=0;i<arrayProxy->newDataAlloc;i++)
		{
		RouteTableTreeBranchBlock *oldBranchData=NULL;
		int oldBranchChildAlloc=0;

		if(arrayProxy->dataBlock!=NULL && i<arrayProxy->dataBlock->dataAlloc && arrayProxy->dataBlock->data[i]!=NULL)
			{
			oldBranchData=(RouteTableTreeBranchBlock *)(arrayProxy->dataBlock->data[i]);
			oldBranchChildAlloc=oldBranchData->childAlloc;
			}

		if(arrayProxy->newData[i]!=NULL)
			{
			RouteTableTreeBranchBlock *newBranchData=(RouteTableTreeBranchBlock *)(arrayProxy->newData[i]);

			if(newBranchData->childAlloc!=oldBranchChildAlloc)
				{
				arrayProxy->dataBlock->data[i]=newData;

				s32 headerSize=rtEncodeArrayBlockHeader(arrayNum, ARRAY_TYPE_SHALLOW_DATA, indexSize, index, i, newData);
				newData+=headerSize;

				s32 dataSize=sizeof(RouteTableTreeBranchBlock)+sizeof(s16)*(newBranchData->childAlloc);
				memcpy(newData, newBranchData, dataSize);

				LOG(LOG_INFO,"Copying %i bytes of branch data from %p to %p",dataSize,newBranchData,newData);

				newData+=dataSize;


				}
			else
				{
				s32 headerSize=rtGetArrayBlockHeaderSize(indexSize,1);
				s32 dataSize=sizeof(RouteTableTreeBranchBlock)+sizeof(s16)*(newBranchData->childAlloc);
				memcpy(arrayProxy->dataBlock->data[i]+headerSize, newBranchData, dataSize);
				newData+=dataSize;
				}
			}
		}
}




s32 mergeTopArrayUpdates_leaf_accumulateSize(RouteTableTreeArrayProxy *arrayProxy, int indexSize)
{
	if(arrayProxy->newData==NULL)
		{
		return 0;
		}

	s32 totalSize=0;

	LOG(LOG_INFO,"New data Alloc %i",arrayProxy->newDataAlloc);

	for(int i=0;i<arrayProxy->newDataAlloc;i++)
		{
		int oldLeafEntryAlloc=0;

		if(arrayProxy->dataBlock!=NULL && i<arrayProxy->dataBlock->dataAlloc && arrayProxy->dataBlock->data[i]!=NULL)
			{
			RouteTableTreeLeafBlock *oldLeafData=(RouteTableTreeLeafBlock *)(arrayProxy->dataBlock->data[i]);
			oldLeafEntryAlloc=oldLeafData->entryAlloc;
			}

		if(arrayProxy->newData[i]!=NULL)
			{
			RouteTableTreeLeafBlock *newLeafData=(RouteTableTreeLeafBlock *)(arrayProxy->newData[i]);

			if(newLeafData->entryAlloc!=oldLeafEntryAlloc)
				totalSize+=rtGetArrayBlockHeaderSize(indexSize,1)+sizeof(RouteTableTreeLeafBlock)+sizeof(RouteTableTreeLeafEntry)*newLeafData->entryAlloc;
			}
		}

	return totalSize;
}


void mergeTopArrayUpdates_leaf(RouteTableTreeArrayProxy *arrayProxy, int arrayNum, int indexSize, int index, u8 *newData, s32 newDataSize)
{
	if(arrayProxy->newData==NULL)
		return ;

	//if(arrayProxy->dataBlock->dataAlloc==0)
		//{
		//LOG(LOG_INFO,"New Leaf Array - setting array length to %i",arrayProxy->newDataAlloc);

	arrayProxy->dataBlock->dataAlloc=arrayProxy->newDataAlloc;

		//}

	for(int i=0;i<arrayProxy->newDataAlloc;i++)
		{
		RouteTableTreeLeafBlock *oldLeafData=NULL;
		int oldLeafEntryAlloc=0;

		if(arrayProxy->dataBlock!=NULL && i<arrayProxy->dataBlock->dataAlloc && arrayProxy->dataBlock->data[i]!=NULL)
			{
			oldLeafData=(RouteTableTreeLeafBlock *)(arrayProxy->dataBlock->data[i]);
			oldLeafEntryAlloc=oldLeafData->entryAlloc;
			}

		if(arrayProxy->newData[i]!=NULL)
			{
			RouteTableTreeLeafBlock *newLeafData=(RouteTableTreeLeafBlock *)(arrayProxy->newData[i]);

			if(newLeafData->entryAlloc!=oldLeafEntryAlloc)
				{
				arrayProxy->dataBlock->data[i]=newData;

				s32 headerSize=rtEncodeArrayBlockHeader(arrayNum, ARRAY_TYPE_SHALLOW_DATA, indexSize, index, i, newData);
				newData+=headerSize;

				s32 dataSize=sizeof(RouteTableTreeLeafBlock)+sizeof(RouteTableTreeLeafEntry)*(newLeafData->entryAlloc);
				memcpy(newData, newLeafData, dataSize);
				newData+=dataSize;
				}
			else
				{
				s32 headerSize=rtGetArrayBlockHeaderSize(indexSize,1);
				s32 dataSize=sizeof(RouteTableTreeLeafBlock)+sizeof(RouteTableTreeLeafEntry)*(newLeafData->entryAlloc);
				memcpy(arrayProxy->dataBlock->data[i]+headerSize, newLeafData, dataSize);
				newData+=dataSize;
				}
			}
		}
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

	rttDumpRoutingTable(routingBuilder->treeBuilder);

	if(routingBuilder->upgradedToTree)
		{
		u8 *oldData=*(routingBuilder->rootPtr);

		if(oldData!=NULL)
			*oldData&=~ALLOC_HEADER_LIVE_MASK; // Mark array block as dead
		}

	s16 indexSize=varipackLength(sliceIndex);
	RouteTableTreeBuilder *treeBuilder=routingBuilder->treeBuilder;

	LOG(LOG_INFO,"Writing as indirect data (index size is %i)",indexSize);

	RouteTableTreeTopBlock *topPtr=NULL;

	if(routingBuilder->topDataPtr==NULL)
		{
		routingBuilder->topDataBlock.dataSize=sizeof(RouteTableTreeTopBlock);
		routingBuilder->topDataBlock.headerSize=getGapBlockHeaderSize();

		s32 totalSize=routingBuilder->topDataBlock.dataSize+routingBuilder->topDataBlock.headerSize;

		s32 oldTagOffset=0;
		u8 *newTopData=circAlloc(circHeap, totalSize, sliceTag, sliceIndex, &oldTagOffset);

		s32 diff=sliceIndex-oldTagOffset;

		encodeGapTopBlockHeader(diff, newTopData);

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
		LOG(LOG_INFO,"Array %i had %i, now has %i",i,arrayProxy->dataCount,arrayProxy->newDataCount);

		neededBlocks[i].dataSize=rttGetTopArraySize(arrayProxy);
		}

	int totalNeededSize=0;

	for(int i=0;i<ROUTE_TOPINDEX_MAX;i++)
		{
		s32 existingSize=treeBuilder->dataBlocks[i].headerSize+treeBuilder->dataBlocks[i].dataSize;
		s32 neededSize=neededBlocks[i].headerSize+neededBlocks[i].dataSize;

		if(existingSize!=neededSize)
			totalNeededSize+=neededSize;

		LOG(LOG_INFO,"Array %i has %i %i, needs %i %i",i,
				treeBuilder->dataBlocks[i].headerSize,treeBuilder->dataBlocks[i].dataSize, neededBlocks[i].headerSize,neededBlocks[i].dataSize);
		}

	if(totalNeededSize>0)
		{
		u8 *newArrayData=circAlloc(circHeap, totalNeededSize, sliceTag, sliceIndex, NULL);
		memset(newArrayData,0,totalNeededSize);

		topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+getGapBlockHeaderSize()); // Rebind root after alloc

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
			newArrayData+=neededBlocks[ROUTE_TOPINDEX_PREFIX].headerSize+neededBlocks[ROUTE_TOPINDEX_PREFIX].dataSize;

			if(oldData!=NULL)
				*oldData&=~ALLOC_HEADER_LIVE_MASK; // Mark old block as dead
			}
		else if(existingSize>neededSize)
			LOG(LOG_CRITICAL,"Unexpected reduction in size %i to %i",existingSize,neededSize);

		// Handle Suffix Resize

		existingSize=treeBuilder->dataBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize+treeBuilder->dataBlocks[ROUTE_TOPINDEX_SUFFIX].dataSize;
		neededSize=neededBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize+neededBlocks[ROUTE_TOPINDEX_SUFFIX].dataSize;

		if(existingSize<neededSize)
			{
			rtEncodeTailBlockHeader(1, indexSize, sliceIndex, newArrayData);
			u8 *oldData=topPtr->data[ROUTE_TOPINDEX_SUFFIX];

			topPtr->data[ROUTE_TOPINDEX_SUFFIX]=newArrayData;
			newArrayData+=neededBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize+neededBlocks[ROUTE_TOPINDEX_SUFFIX].dataSize;

			if(oldData!=NULL)
				*oldData&=~ALLOC_HEADER_LIVE_MASK; // Mark old block as dead
			}
		else if(existingSize>neededSize)
				LOG(LOG_CRITICAL,"Unexpected reduction in size %i to %i",existingSize,neededSize);

		// Handle Leaf/Branch Arrays

		for(int i=ROUTE_TOPINDEX_FORWARD_LEAF;i<ROUTE_TOPINDEX_FORWARD_OFFSET;i++)
			{
			existingSize=treeBuilder->dataBlocks[i].headerSize+treeBuilder->dataBlocks[i].dataSize;
			neededSize=neededBlocks[i].headerSize+neededBlocks[i].dataSize;

			s32 sizeDiff=neededSize-existingSize;

			if(sizeDiff>0)
				{
				rtEncodeArrayBlockHeader(i,0, indexSize, sliceIndex, 0, newArrayData);
				u8 *oldData=topPtr->data[i];

				topPtr->data[i]=newArrayData;
				newArrayData+=neededBlocks[i].headerSize;

				if(oldData!=NULL)
					{
					memcpy(newArrayData, oldData+treeBuilder->dataBlocks[i].headerSize, treeBuilder->dataBlocks[i].dataSize);
					memset(newArrayData+existingSize, 0, sizeDiff);
					*oldData&=~ALLOC_HEADER_LIVE_MASK; // Mark old block as dead
					}
				else
					{
					LOG(LOG_INFO,"Setting %i from %p to zero",neededBlocks[i].dataSize, newArrayData);
					memset(newArrayData, 0, neededBlocks[i].dataSize);
					}

				newArrayData+=neededBlocks[i].dataSize;
				}
			else
				if(existingSize>neededSize)
					LOG(LOG_CRITICAL,"Unexpected reduction in size %i to %i",existingSize,neededSize);
			}
		}


	LOG(LOG_INFO,"Tails");

	if((routingBuilder->upgradedToTree) || getSeqTailBuilderDirty(&(routingBuilder->prefixBuilder)))
		{
		LOG(LOG_INFO,"Prefix to %p %p",topPtr->data[ROUTE_TOPINDEX_PREFIX],topPtr->data[ROUTE_TOPINDEX_PREFIX]+neededBlocks[ROUTE_TOPINDEX_PREFIX].headerSize);
		writeSeqTailBuilderPackedData(&(routingBuilder->prefixBuilder), topPtr->data[ROUTE_TOPINDEX_PREFIX]+neededBlocks[ROUTE_TOPINDEX_PREFIX].headerSize);
		}

	if((routingBuilder->upgradedToTree) || getSeqTailBuilderDirty(&(routingBuilder->suffixBuilder)))
		writeSeqTailBuilderPackedData(&(routingBuilder->suffixBuilder), topPtr->data[ROUTE_TOPINDEX_SUFFIX]+neededBlocks[ROUTE_TOPINDEX_SUFFIX].headerSize);

	LOG(LOG_INFO,"Leaves");

	for(int i=ROUTE_TOPINDEX_FORWARD_LEAF; i<ROUTE_TOPINDEX_FORWARD_BRANCH;i++)
		{
		RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);

		if((routingBuilder->upgradedToTree) || rttGetTopArrayDirty(arrayProxy))
			{
			LOG(LOG_INFO,"Calc space");

			s32 size=mergeTopArrayUpdates_leaf_accumulateSize(arrayProxy, indexSize);
			LOG(LOG_INFO,"Need to allocate %i to write leaf array %i",size, i);

			u8 *newLeafData=circAlloc(circHeap, size, sliceTag, sliceIndex, NULL);

			LOG(LOG_INFO,"Allocating");

			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+getGapBlockHeaderSize()); // Rebind top/array data in case it moved

			LOG(LOG_INFO,"Binding to %p",topPtr->data[i]+neededBlocks[i].headerSize);
			rttBindBlockArrayProxy(arrayProxy, topPtr->data[i]+neededBlocks[i].headerSize);

			LOG(LOG_INFO,"Merging - alloc %i",arrayProxy->dataBlock->dataAlloc);


			//void mergeTopArrayUpdates_leaf(RouteTableTreeArrayProxy *arrayProxy, int arrayNum, int indexSize, int index, u8 *newData, s32 newDataSize)

			mergeTopArrayUpdates_leaf(arrayProxy, i, indexSize, sliceIndex, newLeafData, size);

			}
		}

	LOG(LOG_INFO,"Branches");

	for(int i=ROUTE_TOPINDEX_FORWARD_BRANCH; i<ROUTE_TOPINDEX_FORWARD_OFFSET;i++)
		{
		RouteTableTreeArrayProxy *arrayProxy=rttGetTopArrayByIndex(treeBuilder, i);

		if((routingBuilder->upgradedToTree)|| rttGetTopArrayDirty(arrayProxy))
			{
			LOG(LOG_INFO,"Calc space");

			s32 size=mergeTopArrayUpdates_branch_accumulateSize(arrayProxy, indexSize);
			LOG(LOG_INFO,"Need to allocate %i to write branch array %i",size, i);

			u8 *newBranchData=circAlloc(circHeap, size, sliceTag, sliceIndex, NULL);

			LOG(LOG_INFO,"Allocating");

			topPtr=(RouteTableTreeTopBlock *)((*(routingBuilder->rootPtr))+getGapBlockHeaderSize()); // Rebind top/array data in case it moved

			LOG(LOG_INFO,"Binding to %p",topPtr->data[i]+neededBlocks[i].headerSize);
			rttBindBlockArrayProxy(arrayProxy, topPtr->data[i]+neededBlocks[i].headerSize);

			LOG(LOG_INFO,"Merging - alloc %i",arrayProxy->dataBlock->dataAlloc);


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

		if((header&ALLOC_HEADER_LIVE_GAP_ROOT_MASK)==ALLOC_HEADER_LIVE_GAP_ROOT_DIRECT_VALUE)
			{
			createBuildersFromDirectData(&routingBuilder);
			}
		// ALLOC_HEADER_LIVE_INDIRECT_ROOT_VALUE

		else if((header&ALLOC_HEADER_LIVE_GAP_ROOT_MASK)==ALLOC_HEADER_LIVE_GAP_ROOT_TOP_VALUE)
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








