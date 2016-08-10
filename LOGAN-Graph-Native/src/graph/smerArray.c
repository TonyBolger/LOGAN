

#include "common.h"


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

		arraySlices[i].slicePackStack=packStackAlloc();

		arraySlices[i].totalAlloc=0;
		arraySlices[i].totalAllocPrefix=0;
		arraySlices[i].totalAllocSuffix=0;
		arraySlices[i].totalAllocRoutes=0;

		arraySlices[i].totalRealloc=0;
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

		/*
		LOG(LOG_INFO,"Slice %i has %i smers with %li alloc (%i %i %i) and %li realloc",i,smerArray->slice[i].smerCount,
				smerArray->slice[i].totalAlloc,
				smerArray->slice[i].totalAllocPrefix,
				smerArray->slice[i].totalAllocSuffix,
				smerArray->slice[i].totalAllocRoutes,
				smerArray->slice[i].totalRealloc)
*/

		packStackFree(smerArray->slice[i].slicePackStack);
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


s32 saFindSmer(SmerArray *smerArray, SmerId smerId)
{
	u64 hash = hashForSmer(smerId);
	int sliceNum=sliceForSmer(smerId, hash);

	SmerEntry smerEntry=SMER_GET_BOTTOM(smerId);
	SmerArraySlice *slice=smerArray->slice+sliceNum;

	s32 index=saFindSmerEntry(slice, smerEntry);
	return index;
}


u8 *saFindSmerAndData(SmerArray *smerArray, SmerId smerId, s32 *sliceNumPtr, s32 *indexPtr)
{
	u64 hash = hashForSmer(smerId);
	int sliceNum=sliceForSmer(smerId, hash);

	SmerEntry smerEntry=SMER_GET_BOTTOM(smerId);
	SmerArraySlice *slice=smerArray->slice+sliceNum;
	if(sliceNumPtr!=NULL)
		*sliceNumPtr=sliceNum;

	s32 index=saFindSmerEntry(slice, smerEntry);
	if(indexPtr!=NULL)
		*indexPtr=index;

	if(index>=0)
		return slice->smerData[index];

	return NULL;
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







/*


typedef struct smerLinkedStr
{
	SmerId smerId;
	u32 compFlag;

	u64 *prefixData;
	SmerId *prefixSmers;
	u8 *prefixSmerExists;
	u32 prefixCount;

	u64 *suffixData;
	SmerId *suffixSmers;
	u8 *suffixSmerExists;
	u32 suffixCount;

	RouteTableEntry *forwardRouteEntries;
	RouteTableEntry *reverseRouteEntries;
	u32 forwardRouteCount;
	u32 reverseRouteCount;

} SmerLinked;



 */

SmerLinked *saGetLinkedSmer(SmerArray *smerArray, SmerId rootSmerId, MemDispenser *disp)
{
	int sliceNum, index;
	u8 *data=saFindSmerAndData(smerArray, rootSmerId, &sliceNum, &index);

	if(index<0)
		{
		//LOG(LOG_INFO,"Linked Smer: Not Found");
		return NULL;
		}

	SmerLinked *smerLinked=dAlloc(disp,sizeof(SmerLinked));
	smerLinked->smerId=rootSmerId;

	if(data==NULL)
		{
		//LOG(LOG_INFO,"Linked Smer: No data");
		memset(smerLinked,0,sizeof(SmerLinked));
		}
	else
		{
		//LOG(LOG_INFO,"Linked Smer: Got data 1");

		data=unpackPrefixesForSmerLinked(smerLinked, data, disp);

		data=unpackSuffixesForSmerLinked(smerLinked, data, disp);

		unpackRouteTableForSmerLinked(smerLinked, data, disp);

		for(int i=0;i<smerLinked->prefixCount;i++)
			if(saFindSmer(smerArray, smerLinked->prefixSmers[i])<0)
				smerLinked->prefixSmerExists[i]=0;

		for(int i=0;i<smerLinked->suffixCount;i++)
			if(saFindSmer(smerArray, smerLinked->suffixSmers[i])<0)
				smerLinked->suffixSmerExists[i]=0;

		//LOG(LOG_INFO,"Linked Smer: Got data 4 - %i %i",smerLinked->forwardRouteCount, smerLinked->reverseRouteCount);
		}

	smerLinked->smerId=rootSmerId;

	return smerLinked;
}



u32 saGetSmerCount(SmerArray *smerArray) {
	int total = 0;

	for (int i = 0; i < SMER_SLICES; i++)
		total+=smerArray->slice[i].smerCount;

	return total;
}

static int saGetSliceSmerIds(SmerArraySlice *smerArraySlice, int sliceNum, SmerId *smerIdArray)
{
	SmerId *ptr=smerIdArray;

	for(int i=0;i<smerArraySlice->smerCount;i++)
		{
		SmerEntry entry=smerArraySlice->smerIT[i];
		SmerId id=recoverSmerId(sliceNum, entry);
		*(ptr++)=id;
		}

	int count=ptr-smerIdArray;

	return count;
}



void saGetSmerIds(SmerArray *smerArray, SmerId *smerIds)
{
	for (int i = 0; i < SMER_SLICES; i++)
		{
		int count=saGetSliceSmerIds(smerArray->slice+i, i, smerIds);
		smerIds+=count;
		}
}





