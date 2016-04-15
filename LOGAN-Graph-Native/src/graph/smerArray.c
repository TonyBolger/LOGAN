

#include "../common.h"


s32 saInitSmerArray(SmerArray *smerArray, SmerMap *smerMap) {

	memset(smerArray, 0, sizeof(SmerArray));

	SmerArraySlice *arraySlices=smerArray->slice;
	SmerMapSlice *mapSlices=smerMap->slice;

	int i;
	int total=0;

	for(i=0;i<SMER_SLICES; i++)
		{
		int count=smGetSmerSliceCount(mapSlices+i);
		total+=count;

		SmerEntry *smerTmp=smSmerEntryArrayAlloc(count);
		smGetSortedSliceSmerEntries(mapSlices+i,smerTmp);

		arraySlices[i].smerIT=siitInitImplicitTree(smerTmp,count);
		arraySlices[i].smerData=smSmerDataArrayAlloc(count);

		for(int j=0;j<count;j++)
			arraySlices[i].smerData[j]=NULL;

		arraySlices[i].smerCount=count;

		Bloom *bloom=&(arraySlices[i].bloom);

		initBloom(bloom, count, 4, 8);

		for(int j=0;j<count;j++)
			setBloom(bloom,smerTmp[j]);

		smSmerEntryArrayFree(smerTmp);
		}

	return total;
}

void saCleanupSmerArray(SmerArray *smerArray) {

	int i = 0;
	for (i = 0; i < SMER_SLICES; i++)
		{
		if (smerArray->slice[i].smerIT != NULL)
			{
			smSmerEntryArrayFree(smerArray->slice[i].smerIT);
			smSmerDataArrayFree(smerArray->slice[i].smerData);
			}

		freeBloom(&(smerArray->slice[i].bloom));
		}

	memset(smerArray, 0, sizeof(SmerArray));
}


int saFindSmerEntry(SmerArraySlice *slice, SmerEntry smerEntry)
{
	if(!testBloom(&(slice->bloom),smerEntry))
		return -1;

	s32 index=siitFindSmer(slice->smerIT, slice->smerCount, smerEntry);

	return index;
}


int saFindSmer(SmerArray *smerArray, SmerId smerId)
{
	u64 hash = hashForSmer(smerId);
	int sliceNo=sliceForSmer(smerId, hash);

	SmerEntry smerEntry=SMER_GET_BOTTOM(smerId);
	SmerArraySlice *slice=smerArray->slice+sliceNo;

	s32 index=saFindSmerEntry(slice, smerEntry);

	/*
	if(index!=-1)
		{
		char buffer[SMER_BASES+1];

		unpackSmer(smerId, buffer);
		LOG(LOG_INFO,"%05i %016lx %012lx %s",sliceNo, hash, smerId, buffer);
		}
*/

	return index;
}





u32 saFindIndexesOfExistingSmers(SmerArray *smerArray, u8 *data, s32 maxIndex,
		s32 *oldIndexes, SmerId *smerIds) {
	u32 oldIndexCount = 0;
	u32 i;

	for (i = 0; i <= maxIndex; i++) {
		SmerId smerId = smerIds[i];

		int smerIndex=saFindSmer(smerArray, smerId);
		if (smerIndex != -1)
			oldIndexes[oldIndexCount++] = i;
	}

	return oldIndexCount;
}



void saVerifyIndexing(s32 maxAllowedDistance, s32 *indexes, u32 indexCount, int dataLength, int maxValidIndex)
{
		//LOG(LOG_INFO, "Maximum index distance %i",maxAllowedDistance);
		int i=0;

		int prevIndex=-1;
		s32 dist;
		s32 maxDist=0;

		for(i=0;i<indexCount;i++)
			{
			s32 index=indexes[i];

			dist=index-prevIndex;
			if(dist>maxDist)
				maxDist=dist;

			prevIndex=index;
			}

		dist=maxValidIndex-prevIndex;
		if(dist>maxDist)
			maxDist=dist;

		if(maxDist>maxAllowedDistance)
			LOG(LOG_WARNING, "Warning: Asked to add path of %i with %i indexes at max dist %i", dataLength,indexCount,maxDist);
}




