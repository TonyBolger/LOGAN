

#include "common.h"

void setSmerArraySliceEntries(SmerArraySlice *arraySlice, SmerEntry *smerEntries, s32 smerCount, s32 isSiit)
{
	if(arraySlice->smerIT!=NULL)
		siitFreeImplicitTree(arraySlice->smerIT, arraySlice->smerCount);

	if(arraySlice->smerData!=NULL)
		smSmerDataArrayFree(arraySlice->smerData, arraySlice->smerCount);

	if(arraySlice->bloom.data!=NULL)
		freeBloom(&(arraySlice->bloom));


	arraySlice->smerIT=siitInitImplicitTree(smerEntries,smerCount, isSiit);
	arraySlice->smerData=smSmerDataArrayAlloc(smerCount);

	for(int j=0;j<smerCount;j++)
		arraySlice->smerData[j]=NULL;

	arraySlice->smerCount=smerCount;

	Bloom *bloom=&(arraySlice->bloom);
	initBloom(bloom, smerCount, 4, 8);

	for(int j=0;j<smerCount;j++)
		setBloom(bloom,smerEntries[j]);


}


s32 saInitSmerArray(SmerArray *smerArray, SmerMap *smerMap) {

	memset(smerArray, 0, sizeof(SmerArray));

	SmerArraySlice *arraySlices=smerArray->slice;
	SmerMapSlice *mapSlices=smerMap->slice;

	int i;

	for(i=0;i<SMER_DISPATCH_GROUPS;i++)
		//smerArray->heaps[i]=colHeapAlloc(itemSizeResolver);
		smerArray->heaps[i]=mhHeapAlloc(rtReclaimIndexer, rtRelocater);

	int total=0;
	for(i=0;i<SMER_SLICES; i++)
		{
		int count=smGetSmerSliceCount(mapSlices+i);
		total+=count;

		SmerEntry *smerTmp=smSmerEntryArrayAlloc(count);
		smGetSortedSliceSmerEntries(mapSlices+i,smerTmp);
/*
		for(int j=0;j<count;j++)
			{
			//LOG(LOG_INFO,"Smer: %lx in %i",smerTmp[j], i);

			SmerId smerId=recoverSmerId(i, smerTmp[j]);
			u8 smerBuf[SMER_BASES+1];
			unpackSmer(smerId, smerBuf);

			SmerId cSmerId=complementSmerId(smerId);
			u8 cSmerBuf[SMER_BASES+1];
			unpackSmer(cSmerId, cSmerBuf);

			LOG(LOG_INFO,"SMER: %s %012lx %s %012lx", smerBuf, smerId, cSmerBuf, cSmerId);
			}
*/

		setSmerArraySliceEntries(&arraySlices[i], smerTmp, count, 0);

		smSmerEntryArrayFree(smerTmp, count);

		MemHeap *circHeap=smerArray->heaps[i>>SMER_DISPATCH_GROUP_SHIFT];
		mhHeapRegisterTagData(circHeap,i&SMER_DISPATCH_GROUP_SLICEMASK,arraySlices[i].smerData, arraySlices[i].smerCount);

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

#ifdef FEATURE_ENABLE_SMER_STATS
	                   //012345678901  01234567890123456789012
	LOGN(LOG_INFO,"STAT: SmerID      \tSmerBases              \tPtail\tPbyte\tStail\tSbyte\t"
			"RFmt\tRFe\tRFr\tRRe\tRRr\tRe\tRr\t"
			"RAbyte\t"
			"RTTopB\tRTAryB\t"
			"RTlB\tRTloB\tRTleB\t"
			"RTbB\tRTboB\tRTbcB\t"
			"RTByte\tByte");
	MemDispenser *disp=dispenserAlloc(MEMTRACKID_DISPENSER_STATS, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);
#endif
	for (i = 0; i < SMER_SLICES; i++)
		{
		if (smerArray->slice[i].smerIT != NULL)
			{
			int smerCount=smerArray->slice[i].smerCount;

#ifdef FEATURE_ENABLE_SMER_STATS
			SmerRoutingStats *stats=rtGetRoutingStats(smerArray->slice+i, i, disp);

			for(int j=0;j<smerCount;j++)
				{
				s64 frEntries=stats[j].routeTableForwardRouteEntries+stats[j].routeTableReverseRouteEntries;
				s64 frRoutes=stats[j].routeTableForwardRoutes+stats[j].routeTableReverseRoutes;

				LOGN(LOG_INFO,"SMER: %012x\t%s\t%i\t%i\t%i\t%i\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li\t%li",
						stats[j].smerId, stats[j].smerStr,
						stats[j].prefixTails, stats[j].prefixBytes, stats[j].suffixTails, stats[j].suffixBytes, stats[j].routeTableFormat,
						stats[j].routeTableForwardRouteEntries, stats[j].routeTableForwardRoutes, stats[j].routeTableReverseRouteEntries, stats[j].routeTableReverseRoutes,
						frEntries, frRoutes,
						stats[j].routeTableArrayBytes, stats[j].routeTableTreeTopBytes, stats[j].routeTableTreeArrayBytes,
						stats[j].routeTableTreeLeafBytes, stats[j].routeTableTreeLeafOffsetBytes, stats[j].routeTableTreeLeafEntryBytes,
						stats[j].routeTableTreeBranchBytes, stats[j].routeTableTreeBranchOffsetBytes, stats[j].routeTableTreeBranchChildBytes,
						stats[j].routeTableTotalBytes, stats[j].smerTotalBytes);
				}

			// 00001b1ec071	AGTAAAGACGTACTGTAAACTAC	14	45	11	38	2	451	8688	370	7118	821	15806	0	48	392	7272	1188	5808	56	0	48	7768	7863
			// 000000000000	AAAAAAAAAAAAAAAAAAAAAAA	4	8	8	16	2	55054	1890803	12093	267641	67147	2158444	0	48	1032	496074	3408	491418	224	0	216	497378	497414

			dispenserReset(disp);
#endif
			siitFreeImplicitTree(smerArray->slice[i].smerIT, smerCount);
			smSmerDataArrayFree(smerArray->slice[i].smerData, smerCount);
			}

		freeBloom(&(smerArray->slice[i].bloom));
		}

#ifdef FEATURE_ENABLE_SMER_STATS
	dispenserFree(disp);
#endif

	for(i=0;i<SMER_DISPATCH_GROUPS;i++)
		//colHeapFree(smerArray->heaps[i]);
		mhHeapFree(smerArray->heaps[i]);

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



int saGetSliceSmerIds(SmerArraySlice *smerArraySlice, int sliceNum, SmerId *smerIdArray)
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



void saSetSliceSmerIds(SmerArray *smerArray, int sliceNum, SmerId *smerIdArray, int smerCount, MemDispenser *disp)
{
	SmerArraySlice *smerArraySlice=&(smerArray->slice[sliceNum]);
	SmerEntry *smerEntries=dAlloc(disp, sizeof(SmerEntry)*smerCount);

	for(int i=0;i<smerCount;i++)
		{
		SmerId smerId=smerIdArray[i];
		int slice=sliceForSmer(smerId, hashForSmer(smerId));

		if(slice!=sliceNum)
			LOG(LOG_CRITICAL,"Smer %012lx - %i", smerIdArray[i], slice);

		smerEntries[i]=SMER_GET_BOTTOM(smerId);
		}

	// Hacky - needed because smer array retrieval doesn't maintain order
	qsort(smerEntries, smerCount, sizeof(SmerEntry), smerEntryCompar);

	setSmerArraySliceEntries(smerArraySlice, smerEntries, smerCount, 0);

	MemHeap *circHeap=smerArray->heaps[sliceNum>>SMER_DISPATCH_GROUP_SHIFT];
	mhHeapRegisterTagData(circHeap,sliceNum&SMER_DISPATCH_GROUP_SLICEMASK,smerArraySlice->smerData, smerArraySlice->smerCount);
}

void saSetSliceSmerEntries(SmerArray *smerArray, int sliceNum, SmerEntry *smerEntries, int smerCount)
{
	SmerArraySlice *smerArraySlice=&(smerArray->slice[sliceNum]);

	setSmerArraySliceEntries(smerArraySlice, smerEntries, smerCount, 1);

	MemHeap *circHeap=smerArray->heaps[sliceNum>>SMER_DISPATCH_GROUP_SHIFT];
	mhHeapRegisterTagData(circHeap,sliceNum&SMER_DISPATCH_GROUP_SLICEMASK,smerArraySlice->smerData, smerArraySlice->smerCount);

}


