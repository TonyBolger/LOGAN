/*
 * common.h
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#ifndef __COMMON_H
#define __COMMON_H

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <alloca.h>
#include <malloc.h>
#include <pthread.h>
#include <stdarg.h>
#include <signal.h>
#include <limits.h>
#include <stdint.h>

#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>


//#define FEATURE_ENABLE_SMER_STATS
//#define FEATURE_ENABLE_HEAP_STATS
//#define FEATURE_ENABLE_MEMTRACK
//#define FEATURE_ENABLE_MEMTRACK_CONTINUOUS
//#define FEATURE_ENABLE_MEMKIND
//#define FEATURE_ENABLE_TICKTOCK

#define DEFAULT_SLEEP_NANOS 1000000
#define DEFAULT_SLEEP_SECS 0

#define THREAD_INDEXING_STACKSIZE (16*1024*1024)
#define THREAD_ROUTING_STACKSIZE (16*1024*1024)


#define TASK_INGRESS_BASESTOTAL 50*1000*1000

#define TI_INGRESS_BLOCKSIZE 1000
#define TR_INGRESS_BLOCKSIZE 10000
#define TR_ROUTING_BLOCKSIZE 10000

#define TR_MAX_SEQUENCES_IN_FLIGHT ((int)(7.5*1000*1000))
#define TR_SEQUENCE_IN_FLIGHT_ROUTING_THRESHOLD ((u64)(TR_MAX_SEQUENCES_IN_FLIGHT*0.95))


// ~60K usable bricks per chunk, so one chunk per 6 blocks: 7.5M -> 125 (140 with margin) x per-seq ratio
//
// Short read - Approximate: 1 : 1.5-2 : 2 - 3
// Long read - Approximate: 5 : 1.5-2 : 2 - 3

#define TR_BRICKCHUNKS_BASE_MAX 140

#define TR_BRICKCHUNKS_SEQUENCE_MIN 50
#define TR_BRICKCHUNKS_SEQUENCE_MAX (TR_BRICKCHUNKS_BASE_MAX*5)

#define TR_BRICKCHUNKS_LOOKUP_MIN 50
#define TR_BRICKCHUNKS_LOOKUP_MAX (TR_BRICKCHUNKS_BASE_MAX*2)

#define TR_BRICKCHUNKS_DISPATCH_MIN 50
#define TR_BRICKCHUNKS_DISPATCH_MAX (TR_BRICKCHUNKS_BASE_MAX*3)


#define TR_INGRESS_LOOKUP_BLOCKMARGIN 50
#define TR_INGRESS_LOOKUP_PILEMARGIN (TR_BRICKCHUNKS_LOOKUP_MIN*3000)
#define TR_INGRESS_DISPATCH_PILEMARGIN (TR_BRICKCHUNKS_DISPATCH_MIN*6000)

#define TR_LOOKUP_RECYCLE_BLOCKMARGIN 10
#define TR_LOOKUP_RECYCLE_BLOCKSIZE 1000

#define TR_LOOKUP_RECYCLE_BLOCK_THRESHOLD_FORCE 10
#define TR_LOOKUP_RECYCLE_BLOCK_THRESHOLD_TIGHT 50

#define TR_LOOKUP_RECYCLE_TIGHT_BLOCKMARGIN 30

#define TR_READBLOCK_INGRESS_INFLIGHT 1
#define TR_READBLOCK_LOOKUPS_INFLIGHT 300

//#define TR_READBLOCK_DISPATCHES_INFLIGHT 250



#define PAD_1BITLENGTH_BYTE(L) (((L)+7)>>3)
#define PAD_2BITLENGTH_BYTE(L) (((L)+3)>>2)
#define PAD_4BITLENGTH_BYTE(L) (((L)+1)>>1)

#define PAD_NEXT_2(L) (((L)+1)&~1)
#define PAD_NEXT_4(L) (((L)+3)&~3)
#define PAD_NEXT_8(L) (((L)+7)&~7)


#define PAD_BYTELENGTH_2BYTE(L) (((L)+1)&~1)
#define PAD_BYTELENGTH_4BYTE(L) (((L)+3)&~3)
#define PAD_BYTELENGTH_8BYTE(L) (((L)+7)&~7)

#define PAD_BYTELENGTH_16BYTE(L) (((L)+15)&~15)
#define PAD_BYTELENGTH_32BYTE(L) (((L)+31)&~31)
#define PAD_BYTELENGTH_64BYTE(L) (((L)+63)&~63)

#define QUAD_ALIGNMENT_MASK 7
#define QUAD_ALIGNMENT_SIZE 8

#define CACHE_ALIGNMENT_MASK 63
#define CACHE_ALIGNMENT_SIZE 64

#define PAGE_ALIGNMENT_MASK 4095
#define PAGE_ALIGNMENT_SIZE 4096


#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

typedef unsigned char u8;
typedef signed char s8;

typedef unsigned short u16;
typedef signed short s16;

typedef unsigned int u32;
typedef signed int s32;

typedef unsigned long long u64;
typedef signed long long s64;

#define U8MAX 0xFF
#define U16MAX 0xFFFF
#define U24MAX 0xFFFFFF
#define U32MAX 0xFFFFFFFF

#ifndef SMER_BASES
  #define SMER_BASES 23
//  #define SMER_BASES 21
//  #define SMER_BASES 19
//  #define SMER_BASES 17
//  #define SMER_BASES 15
#endif


#if SMER_BASES == 23

#define SMER_PACKED_BYTES 6
#define SMER_BITS 46
#define SMER_TOP_BASES 7
#define SMER_BOTTOM_BASES 16

#define SMER_CBITS 64
#define SMER_CBITS_EMPTY 18
typedef u64 SmerId;
typedef u32 SmerEntry;

#define SMER_MASK 0x00003FFFFFFFFFFFL
#define SMER_DUMMY 0x00003FFFFFFFFFFFL
#define SMER_SLICE_DUMMY 0xFFFFFFFF

#define SMER_RMASK 0xFFFFC00000000000L

#define SMER_GET_TOP(SMER) (((SMER)>>32)&0x3FFF)
#define SMER_GET_BOTTOM(SMER) ((SMER)&0xFFFFFFFF)

#define SMER_TOP_COUNT 0x4000

#define SMER_SLICES 16384
#define SMER_SLICE_MASK (SMER_SLICES-1)
//#define SMER_SLICE_PRIME 12289
#define SMER_SLICE_PRIME 12721
//#define SMER_SLICE_PRIME 8171
//#define SMER_SLICE_PRIME 5

#define SMER_LOOKUP_PERCOLATES 64
#define SMER_LOOKUP_PERCOLATE_SHIFT 8
#define SMER_LOOKUP_PERCOLATE_MASK (SMER_LOOKUP_PERCOLATES-1)
#define SMER_LOOKUP_PERCOLATE_SLICES 256
#define SMER_LOOKUP_PERCOLATE_SLICEMASK (SMER_LOOKUP_PERCOLATES_SLICES-1)

//#define SMER_LOOKUP_GROUPS 64
//#define SMER_LOOKUP_GROUP_SHIFT 8
//#define SMER_LOOKUP_GROUP_MASK (SMER_DISPATCH_GROUPS-1)
//#define SMER_LOOKUP_GROUP_SLICES 256
//#define SMER_LOOKUP_GROUP_SLICEMASK (SMER_DISPATCH_GROUP_SLICES-1)


#define SMER_DISPATCH_GROUPS 64
#define SMER_DISPATCH_GROUP_SHIFT 8
#define SMER_DISPATCH_GROUP_MASK (SMER_DISPATCH_GROUPS-1)
#define SMER_DISPATCH_GROUP_SLICES 256
#define SMER_DISPATCH_GROUP_SLICEMASK (SMER_DISPATCH_GROUP_SLICES-1)

#define SMER_DISPATCH_GROUP_PRIME 1



#elif SMER_BASES == 21

#error SMER_BASES of 21 not yet supported

#elif SMER_BASES == 19

#error SMER_BASES of 19 not yet supported

#elif SMER_BASES == 17

#error SMER_BASES of 17 not yet supported

#elif SMER_BASES == 15

#error SMER_BASES of 15 not yet supported

/* 15 BASE SMER
// Number of bases per Smer
#define SMER_BASES 15
#define SMER_BITS 30

// Number of bits in the Smer container type
#define SMER_CBITS 32

// Various marks for SMERs
#define SMER_CBITS_EMPTY 2
#define SMER_MASK 0x3FFFFFFF
#define SMER_REST_MASK 0xC0000000

typedef u32 SmerId;
*/


#else

#error SMER_BASES not defined

#endif


#define ERRORBUF 1000


// Hack to prevent memcpy version problems on old server versions
__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");


// Doesn't really belong here, but nowhere obviously better to put it

typedef struct sequenceWithQualityStr {
	u8 *seq;
	u8 *qual;
	s32 seqLength;
	u8 *tagData;
	u8 tagLength;
} SequenceWithQuality;

typedef struct swqBufferStr {
	u8 *seqBuffer;
	u8 *qualBuffer;
	u8 *tagBuffer;
	SequenceWithQuality *rec;

	s32 maxSequenceTotalLength;
	s32 maxTagTotalLength;
	s32 maxSequences;

	s32 maxSequenceLength;
	s32 maxTagLength;

	s32 numSequences;
	s32 usageCount;
} SwqBuffer;


#include "mem/memTracker.h"
#include "mem/memSlab.h"

#include "mem/memBricks.h"
#include "mem/memBricksDouble.h"
#include "mem/memBricksSingle.h"
#include "mem/memDispenser.h"
#include "mem/memPackStack.h"
#include "mem/memHeapCirc.h"
#include "mem/memHeapFixed.h"
#include "mem/memHeap.h"

#include "util/arrayPacking.h"
#include "util/bitMap.h"
#include "util/bitPacking.h"
#include "util/bloom.h"
#include "util/intObjectHash.h"
#include "util/log.h"
#include "util/misc.h"
#include "util/pack.h"
#include "util/queue.h"
#include "util/smerImplicitTree.h"
#include "util/varipack.h"


#include "task/task.h"

#include "graph/smer.h"
#include "graph/seqTail.h"
#include "graph/seqIndex.h"
#include "graph/routeTablePacking.h"
#include "graph/routeTable.h"
#include "graph/routeTableTags.h"
#include "graph/routeTableArray.h"
#include "graph/routeTableTreeArray.h"
#include "graph/routeTableTreeBranch.h"
#include "graph/routeTableTreeLeaf.h"
#include "graph/routeTableTreeProxy.h"
#include "graph/routeTableTreeWalker.h"
#include "graph/routeTableTree.h"
#include "graph/routing.h"
#include "graph/graph.h"

#include "task/taskIndexing.h"
#include "task/taskRouting.h"
#include "task/taskRoutingIngress.h"
#include "task/taskRoutingLookup.h"
#include "task/taskRoutingDispatch.h"

#include "io/parser.h"
#include "io/fastqParser.h"
#include "io/fastaParser.h"
#include "io/serdes.h"

// Leave mem.h to end (it needs all struct/typedefs defined)
#include "mem/mem.h"

#endif
