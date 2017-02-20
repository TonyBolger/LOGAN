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


// Header consists of Live Bit, 4-bits type/index, then either 3-bit gap, or variant bit plus 2-bit index size

// Check Live
#define ALLOC_HEADER_LIVE_MASK 0x80							// ? - - -  - - - -

// No Offset version:
// x 0 0 0 -> Gap: 0ggg / 1ggg
// x 0 0 1 -> Tail: 0?ii / 1?ii
// x 0 1 0 -> Branch Array (top) 0?ii / 1?ii
// x 0 1 1 -> Leaf Array (top) 0lii / 1lii
// x 1 0 0 -> Leaf Array (mid) 0?ii / 1?ii
// x 1 0 1 -> Branches: 0?ii / 1?ii
// x 1 1 0 -> Leaves (shallow): 0?ii / 1?ii
// x 1 1 1 -> Leaves (deep): 0?ii / 1?ii

// Check Gap-coded/tail/array/entity						// . 0 0 0  - g g g: g=gap exponent(1+)
#define ALLOC_HEADER_MAIN_MASK 0x70	 						// - ? ? ?  - - - -
#define ALLOC_HEADER_MAIN_GAP 0x00							// - 0 0 0  - - - -
#define ALLOC_HEADER_MAIN_TAIL 0x10							// - 0 0 1  - - - -
#define ALLOC_HEADER_MAIN_BRANCH_ARRAY0 0x20				// - 0 1 0  - - - -
#define ALLOC_HEADER_MAIN_LEAF_ARRAY0 0x30					// - 0 1 1  - - - -
#define ALLOC_HEADER_MAIN_LEAF_ARRAY1 0x40					// - 1 0 0  - - - -
#define ALLOC_HEADER_MAIN_BRANCH1 0x50						// - 1 0 1  - - - -
#define ALLOC_HEADER_MAIN_LEAF1 0x60						// - 1 1 0  - - - -
#define ALLOC_HEADER_MAIN_LEAF2 0x70						// - 1 1 1  - - - -

#define ALLOC_HEADER_GAPSIZE_MASK 0x07				        // - - - -  - ? ? ?
#define ALLOC_HEADER_INDEXSIZE_MASK 0x03				    // - - - -  - - ? ?

// Check Gap-coded & live
#define ALLOC_HEADER_GAP_ROOT_MASK 0x78	 					// - ? ? ?  ? - - -
#define ALLOC_HEADER_GAP_ROOT_DIRECT_VALUE 0x00				// - 0 0 0  0 - - -
#define ALLOC_HEADER_GAP_ROOT_TOP_VALUE 0x08				// - 0 0 0  1 - - -

// Check Gap-coded & live
#define ALLOC_HEADER_LIVE_GAP_MASK 0xB8						// ? - ? ?  ? - - -
#define ALLOC_HEADER_LIVE_GAP_ROOT_VALUE 0x80				// 1 - 0 0  0 - - -

#define ALLOC_HEADER_LIVE_GAP_ROOT_MASK 0xF8	 			// ? ? ? ?  ? - - -
#define ALLOC_HEADER_LIVE_GAP_ROOT_DIRECT_VALUE 0x80		// 1 0 0 0  0 - - -
#define ALLOC_HEADER_LIVE_GAP_ROOT_TOP_VALUE 0x88			// 1 0 0 0  1 - - -

// Top/Non-type Entries

#define ALLOC_HEADER_TOP_MASK 0x78							// - ? ? ?  ? - - -
#define ALLOC_HEADER_TOP_TAIL_PREFIX 0x10					// - 0 0 1  0 - - -
#define ALLOC_HEADER_TOP_TAIL_SUFFIX 0x18					// - 0 0 1  8 - - -
#define ALLOC_HEADER_TOP_BRANCH_ARRAY0_FWD 0x20				// - 0 1 0  0 - - -
#define ALLOC_HEADER_TOP_BRANCH_ARRAY0_REV 0x28				// - 0 1 0  1 - - -
#define ALLOC_HEADER_TOP_LEAF_ARRAY0_FWD 0x30				// - 0 1 1  0 - - -
#define ALLOC_HEADER_TOP_LEAF_ARRAY0_REV 0x38				// - 0 1 1  1 - - -

#define ALLOC_HEADER_TOP_LEAF_ARRAY1_FWD 0x40				// - 1 0 0  0 - - -
#define ALLOC_HEADER_TOP_LEAF_ARRAY1_REV 0x48				// - 1 0 0  1 - - -
#define ALLOC_HEADER_TOP_BRANCH1_FWD 0x50					// - 1 0 1  0 - - -
#define ALLOC_HEADER_TOP_BRANCH1_REV 0x58					// - 1 0 1  1 - - -
#define ALLOC_HEADER_TOP_LEAF1_FWD 0x60						// - 1 1 0  0 - - -
#define ALLOC_HEADER_TOP_LEAF1_REV 0x68						// - 1 1 0  1 - - -
#define ALLOC_HEADER_TOP_LEAF2_FWD 0x70						// - 1 1 1  0 - - -
#define ALLOC_HEADER_TOP_LEAF2_REV 0x78						// - 1 1 1  1 - - -

// Tails													// - 0 0 1  ? ? i i
#define ALLOC_HEADER_TAIL_MASK 0x08	 						// - - - -  ? - - -
#define ALLOC_HEADER_TAIL_PREFIX 0x00						// - - - -  0 - - -
#define ALLOC_HEADER_TAIL_SUFFIX 0x00						// - - - -  1 - - -

// Branches	/ Leaves

#define ALLOC_HEADER_ARRAY_DIRECTION 0x10					// - - - -  - ? - -
#define ALLOC_HEADER_VARIANT_MASK 0x04						// - - - -  - ? - -


// Inits

#define ALLOC_HEADER_INIT_LIVE_DIRECT_GAPSMALL 0x81         // 1 0 0 0  0 0 0 1
#define ALLOC_HEADER_INIT_LIVE_TOP_GAPSMALL 0x89		    // 1 0 0 0  1 0 0 1

#define ALLOC_HEADER_INIT_LIVE_PREFIX	   0x90				// 1 0 0 1  0 0 0 0
#define ALLOC_HEADER_INIT_LIVE_SUFFIX	   0x98				// 1 0 0 1  1 0 0 0

#define ALLOC_HEADER_INIT_LIVE_BRANCH_ARRAY0 0xA0			// 1 0 1 0  - - - -
#define ALLOC_HEADER_INIT_LIVE_LEAF_ARRAY0 0xB0				// 1 0 1 1  - - - -

#define ALLOC_HEADER_INIT_LIVE_LEAF_ARRAY1 0xC0				// 1 1 0 0  - - - -
#define ALLOC_HEADER_INIT_LIVE_BRANCH1 0xD0					// 1 1 0 1  - - - -
#define ALLOC_HEADER_INIT_LIVE_LEAF1 0xE0					// 1 1 1 0  - - - -
#define ALLOC_HEADER_INIT_LIVE_LEAF2 0xF0					// 1 1 1 1  - - - -


/*
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
*/
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
		*data++=ALLOC_HEADER_INIT_LIVE_DIRECT_GAPSMALL;
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

		u8 data1=ALLOC_HEADER_INIT_LIVE_DIRECT_GAPSMALL + (exp);

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
		*data++=ALLOC_HEADER_INIT_LIVE_TOP_GAPSMALL;
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

		u8 data1=ALLOC_HEADER_INIT_LIVE_TOP_GAPSMALL+exp;

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

	*data++=(prefixSuffix?ALLOC_HEADER_INIT_LIVE_SUFFIX:ALLOC_HEADER_INIT_LIVE_PREFIX)+(indexSize-1);
	varipackEncode(index, data);

	return 1+indexSize;
}

s32 rtDecodeTailBlockHeader(u8 *data, u32 *prefixSuffixPtr, u32 *indexSizePtr, u32 *indexPtr)
{
	u8 header=*data++;

	u32 prefixSuffix=(header&ALLOC_HEADER_TAIL_MASK)!=0;

	if(prefixSuffixPtr!=NULL)
		*prefixSuffixPtr=prefixSuffix;

	u32 indexSize=(header&ALLOC_HEADER_INDEXSIZE_MASK)+1;

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
	return 1+indexSize;
}




s32 rtEncodeArrayBlockHeader_Branch0(s32 fwdRev, s32 indexSize, s32 index, u8 *data)
{
	*data++=(ALLOC_HEADER_INIT_LIVE_BRANCH_ARRAY0+(fwdRev<<3)+(indexSize-1));
	data+=varipackEncode(index, data);

	return 1+indexSize;
}

s32 rtDecodeArrayBlockHeader_Branch0(u8 *data, s32 *indexSizePtr, s32 *indexPtr)
{
	u8 header=*data++;

	s32 indexSize=(header&ALLOC_HEADER_INDEXSIZE_MASK)+1;
	s32 index=varipackDecode(indexSize, data);

	if(indexSizePtr!=NULL)
		*indexSizePtr=indexSize;

	if(indexPtr!=NULL)
		*indexPtr=index;

	return 1+indexSize;
}

s32 rtEncodeArrayBlockHeader_Leaf0(s32 fwdRev, s32 levels, s32 indexSize, s32 index, u8 *data)
{
	*data++=(ALLOC_HEADER_INIT_LIVE_LEAF_ARRAY0+(fwdRev<<3)+(levels<<2)+(indexSize-1));
	data+=varipackEncode(index, data);

	return 1+indexSize;
}

s32 rtDecodeArrayBlockHeader_Leaf0(u8 *data, s32 *levelsPtr, s32 *indexSizePtr, s32 *indexPtr)
{
	u8 header=*data++;

	s32 levels=header&ALLOC_HEADER_VARIANT_MASK;
	s32 indexSize=(header&ALLOC_HEADER_INDEXSIZE_MASK)+1;
	s32 index=varipackDecode(indexSize, data);

	if(levelsPtr!=NULL)
		*levelsPtr=levels;

	if(indexSizePtr!=NULL)
		*indexSizePtr=indexSize;

	if(indexPtr!=NULL)
		*indexPtr=index;

	return 1+indexSize;
}

s32 rtDecodeIndexedBlockHeader_0(u8 *data, s32 *variantPtr, s32 *indexSizePtr, s32 *indexPtr)
{
	u8 header=*data++;

	s32 indexSize=(header&ALLOC_HEADER_INDEXSIZE_MASK)+1;
	s32 variant=(header&ALLOC_HEADER_VARIANT_MASK)!=0;

	s32 index=varipackDecode(indexSize, data);

	if(variantPtr!=NULL)
		*variantPtr=variant;

	if(indexSizePtr!=NULL)
		*indexSizePtr=indexSize;

	if(indexPtr!=NULL)
		*indexPtr=index;

	return 1+indexSize;
}


s32 rtGetIndexedBlockHeaderSize_0(s32 indexSize)
{
	return 1+indexSize;
}


s32 rtEncodeArrayBlockHeader_Leaf1(s32 fwdRev, s32 indexSize, s32 index, s32 subindex, u8 *data)
{
	*data++=(ALLOC_HEADER_INIT_LIVE_LEAF_ARRAY1+(fwdRev<<3)+(indexSize-1));
	data+=varipackEncode(index, data);
	*data++=(u8)subindex;

	return 2+indexSize;
}

s32 rtDecodeArrayBlockHeader_Leaf1(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexPtr)
{
	u8 header=*data++;

	s32 indexSize=(header&ALLOC_HEADER_INDEXSIZE_MASK)+1;
	s32 index=varipackDecode(indexSize, data);

	data+=indexSize;

	s32 subindex=*data;

	if(indexSizePtr!=NULL)
		*indexSizePtr=indexSize;

	if(indexPtr!=NULL)
		*indexPtr=index;

	if(subindexPtr!=NULL)
		*subindexPtr=subindex;

	return 2+indexSize;
}


s32 rtEncodeEntityBlockHeader_Branch1(s32 fwdRev, s32 indexSize, s32 index, s32 subindex, u8 *data)
{
	*data++=(ALLOC_HEADER_INIT_LIVE_BRANCH1+(fwdRev<<3)+(indexSize-1));
	data+=varipackEncode(index, data);
	*data++=(u8)subindex;

	return 2+indexSize;
}

s32 rtDecodeEntityBlockHeader_Branch1(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexPtr)
{
	u8 header=*data++;

	s32 indexSize=(header&ALLOC_HEADER_INDEXSIZE_MASK)+1;
	s32 index=varipackDecode(indexSize, data);

	data+=indexSize;

	s32 subindex=*data;

	if(indexSizePtr!=NULL)
		*indexSizePtr=indexSize;

	if(indexPtr!=NULL)
		*indexPtr=index;

	if(subindexPtr!=NULL)
		*subindexPtr=subindex;

	return 2+indexSize;
}


s32 rtEncodeEntityBlockHeader_Leaf1(s32 fwdRev, s32 indexSize, s32 index, s32 subindex, u8 *data)
{
//	LOG(LOG_INFO,"Encode Leaf %i %i %i to %p",fwdRev, index, subindex, data);

	*data++=(ALLOC_HEADER_INIT_LIVE_LEAF1+(fwdRev<<3)+(indexSize-1));
	data+=varipackEncode(index, data);
	*data++=(u8)subindex;

	return 2+indexSize;
}

s32 rtDecodeEntityBlockHeader_Leaf1(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexPtr)
{
//	LOG(LOG_INFO,"Decode Leaf %p",data);

	u8 header=*data++;

	s32 indexSize=(header&ALLOC_HEADER_INDEXSIZE_MASK)+1;
	s32 index=varipackDecode(indexSize, data);

	data+=indexSize;

	s32 subindex=*data;

//	LOG(LOG_INFO,"Decode Leaf %i %i",index,subindex);

	if(indexSizePtr!=NULL)
		*indexSizePtr=indexSize;

	if(indexPtr!=NULL)
		*indexPtr=index;

	if(subindexPtr!=NULL)
		*subindexPtr=subindex;

	return 2+indexSize;
}

s32 rtDecodeIndexedBlockHeader_1(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexPtr)
{
	//LOG(LOG_INFO,"Decode Indexed1 %p",data);

	u8 header=*data++;

	s32 indexSize=(header&ALLOC_HEADER_INDEXSIZE_MASK)+1;
	s32 index=varipackDecode(indexSize, data);

	data+=indexSize;

	s32 subindex=*data;

	//LOG(LOG_INFO,"Decode Indexed %i %i",index,subindex);

	if(indexSizePtr!=NULL)
		*indexSizePtr=indexSize;

	if(indexPtr!=NULL)
		*indexPtr=index;

	if(subindexPtr!=NULL)
		*subindexPtr=subindex;

	return 2+indexSize;
}

s32 rtGetIndexedBlockHeaderSize_1(s32 indexSize)
{
	return 2+indexSize;
}


s32 rtEncodeEntityBlockHeader_Leaf2(s32 fwdRev, s32 indexSize, s32 index, s32 subindex, u8 *data)
{
	*data++=(ALLOC_HEADER_INIT_LIVE_LEAF2+(fwdRev<<3)+(indexSize-1));
	data+=varipackEncode(index, data);
	*((u16 *)data)=subindex;

	return 3+indexSize;
}

s32 rtDecodeEntityBlockHeader_Leaf2(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexPtr)
{
	u8 header=*data++;

	s32 indexSize=(header&ALLOC_HEADER_INDEXSIZE_MASK)+1;
	s32 index=varipackDecode(indexSize, data);

	data+=indexSize;

	s32 subindex=*((u16 *)data);

	if(indexSizePtr!=NULL)
		*indexSizePtr=indexSize;

	if(indexPtr!=NULL)
		*indexPtr=index;

	if(subindexPtr!=NULL)
		*subindexPtr=subindex;

	return 3+indexSize;
}

s32 rtGetIndexedBlockHeaderSize_2(s32 indexSize)
{
	return 3+indexSize;
}

s32 rtDecodeIndexedBlockHeader(u8 *data, s32 *topindexPtr, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexSizePtr, s32 *subindexPtr)
{
//	LOG(LOG_INFO,"Decode Indexed: %p",data);

	u8 header=*data++;

	s32 topindex=((header & ALLOC_HEADER_TOP_MASK)-ALLOC_HEADER_TOP_TAIL_PREFIX)>>3; // Allow for Direct and Top
	s32 indexSize=(header&ALLOC_HEADER_INDEXSIZE_MASK)+1;
	s32 index=varipackDecode(indexSize, data);

	data+=indexSize;

	if(topindexPtr!=NULL)
		*topindexPtr=topindex;

	if(indexSizePtr!=NULL)
		*indexSizePtr=indexSize;

	if(indexPtr!=NULL)
		*indexPtr=index;

	s32 subindexSize=0, subindex=-1;

	if(topindex>=ROUTE_PSEUDO_INDEX_FORWARD_LEAF_2)
		{
		subindexSize=2;
		subindex=*((u16 *)data);
		}
	else if(topindex>=ROUTE_PSEUDO_INDEX_FORWARD_LEAF_ARRAY_1)
		{
		subindexSize=1;
		subindex=*data;
		}

	//LOG(LOG_INFO,"Decode Indexed: %i %i %i %i %i",topindex, indexSize, index, subindexSize, subindex);

	if(subindexSizePtr!=NULL)
		*subindexSizePtr=subindexSize;

	if(subindexPtr!=NULL)
		*subindexPtr=subindex;

	return 1+indexSize+subindexSize;
}

/*

s32 rtDecodeArrayBlockHeader(u8 *data, s32 *arrayNumPtr, s32 *arrayTypePtr, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexSizePtr, s32 *subindexPtr)
{
	u8 header=*data++;

	switch(header & ALLOC_HEADER_MAIN_MASK)
		{

		}


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

	s32 subindexSize=0, subindex=-1;

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

*/
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
			if(scanCount>100)
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


static s32 scanTagDataIndexed_1(u8 **tagData, s32 smerIndex, s32 topindex, s32 subindex, u8 *wanted)
{
	u8 *primaryPtr=tagData[smerIndex];

	if(primaryPtr==NULL)
		LOG(LOG_CRITICAL,"Attempt to scan nested entry %i with NULL data",smerIndex);

	//LOG(LOG_INFO,"Scan for %i",subindex);

	u8 header=*primaryPtr;

	if((header & ALLOC_HEADER_LIVE_GAP_ROOT_MASK) != ALLOC_HEADER_LIVE_GAP_ROOT_TOP_VALUE)
		LOG(LOG_CRITICAL,"Attempt to scan nested entry %i with invalid header %i",header);

	RouteTableTreeTopBlock *topPtr=(RouteTableTreeTopBlock *)((primaryPtr)+rtGetGapBlockHeaderSize());

	u8 *arrayBlockPtr=topPtr->data[topindex];
	s32 variant=0;

	s32 headerSize=rtDecodeIndexedBlockHeader_0(arrayBlockPtr,&variant, NULL,NULL);
	RouteTableTreeArrayBlock *array1=(RouteTableTreeArrayBlock *)(arrayBlockPtr+headerSize);

	//LOG(LOG_INFO,"Array1 is %p Variant is %i",array1,variant);

	subindex<<=ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_SHIFT;

	if(variant)
		{
		int subindex1=subindex >> ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_SHIFT;

		u8 *array2BlockPtr=array1->data[subindex1];
		s32 header2Size=rtDecodeIndexedBlockHeader_1(array2BlockPtr,NULL,NULL,NULL);
		RouteTableTreeArrayBlock *array2=(RouteTableTreeArrayBlock *)(array2BlockPtr+header2Size);

		int subindex2=subindex & ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_MASK;
		int entriesToScan=MIN(subindex2+ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_RANGE, array2->dataAlloc);

		for(int i=subindex2; i<entriesToScan; i++)
			{
			if(array2->data[i]==wanted)
				return i+(subindex&~ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_MASK);
			}

		LOG(LOG_CRITICAL,"Failed to find indexed entity2 %i %i %i %i",subindex, subindex1, subindex2, entriesToScan);
		}
	else
		{
		int entriesToScan=MIN(subindex+ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_RANGE, array1->dataAlloc);

		for(int i=subindex; i<entriesToScan; i++)
			{
			if(array1->data[i]==wanted)
				return i;
			}

		LOG(LOG_CRITICAL,"Failed to find indexed entity1");
		}

	return -1;
}
/*
static s32 scanTagDataIndexed_2(u8 **tagData, s32 smerIndex, s32 topindex, s32 subIndex, u8 *wanted)
{
	u8 *primaryPtr=tagData[smerIndex];

	if(primaryPtr==NULL)
		LOG(LOG_CRITICAL,"Attempt to scan nested entry %i with NULL data",smerIndex);

	u8 header=*primaryPtr;

	if((header & ALLOC_HEADER_LIVE_GAP_ROOT_MASK) != ALLOC_HEADER_LIVE_GAP_ROOT_TOP_VALUE)
		LOG(LOG_CRITICAL,"Attempt to scan nested entry %i with invalid header %i",header);

	RouteTableTreeTopBlock *topPtr=(RouteTableTreeTopBlock *)((primaryPtr)+rtGetGapBlockHeaderSize());

	u8 *array1BlockPtr=topPtr->data[topindex];
	s32 header1Size=rtDecodeIndexedBlockHeader_0(array1BlockPtr,NULL, NULL,NULL);
	RouteTableTreeArrayBlock *array1=(RouteTableTreeArrayBlock *)(array1BlockPtr+header1Size);

	u8 *array2BlockPtr=array1->data[subIndex >> ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_SHIFT];
	s32 header2Size=rtDecodeIndexedBlockHeader_1(array2BlockPtr,NULL,NULL,NULL);
	RouteTableTreeArrayBlock *array2=(RouteTableTreeArrayBlock *)(array2BlockPtr+header2Size);

	subIndex&=ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_MASK;

	int entriesToScan=MIN(subIndex+ROUTE_TABLE_TREE_DATA_ARRAY_SUBINDEX_RANGE, array2->dataAlloc);

	for(int i=subIndex; i<entriesToScan; i++)
		{
		if(array2->data[i]==wanted)
			{
			return i;
			}
		}

	LOG(LOG_CRITICAL,"Failed to find indexed entity2");
	return -1;

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
		return;

	//LOG(LOG_INFO,"Validate indexEntry: Index: %i Size: %i TopIndex: %i SubIndex: %i",indexEntry->index, indexEntry->size, indexEntry->topindex, indexEntry->subindex);

	RouteTableTreeTopBlock *topPtr=(RouteTableTreeTopBlock *)((*primaryPtr)+rtGetGapBlockHeaderSize());
	//u8 *dataPtr=NULL;

	if(topindex>=0 && topindex<ROUTE_TOPINDEX_MAX)
		{
		if(topPtr->data[topindex]!=heapPtr)
			LOG(LOG_CRITICAL,"Level 0 Pointer mismatch in array during heap validation (Ref: %p vs Heap: %p) - Top: %p",topPtr->data[topindex],heapPtr, (*primaryPtr));

		return;
		}
	else if(topindex<=ROUTE_PSEUDO_INDEX_REVERSE_LEAF_ARRAY_1)
		{
		s32 subindex=indexEntry->subindex;
		u8 *array1BlockPtr=topPtr->data[topindex-ROUTE_PSEUDO_INDEX_MID_ADJUST];

		s32 header1Size=rtDecodeIndexedBlockHeader_0(array1BlockPtr, NULL, NULL, NULL);

		RouteTableTreeArrayBlock *array1=(RouteTableTreeArrayBlock *)(array1BlockPtr+header1Size);

		if(array1->data[subindex]!=heapPtr)
			LOG(LOG_CRITICAL,"Level 1 Pointer mismatch in array entry during validation (%p vs %p) - Top: %p Array: %p",array1->data[subindex],heapPtr,(*primaryPtr),array1);

		return;
		}
	else if (topindex<=ROUTE_PSEUDO_INDEX_REVERSE_LEAF_1)
		topindex-=ROUTE_PSEUDO_INDEX_ENTITY_1_ADJUST;
	else
		topindex-=ROUTE_PSEUDO_INDEX_LEAF_2_ADJUST;

	s32 subindex=indexEntry->subindex;
	u8 *array1BlockPtr=topPtr->data[topindex];

	s32 variant;
	s32 header1Size=rtDecodeIndexedBlockHeader_0(array1BlockPtr, &variant, NULL, NULL);

	RouteTableTreeArrayBlock *array1=(RouteTableTreeArrayBlock *)(array1BlockPtr+header1Size);

	if(!variant)
		{
		if(array1->data[subindex]!=heapPtr)
			LOG(LOG_CRITICAL,"Level 1 Pointer mismatch in array entry during validation (%p vs %p) - Top: %p Array: %p",array1->data[subindex],heapPtr,(*primaryPtr),array1);

		return;
		}
	else
		{
		int subindex1=subindex >> ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_SHIFT;

		u8 *array2BlockPtr=array1->data[subindex1];
		s32 header2Size=rtDecodeIndexedBlockHeader_1(array2BlockPtr,NULL,NULL,NULL);
		RouteTableTreeArrayBlock *array2=(RouteTableTreeArrayBlock *)(array2BlockPtr+header2Size);

		int subindex2=subindex & ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_MASK;

		if(array2->data[subindex2]!=heapPtr)
			LOG(LOG_CRITICAL,"Level 2 Pointer mismatch in array entry during validation (%p vs %p) - Top: %p Array: %p %p",array2->data[subindex2],heapPtr,(*primaryPtr),array1, array2);

		return;
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

		if((header & ALLOC_HEADER_MAIN_MASK) == ALLOC_HEADER_MAIN_GAP) // Gap encoded: Direct or top
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
					index->entries[entry].topindex=ROUTE_PSEUDO_INDEX_DIRECT;
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
//				LOG(LOG_INFO,"rtReclaimIndexer top");

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
					index->entries[entry].topindex=ROUTE_PSEUDO_INDEX_TOP;
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
			if((header & ALLOC_HEADER_MAIN_MASK)==ALLOC_HEADER_MAIN_TAIL) // Tail: Prefix or Suffix
				{
//				LOG(LOG_INFO,"rtReclaimIndexer tail");

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

//					LOG(LOG_INFO,"Index Entry: Index: %i TopIndex: %i SubIndex: %i Size: %i",
//							index->entries[entry].index, index->entries[entry].topindex,
//							index->entries[entry].subindex, index->entries[entry].size);

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
				s32 topindex=0;
				s32 smerIndex=0;
				s32 subindex=-1;

				s32 headerSize=rtDecodeIndexedBlockHeader(heapDataPtr, &topindex, NULL, &smerIndex, NULL, &subindex);
				u8 *scanPtr=heapDataPtr+headerSize;

				u32 dataSize=0;

				if(topindex<=ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0) // Level 0 leaf/branch array
					{
					dataSize=getRouteTableTreeArraySize_Existing((RouteTableTreeArrayBlock *)scanPtr);
					}
				else if(topindex<=ROUTE_PSEUDO_INDEX_REVERSE_LEAF_ARRAY_1)
					{
					dataSize=getRouteTableTreeArraySize_Existing((RouteTableTreeArrayBlock *)scanPtr);
					}
				else if(topindex<=ROUTE_PSEUDO_INDEX_REVERSE_BRANCH_1)
					{
					dataSize=getRouteTableTreeBranchSize_Existing((RouteTableTreeBranchBlock *)scanPtr);

					if(header & ALLOC_HEADER_LIVE_MASK)
						subindex=scanTagDataIndexed_1(tagData,smerIndex,topindex-ROUTE_PSEUDO_INDEX_ENTITY_1_ADJUST,subindex,heapDataPtr);
					}
				else if(topindex<=ROUTE_PSEUDO_INDEX_REVERSE_LEAF_1)
					{
					dataSize=getRouteTableTreeLeafSize_Existing((RouteTableTreeLeafBlock *)scanPtr);

					if(header & ALLOC_HEADER_LIVE_MASK)
						subindex=scanTagDataIndexed_1(tagData,smerIndex,topindex-ROUTE_PSEUDO_INDEX_ENTITY_1_ADJUST,subindex,heapDataPtr);
					}
				else
					{
					dataSize=getRouteTableTreeLeafSize_Existing((RouteTableTreeLeafBlock *)scanPtr);

					if(header & ALLOC_HEADER_LIVE_MASK)
						subindex=scanTagDataIndexed_1(tagData,smerIndex,topindex-ROUTE_PSEUDO_INDEX_LEAF_2_ADJUST,subindex,heapDataPtr);
					}

				scanPtr+=dataSize;
				u32 size=scanPtr-heapDataPtr;

				if(header & ALLOC_HEADER_LIVE_MASK)
					{
					if(entry==index->entryAlloc)
						index=allocOrExpandIndexer(index, disp);

					index->entries[entry].index=smerIndex;
					index->entries[entry].topindex=topindex;
					index->entries[entry].subindex=subindex;
					index->entries[entry].size=size;

//					LOG(LOG_INFO,"Index Entry: Index: %i TopIndex: %i SubIndex: %i Size: %i",
//							index->entries[entry].index, index->entries[entry].topindex,
//							index->entries[entry].subindex, index->entries[entry].size);

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

			s32 diff=sindex-prevOffset;
			blockHeaderSize=rtGetGapBlockHeaderSize();

			if(topindex==ROUTE_PSEUDO_INDEX_DIRECT) 			// Direct
				{
				rtEncodeGapDirectBlockHeader(diff, newChunk);
				}
			else												// Top
				{
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
			//s32 subindex=index->entries[i].subindex;

			if(topindex<=ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0)		// Zero level Array (directly in top)
				{
				memmove(newChunk,topPtr->data[topindex],size);
				topPtr->data[topindex]=newChunk;
				newChunk+=size;
				}
			else if(topindex<=ROUTE_PSEUDO_INDEX_REVERSE_LEAF_ARRAY_1)	// Mid level array
				{
				s32 subindex=index->entries[i].subindex;

				u8 *arrayBlockPtr=topPtr->data[topindex-ROUTE_PSEUDO_INDEX_MID_ADJUST];
				s32 headerSize=rtDecodeIndexedBlockHeader_0(arrayBlockPtr,NULL, NULL,NULL);
				RouteTableTreeArrayBlock *array=(RouteTableTreeArrayBlock *)(arrayBlockPtr+headerSize);

				memmove(newChunk,array->data[subindex],size);
				array->data[subindex]=newChunk;
				newChunk+=size;
				}
			else
				{
				if(topindex<=ROUTE_PSEUDO_INDEX_REVERSE_LEAF_1)
					topindex-=ROUTE_PSEUDO_INDEX_ENTITY_1_ADJUST;
				else
					topindex-=ROUTE_PSEUDO_INDEX_LEAF_2_ADJUST;

				u8 *arrayBlockPtr=topPtr->data[topindex];

				s32 variant=0;
				s32 headerSize=rtDecodeIndexedBlockHeader_0(arrayBlockPtr,&variant, NULL,NULL);
				RouteTableTreeArrayBlock *array=(RouteTableTreeArrayBlock *)(arrayBlockPtr+headerSize);

				s32 subindex=index->entries[i].subindex;

				if(variant)
					{
					int subindex1=subindex >> ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_SHIFT;

					u8 *array2BlockPtr=array->data[subindex1];
					s32 header2Size=rtDecodeIndexedBlockHeader_1(array2BlockPtr,NULL,NULL,NULL);
					RouteTableTreeArrayBlock *array2=(RouteTableTreeArrayBlock *)(array2BlockPtr+header2Size);

					int subindex2=subindex & ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_MASK;

					memmove(newChunk,array2->data[subindex2],size);
					array2->data[subindex2]=newChunk;
					}
				else
					{
					memmove(newChunk,array->data[subindex],size);
					array->data[subindex]=newChunk;
					}

				newChunk+=size;
				}
			}
		}

	if(newChunk!=endPtr)
		{
		LOG(LOG_CRITICAL,"Mismatch Actual: %p vs Expected: %p after relocate",newChunk,endPtr);
		}

//	LOG(LOG_INFO,"Relocated for %p with data %p",index,index->newChunk);
}


void initHeapDataBlock(HeapDataBlock *block, s32 sliceIndex)
{
	memset(block, 0, sizeof(HeapDataBlock));
	block->sliceIndex=sliceIndex;
}


