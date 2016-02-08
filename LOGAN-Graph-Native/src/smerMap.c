#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "common.h"

static const float MAX_LOAD_FACTOR = 0.60; // 0.68 -> 0.65 -> 0.60: Reduced to eliminate large scan counts

// Ecoli-1_Q20 gives 1.8M smers - 0.45M/ slice(4)
// Ecoli-2_Q20 gives 5.7M smers - 1.4M / slice(4)
// set to 14 for testing - hits 50% limit at about 100K reads per slice on Ecoli-2_Q20
//

static const u32 INIT_SHIFT=7;

//static const u32 INIT_SHIFT=18;

//static const u32 HASH_PRIME=43;
//static const u64 HASH_FACTOR=1719;
//static const u32 SLICE_FACTOR=455;

void smInitSmerMap(SmerMap *smerMap) {

	memset(smerMap, 0, sizeof(SmerMap));

	u32 shift=INIT_SHIFT;
	u32 size=(1<<shift);
	u32 mask=size-1;

	int i,j;

	for (i = 0; i < SMER_MAP_SLICES; i++) {
		smerMap->slice[i].shift = shift;
		smerMap->slice[i].mask = mask;

		smerMap->slice[i].smers = smSmerIdArrayAlloc(size);

		for(j=0;j<size;j++)
			smerMap->slice[i].smers[j]=SMER_DUMMY;

		pthread_rwlock_init(&(smerMap->slice[i].lock), NULL);
	}

	LOG(LOG_INFO, "Allocated SmerMap with %i slices of %i entries",SMER_MAP_SLICES,size);

}

void smFreeSmerMap(SmerMap *smerMap) {

	int i = 0;
	for (i = 0; i < SMER_MAP_SLICES; i++) {
		if (smerMap->slice[i].smers != NULL)
			smSmerIdArrayFree(smerMap->slice[i].smers);

		pthread_rwlock_destroy(&(smerMap->slice[i].lock));
	}

	memset(smerMap, 0, sizeof(SmerMap));
}


/*
 *
 *

static const uint64_t k0 = 0xc3a5c85c97cb3127ULL;
static const uint64_t k1 = 0xb492b66fbe98f273ULL;
static const uint64_t k2 = 0x9ae16a3b2f90404fULL;

static inline uint64_t hash_len_16(uint64_t u, uint64_t v, uint64_t mul)
{
uint64_t a, b;
a = (u ^ v) * mul;
a ^= (a >> 47);
b = (v ^ a) * mul;
b ^= (b >> 47);
b *= mul;
return b;
}

uint64_t mul = k2 + len * 2;
uint64_t a = fetch32(s);
return hash_len_16(len + (a << 3), fetch32(s + len - 4), mul);




 *
 *
 */


static const u64 sc_const = 0xdeadbeefdeadbeefLL;

static u64 Rot64(u64 x, int k)
{
   return (x << k) | (x >> (64 - k));
}

/*
static void ShortMix(u64 *h0p, u64 *h1p, u64 *h2p, u64 *h3p)
 {
	u64 h0=*h0p;
	u64 h1=*h1p;
	u64 h2=*h2p;
	u64 h3=*h3p;

     h2 = Rot64(h2,50);  h2 += h3;  h0 ^= h2;
     h3 = Rot64(h3,52);  h3 += h0;  h1 ^= h3;
     h0 = Rot64(h0,30);  h0 += h1;  h2 ^= h0;
     h1 = Rot64(h1,41);  h1 += h2;  h3 ^= h1;
     h2 = Rot64(h2,54);  h2 += h3;  h0 ^= h2;
     h3 = Rot64(h3,48);  h3 += h0;  h1 ^= h3;
     h0 = Rot64(h0,38);  h0 += h1;  h2 ^= h0;
     h1 = Rot64(h1,37);  h1 += h2;  h3 ^= h1;
     h2 = Rot64(h2,62);  h2 += h3;  h0 ^= h2;
     h3 = Rot64(h3,34);  h3 += h0;  h1 ^= h3;
     h0 = Rot64(h0,5);   h0 += h1;  h2 ^= h0;
     h1 = Rot64(h1,36);  h1 += h2;  h3 ^= h1;

     *h0p=h0;
     *h1p=h1;
     *h2p=h2;
     *h3p=h3;

 }

static void ShortEnd(u64 *h0p, u64 *h1p, u64 *h2p, u64 *h3p)
{
	u64 h0=*h0p;
	u64 h1=*h1p;
	u64 h2=*h2p;
	u64 h3=*h3p;

    h3 ^= h2;  h2 = Rot64(h2,15);  h3 += h2;
    h0 ^= h3;  h3 = Rot64(h3,52);  h0 += h3;
    h1 ^= h0;  h0 = Rot64(h0,26);  h1 += h0;
    h2 ^= h1;  h1 = Rot64(h1,51);  h2 += h1;
    h3 ^= h2;  h2 = Rot64(h2,28);  h3 += h2;
    h0 ^= h3;  h3 = Rot64(h3,9);   h0 += h3;
    h1 ^= h0;  h0 = Rot64(h0,47);  h1 += h0;
    h2 ^= h1;  h1 = Rot64(h1,54);  h2 += h1;
    h3 ^= h2;  h2 = Rot64(h2,32);  h3 += h2;
    h0 ^= h3;  h3 = Rot64(h3,25);  h0 += h3;
    h1 ^= h0;  h0 = Rot64(h0,63);  h1 += h0;

    *h0p=h0;
    *h1p=h1;
    *h2p=h2;
    *h3p=h3;
}

*/

static const u64 k0 = 0xc3a5c85c97cb3127ULL;
static const u64 k1 = 0xb492b66fbe98f273ULL;
static const u64 k2 = 0x9ae16a3b2f90404fULL;

static u64 chash64(u64 u, u64 v, u64 mul, int s1, int s2)
{
	u64 a, b;

	a = (u ^ v) * mul;
	a ^= (a >> s1);
	b = (v ^ a) * mul;
	b ^= (b >> s2);
	b *= mul;

	return b;
}

// FIXME: 64 vs 32 bits
static u64 hashForSmer(SmerId id) {
//	SmerId hash = id;

//   32-bit version
//
//	hash += ~(hash << 9);
//	hash ^= ((hash >> 14) | (hash << 18));
//	hash += (hash << 4);
//	hash ^= ((hash >> 10) | (hash << 22));

/*
	hash += ~(hash << 9);
	hash ^= ((hash >> 46) | (hash << 18));
	hash += (hash << 4);
	hash ^= ((hash >> 42) | (hash << 22));

	return (u32)hash;
*/
	//SmerId hash = SMER_GET_BOTTOM(id) * HASH_PRIME;

	u64 v = SMER_GET_BOTTOM(id);
	v ^= (v << 35);

	u64 u = 4 + (v << 3);

	u64 a = chash64(u,v,k2+4, 47, 47);

	return a ^ Rot64(a, 17) ^ Rot64(a,29);

	/*u64 b = chash64(u,v,k1, 43, 49);
	u64 c = chash64(u,v,k2, 45, 41);

	ShortMix(&a,&b,&c,&u);

	ShortEnd(&a,&b,&c,&v);

	return a ^ Rot64(b,5);
*/
}

static u32 sliceForSmer(SmerMap *smerMap, SmerId smer, u64 hash) {
	//int slice = hash % SMER_MAP_SLICES;

	return ((smer ^ hash) >> 32) & SMER_MAP_SLICE_MASK;

}

static SmerId recoverSmerId(u64 slice, SmerId smer)
{
	slice <<= 32;

	u64 hash=hashForSmer(smer);
	u64 comp = hash & ((u64)SMER_MAP_SLICE_MASK << 32);

	return (slice ^ comp) | SMER_GET_BOTTOM(smer);
}


static int scanForSmer_HS(SmerMapSlice *smerMapSlice, SmerId id, u32 hash, u32 sliceNo)
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

	if (scanCount > 40) {
		LOG(LOG_INFO,"Scan count %i from %i in %i for %012lx",scanCount,(hash & mask), sliceNo, id);
	}

	return position;
}




static int findSmer_HS(SmerMapSlice *smerMapSlice, SmerId id, u32 hash, u32 sliceNo)
{
	//LOG(LOG_INFO,"F Look for %li",id);

	int index = scanForSmer_HS(smerMapSlice, id, hash, sliceNo);

	if (smerMapSlice->smers[index] == id) {
		//LOG(LOG_INFO,"F found %li",id);
		return index;
	}

	//LOG(LOG_INFO,"F not found %li",id);

	return -1;
}


static void findOrCreateSmer_HS(SmerMapSlice *smerMapSlice, SmerId id, u32 hash, u32 sliceNo)
{
	while(1)
		{
		int index=scanForSmer_HS(smerMapSlice, id, hash, sliceNo);

		if (smerMapSlice->smers[index]==id)
			return;

		if(__sync_val_compare_and_swap(smerMapSlice->smers+index, SMER_DUMMY, id)==SMER_DUMMY)
			return;
		}
}

void smCreateIndexedSmers(SmerMap *smerMap, u32 indexCount, s32 *indexes,
		SmerId *smerIds) {
	int i;

	for (i = 0; i < indexCount; i++) {
		s32 index = indexes[i];

		SmerId smerId = smerIds[index];
		u64 hash = hashForSmer(smerId);
		int sliceNo=sliceForSmer(smerMap, smerId, hash);
		SmerMapSlice *smerMapSlice = smerMap->slice+sliceNo;

		findOrCreateSmer_HS(smerMapSlice, smerId, hash & smerMapSlice->mask, sliceNo);
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

		u64 hash = hashForSmer(smerId);
		int sliceNo=sliceForSmer(smerMap, smerId, hash);
		SmerMapSlice *smerMapSlice = smerMap->slice+sliceNo;

		if (findSmer_HS(smerMapSlice, smerId, hash & smerMapSlice->mask, sliceNo) != -1)
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
		u64 hash = hashForSmer(smerId);
		int sliceNo=sliceForSmer(smerMap, smerId, hash);
		SmerMapSlice *smerMapSlice = smerMap->slice+sliceNo;

		findOrCreateSmer_HS(smerMapSlice, smerId, hash & smerMapSlice->mask, sliceNo);
		}
}


static u32 smGetSmerCount_S(SmerMapSlice *smerMapSlice)
{
	int count = 0;
	int i;

	int size=smerMapSlice->mask;

	for(i=0;i<=size;i++)
		{
		if(smerMapSlice->smers[i]!=SMER_DUMMY)
			count++;
		}

	return count;
}





static void resize_S(SmerMapSlice *smerMapSlice, u32 sliceNo) {

	u32 oldShift=smerMapSlice->shift;
	u32 oldMask=smerMapSlice->mask;

	u32 shift=oldShift+1;
	u32 size=(1<<shift);

	//LOG(LOG_INFO,"Resize to %i",shift);

	smerMapSlice->shift=shift;
	smerMapSlice->mask=size-1;

	SmerId *oldSmers=smerMapSlice->smers;
	smerMapSlice->smers = smSmerIdArrayAlloc(size);

	int i=0;

	for(i=0;i<size;i++)
		smerMapSlice->smers[i]=SMER_DUMMY;

	for(i=0;i<=oldMask;i++)
		{
		SmerId id=oldSmers[i];

		if(id!=SMER_DUMMY)
			{
			u32 hash = hashForSmer(id);

			int index = scanForSmer_HS(smerMapSlice, id, hash, sliceNo);

			smerMapSlice->smers[index]=id;

			}
		}
	smSmerIdArrayFree(oldSmers);
}



void smConsiderResize(SmerMap *smerMap, int sliceNum)
{
	SmerMapSlice *slice=smerMap->slice+sliceNum;

	u32 pop=smGetSmerCount_S(slice);
	//LOG(LOG_INFO, "Slice %i has %i",sliceNum,pop);

	if(pop>(slice->mask*MAX_LOAD_FACTOR))
		resize_S(slice, sliceNum);

}

static u32 smGetSmerCountDump(SmerMapSlice *smerMapSlice, int sliceNum)
{
	int count = 0;
	int i;

	int size=smerMapSlice->mask;
	char buffer[SMER_BASES+1];

	for(i=0;i<=size;i++)
		{
		SmerId smer=smerMapSlice->smers[i];

		if(smer!=SMER_DUMMY)
			{
			unpackSmer(smer, buffer);
			LOG(LOG_INFO,"SLICE %i HASH %li OFFSET %i SMER %012lx %s",sliceNum, hashForSmer(smer), i, smer, buffer);

			SmerId recSmer=recoverSmerId(sliceNum, smer);

			if(recSmer!=smer)
				LOG(LOG_INFO,"MISMATCH");

			count++;
			}
		}

	LOG(LOG_INFO,"SLICE %i SIZE %i",sliceNum, count);

	return count;
}



u32 smGetSmerCount(SmerMap *smerMap) {
	int total = 0;
	int i;

	for (i = 0; i < SMER_MAP_SLICES; i++)
		{
		int count=smGetSmerCountDump(smerMap->slice+i, i);
		total+=count;
		}

	return total;
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



