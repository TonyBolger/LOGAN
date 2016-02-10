/*
 * smer.h
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#ifndef __SMER_H
#define __SMER_H


#include "common.h"


/********************** Definitions for SmerMap **********************/

// Each slice represents up to 65535 Smers, with a common 7bp prefix

typedef struct smerMapSliceStr
{
    u32 shift;
    u32 mask;

    SmerMapEntry *smers;
} SmerMapSlice;


typedef struct smerMapStr
{
	SmerMapSlice slice[SMER_MAP_SLICES];
} SmerMap;


/********************** Definitions for SmerArray **********************/

// Each block represents up to 255 Smers, with a common 15bp prefix

typedef struct smerArrayBlockStr
{
	SmerArrayEntry *smer;
	u32 numSmers;



} SmerArrayBlock;


// Each slice represents up to 65535 Smers, with a common 7bp prefix

typedef struct smerArraySliceStr
{
	SmerArrayBlock blocks[SMER_MAP_BLOCKS_PER_SLICE];

} SmerArraySlice;


typedef struct smerArrayStr
{
	SmerArraySlice slice[SMER_MAP_SLICES];
} SmerArray;




/********************** Shared functionality **********************/

SmerId complementSmerId(SmerId id);
SmerId shuffleSmerIdRight(SmerId id, u8 base);
SmerId shuffleSmerIdLeft(SmerId id, u8 base);

int smerIdcompar(const void *ptr1, const void *ptr2);
int smerIdcomparL(const void *ptr1, const void *ptr2);
int u32compar(const void *ptr1, const void *ptr2);

void calculatePossibleSmers(u8 *data, s32 maxIndex, SmerId *smerIds, u32 *compFlags);


/********************** SmerMap functionality **********************/

void smInitSmerMap(SmerMap *smerMap);
void smCleanupSmerMap(SmerMap *smerMap);

void smCreateIndexedSmers(SmerMap *smerMap, u32 indexCount, s32 *indexes, SmerId *smerIds);
u32 smFindIndexesOfExistingSmers(SmerMap *smerMap, u8 *data, s32 maxIndex, s32 *oldIndexes, SmerId *smerIds, s32 maxDistance);
void smCreateSmers(SmerMap *smerMap, SmerId *smerIds, u32 smerCount);
void smConsiderResize(SmerMap *smerMap, int sliceNum);


u32 smGetSmerCount(SmerMap *smerMap);
void smGetSortedSmerIds(SmerMap *smerMap, SmerId *array);
void smGetSmerPathCounts(SmerMap *smerMap, SmerId *smerIds, u32 *smerPaths);



#endif

