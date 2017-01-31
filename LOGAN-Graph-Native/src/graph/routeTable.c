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


s32 rtHeaderIsLive(u8 data)
{
	return data&ALLOC_HEADER_LIVE_MASK;
}

s32 rtHeaderIsLiveDirect(u8 data)
{
	return (data&ALLOC_HEADER_LIVE_GAP_ROOT_MASK)==ALLOC_HEADER_LIVE_GAP_ROOT_DIRECT_VALUE;
}

s32 rtHeaderIsLiveTop(u8 data)
{
	return (data&ALLOC_HEADER_LIVE_GAP_ROOT_MASK)==ALLOC_HEADER_LIVE_GAP_ROOT_TOP_VALUE;
}

void rtHeaderMarkDead(u8 *data)
{
	if(data!=NULL)
		*data&=~ALLOC_HEADER_LIVE_MASK;
}


static void dumpTagData(u8 **tagData, s32 tagDataLength)
{
	for(int i=0;i<tagDataLength;i++)
		{
		LOG(LOG_INFO,"Tag %i %p",i,tagData[i]);
		}
}

s32 rtEncodeGapDirectBlockHeader(s32 tagOffsetDiff, u8 *data)
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


s32 rtEncodeGapTopBlockHeader(s32 tagOffsetDiff, u8 *data)
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

s32 rtDecodeGapBlockHeader(u8 *data, s32 *tagOffsetDiffPtr)
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
s32 rtGetGapBlockHeaderSize()
{
	return 2;
}



s32 rtEncodeTailBlockHeader(u32 prefixSuffix, u32 indexSize, u32 index, u8 *data)
{
	//LOG(LOG_INFO,"rtEncodeTailBlockHeader: %i Index: %i %i -> %p",prefixSuffix, indexSize, index,data);

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

	//LOG(LOG_INFO,"rtDecodeTailBlockHeader: %i Index: %i %i <- %p (%02x)",prefixSuffix, indexSize, index, (data-1), header);

	return 1+indexSize;
}

s32 rtGetTailBlockHeaderSize(int indexSize)
{
//	LOG(LOG_INFO,"rtGetTailBlockHeader IndexSize: %i",indexSize);

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
		case ARRAY_TYPE_DEEP_PTR:
		case ARRAY_TYPE_SHALLOW_DATA:
			*data=subindex;
			return 2+indexSize;

		case ARRAY_TYPE_DEEP_DATA:
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
	int scanCount=0;
	for(int i=startIndex;i<tagDataLength;i++)
		{
		if(tagData[i]==wanted)
			{
			if(scanCount>10)
				LOG(LOG_INFO,"Scan count of %i from %i of %i to find %p",scanCount,startIndex,tagDataLength,wanted);

			return i;
			}
		scanCount++;
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

/*
//static
s32 scanTagDataNested(u8 **tagData, s32 smerIndex, s32 arrayNum, s32 subIndex) // TODO
{

	return 0;
}

*/

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


void dumpRawArrayBlock(u8 *data)
{
	u32 arrayNum=0, arrayType=0, indexSize=0, index=0, subindexSize=0, subIndex=0;

	int headerSize=rtDecodeArrayBlockHeader(data, &arrayNum, &arrayType, &indexSize, &index, &subindexSize, &subIndex);

	RouteTableTreeArrayBlock *block=(RouteTableTreeArrayBlock *)(data+headerSize);

	LOGS(LOG_INFO,"DumpArray %p (%2x header %i alloc):",data, *data, block->dataAlloc);

	for(int i=0;i<block->dataAlloc;i++)
		LOGS(LOG_INFO," %p",block->data[i]);

	LOGN(LOG_INFO,"");

}

void dumpRawLeafBlock()
{

}

void dumpRawBranchBlock()
{

}


void validateReclaimIndexEntry(MemCircHeapChunkIndexEntry *indexEntry, u8 *heapPtr, u8 tag, u8 **tagData, s32 tagDataLength)
{
	s32 sindex=indexEntry->index;

	u8 **primaryPtr=&tagData[sindex];

	if((*primaryPtr)==NULL)
		{
		LOG(LOG_CRITICAL,"Attempt to validate entry %i indexing NULL data",sindex);
		}


	//s32 size=indexEntry->size;
	s32 topindex=indexEntry->topindex;

	if(topindex<0) // Gap encoded: tag offset needs updating
		{
		return;
		}
	else
		{
		//LOG(LOG_INFO,"Validate indexEntry: Index: %i Size: %i TopIndex: %i SubIndex: %i",indexEntry->index, indexEntry->size, indexEntry->topindex, indexEntry->subindex);

		RouteTableTreeTopBlock *topPtr=(RouteTableTreeTopBlock *)((*primaryPtr)+rtGetGapBlockHeaderSize());

		//u8 *dataPtr=NULL;
		s32 subindex=indexEntry->subindex;

		if(subindex==-1)
			{
			if(topPtr->data[topindex]!=heapPtr)
				LOG(LOG_CRITICAL,"Pointer mismatch in array during heap validation (%p vs %p) - Top: %p",topPtr->data[topindex],heapPtr, (*primaryPtr));

			}
		else
			{
			u8 *arrayBlockPtr=topPtr->data[topindex];
			s32 headerSize=rtDecodeArrayBlockHeader(arrayBlockPtr,NULL,NULL,NULL,NULL,NULL,NULL);
			RouteTableTreeArrayBlock *array=(RouteTableTreeArrayBlock *)(arrayBlockPtr+headerSize);

			if(array->data[subindex]!=heapPtr)
				LOG(LOG_CRITICAL,"Pointer mismatch in array entry during validation (%p vs %p) - Top: %p Array: %p",array->data[subindex],heapPtr,(*primaryPtr),array);
			}

		}
}





MemCircHeapChunkIndex *rtReclaimIndexer(u8 *heapDataPtr, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, s32 tagSearchOffset, MemDispenser *disp)
{
	//LOG(LOG_INFO,"Reclaim Index: HeapDataPtr %p TargetAmt %li Tag: %x Disp: %p",heapDataPtr,targetAmount,tag,disp);

	MemCircHeapChunkIndex *index=allocOrExpandIndexer(NULL, disp);

	u8 *startOfData=heapDataPtr;
	u8 *endOfData=heapDataPtr+targetAmount;

	s64 liveSize=0;
	s64 deadSize=0;

	s32 currentIndex=tagSearchOffset;
	s32 firstTagOffset=-1;

	index->lastLiveTagOffset=-1;

	int entry=0;

	//LOG(LOG_INFO,"Begin reclaimIndexing from %i",tagSearchOffset);

	while(heapDataPtr<endOfData)
		{
		u8 header=*heapDataPtr;
//		LOG(LOG_INFO,"rtReclaimIndexer %p %02x",heapDataPtr,header);

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
				rtDecodeGapBlockHeader(heapDataPtr, &chunkTagOffsetDiff);

				currentIndex+=chunkTagOffsetDiff;
				u8 *scanPtr=heapDataPtr+rtGetGapBlockHeaderSize();

				scanPtr=scanTails(scanPtr);
				scanPtr=scanTails(scanPtr);
				scanPtr=rtaScanRouteTableArray(scanPtr);

				s32 size=scanPtr-heapDataPtr;

				if(header & ALLOC_HEADER_LIVE_MASK)
					{
					currentIndex=scanTagData(tagData, tagDataLength, currentIndex, heapDataPtr);

					if(currentIndex==-1)
						{
						LOG(LOG_INFO,"Failed to find expected direct heap pointer %p in tag data",heapDataPtr);
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

					//validateReclaimIndexEntry(index->entries+entry, heapDataPtr, tag, tagData, tagDataLength);

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
				rtDecodeGapBlockHeader(heapDataPtr, &chunkTagOffsetDiff);

				currentIndex+=chunkTagOffsetDiff;
				s32 size=rtGetGapBlockHeaderSize()+sizeof(RouteTableTreeTopBlock);

				u8 *scanPtr=heapDataPtr+size;

				if(header & ALLOC_HEADER_LIVE_MASK)
					{
					currentIndex=scanTagData(tagData, tagDataLength, currentIndex, heapDataPtr);

					if(currentIndex==-1)
						{
						LOG(LOG_INFO,"Failed to find expected top heap pointer %p in tag data",heapDataPtr);
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

					//validateReclaimIndexEntry(index->entries+entry, heapDataPtr, tag, tagData, tagDataLength);

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

					validateReclaimIndexEntry(index->entries+entry, heapDataPtr, tag, tagData, tagDataLength);

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

//				LOG(LOG_INFO,"Decoded Array at %p: ArrayNum: %i ArrayType: %i SmerIndex: %i SubIndex: %i",heapDataPtr,arrayNum,arrayType,smerIndex,subindex);

/*
				char *status="Dead";
				if(header & ALLOC_HEADER_LIVE_MASK)
					status="Alive";
*/
				u32 dataSize=0;
				u16 alloc=0;

				switch(arrayType)
					{
					case ARRAY_TYPE_SHALLOW_PTR: // Shallow Ptr (no subindex)

						alloc=((RouteTableTreeArrayBlock *)scanPtr)->dataAlloc;
						dataSize=sizeof(RouteTableTreeArrayBlock)+sizeof(u8 *)*alloc;
//						LOGS(LOG_INFO,"Indexing %s Shallow Array %p %i (%i alloc):",status, heapDataPtr, dataSize, alloc);
						//dumpRawArrayBlock(heapDataPtr);

						break;

					case ARRAY_TYPE_DEEP_PTR: // Deep Ptr (1-byte subindex)
//						LOG(LOG_CRITICAL,"Deep Array: Not implemented");
						break;

					case ARRAY_TYPE_SHALLOW_DATA: // Shallow Data Ptr (1-byte subindex)
						switch(arrayNum)
							{
							case ROUTE_TOPINDEX_FORWARD_LEAF:
							case ROUTE_TOPINDEX_REVERSE_LEAF:
								alloc=((RouteTableTreeLeafBlock *)scanPtr)->entryAlloc;
								dataSize=sizeof(RouteTableTreeLeafBlock)+sizeof(RouteTableTreeLeafEntry)*alloc;
//								LOG(LOG_INFO,"Indexing %s Shallow Leaf Data %p %i (%i alloc)",status, heapDataPtr, dataSize, alloc);
								break;

							case ROUTE_TOPINDEX_FORWARD_BRANCH:
							case ROUTE_TOPINDEX_REVERSE_BRANCH:
								alloc=((RouteTableTreeBranchBlock *)scanPtr)->childAlloc;
								dataSize=sizeof(RouteTableTreeBranchBlock)+sizeof(s16)*alloc;
								//LOG(LOG_INFO,"Indexing %s Shallow Branch Data %p %i (%i alloc)",status, heapDataPtr, dataSize, alloc);
								break;

							case ROUTE_TOPINDEX_FORWARD_OFFSET:
							case ROUTE_TOPINDEX_REVERSE_OFFSET:
							default:
								LOG(LOG_CRITICAL,"ShallowData with ArrayNum %i: Not implemented",arrayNum);
							}
						break;

					case ARRAY_TYPE_DEEP_DATA: // Deep Ptr (2-byte subindex)
						//LOG(LOG_CRITICAL,"Deep Data: Not implemented");
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

					validateReclaimIndexEntry(index->entries+entry, heapDataPtr, tag, tagData, tagDataLength);

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
	u8 *endPtr=newChunk+index->sizeLive;

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

			//LOG(LOG_INFO,"Transfer tag-enc (%i) %i bytes from %p to %p",topindex, size,(*primaryPtr),newChunk);

			s32 diff=sindex-prevOffset;
			blockHeaderSize=rtGetGapBlockHeaderSize();

			if(topindex==ROUTE_TOPINDEX_DIRECT) // Direct
				rtEncodeGapDirectBlockHeader(diff, newChunk);
			else	// Top
				{
//				LOG(LOG_INFO,"Transfer top %i bytes from %p to %p",size,(*primaryPtr),newChunk);
				rtEncodeGapTopBlockHeader(diff, newChunk);
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
		else // Tail, Leaf, Branch or Offset
			{
			//LOG(LOG_INFO,"Leaf/Branch %i %i %i",size,sindex,subindex);

			//int headerSize=de

			RouteTableTreeTopBlock *topPtr=(RouteTableTreeTopBlock *)((*primaryPtr)+rtGetGapBlockHeaderSize());

			//u8 *dataPtr=NULL;
			s32 subindex=index->entries[i].subindex;

			if(subindex==-1)
				{
//				LOG(LOG_INFO,"Transfer array %i bytes (topIdx %i) from %p (top %p) to %p",
//						size,topindex,topPtr->data[topindex],(*primaryPtr),newChunk);

//				if(topindex>=ROUTE_TOPINDEX_FORWARD_LEAF && topindex<=ROUTE_TOPINDEX_REVERSE_BRANCH)
//					dumpRawArrayBlock(topPtr->data[topindex]);

				memmove(newChunk,topPtr->data[topindex],size);
				topPtr->data[topindex]=newChunk;
				newChunk+=size;
				}
			else
				{
				u8 *arrayBlockPtr=topPtr->data[topindex];
				s32 headerSize=rtDecodeArrayBlockHeader(arrayBlockPtr,NULL,NULL,NULL,NULL,NULL,NULL);
				RouteTableTreeArrayBlock *array=(RouteTableTreeArrayBlock *)(arrayBlockPtr+headerSize);

//				LOG(LOG_INFO,"Transfer data %i bytes (topIdx %i subIdx %i) from %p (top %p array %p) to %p",
//						size,topindex,subindex,array->data[subindex],(*primaryPtr), arrayBlockPtr,newChunk);

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

	if(newChunk!=endPtr)
		{
		LOG(LOG_CRITICAL,"Mismatch %p vs %p after relocate",newChunk,endPtr);
		}

//	LOG(LOG_INFO,"Relocated for %p with data %p",index,index->newChunk);
}


