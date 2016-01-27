#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "common.h"

static const float MAX_LOAD_FACTOR = 0.50; // 0.68 -> 0.65 -> 0.60: Reduced to eliminate large scan counts

// Ecoli-1_Q20 gives 1.8M smers - 0.45M/ slice (
// Ecoli-2_Q20 gives 5.7M smers - 1.4M / slice
// set to 14 for testing - hits 50% limit at about 100K reads per slice on Ecoli-2_Q20
//

//static const u32 INIT_SHIFT=18;

static const u32 INIT_SHIFT=18;

//static const u32 PRIME = 49157;


/*
static const u32 PRIMES[] = { 12289, 24593, 49157, 98317, 196613, 393241,
		786433, 1572869, 3145739, 6291469, 12582917, 25165843, 50331653,
		100663319, 201326611, 402653189, 805306457, 1610612741 };

static const u32 PRIME_COUNT = sizeof(PRIMES) / sizeof(u32);

static u32 nextPrime(int currentPrime) {
	u32 i = 0;

	for (i = 0; i < PRIME_COUNT; i++) {
		if (currentPrime < PRIMES[i])
			return PRIMES[i];
	}

	return 0;
}

static SmerMapEntry *allocSmerMapEntry(SmerMapSlice *smerMapSlice) {
	u32 bucketPosition = (smerMapSlice->entryCount) % SMER_BUCKET_SIZE;

	SmerMapEntryBucket *bucket;

	if (bucketPosition == 0) {
		bucket=smEntryBucketAlloc();

		bucket->nextBucket = smerMapSlice->bucket;
		smerMapSlice->bucket = bucket;
	} else
		bucket = smerMapSlice->bucket;

	return bucket->smers + bucketPosition;
}

static void freeSmerMapEntry(SmerMapEntry *smer) {

}

void smInitSmerMap(SmerMap *smerMap) {
	u32 prime;

	memset(smerMap, 0, sizeof(SmerMap));

	prime = nextPrime(0);

	int i;

	for (i = 0; i < SMER_MAP_SLICES; i++) {
		smerMap->slice[i].prime = prime;
		smerMap->slice[i].entryLimit = ((float) prime) * MAX_LOAD_FACTOR;
		smerMap->slice[i].entryCount = 0;

		smerMap->slice[i].smers = smEntryPtrArrayAlloc(prime);
		smerMap->slice[i].bucket = NULL;

		pthread_rwlock_init(&(smerMap->slice[i].lock), NULL);
	}

	//LOG(LOG_INFO, "Allocated SmerMap with %i slices of %i entries",SMER_MAP_SLICES,prime);
}

void smFreeSmerMap(SmerMap *smerMap) {
	u32 bucketCount = 0;
	u32 smerCount = 0;
	u32 smerLimit = 0;
	u32 smerPrime = 0;

	int i = 0;
	for (i = 0; i < SMER_MAP_SLICES; i++) {
		smerCount += smerMap->slice[i].entryCount;
		smerLimit += smerMap->slice[i].entryLimit;
		smerPrime += smerMap->slice[i].prime;

		if (smerMap->slice[i].smers != NULL)
			smEntryPtrArrayFree(smerMap->slice[i].smers);

		SmerMapEntryBucket *bucket = smerMap->slice[i].bucket;

		while (bucket != NULL) {
			SmerMapEntryBucket *next = bucket->nextBucket;

			int j;
			for (j = 0; j < SMER_BUCKET_SIZE; j++)
				freeSmerMapEntry(bucket->smers + j);

			smEntryBucketFree(bucket);

			bucket = next;
			bucketCount++;
		}

		pthread_rwlock_destroy(&(smerMap->slice[i].lock));
	}

	memset(smerMap, 0, sizeof(SmerMap));

	//LOG(LOG_INFO, "Freed SmerMap with %i entries of %i using %i using %i SmerBuckets of %i",smerCount,smerLimit,smerPrime,bucketCount,sizeof(SmerMapEntryBucket));
}

*/



void smInitSmerMap(SmerMap *smerMap) {

	memset(smerMap, 0, sizeof(SmerMap));

	u32 shift=INIT_SHIFT;
	u32 size=(1<<shift);
	u32 mask=size-1;

	int i,j;

	for (i = 0; i < SMER_MAP_SLICES; i++) {
		smerMap->slice[i].shift = shift;
		smerMap->slice[i].mask = mask;
//LOCKFREE
//		smerMap->slice[i].entryLimit = ((float) size) * MAX_LOAD_FACTOR;
//		smerMap->slice[i].entryCount = 0;

		smerMap->slice[i].smers = smSmerIdArrayAlloc(size);

		for(j=0;j<size;j++)
			smerMap->slice[i].smers[j]=SMER_DUMMY;

		pthread_rwlock_init(&(smerMap->slice[i].lock), NULL);
	}

	LOG(LOG_INFO, "Allocated SmerMap with %i slices of %i entries",SMER_MAP_SLICES,size);

}

void smFreeSmerMap(SmerMap *smerMap) {

//	u32 smerCount = 0;
//	u32 smerLimit = 0;

	int i = 0;
	for (i = 0; i < SMER_MAP_SLICES; i++) {
//		smerCount += smerMap->slice[i].entryCount;
//		smerLimit += smerMap->slice[i].entryLimit;

		if (smerMap->slice[i].smers != NULL)
			smSmerIdArrayFree(smerMap->slice[i].smers);

		pthread_rwlock_destroy(&(smerMap->slice[i].lock));
	}

	memset(smerMap, 0, sizeof(SmerMap));

	//LOG(LOG_INFO, "Freed SmerMap with %i entries of %i using %i using %i SmerBuckets of %i",smerCount,smerLimit,smerPrime,bucketCount,sizeof(SmerMapEntryBucket));
}















// FIXME: 64 vs 32 bits
static u32 hashForSmer(SmerId id) {
	SmerId hash = id;

//   32-bit version
//
//	hash += ~(hash << 9);
//	hash ^= ((hash >> 14) | (hash << 18));
//	hash += (hash << 4);
//	hash ^= ((hash >> 10) | (hash << 22));


	hash += ~(hash << 9);
	hash ^= ((hash >> 46) | (hash << 18));
	hash += (hash << 4);
	hash ^= ((hash >> 42) | (hash << 22));


	return (u32)hash;
}

static SmerMapSlice *sliceForHash(SmerMap *smerMap, u32 hash) {
	//int slice = hash % SMER_MAP_SLICES;

	int slice= (hash ^ (hash >> 16)) & 0x3F;
	return (smerMap->slice) + slice;
}

/*
static SmerMapEntry **scanForSmer_HS(SmerMapSlice *smerMapSlice, SmerId id, u32 hash) {
	u32 prime = smerMapSlice->prime;

	u32 position = hash % prime;

	int scanCount = 0;

	SmerMapEntry **ptr = smerMapSlice->smers + position;
	while ((*ptr) != NULL) {
		if ((*ptr)->id == id)
			return ptr;

		ptr++;
		position++;
		scanCount++;

		if (position >= prime) {
			ptr = smerMapSlice->smers;
			position = 0;
		}
	}

	if (scanCount > 100) {
		u32 originalPosition = hash % prime;
		LOG(LOG_INFO,"Scan count %i from %i to %i",scanCount, originalPosition, position);
	}

	//LOG(LOG_INFO,"Pos: %i Prime: %i Ptr %p",position,prime,ptr);

	return ptr;
}

static int resize_S(SmerMapSlice *smerMapSlice) {
	u32 oldPrime, newPrime;
	SmerMapEntry **newSmers;
	u32 i;

	oldPrime = smerMapSlice->prime;
	newPrime = nextPrime(oldPrime);

	smEntryPtrArrayFree(smerMapSlice->smers);
	smerMapSlice->smers = NULL;

	if (newPrime == 0)
		return 0;

	newSmers = smEntryPtrArrayAlloc(newPrime);

	smerMapSlice->prime = newPrime;
	smerMapSlice->smers = newSmers;
	smerMapSlice->entryLimit = ((float) newPrime) * MAX_LOAD_FACTOR;

	SmerMapEntryBucket *bucket = smerMapSlice->bucket;
	int bucketContents = (smerMapSlice->entryCount) % SMER_BUCKET_SIZE;

	while (bucket != NULL) {
		for (i = 0; i < bucketContents; i++) {
			SmerMapEntry **newPtr;

			SmerId id = bucket->smers[i].id;
			u32 hash = hashForSmer(id);

			newPtr = scanForSmer_HS(smerMapSlice, id, hash);
			*newPtr = bucket->smers + i;
		}

		bucket = bucket->nextBucket;
		bucketContents = SMER_BUCKET_SIZE;
	}

	return smerMapSlice->prime;
}

static SmerMapEntry *findSmer_HS(SmerMapSlice *smerMapSlice, SmerId id, u32 hash) {
	SmerMapEntry **ptr = scanForSmer_HS(smerMapSlice, id, hash);

	if (*ptr != NULL) {
		SmerMapEntry *smer = *ptr;

		if (smer->id != id)
			LOG(LOG_INFO,"Returned wrong smer");
	}

	return *ptr;
}

SmerMapEntry *smFindSmer(SmerMap *smerMap, SmerId id) {
	u32 hash = hashForSmer(id);
	SmerMapSlice *smerMapSlice = sliceForHash(smerMap, hash);

	return findSmer_HS(smerMapSlice, id, hash);
}

static SmerMapEntry *findOrCreateSmer_HS(SmerMapSlice *smerMapSlice, SmerId id,
		u32 hash) {
	SmerMapEntry **ptr = scanForSmer_HS(smerMapSlice, id, hash);

	if ((*ptr) != NULL) {
		SmerMapEntry *smer = *ptr;

		if (smer->id != id)
			LOG(LOG_INFO,"Returned wrong smer");

		return *ptr;
	}

	if (smerMapSlice->entryCount >= smerMapSlice->entryLimit) {
//		int oldSize = smerMapSlice->prime;

		resize_S(smerMapSlice);

		//LOG(LOG_INFO,"Resized from %i to %i",oldSize,smerMapSlice->prime);

		ptr = scanForSmer_HS(smerMapSlice, id, hash);
	}

	//LOG(LOG_INFO,"Need to create %i",id);

	(*ptr) = allocSmerMapEntry(smerMapSlice);
	smerMapSlice->entryCount++;

	(*ptr)->id = id;

	return *ptr;
}

*/

//static
int scanForSmer_HS(SmerMapSlice *smerMapSlice, SmerId id, u32 hash)
{
	u32 mask=smerMapSlice->mask;
	u32 position = hash & mask;

	int scanCount = 0;

	SmerId *ptr = smerMapSlice->smers + position;
	while ((*ptr) != SMER_DUMMY) {
		if ((*ptr) == id)
			return position;

		ptr++;
		position++;
		scanCount++;

		if (position > mask) {
			ptr = smerMapSlice->smers;
			position = 0;
		}
	}

	if (scanCount > 50) {
		LOG(LOG_INFO,"Scan count %i from %i",scanCount,(hash & mask));
	}

	return position;
}


//static
void resize_S(SmerMapSlice *smerMapSlice) {

	u32 oldShift=smerMapSlice->shift;
	u32 oldMask=smerMapSlice->mask;

	u32 shift=oldShift+1;
	u32 size=(1<<shift);

	LOG(LOG_INFO,"Resize to %i",shift);

	smerMapSlice->shift=shift;
	smerMapSlice->mask=size-1;

	SmerId *oldSmers=smerMapSlice->smers;
	smerMapSlice->smers = smSmerIdArrayAlloc(size);

	int i=0;

	for(i=0;i<size;i++)
		smerMapSlice->smers[i]=SMER_DUMMY;

// LOCKFREE
//	smerMapSlice->entryLimit = ((float) size) * MAX_LOAD_FACTOR;

	for(i=0;i<=oldMask;i++)
		{
		SmerId id=oldSmers[i];

		if(id!=SMER_DUMMY)
			{
			u32 hash = hashForSmer(id);

			int index = scanForSmer_HS(smerMapSlice, id, hash);

			smerMapSlice->smers[index]=id;

			}
		}

	smSmerIdArrayFree(oldSmers);

}




//static
int findSmer_HS(SmerMapSlice *smerMapSlice, SmerId id, u32 hash)
{
	//LOG(LOG_INFO,"F Look for %li",id);

	int index = scanForSmer_HS(smerMapSlice, id, hash);

	if (smerMapSlice->smers[index] == id) {
		//LOG(LOG_INFO,"F found %li",id);
		return index;
	}

	//LOG(LOG_INFO,"F not found %li",id);

	return -1;
}


//static
void findOrCreateSmer_HS(SmerMapSlice *smerMapSlice, SmerId id, u32 hash)
{
	while(1)
		{
		int index=scanForSmer_HS(smerMapSlice, id, hash);

		if (smerMapSlice->smers[index]==id) {
			return;
		}

// LOCKFREE
//	if (smerMapSlice->entryCount >= smerMapSlice->entryLimit) {
//		resize_S(smerMapSlice);
//		index = scanForSmer_HS(smerMapSlice, id, hash);
//	}

//	smerMapSlice->entryCount++;

		if(__sync_val_compare_and_swap(smerMapSlice->smers+index, SMER_DUMMY, id)==SMER_DUMMY)
			return;
		}

	//smerMapSlice->smers[index]=id;
}

void smCreateIndexedSmers(SmerMap *smerMap, u32 indexCount, s32 *indexes,
		SmerId *smerIds) {
	int i;

	for (i = 0; i < indexCount; i++) {
		s32 index = indexes[i];

		SmerId smerId = smerIds[index];
		u32 hash = hashForSmer(smerId);
		SmerMapSlice *smerMapSlice = sliceForHash(smerMap, hash);

//LOCKFREE
//		pthread_rwlock_wrlock(&(smerMapSlice->lock));

		findOrCreateSmer_HS(smerMapSlice, smerId, hash);

//LOCKFREE
//		pthread_rwlock_unlock(&(smerMapSlice->lock));
	}
}

u32 smFindIndexesOfExistingSmers(SmerMap *smerMap, u8 *data, s32 maxIndex,
		s32 *oldIndexes, SmerId *smerIds, int maxDistance) {
	u32 oldIndexCount = 0;

	u32 i=maxDistance-1;
	if(i>=maxIndex)
		i=maxIndex;


	u32 earliestToCheck=0,skipLimit=i;

	while(i<=maxIndex)
		{
		//LOG(LOG_INFO,"Test %i",i);

		SmerId smerId = smerIds[i];

		u32 hash = hashForSmer(smerId);
		SmerMapSlice *smerMapSlice = sliceForHash(smerMap, hash);

		if (findSmer_HS(smerMapSlice, smerId, hash) != -1)
			{
			oldIndexes[oldIndexCount++] = i;
			earliestToCheck=i+1;
			i+=maxDistance;
			skipLimit=i;
			}
		else
			{
			if(i<=skipLimit)
				{
				if(i>earliestToCheck)
					i--;
				else
					i=skipLimit+1;
				}
			else
				i++;
			}
	}


/*
	u32 i=0;

	while(i<=maxIndex)
		{
		SmerId smerId = smerIds[i];

		u32 hash = hashForSmer(smerId);
		SmerMapSlice *smerMapSlice = sliceForHash(smerMap, hash);

		if (findSmer_HS(smerMapSlice, smerId, hash) != -1)
			{
			oldIndexes[oldIndexCount++] = i;
			}
		i++;
		}
*/

	return oldIndexCount;
}


void smCreateSmers(SmerMap *smerMap, SmerId *smerIds, u32 smerCount)
{
	int i;

	for (i = 0;i<smerCount;i++)
		{
		SmerId smerId = smerIds[i];
		u32 hash = hashForSmer(smerId);
		SmerMapSlice *smerMapSlice = sliceForHash(smerMap, hash);

		findOrCreateSmer_HS(smerMapSlice, smerId, hash);
		}
}





u32 smGetSmerCount(SmerMap *smerMap) {
	int count = 0;
	int i,j;

	for (i = 0; i < SMER_MAP_SLICES; i++)
	{
		int size=smerMap->slice[i].mask;

		for(j=0;j<=size;j++)
			{
			if(smerMap->slice[i].smers[j]!=SMER_DUMMY)
				count++;
			}
	}

//LOCKFREE
//	for (i = 0; i < SMER_MAP_SLICES; i++)
//		count += smerMap->slice[i].entryCount;

	return count;
}


void smGetSortedSmerIds(SmerMap *smerMap, SmerId *array)
{
	int i,j;
	SmerId *ptr=array;

	for (i = 0; i < SMER_MAP_SLICES; i++)
		{
		SmerMapSlice *smerMapSlice=smerMap->slice+i;

		for(j=0;j<=smerMapSlice->mask;j++)
			{
			SmerId id=smerMapSlice->smers[j];
			if(id!=SMER_DUMMY)
				*(ptr++)=id;
			}
		}

	int count=ptr-array;

	qsort(array, count, sizeof(SmerId), smerIdcompar);

}



void smGetSmerPathCounts(SmerMap *smerMap, SmerId *smerIds, u32 *smerPaths)
{
	/*
	int i;

	for (i = 0; i < SMER_MAP_SLICES; i++)
		{
		SmerMapSlice *smerMapSlice = smerMap->slice + i;

		SmerMapEntryBucket *bucket = smerMapSlice->bucket;
		int maxBucketPosition = (smerMapSlice->entryCount) % SMER_BUCKET_SIZE;

		while (bucket != NULL) {
			int j;

			for (j = 0; j < maxBucketPosition; j++) {
				*(smerIds++) = bucket->smers[j].id;
				*(smerPaths++) = bucket->smers[j].pathCount;
			}

			bucket = bucket->nextBucket;
			maxBucketPosition = SMER_BUCKET_SIZE;
		}

	}
	*/
}



