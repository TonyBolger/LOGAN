#include "common.h"


//	Old Format:
//		#PrefixTails(2B), PrefixTailData(packed), #SuffixTails(2B), SuffixTailData(packed), RouteHeader(2-10B), ForwardRoutes(packed), ReverseRoutes(packed)
//
//
//	Future Formats:
//
//  Compact Format: Tail counts in first byte (<=7)
//		0 0 P P P S S S,  PrefixTailData(packed), SuffixTailData(packed), RouteHeader(2-10B), ForwardRoutes(packed), ReverseRoutes(packed)
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//


static int forwardPrefixSorter(const void *a, const void *b)
{
	RoutePatch *pa=(RoutePatch *)a;
	RoutePatch *pb=(RoutePatch *)b;

	int diff=pa->prefixIndex-pb->prefixIndex;
	if(diff)
		return diff;

	return pa->rdiPtr-pb->rdiPtr;
	//return pa->rdiIndex-pb->rdiIndex; // Follow original order
}

static int reverseSuffixSorter(const void *a, const void *b)
{
	RoutePatch *pa=(RoutePatch *)a;
	RoutePatch *pb=(RoutePatch *)b;

	int diff=pa->suffixIndex-pb->suffixIndex;
	if(diff)
		return diff;

	return pa->rdiPtr-pb->rdiPtr;
//	return pa->rdiIndex-pb->rdiIndex; // Follow original order
//	return pb->rdiIndex-pa->rdiIndex; // Reverse original order
}




static void compactSliceData(SmerArraySlice *slice, MemDispenser *disp)
{
	int smerCount=slice->smerCount;
	MemPackStackRetain *retains=dAlloc(disp, sizeof(MemPackStackRetain)*smerCount);

	for(int i=0;i<smerCount;i++)
		{
		retains[i].index=i;
		u8 *ptr=slice->smerData[i];
		u8 *scanPtr;

		scanPtr=scanTails(ptr);
		scanPtr=scanTails(scanPtr);
		scanPtr=scanRouteTable(scanPtr);

		int size=scanPtr-ptr;

		retains[i].oldPtr=ptr;
		retains[i].size=size;
		}

	slice->slicePackStack=psCompact(slice->slicePackStack, retains, smerCount);

	//LOG(LOG_INFO,"Compacted stack: Size %i: Peak: %i Realloc: %i",slice->slicePackStack->currentSize, slice->slicePackStack->peakAlloc, slice->slicePackStack->realloc);

	for(int i=0;i<smerCount;i++)
		slice->smerData[retains[i].index]=retains[i].newPtr;

}




int rtRouteReadsForSmer(RoutingReadReferenceBlock *rdi, u32 sliceIndex, SmerArraySlice *slice, RoutingReadData **orderedDispatches, MemDispenser *disp)
{
	u8 *smerData=slice->smerData[sliceIndex];

	//if(smerData!=NULL)
//		LOG(LOG_INFO, "Index: %i of %i Entries %i Data: %p",sliceIndex,slice->smerCount, rdi->entryCount, smerData);

	SeqTailBuilder prefixBuilder, suffixBuilder;
	RouteTableBuilder routeTableBuilder;

	u8 *tmp;

	tmp=smerData;
	smerData=initSeqTailBuilder(&prefixBuilder, smerData, disp);
	int oldSizePrefix=smerData-tmp;

	tmp=smerData;
	smerData=initSeqTailBuilder(&suffixBuilder, smerData, disp);
	int oldSizeSuffix=smerData-tmp;

	tmp=smerData;
	smerData=initRouteTableBuilder(&routeTableBuilder, smerData, disp);
	int oldSizeRoutes=smerData-tmp;

	int oldSize=smerData-slice->smerData[sliceIndex];

	int entryCount=rdi->entryCount;

	int forwardCount=0;
	int reverseCount=0;

	RoutePatch *forwardPatches=dAlloc(disp,sizeof(RoutePatch)*entryCount);
	RoutePatch *reversePatches=dAlloc(disp,sizeof(RoutePatch)*entryCount);

//	memset(forwardPatches,0,sizeof(RoutePatch)*entryCount);
//	memset(reversePatches,0,sizeof(RoutePatch)*entryCount);

	//int debug=0;

	// First, lookup tail indexes, creating tails if necessary

	s32 maxNewPrefix=0;
	s32 maxNewSuffix=0;

	SmerId smerId=SMER_DUMMY;

	for(int i=0;i<entryCount;i++)
	{
		RoutingReadData *rdd=rdi->entries[i];

		int index=rdd->indexCount;

		if(0)
		{
			SmerId currSmer=rdd->fsmers[index];
			SmerId prevSmer=rdd->fsmers[index+1];
			SmerId nextSmer=rdd->fsmers[index-1];

			int upstreamLength=rdd->readIndexes[index]-rdd->readIndexes[index+1];
			int downstreamLength=rdd->readIndexes[index-1]-rdd->readIndexes[index];

			char bufferP[SMER_BASES+1]={0};
			char bufferC[SMER_BASES+1]={0};
			char bufferN[SMER_BASES+1]={0};

			unpackSmer(prevSmer, bufferP);
			unpackSmer(currSmer, bufferC);
			unpackSmer(nextSmer, bufferN);

			LOG(LOG_INFO,"Read Orientation: %s (%i) %s %s (%i)",bufferP, upstreamLength, bufferC, bufferN, downstreamLength);
		}

		SmerId currFmer=rdd->fsmers[index];
		SmerId currRmer=rdd->rsmers[index];

		if(currFmer<=currRmer) // Canonical Read Orientation
			{
				smerId=currFmer;
				//LOG(LOG_INFO,"Adding forward route to %li",currFmer);

				SmerId prefixSmer=rdd->rsmers[index+1]; // Previous smer in read, reversed
				SmerId suffixSmer=rdd->fsmers[index-1]; // Next smer in read

				int prefixLength=rdd->readIndexes[index]-rdd->readIndexes[index+1];
				int suffixLength=rdd->readIndexes[index-1]-rdd->readIndexes[index];

				forwardPatches[forwardCount].next=NULL;
				forwardPatches[forwardCount].rdiPtr=rdi->entries+i;
//				forwardPatches[forwardCount].rdiIndex=i;

				forwardPatches[forwardCount].prefixIndex=findOrCreateSeqTail(&prefixBuilder, prefixSmer, prefixLength);
				forwardPatches[forwardCount].suffixIndex=findOrCreateSeqTail(&suffixBuilder, suffixSmer, suffixLength);

				maxNewPrefix=MAX(maxNewPrefix,forwardPatches[forwardCount].prefixIndex);
				maxNewSuffix=MAX(maxNewSuffix,forwardPatches[forwardCount].suffixIndex);


				if(0)
					{
					char bufferP[SMER_BASES+1]={0};
					char bufferN[SMER_BASES+1]={0};
					char bufferS[SMER_BASES+1]={0};

					unpackSmer(prefixSmer, bufferP);
					unpackSmer(currFmer, bufferN);
					unpackSmer(suffixSmer, bufferS);

					LOG(LOG_INFO,"Node Orientation: %s (%i) @ %i %s %s (%i) @ %i",
							bufferP, prefixLength, forwardPatches[forwardCount].prefixIndex,  bufferN, bufferS, suffixLength, forwardPatches[forwardCount].suffixIndex);
					}

				forwardCount++;
			}
		else	// Reverse-complement Read Orientation
			{
				smerId=currRmer;
				//LOG(LOG_INFO,"Adding reverse route to %li",currRmer);

				SmerId prefixSmer=rdd->fsmers[index-1]; // Next smer in read
				SmerId suffixSmer=rdd->rsmers[index+1]; // Previous smer in read, reversed


				int prefixLength=rdd->readIndexes[index-1]-rdd->readIndexes[index];
				int suffixLength=rdd->readIndexes[index]-rdd->readIndexes[index+1];

				reversePatches[reverseCount].next=NULL;
				reversePatches[reverseCount].rdiPtr=rdi->entries+i;
//				reversePatches[reverseCount].rdiIndex=i;

				reversePatches[reverseCount].prefixIndex=findOrCreateSeqTail(&prefixBuilder, prefixSmer, prefixLength);
				reversePatches[reverseCount].suffixIndex=findOrCreateSeqTail(&suffixBuilder, suffixSmer, suffixLength);

				maxNewPrefix=MAX(maxNewPrefix,reversePatches[reverseCount].prefixIndex);
				maxNewSuffix=MAX(maxNewSuffix,reversePatches[reverseCount].suffixIndex);

				if(0)
					{
					char bufferP[SMER_BASES+1]={0};
					char bufferN[SMER_BASES+1]={0};
					char bufferS[SMER_BASES+1]={0};

					unpackSmer(prefixSmer, bufferP);
					unpackSmer(currRmer, bufferN);
					unpackSmer(suffixSmer, bufferS);

					LOG(LOG_INFO,"Node Orientation: %s (%i) @ %i %s %s (%i) @ %i",
							bufferP, prefixLength, reversePatches[forwardCount].prefixIndex,  bufferN, bufferS, suffixLength, reversePatches[forwardCount].suffixIndex);
					}

				reverseCount++;
			}
	}

	// Then sort new forward and reverse routes, if more than one

	if(forwardCount>1)
		{
		qsort(forwardPatches, forwardCount, sizeof(RoutePatch), forwardPrefixSorter);
/*
		if(forwardCount>100)
			{
			LOG(LOG_INFO,"Forward Patches %i",forwardCount);
			dumpPatches(forwardPatches, forwardCount);
			}
*/
		}

	if(reverseCount>1)
		{
		qsort(reversePatches, reverseCount, sizeof(RoutePatch), reverseSuffixSorter);
/*
		if(reverseCount>100)
			{
			LOG(LOG_INFO,"Reverse Patches %i",reverseCount);
			dumpPatches(reversePatches, reverseCount);
			}
*/
		}

/*
	if(debug)
		{
		LOG(LOG_INFO,"Builder has Size: %i Max: %i %i %i Count: %i %i",
				routeTableBuilder.totalPackedSize,
				routeTableBuilder.maxPrefix,routeTableBuilder.maxSuffix,routeTableBuilder.maxWidth,
				routeTableBuilder.oldForwardEntryCount,routeTableBuilder.oldReverseEntryCount);
		LOG(LOG_INFO,"Adding %i %i",forwardCount,reverseCount);
		}
*/
	mergeRoutes(&routeTableBuilder, forwardPatches, reversePatches, forwardCount, reverseCount, maxNewPrefix, maxNewSuffix, orderedDispatches, disp);

	/*
	for(int i=0;i<forwardCount;i++)
		*(orderedDispatches++)=*(forwardPatches[i].rdiPtr);

	for(int i=0;i<reverseCount;i++)
		*(orderedDispatches++)=*(reversePatches[i].rdiPtr);
*/

	int oldRouteEntries=routeTableBuilder.oldForwardEntryCount+routeTableBuilder.oldReverseEntryCount;

	int newRouteEntries=
			MAX(routeTableBuilder.oldForwardEntryCount,routeTableBuilder.newForwardEntryCount)+
			MAX(routeTableBuilder.oldReverseEntryCount,routeTableBuilder.newReverseEntryCount);

	if(newRouteEntries>16083)
		{
		int oldShifted=oldRouteEntries>>14;
		int newShifted=newRouteEntries>>14;

		if(oldShifted!=newShifted)
			{
			char bufferN[SMER_BASES+1]={0};
			unpackSmer(smerId, bufferN);

			LOG(LOG_INFO,"LARGE SMER: %s %012lx has %i %i %i %i",bufferN,smerId,
				routeTableBuilder.oldForwardEntryCount,routeTableBuilder.oldReverseEntryCount,
				routeTableBuilder.newForwardEntryCount,routeTableBuilder.newReverseEntryCount);
			}
		}

/*
	if(debug)
		LOG(LOG_INFO,"Builder has Size: %i Max: %i %i %i Count: %i %i %i %i",
				routeTableBuilder.totalPackedSize,
				routeTableBuilder.maxPrefix,routeTableBuilder.maxSuffix,routeTableBuilder.maxWidth,
				routeTableBuilder.oldForwardEntryCount,routeTableBuilder.oldReverseEntryCount,
				routeTableBuilder.newForwardEntryCount,routeTableBuilder.newReverseEntryCount);
*/
	if(getSeqTailBuilderDirty(&prefixBuilder) || getSeqTailBuilderDirty(&suffixBuilder) || getRouteTableBuilderDirty(&routeTableBuilder))
		{
		int prefixPackedSize=getSeqTailBuilderPackedSize(&prefixBuilder);
		int suffixPackedSize=getSeqTailBuilderPackedSize(&suffixBuilder);
		int routeTablePackedSize=getRouteTableBuilderPackedSize(&routeTableBuilder);

		int diffPrefix=prefixPackedSize-oldSizePrefix;
		int diffSuffix=suffixPackedSize-oldSizeSuffix;
		int diffRoutes=routeTablePackedSize-oldSizeRoutes;

		int totalSize=prefixPackedSize+suffixPackedSize+routeTablePackedSize;
		int sizeDiff=totalSize-oldSize;

		/*
		LOG(LOG_INFO,"Slice Alloc: %i %i (%i %i) %i %i (%i %i) %i %i (%i %i %i %i)",
				prefixPackedSize, diffPrefix, prefixBuilder.oldTailCount,prefixBuilder.newTailCount,
				suffixPackedSize, diffSuffix, suffixBuilder.oldTailCount,suffixBuilder.newTailCount,
				routeTablePackedSize, diffRoutes,
				routeTableBuilder.oldForwardEntryCount,routeTableBuilder.oldReverseEntryCount,
				routeTableBuilder.newForwardEntryCount,routeTableBuilder.newReverseEntryCount);
*/

		slice->totalAlloc+=sizeDiff;

		slice->totalAllocPrefix+=diffPrefix;
		slice->totalAllocSuffix+=diffSuffix;
		slice->totalAllocRoutes+=diffRoutes;

		slice->totalRealloc+=oldSize;

		u8 *oldData=slice->smerData[sliceIndex];
		u8 *newData;



//		LOG(LOG_INFO,"Was %i Now %i",oldSize,totalSize);

		if(oldData!=NULL)
			newData=psRealloc(slice->slicePackStack, oldData, oldSize, totalSize);
		else
			newData=psAlloc(slice->slicePackStack, totalSize);

		if(newData==NULL)
			{
			compactSliceData(slice, disp);

			if(oldData!=NULL)
				newData=psRealloc(slice->slicePackStack, oldData, oldSize, totalSize);
			else
				newData=psAlloc(slice->slicePackStack, totalSize);
			}

		if(newData==NULL)
				LOG(LOG_CRITICAL,"Failed at alloc after compact: Wanted %i",totalSize);

		u8 *dataTmp=writeSeqTailBuilderPackedData(&prefixBuilder, newData);
		dataTmp=writeSeqTailBuilderPackedData(&suffixBuilder, dataTmp);
		dataTmp=writeRouteTableBuilderPackedData(&routeTableBuilder, dataTmp);



		slice->smerData[sliceIndex]=newData;
		}

	return entryCount;
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

SmerLinked *rtGetLinkedSmer(SmerArray *smerArray, SmerId rootSmerId, MemDispenser *disp)
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


