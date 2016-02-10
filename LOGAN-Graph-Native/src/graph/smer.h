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

    //u32 entryLimit;
    //u32 entryCount;

    SmerMapEntry *smers;

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

void smCreateIndexedSmers(SmerMap *smerMap, u32 indexCount, s32 *indexes, SmerId *smerIds);
u32 smFindIndexesOfExistingSmers(SmerMap *smerMap, u8 *data, s32 maxIndex, s32 *oldIndexes, SmerId *smerIds, s32 maxDistance);
void smCreateSmers(SmerMap *smerMap, SmerId *smerIds, u32 smerCount);
void smConsiderResize(SmerMap *smerMap, int sliceNum);


u32 smGetSmerCount(SmerMap *smerMap);
void smGetSortedSmerIds(SmerMap *smerMap, SmerId *array);
void smGetSmerPathCounts(SmerMap *smerMap, SmerId *smerIds, u32 *smerPaths);



#endif

