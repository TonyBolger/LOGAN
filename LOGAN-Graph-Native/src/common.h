/*
 * common.h
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#ifndef __COMMON_H
#define __COMMON_H

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

#define SMER_MASK      0x00003FFFFFFFFFFFL
#define SMER_RMASK     0xFFFFC00000000000L

#define SMER_GET_TOP(SMER) (((SMER)>>32)&0x3FFF)
#define SMER_GET_BOTTOM(SMER) ((SMER)&0xFFFFFFFF)

#define SMER_TOP_COUNT 0x4000

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

#define SMER_DUMMY SMER_MASK

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

#endif
