/*
 * smer.h
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#ifndef __SMER_H
#define __SMER_H


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

	//MemPackStack *slicePackStack;

	//long totalAlloc;
	//long totalAllocPrefix;
	//long totalAllocSuffix;
	//long totalAllocRoutes;
	//long totalRealloc;
} SmerArraySlice;


typedef struct smerArrayStr
{
	SmerArraySlice slice[SMER_SLICES];
	MemCircHeap *heaps[SMER_DISPATCH_GROUPS];
} SmerArray;



/******************* Definitions for Data Insert/Retrieval ******************/

typedef struct routeTableEntryStr
{
	s32 prefix;
	s32 suffix;
	s32 width;
} RouteTableEntry;


#define SMER_EXISTS_FORWARD (1)
#define SMER_EXISTS_NOT (0)
#define SMER_EXISTS_REVERSE (-1)

typedef struct smerLinkedStr
{
	SmerId smerId;

	s64 *prefixes;
	SmerId *prefixSmers;
	s8 *prefixSmerExists;
	u32 prefixCount;

	s64 *suffixes;
	SmerId *suffixSmers;
	s8 *suffixSmerExists;
	u32 suffixCount;

	RouteTableEntry *forwardRouteEntries;
	RouteTableEntry *reverseRouteEntries;
	u32 forwardRouteCount;
	u32 reverseRouteCount;

} SmerLinked;



#define CANONICAL_SMER(F,R) ((fsmer)<=(rsmer)?(fsmer):(rsmer))

/********************** Shared functionality **********************/

int smerIdCompar(const void *ptr1, const void *ptr2);
int smerIdComparL(const void *ptr1, const void *ptr2);

int smerEntryCompar(const void *ptr1, const void *ptr2);

//int u32Compar(const void *ptr1, const void *ptr2);

SmerId complementSmerId(SmerId id);


//SmerId shuffleSmerIdRight(SmerId id, u8 base);
//SmerId shuffleSmerIdLeft(SmerId id, u8 base);

void calculatePossibleSmers(u8 *data, s32 maxIndex, SmerId *smerIds);
//void calculatePossibleSmersAndOrientation(u8 *data, s32 smerCount, SmerId *smerIds, u32 *revCompPtr);
void calculatePossibleSmersAndOrientation(u8 *data, u8 *secondData, s32 dataBytes, s32 secondDataBytes, s32 smerCount, SmerId *smerIds, u32 *revCompPtr);

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

void smAddPathSmers(SmerMap *smerMap, u32 dataLength, u8 *data, s32 nodeSize, s32 sparseness);

u32 smGetSmerCount(SmerMap *smerMap);
void smGetSmerIds(SmerMap *smerMap, SmerId *smerIds);

//void smGetSmerPathCounts(SmerMap *smerMap, SmerId *smerIds, u32 *smerPaths);


/********************** SmerArray functionality **********************/


s32 saInitSmerArray(SmerArray *smerArray, SmerMap *smerMap);
void saCleanupSmerArray(SmerArray *smerArray);

int saFindSmerEntry(SmerArraySlice *slice, SmerEntry smerEntry);
s32 saFindSmer(SmerArray *smerArray, SmerId smerId);
u8 *saFindSmerAndData(SmerArray *smerArray, SmerId smerId, s32 *sliceNumPtr, s32 *indexPtr);


void saVerifyIndexing(s32 maxAllowedDistance, s32 *indexes, u32 indexCount, int dataLength, int maxValidIndex);


u32 saGetSmerCount(SmerArray *smerArray);
void saGetSmerIds(SmerArray *smerArray, SmerId *smerIds);

#endif

