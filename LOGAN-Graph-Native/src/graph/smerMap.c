
#include "../common.h"

static const float MAX_LOAD_FACTOR = 0.45; // 0.68 -> 0.65 -> 0.60: Reduced to eliminate large scan counts

static const u32 INIT_SHIFT=10;

void smInitSmerMap(SmerMap *smerMap) {

	memset(smerMap, 0, sizeof(SmerMap));

	u32 shift=INIT_SHIFT;
	u32 size=(1<<shift);
	u32 mask=size-1;

	int i,j;

	for (i = 0; i < SMER_SLICES; i++) {
		smerMap->slice[i].shift = shift;
		smerMap->slice[i].mask = mask;

		smerMap->slice[i].smers = smSmerEntryArrayAlloc(size);

		for(j=0;j<size;j++)
			smerMap->slice[i].smers[j]=SMER_DUMMY;
	}

	LOG(LOG_INFO, "Allocated SmerMap with %i slices of %i entries",SMER_SLICES,size);

}

void smCleanupSmerMap(SmerMap *smerMap) {

	int i = 0;
	for (i = 0; i < SMER_SLICES; i++) {
		if (smerMap->slice[i].smers != NULL)
			smSmerEntryArrayFree(smerMap->slice[i].smers);
	}

	memset(smerMap, 0, sizeof(SmerMap));
}

/*

static int scanForSmer_HS(SmerMapSlice *smerMapSlice, SmerEntry entry, u32 hash, u32 sliceNum)
{
	u32 mask=smerMapSlice->mask;
	u32 position = hash & mask;

	int scanCount = 0;

	SmerEntry *ptr = smerMapSlice->smers + position;
	while ((*ptr) != SMER_DUMMY) {
		if ((*ptr) == entry)
			return position;

		position++;
		ptr++;
		scanCount++;

		if (position > mask) {
			ptr = smerMapSlice->smers;
			position = 0;

			if (scanCount > 200) {
				LOG(LOG_CRITICAL,"Filled hashmap: count %i from %i in %i for %i",scanCount,(hash & mask), sliceNum, entry);
				exit(1);
			}
		}
	}

	if (scanCount > 60) {
		LOG(LOG_INFO,"Scan count %i from %i in %i for %i",scanCount,(hash & mask), sliceNum, entry);
	}

	return position;
}
*/

static int scanForSmer_HS(SmerMapSlice *smerMapSlice, SmerEntry entry, u32 hash, u32 sliceNum)
{
	u32 mask=smerMapSlice->mask;
	u32 position = hash & mask;
	int scanCount = 0;

	SmerEntry tmp=__atomic_load_n((smerMapSlice->smers + position), __ATOMIC_SEQ_CST);

	while (tmp != SMER_DUMMY)
		{
		if (tmp == entry)
			return position;

		position++;
		scanCount++;

		if (position > mask)
			{
			position = 0;

			if (scanCount > 200)
				{
				LOG(LOG_CRITICAL,"Filled hashmap: count %i from %i in %i for %x (got %x)",scanCount,(hash & mask), sliceNum, entry, tmp);
				exit(1);
				}
			}

		tmp=__atomic_load_n((smerMapSlice->smers + position), __ATOMIC_SEQ_CST);
		}

	if (scanCount > 60) {
		LOG(LOG_INFO,"Scan count %i from %i in %i for %i",scanCount,(hash & mask), sliceNum, entry);
	}

	return position;
}



static int findSmer_HS(SmerMapSlice *smerMapSlice, SmerEntry entry, u32 hash, u32 sliceNo)
{
	//LOG(LOG_INFO,"F Look for %li",id);

	int index = scanForSmer_HS(smerMapSlice, entry, hash, sliceNo);

	if (smerMapSlice->smers[index] == entry) {
		//LOG(LOG_INFO,"F found %li",id);
		return index;
	}

	//LOG(LOG_INFO,"F not found %li",id);

	return -1;
}


static void findOrCreateSmer_HS(SmerMapSlice *smerMapSlice, SmerEntry entry, u32 hash, u32 sliceNo)
{
	while(1)
		{
		int index=scanForSmer_HS(smerMapSlice, entry, hash, sliceNo);

		if (__atomic_load_n(smerMapSlice->smers+index, __ATOMIC_SEQ_CST)==entry)
			return;

		SmerEntry current=SMER_DUMMY;

		if(__atomic_compare_exchange_n(smerMapSlice->smers+index, &current, entry, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
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
		int sliceNo=sliceForSmer(smerId, hash);
		SmerMapSlice *smerMapSlice = smerMap->slice+sliceNo;
		SmerEntry entry=SMER_GET_BOTTOM(smerId);

		findOrCreateSmer_HS(smerMapSlice, entry, hash & smerMapSlice->mask, sliceNo);
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
		int sliceNo=sliceForSmer(smerId, hash);
		SmerMapSlice *smerMapSlice = smerMap->slice+sliceNo;
		SmerEntry entry=SMER_GET_BOTTOM(smerId);

		if (findSmer_HS(smerMapSlice, entry, hash & smerMapSlice->mask, sliceNo) != -1)
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
		int sliceNo=sliceForSmer(smerId, hash);
		SmerMapSlice *smerMapSlice = smerMap->slice+sliceNo;
		SmerEntry entry=SMER_GET_BOTTOM(smerId);

		findOrCreateSmer_HS(smerMapSlice, entry, hash & smerMapSlice->mask, sliceNo);
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

	SmerEntry *oldEntries=smerMapSlice->smers;
	smerMapSlice->smers = smSmerEntryArrayAlloc(size);

	int i=0;

	for(i=0;i<size;i++)
		smerMapSlice->smers[i]=SMER_DUMMY;

	for(i=0;i<=oldMask;i++)
		{
		SmerEntry entry=oldEntries[i];

		if(entry!=SMER_DUMMY)
			{
			u64 hash = hashForSmer(entry);

			int index = scanForSmer_HS(smerMapSlice, entry, hash, sliceNo);

			smerMapSlice->smers[index]=entry;

			}
		}
	smSmerEntryArrayFree(oldEntries);
}



void smConsiderResize(SmerMap *smerMap, int sliceNum)
{
	SmerMapSlice *slice=smerMap->slice+sliceNum;

	u32 pop=smGetSmerCount_S(slice);
	//LOG(LOG_INFO, "Slice %i has %i",sliceNum,pop);

	if(pop>(slice->mask*MAX_LOAD_FACTOR))
		resize_S(slice, sliceNum);

}

static void smDumpSmerMapSlice(SmerMapSlice *smerMapSlice, int sliceNum)
{
	int count = 0;
	int i;

	int size=smerMapSlice->mask;

	for(i=0;i<=size;i++)
		{
		SmerEntry entry=smerMapSlice->smers[i];

		if(entry!=SMER_DUMMY)
			{
			char buffer[SMER_BASES+1];

			SmerId smer=recoverSmerId(sliceNum, entry);

			unpackSmer(smer, buffer);
			LOG(LOG_INFO,"SLICE %05i HASH %016lx OFFSET %i SMER %012lx %s",sliceNum, hashForSmer(smer), i, smer, buffer);

			count++;
			}
		}

	LOG(LOG_INFO,"SLICE %i SIZE %i",sliceNum, count);
}


void smDumpSmerMap(SmerMap *smerMap)
{
	int i;

	for (i = 0; i < SMER_SLICES; i++)
		smDumpSmerMapSlice(smerMap->slice+i, i);
}


u32 smGetSmerSliceCount(SmerMapSlice *smerMapSlice)
{
	int count = 0;
	int i;

	int size=smerMapSlice->mask;

	for(i=0;i<=size;i++)
		{
		SmerEntry entry=smerMapSlice->smers[i];

		if(entry!=SMER_DUMMY)
			count++;
		}

	return count;
}



void smGetSortedSliceSmerEntries(SmerMapSlice *smerMapSlice, SmerEntry *array)
{
	int i;
	SmerEntry *ptr=array;

	for(i=0;i<=smerMapSlice->mask;i++)
		{
		SmerEntry entry=smerMapSlice->smers[i];
		if(entry!=SMER_DUMMY)
			{
			SmerId id=recoverSmerId(i, entry);
			*(ptr++)=id;
			}
		}

	int count=ptr-array;

	qsort(array, count, sizeof(SmerEntry), smerEntryCompar);
}



u32 smGetSmerCount(SmerMap *smerMap) {
	int total = 0;
	int i;

	for (i = 0; i < SMER_SLICES; i++)
		{
		int count=smGetSmerSliceCount(smerMap->slice+i);
		total+=count;
		}

	return total;
}




/*


void smGetSortedSmerIds(SmerMap *smerMap, SmerId *array)
{
	int i,j;
	SmerId *ptr=array;

	for (i = 0; i < SMER_SLICES; i++)
		{
		SmerMapSlice *smerMapSlice=smerMap->slice+i;

		for(j=0;j<=smerMapSlice->mask;j++)
			{
			SmerEntry entry=smerMapSlice->smers[j];
			if(entry!=SMER_DUMMY)
				{
				SmerId id=recoverSmerId(i, entry);
				*(ptr++)=id;
				}
			}
		}

	int count=ptr-array;

	qsort(array, count, sizeof(SmerId), smerIdCompar);
}

*/

void smGetSmerPathCounts(SmerMap *smerMap, SmerId *smerIds, u32 *smerPaths)
{
	/*
	int i;

	for (i = 0; i < SMER_MAP_SLICES; i++)
		{
		SmerMapSlice *smerMapSlice = smerMap->slice + i;

		SmerEntryBucket *bucket = smerMapSlice->bucket;
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



