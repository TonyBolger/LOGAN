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
#include <string.h>
#include <alloca.h>
#include <pthread.h>


#define PAD_BITLENGTH_BYTE(L) (((L)+7)>>3)
#define PAD_BITLENGTH_WORD(L) (((L)+15)>>4)
#define PAD_BITLENGTH_DWORD(L) (((L)+31)>>5)
#define PAD_BITLENGTH_QWORD(L) (((L)+63)>>6)

#define PAD_2BITLENGTH_BYTE(L) (((L)+3)>>2)
#define PAD_2BITLENGTH_WORD(L) (((L)+7)>>3)
#define PAD_2BITLENGTH_DWORD(L) (((L)+15)>>4)
#define PAD_2BITLENGTH_QWORD(L) (((L)+31)>>5)

#define PAD_4BITLENGTH_BYTE(L) (((L)+1)>>1)
#define PAD_4BITLENGTH_WORD(L) (((L)+3)>>2)
#define PAD_4BITLENGTH_DWORD(L) (((L)+7)>>3)
#define PAD_4BITLENGTH_QWORD(L) (((L)+15)>>4)

//#define PAD_BYTELENGTH_BYTE(L) (((L)+0)>>0)
#define PAD_BYTELENGTH_WORD(L) (((L)+1)>>1)
#define PAD_BYTELENGTH_DWORD(L) (((L)+3)>>2)
#define PAD_BYTELENGTH_QWORD(L) (((L)+7)>>3)



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

#ifndef SMER_BASES
  #define SMER_BASES 23
  //#define SMERSIZE21
  //#define SMERSIZE19
  //#define SMERSIZE17
  //#define SMERSIZE15
#endif


#if SMER_BASES == 23

#define SMER_BITS 46
#define SMER_TOP_BASES 7
#define SMER_BOTTOM_BASES 16

#define SMER_CBITS 64
#define SMER_CBITS_EMPTY 18
typedef u64 SmerId;
typedef u32 SmerEntry;

#define SMER_MASK 0x00003FFFFFFFFFFFL
#define SMER_DUMMY 0xFFFFFFFF

#define SMER_RMASK 0xFFFFC00000000000L

#define SMER_GET_TOP(SMER) (((SMER)>>32)&0x3FFF)
#define SMER_GET_BOTTOM(SMER) ((SMER)&0xFFFFFFFF)

#define SMER_TOP_COUNT 0x4000

#define SMER_SLICES 16384
#define SMER_SLICE_MASK (SMER_SLICES-1)

#define SMER_MAP_BLOCKS_PER_SLICE 256

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




// Hack to prevent memcpy version problems on old server versions
__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");



typedef struct sequenceWithQualityStr {
	char *seq;
	char *qual;
	int length;
} SequenceWithQuality;

typedef struct swqBufferStr {
	char *seqBuffer;
	char *qualBuffer;
	SequenceWithQuality *rec;
	int maxSequenceTotalLength;
	int maxSequences;
	int maxSequenceLength;
	int numSequences;
	int usageCount;
} SwqBuffer;



#include "util/bitMap.h"
#include "util/bitPacking.h"
#include "util/bloom.h"
#include "util/log.h"
#include "util/pack.h"
#include "util/smerImplicitTree.h"

#include "task/task.h"

#include "graph/smer.h"
#include "graph/graph.h"

#include "task/taskIndexing.h"
#include "task/taskRouting.h"

// Leave mem.h to end (it needs all struct/typedefs defined)
#include "mem/mem.h"

#endif
