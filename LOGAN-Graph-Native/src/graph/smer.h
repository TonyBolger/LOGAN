/*
 * smer.h
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#ifndef __SMER_H
#define __SMER_H


#include "../common.h"


/********************** Definitions for SmerMap **********************/

typedef struct smerMapSliceStr
{
    u32 shift;
    u32 mask;

    SmerEntry *smers;
} SmerMapSlice;


typedef struct smerMapStr
{
	SmerMapSlice slice[SMER_SLICES];
} SmerMap;


/********************** Definitions for SmerArray **********************/


// Each slice smers, with a common 7bp prefix

typedef struct smerArraySliceStr
{
	//SmerArrayBlock *blocks[SMER_MAP_BLOCKS_PER_SLICE];
	SmerEntry *smerIT;
	u8 **smerData;
	s32 smerCount;
	Bloom bloom;

} SmerArraySlice;


typedef struct smerArrayStr
{
	SmerArraySlice slice[SMER_SLICES];
} SmerArray;


#define CANONICAL_SMER(F,R) ((fsmer)<=(rsmer)?(fsmer):(rsmer))

/********************** Shared functionality **********************/

int smerIdCompar(const void *ptr1, const void *ptr2);
int smerIdComparL(const void *ptr1, const void *ptr2);

int smerEntryCompar(const void *ptr1, const void *ptr2);

//int u32Compar(const void *ptr1, const void *ptr2);

//SmerId complementSmerId(SmerId id);
//SmerId shuffleSmerIdRight(SmerId id, u8 base);
//SmerId shuffleSmerIdLeft(SmerId id, u8 base);

void calculatePossibleSmers(u8 *data, s32 maxIndex, SmerId *smerIds);
void calculatePossibleSmersAndCompSmers(u8 *data, s32 maxIndex, SmerId *smerAndCompIds);
//void calculatePossibleSmers2(u8 *data, s32 maxIndex, SmerId *smerIds, u32 *compFlags);

u64 hashForSmer(SmerEntry entry);
u32 sliceForSmer(SmerId smer, u64 hash);
SmerId recoverSmerId(u32 sliceNum, SmerEntry entry);


/********************** SmerMap functionality **********************/

void smInitSmerMap(SmerMap *smerMap);
void smCleanupSmerMap(SmerMap *smerMap);

void smCreateIndexedSmers(SmerMap *smerMap, u32 indexCount, s32 *indexes, SmerId *smerIds);
u32 smFindIndexesOfExistingSmers(SmerMap *smerMap, u8 *data, s32 maxIndex, s32 *oldIndexes, SmerId *smerIds, s32 maxDistance);
void smCreateSmers(SmerMap *smerMap, SmerId *smerIds, u32 smerCount);
void smConsiderResize(SmerMap *smerMap, int sliceNum);

void smDumpSmerMap(SmerMap *smerMap);
u32 smGetSmerSliceCount(SmerMapSlice *smerMapSlice);
void smGetSortedSliceSmerEntries(SmerMapSlice *smerMapSlice, SmerEntry *array);

u32 smGetSmerCount(SmerMap *smerMap);

//void smGetSortedSmerIds(SmerMap *smerMap, SmerId *array);
//void smGetSmerPathCounts(SmerMap *smerMap, SmerId *smerIds, u32 *smerPaths);


/********************** SmerArray functionality **********************/


s32 saInitSmerArray(SmerArray *smerArray, SmerMap *smerMap);
void saCleanupSmerArray(SmerArray *smerArray);

int saFindSmerEntry(SmerArraySlice *slice, SmerEntry smerEntry);
int saFindSmer(SmerArray *smerArray, SmerId smerId);

u32 saFindIndexesOfExistingSmers(SmerArray *smerArray, u8 *data, s32 maxIndex, s32 *oldIndexes, SmerId *smerIds);
void saVerifyIndexing(s32 maxAllowedDistance, s32 *indexes, u32 indexCount, int dataLength, int maxValidIndex);


#endif

