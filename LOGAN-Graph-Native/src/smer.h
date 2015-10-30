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

typedef struct smerMapEntryStr
{
	SmerId id;
	u32 pathCount;
} SmerMapEntry;

#define SMER_BUCKET_SIZE 4096

typedef struct smerMapEntryBucketStr
{
	struct smerMapEntryBucketStr* nextBucket;
	SmerMapEntry smers[SMER_BUCKET_SIZE];
} SmerMapEntryBucket;

#define SMER_MAP_SLICES 41

typedef struct smerMapSliceStr
{
    u32 prime;
    u32 entryLimit;
    u32 entryCount;

    SmerMapEntryBucket *bucket;
    SmerMapEntry **smers;

    pthread_rwlock_t lock;
} SmerMapSlice;

typedef struct smerMapStr
{
	SmerMapSlice slice[SMER_MAP_SLICES];
} SmerMap;




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

SmerMapEntry *smFindSmer(SmerMap *smerMap, SmerId id);

void smCreateIndexedSmers(SmerMap *smerMap, u32 indexCount, s32 *indexes, SmerId *smerIds);
u32 smFindIndexesOfExistingSmers(SmerMap *smerMap, u8 *data, s32 maxIndex, s32 *oldIndexes, SmerId *smerIds);
void smCreateSmers(SmerMap *smerMap, SmerId *smerIds, u32 smerCount);

u32 smGetSmerCount(SmerMap *smerMap);
void smGetSortedSmerIds(SmerMap *smerMap, SmerId *array);
void smGetSmerPathCounts(SmerMap *smerMap, SmerId *smerIds, u32 *smerPaths);



#endif

