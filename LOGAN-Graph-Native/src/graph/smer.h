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

typedef struct smerArrayUpdateStr
{
	u32 seqLength;

	char packedSeq[];
} SmerArrayUpdate;


/*
// Each block represents up to 65536 Smers, with a common bp prefix

typedef struct smerArrayBlockStr
{
	SmerEntry *smer;
	u32 numSmers;

	// Ptr / Offsets to tail and route info
	//

} SmerArrayBlock;
*/

// Each slice smers, with a common 7bp prefix

typedef struct smerArraySliceStr
{
	//SmerArrayBlock *blocks[SMER_MAP_BLOCKS_PER_SLICE];
	SmerEntry *smerIT;
	s32 smerCount;

} SmerArraySlice;


typedef struct smerArrayStr
{
	SmerArraySlice slice[SMER_SLICES];
} SmerArray;




/********************** Shared functionality **********************/

SmerId complementSmerId(SmerId id);
SmerId shuffleSmerIdRight(SmerId id, u8 base);
SmerId shuffleSmerIdLeft(SmerId id, u8 base);

int smerIdCompar(const void *ptr1, const void *ptr2);
int smerEntryCompar(const void *ptr1, const void *ptr2);

int smerIdComparL(const void *ptr1, const void *ptr2);
int u32Compar(const void *ptr1, const void *ptr2);

void calculatePossibleSmers(u8 *data, s32 maxIndex, SmerId *smerIds, u32 *compFlags);

u64 hashForSmer(SmerEntry entry);
u32 sliceForSmer(SmerId smer, u64 hash);
SmerId recoverSmerId(u64 sliceNum, SmerEntry entry);


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

u32 saFindIndexesOfExistingSmers(SmerArray *smerArray, u8 *data, s32 maxIndex, s32 *oldIndexes, SmerId *smerIds);
void saVerifyIndexing(s32 maxAllowedDistance, s32 *indexes, u32 indexCount, int dataLength, int maxValidIndex);


#endif

