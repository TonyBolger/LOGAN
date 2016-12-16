

#include "common.h"


s32 saInitSmerArray(SmerArray *smerArray, SmerMap *smerMap) {

	memset(smerArray, 0, sizeof(SmerArray));

	SmerArraySlice *arraySlices=smerArray->slice;
	SmerMapSlice *mapSlices=smerMap->slice;

	int i;

	for(i=0;i<SMER_DISPATCH_GROUPS;i++)
		//smerArray->heaps[i]=colHeapAlloc(itemSizeResolver);
		smerArray->heaps[i]=circHeapAlloc(rtReclaimIndexer, rtRelocater);

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

		MemCircHeap *circHeap=smerArray->heaps[i>>SMER_DISPATCH_GROUP_SHIFT];
		circHeapRegisterTagData(circHeap,i&SMER_DISPATCH_GROUP_SLICEMASK,arraySlices[i].smerData, arraySlices[i].smerCount);

//		LOG(LOG_INFO,"Slice %i contains %i",i,arraySlices[i].smerCount);


//		MemColHeap *colHeap=smerArray->heaps[i>>SMER_DISPATCH_GROUP_SHIFT];
//		chRegisterRoots(colHeap,i&SMER_DISPATCH_GROUP_SLICEMASK,arraySlices[i].smerData, arraySlices[i].smerCount);

		//arraySlices[i].slicePackStack=packStackAlloc();

		/*
		arraySlices[i].totalAlloc=0;
		arraySlices[i].totalAllocPrefix=0;
		arraySlices[i].totalAllocSuffix=0;
		arraySlices[i].totalAllocRoutes=0;

		arraySlices[i].totalRealloc=0;
		*/
		}

	return total;
}

/*
s32 prefixTails;
s32 prefixBytes;

s32 suffixTails;
s32 suffixBytes;

s32 routeTableFormat; // 0 - null, 1 - array, 2 - tree

s64 routeTableForwardRouteEntries;
s64 routeTableForwardRoutes;

s64 routeTableReverseRouteEntries;
s64 routeTableReverseRoutes;

s64 routeTableArrayBytes;

s64 routeTableTreeTopBytes;
s64 routeTableTreeArrayBytes;
s64 routeTableTreeLeafBytes;
s64 routeTableTreeBranchBytes;

s64 routeTableTotalBytes;
s64 smerTotalBytes;
*/

void saCleanupSmerArray(SmerArray *smerArray) {

	int i = 0;

	                   //012345678901234567890123
	LOGN(LOG_INFO,"STAT: SmerBases               \tPtail\tPbyte\tStail\tSbyte\tRFmt\tRFe\tRFr\tRRe\tRRr\tRAbyte\tRTTopB\tRTAryB\tRTlB\tRTbB\tRTByte\tByte");

	MemDispenser *disp=dispenserAlloc("Stats",DISPENSER_BLOCKSIZE_MEDIUM);

	for (i = 0; i < SMER_SLICES; i++)
		{
		if (smerArray->slice[i].smerIT != NULL)
			{
			SmerRoutingStats *stats=rtGetRoutingStats(smerArray->slice+i, i, disp);
			int smerCount=smerArray->slice[i].smerCount;

			for(int j=0;j<smerCount;j++)
				{
				LOGN(LOG_INFO,"SMER: %s\t%i\t%i\t%i\t%i\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li",
						stats[j].smerStr, stats[j].prefixTails, stats[j].prefixBytes, stats[j].suffixTails, stats[j].suffixBytes, stats[j].routeTableFormat,
						stats[j].routeTableForwardRouteEntries, stats[j].routeTableForwardRoutes, stats[j].routeTableReverseRouteEntries, stats[j].routeTableReverseRoutes,
						stats[j].routeTableArrayBytes, stats[j].routeTableTreeTopBytes, stats[j].routeTableTreeArrayBytes, stats[j].routeTableTreeLeafBytes, stats[j].routeTableTreeBranchBytes,
						stats[j].routeTableTotalBytes, stats[j].smerTotalBytes);
				}

			dispenserReset(disp);

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

		//packStackFree(smerArray->slice[i].slicePackStack);
		}

	dispenserFree(disp);

	for(i=0;i<SMER_DISPATCH_GROUPS;i++)
		//colHeapFree(smerArray->heaps[i]);
		circHeapFree(smerArray->heaps[i]);

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





