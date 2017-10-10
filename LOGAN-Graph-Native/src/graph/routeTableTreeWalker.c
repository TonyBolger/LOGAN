#include "common.h"




void rttwInitTreeWalker(RouteTableTreeWalker *walker, RouteTableTreeProxy *treeProxy)
{
	walker->treeProxy=treeProxy;

	walker->branchProxy=treeProxy->rootProxy;
	walker->branchChildSibdex=-1;

	walker->leafProxy=NULL;
	walker->leafArrayIndex=-1;
	walker->leafEntryIndex=-1;

	walker->upstreamOffsetCount=0;
	walker->upstreamLeafOffsets=NULL;
	walker->upstreamEntryOffsets=NULL;

	walker->downstreamOffsetCount=0;
	walker->downstreamLeafOffsets=NULL;
	walker->downstreamEntryOffsets=NULL;
}


void rttwDumpWalker(RouteTableTreeWalker *walker)
{
	LOG(LOG_CRITICAL,"PackLeaf: dumpWalker TODO");

	/*
	LOG(LOG_INFO,"Walker Dump: Starts");

	if(walker->branchProxy!=NULL)
		{
		LOG(LOG_INFO,"Branch: %i ",walker->branchProxy->brindex);

		}
	else
		LOG(LOG_INFO,"Branch: NULL");

	if(walker->leafProxy!=NULL)
		{
		LOG(LOG_INFO,"Leaf: %i Entry: %i",walker->leafProxy->lindex, walker->leafEntry);
		dumpLeafBlock(walker->leafProxy->dataBlock);
		}
	else
		LOG(LOG_INFO,"Leaf: NULL");

	if(walker->upstreamOffsets!=NULL)
		{
		LOGS(LOG_INFO,"Walker UpstreamOffsets: ");

		for(int j=0;j<walker->upstreamOffsetCount;j++)
			{
			LOGS(LOG_INFO,"%i ",walker->upstreamOffsets[j]);
			if((j&0x1F)==0x1F)
				LOGN(LOG_INFO,"");
			}
		LOGN(LOG_INFO,"");
		}
	else
		LOG(LOG_INFO,"Walker UpstreamOffsets: NULL");

	if(walker->downstreamOffsets!=NULL)
		{
		LOGS(LOG_INFO,"Walker DownstreamOffsets: ");

		for(int j=0;j<walker->downstreamOffsetCount;j++)
			{
			LOGS(LOG_INFO,"%i ",walker->downstreamOffsets[j]);
			if((j&0x1F)==0x1F)
				LOGN(LOG_INFO,"");
			}
		LOGN(LOG_INFO,"");
		}
	else
		LOG(LOG_INFO,"Walker DownstreamOffsets: NULL");

	LOG(LOG_INFO,"Walker Dump: Ends");
	*/
}

void rttwAppendNewLeaf(RouteTableTreeWalker *walker)
{
	if(walker->branchProxy==NULL)
		LOG(LOG_CRITICAL,"Not on a valid branch");

	walker->leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->treeProxy->upstreamCount, walker->treeProxy->downstreamCount, ROUTEPACKING_ENTRYARRAYS_CHUNK);
	treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, walker->leafProxy, &walker->branchProxy, &walker->branchChildSibdex);

	walker->leafArrayIndex=-1;
	walker->leafEntryIndex=-1;
}


void rttwSeekStart(RouteTableTreeWalker *walker)
{
	RouteTableTreeBranchProxy *branchProxy=NULL;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 branchChildSibdex=0;

	treeProxySeekStart(walker->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy);

	walker->branchProxy=branchProxy;
	walker->branchChildSibdex=branchChildSibdex;
	walker->leafProxy=leafProxy;

	if(leafProxy==NULL)
		{
		rttwAppendNewLeaf(walker);
		leafProxy=walker->leafProxy;
		}

	walker->leafArrayIndex=0;
	walker->leafEntryIndex=0;
	walker->leafEntryArray=NULL;
}

void rttwSeekEnd(RouteTableTreeWalker *walker)
{
	RouteTableTreeBranchProxy *branchProxy=NULL;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 branchChildSibdex=0;

	treeProxySeekEnd(walker->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy);

	walker->branchProxy=branchProxy;
	walker->branchChildSibdex=branchChildSibdex;
	walker->leafProxy=leafProxy;

	if(leafProxy!=NULL)
		{
		LOG(LOG_CRITICAL,"PackLeaf: walkerSeekEnd TODO");
//		walker->leafEntry=leafProxy->entryCount;
//		validateRouteTableTreeLeafOffsets(leafProxy);
		}
	else
		{
		walker->leafArrayIndex=-1;
		walker->leafEntryIndex=-1;

		walker->leafEntryArray=NULL;
		}

}

s32 rttwGetCurrentEntry(RouteTableTreeWalker *walker, s16 *upstream, RouteTableUnpackedEntry **entry)
{
	if((walker->leafProxy==NULL)||(walker->leafArrayIndex==-1)||(walker->leafEntryIndex==-1))
		{
		*upstream=32767;
		*entry=NULL;

		return 0;
		}

	RouteTableUnpackedEntryArray *array=walker->leafProxy->unpackedBlock->entryArrays[walker->leafArrayIndex];

	*upstream=array->upstream;
	*entry=array->entries+walker->leafEntryIndex;

	return 1;
}


static s32 ensureSpaceForArray(RouteTableTreeWalker *walker)
{
	RouteTableUnpackedSingleBlock *block=walker->leafProxy->unpackedBlock;

	if(block->entryArrayCount>=ROUTEPACKING_ENTRYARRAYS_MAX)
		{
//		LOG(LOG_INFO,"ensureSpaceForArray");
		rttlMarkDirty(walker->treeProxy, walker->leafProxy);

		RouteTableTreeLeafProxy *newLeafProxy=treeProxySplitLeaf(walker->treeProxy, walker->branchProxy, walker->branchChildSibdex, walker->leafProxy, walker->leafArrayIndex,
				&walker->branchProxy, &walker->branchChildSibdex, &walker->leafProxy, &walker->leafArrayIndex);

		rttlMarkDirty(walker->treeProxy, newLeafProxy);

		return 1;
		}

	return 0;
}


static s32 ensureSpaceForEntries(RouteTableTreeWalker *walker, int entryCount)
{
	RouteTableUnpackedSingleBlock *block=walker->leafProxy->unpackedBlock;

	if(walker->leafArrayIndex<0 || walker->leafArrayIndex>block->entryArrayCount)
		LOG(LOG_CRITICAL,"Invalid array index %i",walker->leafArrayIndex);

	RouteTableUnpackedEntryArray *array=block->entryArrays[walker->leafArrayIndex];

	if((array->entryCount + entryCount) > ROUTEPACKING_ENTRYS_MAX)
		{
//		LOG(LOG_INFO,"ensureSpaceForEntries");

		if(ensureSpaceForArray(walker))
			block=walker->leafProxy->unpackedBlock;

		rtpSplitArray(block, walker->leafArrayIndex, walker->leafEntryIndex, &(walker->leafArrayIndex), &(walker->leafEntryIndex));

		return 1;
		}

	return 0;
}




static void walkerApplyLeafOffsets(RouteTableTreeWalker *walker, RouteTableTreeLeafProxy *leafProxy)
{
	RouteTableUnpackedSingleBlock *block=leafProxy->unpackedBlock;

	int upstreamValidOffsets=MIN(walker->upstreamOffsetCount, block->upstreamOffsetAlloc);

	for(int i=0;i<upstreamValidOffsets;i++)
		walker->upstreamLeafOffsets[i]+=block->upstreamLeafOffsets[i];

	int downstreamValidOffsets=MIN(walker->downstreamOffsetCount, block->downstreamOffsetAlloc);

	for(int i=0;i<downstreamValidOffsets;i++)
		walker->downstreamLeafOffsets[i]+=block->downstreamLeafOffsets[i];
}

static void walkerApplyArrayOffsets(RouteTableTreeWalker *walker, RouteTableTreeLeafProxy *leafProxy, int arrayIndex)
{
	RouteTableUnpackedSingleBlock *block=leafProxy->unpackedBlock;
	RouteTableUnpackedEntryArray *array=block->entryArrays[arrayIndex];

	s32 *downstreamOffsets=walker->downstreamEntryOffsets;
	s32 upstreamOffset=0;

	for(int i=0;i<array->entryCount;i++)
		{
		s32 downstream=array->entries[i].downstream;
		s32 width=array->entries[i].width;

		downstreamOffsets[downstream]+=width;
		upstreamOffset+=width;
		}

	walker->upstreamEntryOffsets[array->upstream]+=upstreamOffset;

}

static void walkerTransferOffsetsLeafToEntry(RouteTableTreeWalker *walker)
{
	memcpy(walker->upstreamEntryOffsets, walker->upstreamLeafOffsets, sizeof(s32)*walker->upstreamOffsetCount);
	memcpy(walker->downstreamEntryOffsets, walker->downstreamLeafOffsets, sizeof(s32)*walker->downstreamOffsetCount);
}



// walkerAdvance Helpers

static s32 walkerAdvanceGetLastUpstream(RouteTableTreeWalker *walker)
{
	RouteTableUnpackedSingleBlock *block=walker->leafProxy->unpackedBlock;
/*
	rttlEnsureFullyUnpacked(walker->treeProxy, walker->leafProxy);
	s32 arrayIndex=block->entryArrayCount-1;
	return (arrayIndex>=0)?block->entryArrays[arrayIndex]->upstream:-1;
*/

	for(int i=block->upstreamOffsetAlloc;i>0;--i)
		{
		if(block->upstreamLeafOffsets[i]>0)
			return i;
		}
	return -1;
}

static s32 walkerAdvanceToNextLeaf(RouteTableTreeWalker *walker, s32 ensureUnpacked)
{
//	LOG(LOG_INFO,"walkerAdvanceToNextNonEmptyLeaf");

	if(!getNextLeafSibling(walker->treeProxy, &(walker->branchProxy), &(walker->branchChildSibdex), &(walker->leafProxy))) // No more leaves
		{
//		LOG(LOG_INFO,"No more leaves");
		return 0;
		}

	walker->leafArrayIndex=0;
	walker->leafEntryIndex=0;

	if(ensureUnpacked)
		{
		rttlEnsureFullyUnpacked(walker->treeProxy, walker->leafProxy);
		if(walker->leafArrayIndex < walker->leafProxy->unpackedBlock->entryArrayCount)
			walker->leafEntryArray=walker->leafProxy->unpackedBlock->entryArrays[walker->leafArrayIndex];
		else
			walker->leafEntryArray=NULL;
		}
	else
		walker->leafEntryArray=NULL;

	return 1;
}

static s32 walkerAdvanceToNextArray(RouteTableTreeWalker *walker)
{
//	LOG(LOG_INFO,"walkerAdvanceToNextArray");

	walker->leafArrayIndex++;

	if(walker->leafArrayIndex>=walker->leafProxy->unpackedBlock->entryArrayCount)
		return walkerAdvanceToNextLeaf(walker, 1);

	walker->leafEntryIndex=0;
	walker->leafEntryArray=walker->leafProxy->unpackedBlock->entryArrays[walker->leafArrayIndex];

	return 1;
}



static s32 walkerAdvanceToNextLeaf_targetUpstream(RouteTableTreeWalker *walker, s32 targetUpstream)
{
//	LOG(LOG_INFO,"walkerAdvanceToNextNonEmptyLeaf");

	RouteTableTreeBranchProxy *branchProxy=walker->branchProxy;
	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;
	s16 branchChildSibdex=walker->branchChildSibdex;

	RouteTableUnpackedSingleBlock *unpackedBlock=NULL;
	if(!getNextLeafSibling(walker->treeProxy, &branchProxy, &branchChildSibdex, &leafProxy)) // No more leaves
		{
//		LOG(LOG_INFO,"No more leaves");
		return 0;
		}

	rttlEnsureFullyUnpacked(walker->treeProxy, walker->leafProxy);

	unpackedBlock=leafProxy->unpackedBlock;

	// First shouldn't happen
	if(unpackedBlock->entryArrayCount == 0 || unpackedBlock->entryArrays[0]->upstream>targetUpstream)
		return 0;

	walker->branchProxy=branchProxy;
	walker->leafProxy=leafProxy;
	walker->branchChildSibdex=branchChildSibdex;

	walker->leafArrayIndex=0;
	walker->leafEntryIndex=0;

	walker->leafEntryArray=unpackedBlock->entryArrays[0];

	return 1;
}


static s32 walkerAdvanceToNextArray_targetUpstream(RouteTableTreeWalker *walker, s32 targetUpstream)
{
//	LOG(LOG_INFO,"walkerAdvanceToNextArray");

	s16 leafArrayIndex=walker->leafArrayIndex+1;

	if(leafArrayIndex>=walker->leafProxy->unpackedBlock->entryArrayCount)
		return walkerAdvanceToNextLeaf_targetUpstream(walker, targetUpstream);
	else
		{
		RouteTableUnpackedEntryArray *leafEntryArray=walker->leafProxy->unpackedBlock->entryArrays[leafArrayIndex];

		if(leafEntryArray->upstream>targetUpstream)
			return 0;

		walker->leafArrayIndex=leafArrayIndex;
		walker->leafEntryIndex=0;
		walker->leafEntryArray=leafEntryArray;

		return 1;
		}
}

/*
static s32 walkerAdvanceToNextEntry_targetUpstream(RouteTableTreeWalker *walker, s32 targetUpstream)
{
	walker->leafEntryIndex++;

	if(walker->leafEntryIndex>=walker->leafEntryArray->entryCount)
		{
		return walkerAdvanceToNextArray_targetUpstream(walker, targetUpstream);
		}

	return 1;
}
*/

/*

// Split version: For profiling of each step

static s32 walkerAdvanceToUpstreamThenOffsetThenDownstream_1(RouteTableTreeWalker *walker, s32 targetUpstream, s32 targetDownstream, s32 targetMinOffset, s32 targetMaxOffset,
		s32 *upstreamPtr, RouteTableUnpackedEntry **entryPtr, s32 *upstreamOffsetPtr, s32 *downstreamOffsetPtr)
{
//	LOG(LOG_INFO,"walkerAdvanceToUpstreamThenOffsetThenDownstream BEGINS");

	if(walker->leafProxy==NULL) // Already past end, fail
		return 0;

	s32 upstream=walkerAdvanceGetLastUpstream(walker);

	if(upstream==-1)
		{
		walker->leafArrayIndex=0;
		walker->leafEntryIndex=-1;

		*entryPtr=NULL;
		*upstreamPtr=upstream;
		*upstreamOffsetPtr=0;
		*downstreamOffsetPtr=0;

		return 0;
		}

	while(upstream < targetUpstream) // One leaf at a time, for upstream
		{
		walkerApplyLeafOffsets(walker, walker->leafProxy);

		if(!walkerAdvanceToNextLeaf(walker, 0))
			{
			rttlEnsureFullyUnpacked(walker->treeProxy, walker->leafProxy);

			walker->leafArrayIndex=walker->leafProxy->unpackedBlock->entryArrayCount;
			walker->leafEntryIndex=-1;
			walker->leafEntryArray=NULL;

			*entryPtr=NULL;
			*upstreamPtr=upstream;
			*upstreamOffsetPtr=walker->upstreamLeafOffsets[targetUpstream];
			*downstreamOffsetPtr=walker->downstreamLeafOffsets[targetDownstream];

			return 0;
			}

		upstream=walkerAdvanceGetLastUpstream(walker);
		}

	walkerTransferOffsetsLeafToEntry(walker);

	return 1;
}

static s32 walkerAdvanceToUpstreamThenOffsetThenDownstream_2(RouteTableTreeWalker *walker, s32 targetUpstream, s32 targetDownstream, s32 targetMinOffset, s32 targetMaxOffset,
		s32 *upstreamPtr, RouteTableUnpackedEntry **entryPtr, s32 *upstreamOffsetPtr, s32 *downstreamOffsetPtr)
{
	rttlEnsureFullyUnpacked(walker->treeProxy, walker->leafProxy);
	walker->leafEntryArray=walker->leafProxy->unpackedBlock->entryArrays[walker->leafArrayIndex];

	s32	upstream=walker->leafEntryArray->upstream;

	while(upstream < targetUpstream) // One array at a time, for upstream
		{
		walkerApplyArrayOffsets(walker, walker->leafProxy, walker->leafArrayIndex);

		if(!walkerAdvanceToNextArray(walker))
			{
			*entryPtr=NULL;
			*upstreamPtr=upstream;
			*upstreamOffsetPtr=walker->upstreamEntryOffsets[targetUpstream];
			*downstreamOffsetPtr=walker->downstreamEntryOffsets[targetDownstream];

			return 0;
			}

		upstream=walker->leafEntryArray->upstream;
		}

	return 1;
}

static s32 walkerAdvanceToUpstreamThenOffsetThenDownstream_3(RouteTableTreeWalker *walker, s32 targetUpstream, s32 targetDownstream, s32 targetMinOffset, s32 targetMaxOffset,
		s32 *upstreamPtr, RouteTableUnpackedEntry **entryPtr, s32 *upstreamOffsetPtr, s32 *downstreamOffsetPtr)
{
	s32	upstream=walker->leafEntryArray->upstream;
	s32 currentUpstreamOffset=walker->upstreamEntryOffsets[targetUpstream];

	RouteTableUnpackedEntry *entry=walker->leafEntryArray->entries+walker->leafEntryIndex;
	s32 snoopUpstreamOffset=currentUpstreamOffset+entry->width;

	while(upstream == targetUpstream && snoopUpstreamOffset<=targetMinOffset) // One entry at a time
		{
		if(snoopUpstreamOffset==targetMinOffset && entry->downstream == targetDownstream)
			break;

		currentUpstreamOffset=snoopUpstreamOffset;
		walker->downstreamEntryOffsets[entry->downstream]+=entry->width;

		walker->leafEntryIndex++;

		if(walker->leafEntryIndex>=walker->leafEntryArray->entryCount)
			{
			if(!walkerAdvanceToNextArray_targetUpstream(walker, targetUpstream))
				{
				walker->upstreamEntryOffsets[targetUpstream]=currentUpstreamOffset;

				*entryPtr=NULL;
				*upstreamPtr=targetUpstream;
				*upstreamOffsetPtr=currentUpstreamOffset;
				*downstreamOffsetPtr=walker->downstreamEntryOffsets[targetDownstream];

				return 0;
				}

			upstream=walker->leafEntryArray->upstream;
			entry=walker->leafEntryArray->entries+walker->leafEntryIndex;
			}
		else
			entry++;

		snoopUpstreamOffset=currentUpstreamOffset+entry->width;
		}

	walker->upstreamEntryOffsets[targetUpstream]=currentUpstreamOffset;

	return 1;
}

static s32 walkerAdvanceToUpstreamThenOffsetThenDownstream_4(RouteTableTreeWalker *walker, s32 targetUpstream, s32 targetDownstream, s32 targetMinOffset, s32 targetMaxOffset,
		s32 *upstreamPtr, RouteTableUnpackedEntry **entryPtr, s32 *upstreamOffsetPtr, s32 *downstreamOffsetPtr)
{
	s32 upstream=walker->leafEntryArray->upstream;
	s32 currentUpstreamOffset=walker->upstreamEntryOffsets[targetUpstream];

	RouteTableUnpackedEntry *entry=walker->leafEntryArray->entries+walker->leafEntryIndex;
	s32 snoopUpstreamOffset=currentUpstreamOffset+entry->width;

	// Current position is valid, but possibly suboptimal

	while(upstream == targetUpstream && snoopUpstreamOffset<=targetMaxOffset && entry->downstream < targetDownstream)
		{
		currentUpstreamOffset=snoopUpstreamOffset;
		walker->downstreamEntryOffsets[entry->downstream]+=entry->width;

		walker->leafEntryIndex++;

		if(walker->leafEntryIndex>=walker->leafEntryArray->entryCount)
			{
			if(!walkerAdvanceToNextArray_targetUpstream(walker, targetUpstream))
				{
				walker->upstreamEntryOffsets[targetUpstream]=currentUpstreamOffset;

				*entryPtr=NULL;
				*upstreamPtr=targetUpstream;
				*upstreamOffsetPtr=currentUpstreamOffset;
				*downstreamOffsetPtr=walker->downstreamEntryOffsets[targetDownstream];

				return 0;
				}

			upstream=walker->leafEntryArray->upstream;
			entry=walker->leafEntryArray->entries+walker->leafEntryIndex;
			}
		else
			entry++;

		snoopUpstreamOffset=currentUpstreamOffset+entry->width;
		}

	walker->upstreamEntryOffsets[targetUpstream]=currentUpstreamOffset;

	*entryPtr=entry;
	*upstreamPtr=upstream;
	*upstreamOffsetPtr=currentUpstreamOffset;
	*downstreamOffsetPtr=walker->downstreamEntryOffsets[targetDownstream];

	return 1;
}

s32 rttwAdvanceToUpstreamThenOffsetThenDownstream(RouteTableTreeWalker *walker, s32 targetUpstream, s32 targetDownstream, s32 targetMinOffset, s32 targetMaxOffset,
		s32 *upstreamPtr, RouteTableUnpackedEntry **entryPtr, s32 *upstreamOffsetPtr, s32 *downstreamOffsetPtr)
{
	if(!walkerAdvanceToUpstreamThenOffsetThenDownstream_1(walker, targetUpstream, targetDownstream, targetMinOffset, targetMaxOffset,
			upstreamPtr, entryPtr, upstreamOffsetPtr, downstreamOffsetPtr))
		return 0;

	if(!walkerAdvanceToUpstreamThenOffsetThenDownstream_2(walker, targetUpstream, targetDownstream, targetMinOffset, targetMaxOffset,
			upstreamPtr, entryPtr, upstreamOffsetPtr, downstreamOffsetPtr))
		return 0;

	if(!walkerAdvanceToUpstreamThenOffsetThenDownstream_3(walker, targetUpstream, targetDownstream, targetMinOffset, targetMaxOffset,
			upstreamPtr, entryPtr, upstreamOffsetPtr, downstreamOffsetPtr))
		return 0;

	return walkerAdvanceToUpstreamThenOffsetThenDownstream_4(walker, targetUpstream, targetDownstream, targetMinOffset, targetMaxOffset,
			upstreamPtr, entryPtr, upstreamOffsetPtr, downstreamOffsetPtr);


}
*/

// Move to last entry that has upstream<=targetUpstream, minOffset<=upstreamOffset<=maxOffset, and if possible downstream<=targetDownstream

s32 rttwAdvanceToUpstreamThenOffsetThenDownstream(RouteTableTreeWalker *walker, s32 targetUpstream, s32 targetDownstream, s32 targetMinOffset, s32 targetMaxOffset,
		s32 *upstreamPtr, RouteTableUnpackedEntry **entryPtr, s32 *upstreamOffsetPtr, s32 *downstreamOffsetPtr)
{
//	LOG(LOG_CRITICAL,"walkerAdvanceToUpstreamThenOffsetThenDownstream disabled");

	//  BEGIN walkerAdvanceToUpstreamThenOffsetThenDownstream_1

	if(walker->leafProxy==NULL) // Already past end, fail
		return 0;

	s32 upstream=walkerAdvanceGetLastUpstream(walker);

	if(upstream==-1)
		{
		walker->leafArrayIndex=0;
		walker->leafEntryIndex=-1;

		*entryPtr=NULL;
		*upstreamPtr=upstream;
		*upstreamOffsetPtr=0;
		*downstreamOffsetPtr=0;

		return 0;
		}

	while(upstream < targetUpstream) // One leaf at a time, for upstream
		{
		walkerApplyLeafOffsets(walker, walker->leafProxy);

		if(!walkerAdvanceToNextLeaf(walker, 0))
			{
			rttlEnsureFullyUnpacked(walker->treeProxy, walker->leafProxy);

			walker->leafArrayIndex=walker->leafProxy->unpackedBlock->entryArrayCount;
			walker->leafEntryIndex=-1;
			walker->leafEntryArray=NULL;

			*entryPtr=NULL;
			*upstreamPtr=upstream;
			*upstreamOffsetPtr=walker->upstreamLeafOffsets[targetUpstream];
			*downstreamOffsetPtr=walker->downstreamLeafOffsets[targetDownstream];

			return 0;
			}

		upstream=walkerAdvanceGetLastUpstream(walker);
		}

	walkerTransferOffsetsLeafToEntry(walker);

	//  BEGIN walkerAdvanceToUpstreamThenOffsetThenDownstream_2

	rttlEnsureFullyUnpacked(walker->treeProxy, walker->leafProxy);
	walker->leafEntryArray=walker->leafProxy->unpackedBlock->entryArrays[walker->leafArrayIndex];

	upstream=walker->leafEntryArray->upstream;

	while(upstream < targetUpstream) // One array at a time, for upstream
		{
		walkerApplyArrayOffsets(walker, walker->leafProxy, walker->leafArrayIndex);

		if(!walkerAdvanceToNextArray(walker))
			{
			*entryPtr=NULL;
			*upstreamPtr=upstream;
			*upstreamOffsetPtr=walker->upstreamEntryOffsets[targetUpstream];
			*downstreamOffsetPtr=walker->downstreamEntryOffsets[targetDownstream];

			return 0;
			}

		upstream=walker->leafEntryArray->upstream;
		}

	//  BEGIN walkerAdvanceToUpstreamThenOffsetThenDownstream_4

	s32 currentUpstreamOffset=walker->upstreamEntryOffsets[targetUpstream];

	RouteTableUnpackedEntry *entry=walker->leafEntryArray->entries+walker->leafEntryIndex;
	s32 snoopUpstreamOffset=currentUpstreamOffset+entry->width;

	while(upstream == targetUpstream && snoopUpstreamOffset<=targetMinOffset) // One entry at a time
		{
		if(snoopUpstreamOffset==targetMinOffset && entry->downstream == targetDownstream)
			break;

		currentUpstreamOffset=snoopUpstreamOffset;
		walker->downstreamEntryOffsets[entry->downstream]+=entry->width;

		walker->leafEntryIndex++;

		if(walker->leafEntryIndex>=walker->leafEntryArray->entryCount)
			{
			if(!walkerAdvanceToNextArray_targetUpstream(walker, targetUpstream))
				{
				walker->upstreamEntryOffsets[targetUpstream]=currentUpstreamOffset;

				*entryPtr=NULL;
				*upstreamPtr=targetUpstream;
				*upstreamOffsetPtr=currentUpstreamOffset;
				*downstreamOffsetPtr=walker->downstreamEntryOffsets[targetDownstream];

				return 0;
				}

			upstream=walker->leafEntryArray->upstream;
			entry=walker->leafEntryArray->entries+walker->leafEntryIndex;
			}
		else
			entry++;

		snoopUpstreamOffset=currentUpstreamOffset+entry->width;
		}

	//walker->upstreamEntryOffsets[targetUpstream]=currentUpstreamOffset;

	//  BEGIN walkerAdvanceToUpstreamThenOffsetThenDownstream_4

	//s32 upstream=walker->leafEntryArray->upstream;
	//s32 currentUpstreamOffset=walker->upstreamEntryOffsets[targetUpstream];

	// RouteTableUnpackedEntry *entry=walker->leafEntryArray->entries+walker->leafEntryIndex;
	// s32 snoopUpstreamOffset=currentUpstreamOffset+entry->width;

	// Current position is valid, but possibly suboptimal

	while(upstream == targetUpstream && snoopUpstreamOffset<=targetMaxOffset && entry->downstream < targetDownstream)
		{
		currentUpstreamOffset=snoopUpstreamOffset;
		walker->downstreamEntryOffsets[entry->downstream]+=entry->width;

		walker->leafEntryIndex++;

		if(walker->leafEntryIndex>=walker->leafEntryArray->entryCount)
			{
			if(!walkerAdvanceToNextArray_targetUpstream(walker, targetUpstream))
				{
				walker->upstreamEntryOffsets[targetUpstream]=currentUpstreamOffset;

				*entryPtr=NULL;
				*upstreamPtr=targetUpstream;
				*upstreamOffsetPtr=currentUpstreamOffset;
				*downstreamOffsetPtr=walker->downstreamEntryOffsets[targetDownstream];

				return 0;
				}

			upstream=walker->leafEntryArray->upstream;
			entry=walker->leafEntryArray->entries+walker->leafEntryIndex;
			}
		else
			entry++;

		snoopUpstreamOffset=currentUpstreamOffset+entry->width;
		}

	walker->upstreamEntryOffsets[targetUpstream]=currentUpstreamOffset;

	*entryPtr=entry;
	*upstreamPtr=upstream;
	*upstreamOffsetPtr=currentUpstreamOffset;
	*downstreamOffsetPtr=walker->downstreamEntryOffsets[targetDownstream];

	return 1;

}



void rttwResetOffsetArrays(RouteTableTreeWalker *walker)
{
	memset(walker->upstreamLeafOffsets,0,sizeof(s32)*walker->upstreamOffsetCount);
	memset(walker->downstreamLeafOffsets,0,sizeof(s32)*walker->downstreamOffsetCount);

	memset(walker->upstreamEntryOffsets,0,sizeof(s32)*walker->upstreamOffsetCount);
	memset(walker->downstreamEntryOffsets,0,sizeof(s32)*walker->downstreamOffsetCount);

}

void rttwInitOffsetArrays(RouteTableTreeWalker *walker, s32 upstreamCount, s32 downstreamCount)
{
	s32 *upLeaf=dAlloc(walker->treeProxy->disp, sizeof(s32)*upstreamCount);
	s32 *downLeaf=dAlloc(walker->treeProxy->disp, sizeof(s32)*downstreamCount);

	s32 *upEntry=dAlloc(walker->treeProxy->disp, sizeof(s32)*upstreamCount);
	s32 *downEntry=dAlloc(walker->treeProxy->disp, sizeof(s32)*downstreamCount);

	memset(upLeaf,0,sizeof(s32)*upstreamCount);
	memset(downLeaf,0,sizeof(s32)*downstreamCount);

	memset(upEntry,0,sizeof(s32)*upstreamCount);
	memset(downEntry,0,sizeof(s32)*downstreamCount);

	walker->upstreamOffsetCount=upstreamCount;
	walker->upstreamLeafOffsets=upLeaf;
	walker->upstreamEntryOffsets=upEntry;

	walker->downstreamOffsetCount=downstreamCount;
	walker->downstreamLeafOffsets=downLeaf;
	walker->downstreamEntryOffsets=downEntry;
}





void rttwMergeRoutes_insertEntry(RouteTableTreeWalker *walker, s32 upstream, s32 downstream)
{
	// New entry, with given up/down and width=1. May require array/leaf/branch split if full or new leaf if not matching

//	LOG(LOG_INFO,"Entry Insert: %i %i",upstream, downstream);


	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	RouteTableUnpackedSingleBlock *block=NULL;

	if(leafProxy==NULL)
		{
//		LOG(LOG_INFO,"Entry Insert: No current leaf - need new leaf append");

		leafProxy=allocRouteTableTreeLeafProxy(walker->treeProxy, walker->upstreamOffsetCount, walker->downstreamOffsetCount, ROUTEPACKING_ENTRYARRAYS_CHUNK);
		treeProxyAppendLeafChild(walker->treeProxy, walker->branchProxy, leafProxy, &walker->branchProxy, &walker->branchChildSibdex);

		walker->leafProxy=leafProxy;

		block=leafProxy->unpackedBlock;
		rtpInsertNewEntryArray(block, 0, upstream, ROUTEPACKING_ENTRYS_CHUNK);

		walker->leafArrayIndex=0;
		walker->leafEntryIndex=0;

//		LOG(LOG_INFO,"Entry Insert: Done new leaf append");
		}
	else
		{
		leafProxy=walker->leafProxy;
		block=leafProxy->unpackedBlock;

		if(walker->leafArrayIndex>=block->entryArrayCount)
			{
//			LOG(LOG_INFO,"ArrayAppend: %i %i TODO",walker->leafArrayIndex,block->entryArrayCount);

			if(ensureSpaceForArray(walker))
				{
				leafProxy=walker->leafProxy;
				block=leafProxy->unpackedBlock;
				}

			rtpInsertNewEntryArray(block, walker->leafArrayIndex, upstream, ROUTEPACKING_ENTRYS_CHUNK);
			walker->leafEntryIndex=0;
			}
		else
			{
			RouteTableUnpackedEntryArray *array=block->entryArrays[walker->leafArrayIndex];
			s32 arrayUpstream=array->upstream;

			if(upstream<arrayUpstream) // Insert new array before specified, possibly including splitting leaf and branches
				{
				if(ensureSpaceForArray(walker))
					block=walker->leafProxy->unpackedBlock;

				array=rtpInsertNewEntryArray(block, walker->leafArrayIndex, upstream, ROUTEPACKING_ENTRYS_CHUNK);
				walker->leafEntryIndex=0;
				}
			else if(upstream>arrayUpstream) // Insert new array after specified (problem)
				{
				LOG(LOG_CRITICAL,"ArrayInsertAfter: How???");
				}
			else // Otherwise upstream matching, check for space
				{
//				LOG(LOG_INFO,"Pre-SpaceCheck: Insert at %i %i with downstream %i", walker->leafArrayIndex, walker->leafEntryIndex, downstream);

				if(ensureSpaceForEntries(walker,1))
					{
					leafProxy=walker->leafProxy;
					block=leafProxy->unpackedBlock;
					}
				}
			}
		}


	if(downstream<0)
		LOG(LOG_CRITICAL,"Insert invalid downstream %i",downstream);


//	LOG(LOG_INFO,"Insert at %i %i with downstream %i", walker->leafArrayIndex, walker->leafEntryIndex, downstream);
	rtpInsertNewEntry(block, walker->leafArrayIndex, walker->leafEntryIndex, downstream, 1);

	if(upstream>=block->upstreamOffsetAlloc)
		LOG(LOG_CRITICAL,"Upstream Offset out of range %i of %i",upstream,block->upstreamOffsetAlloc);

	if(downstream>=block->downstreamOffsetAlloc)
		LOG(LOG_CRITICAL,"Downstream Offset out of range %i of %i",downstream,block->downstreamOffsetAlloc);



	block->upstreamLeafOffsets[upstream]++;
	block->downstreamLeafOffsets[downstream]++;

	if(leafProxy->status!=LEAFPROXY_STATUS_DIRTY)
		rttlMarkDirty(walker->treeProxy, leafProxy);
}


void rttwMergeRoutes_widen(RouteTableTreeWalker *walker)
{
//	LOG(LOG_INFO,"walkerMergeRoutes_widen");

	// Add one to the current entry width. Hopefully width doesn't wrap

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	if(leafProxy==NULL)
		{
		LOG(LOG_CRITICAL,"Entry Widen: Null leaf, should never happen");
		}

	if(walker->leafArrayIndex < 0 || walker->leafEntryIndex<0)
		{
		LOG(LOG_CRITICAL,"Entry Widen: Invalid array / entry index, should never happen");
		}

	//ensureRouteTableTreeLeafOffsetCapacity(walker->treeProxy, leafProxy, walker->downstreamOffsetCount);

//	LOG(LOG_INFO,"Walker Indexes %i %i",walker->leafArrayIndex, walker->leafEntryIndex);

	RouteTableUnpackedEntryArray *arrayPtr=walker->leafProxy->unpackedBlock->entryArrays[walker->leafArrayIndex];
	RouteTableUnpackedEntry *entryPtr=&(arrayPtr->entries[walker->leafEntryIndex]);

	s32 upstream=arrayPtr->upstream;
	s32 downstream=entryPtr->downstream;

	if(upstream<0)
		LOG(LOG_CRITICAL,"Entry Widen: Invalid downstream, should never happen");

	if(downstream<0)
		LOG(LOG_CRITICAL,"Entry Widen: Invalid downstream, should never happen");

	if(entryPtr->width>2000000000)
		LOG(LOG_CRITICAL,"Entry Widen: About to wrap width %i",entryPtr->width);

//	LOG(LOG_INFO,"Entry Widen: U %i D %i W %i",upstream, downstream, entryPtr->width);


	entryPtr->width++;

	RouteTableUnpackedSingleBlock *block=leafProxy->unpackedBlock;

	if(upstream>=block->upstreamOffsetAlloc)
		LOG(LOG_CRITICAL,"Upstream Offset out of range %i of %i",upstream,block->upstreamOffsetAlloc);

	if(downstream>=block->downstreamOffsetAlloc)
		LOG(LOG_CRITICAL,"Downstream Offset out of range %i of %i",downstream,block->downstreamOffsetAlloc);

	block->upstreamLeafOffsets[upstream]++;
	block->downstreamLeafOffsets[downstream]++;

	if(leafProxy->status!=LEAFPROXY_STATUS_DIRTY)
		rttlMarkDirty(walker->treeProxy, leafProxy);
}

void rttwMergeRoutes_split(RouteTableTreeWalker *walker, s32 downstream, s32 width1, s32 width2)
{
	// Add split existing route into two, and insert a new route between

	ensureSpaceForEntries(walker,2);

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;

	RouteTableUnpackedSingleBlock *block=leafProxy->unpackedBlock;
	RouteTableUnpackedEntryArray *array=block->entryArrays[walker->leafArrayIndex];

	s16 splitDownstream=array->entries[walker->leafEntryIndex].downstream;
	array->entries[walker->leafEntryIndex].width=width1;

	// Old entry is (splitDownstream,width1), new entry is (downstream,1) second is (splitDownstream, width2)
	rtpInsertNewDoubleEntry(block, walker->leafArrayIndex, walker->leafEntryIndex+1, downstream, 1, splitDownstream, width2);

	s32 upstream=array->upstream;

	if(upstream>=block->upstreamOffsetAlloc)
		LOG(LOG_CRITICAL,"Upstream Offset out of range %i of %i",upstream,block->upstreamOffsetAlloc);

	if(downstream>=block->downstreamOffsetAlloc)
		LOG(LOG_CRITICAL,"Downstream Offset out of range %i of %i",downstream,block->downstreamOffsetAlloc);

	leafProxy->unpackedBlock->upstreamLeafOffsets[upstream]++;
	leafProxy->unpackedBlock->downstreamLeafOffsets[downstream]++;

	if(leafProxy->status!=LEAFPROXY_STATUS_DIRTY)
		rttlMarkDirty(walker->treeProxy, leafProxy);
}



void rttwAppendPreorderedEntry(RouteTableTreeWalker *walker, RouteTableEntry *entry, int routingTable)
{
	s32 upstream=routingTable==ROUTING_TABLE_FORWARD?entry->prefix:entry->suffix;
	s32 downstream=routingTable==ROUTING_TABLE_FORWARD?entry->suffix:entry->prefix;

	if(walker->leafProxy!=NULL) // Check for space
		{
		RouteTableUnpackedSingleBlock *block=walker->leafProxy->unpackedBlock;

		if(walker->leafArrayIndex<0)
			LOG(LOG_CRITICAL,"Invalid Array Index %i for append- count is %i",walker->leafArrayIndex,block->entryArrayCount);

		if(walker->leafArrayIndex!=(block->entryArrayCount-1))
			LOG(LOG_CRITICAL,"Invalid Array Index %i for append- count is %i",walker->leafArrayIndex,block->entryArrayCount);

		RouteTableUnpackedEntryArray *array=block->entryArrays[walker->leafArrayIndex];
		if(array==NULL)
			LOG(LOG_CRITICAL,"Invalid Null EntryArray at Index %i",walker->leafArrayIndex);

		if((array->upstream!=upstream || array->entryCount>=(ROUTEPACKING_ENTRYS_UPGRADE_MAX-1)) &&
				block->entryArrayCount>=(ROUTEPACKING_ENTRYARRAYS_UPGRADE_MAX-1)) // Not suitable upstream / full array, and no more arrays
			walker->leafProxy=NULL;
		}

	if(walker->leafProxy==NULL)
		rttwAppendNewLeaf(walker);

	RouteTableTreeLeafProxy *leafProxy=walker->leafProxy;
	RouteTableUnpackedSingleBlock *block=leafProxy->unpackedBlock;

	RouteTableUnpackedEntryArray *array=NULL;

	if(walker->leafArrayIndex<0)
		{
		array=rtpInsertNewEntryArray(block, 0, upstream, ROUTEPACKING_ENTRYS_CHUNK);
		walker->leafArrayIndex=0;
		walker->leafEntryIndex=-1;
		}
	else
		{
		array=block->entryArrays[walker->leafArrayIndex];

		if(array->upstream!=upstream || array->entryCount>=(ROUTEPACKING_ENTRYS_UPGRADE_MAX-1))
			{
			walker->leafArrayIndex++;
			array=rtpInsertNewEntryArray(block, walker->leafArrayIndex, upstream, ROUTEPACKING_ENTRYS_CHUNK);
			walker->leafEntryIndex=-1;
			}
		}

	walker->leafEntryIndex++;

	rtpInsertNewEntry(block, walker->leafArrayIndex, walker->leafEntryIndex, downstream, entry->width);

//	LOG(LOG_INFO,"Upgrade Insert Entry: %i %i %i",upstream, downstream, entry->width);

	block->upstreamLeafOffsets[upstream]+=entry->width;
	block->downstreamLeafOffsets[downstream]+=entry->width;

	if(leafProxy->status!=LEAFPROXY_STATUS_DIRTY)
		rttlMarkDirty(walker->treeProxy, leafProxy);
}



void rttwAppendPreorderedEntries(RouteTableTreeWalker *walker, RouteTableEntry *entries, u32 entryCount, int routingTable)
{
	rttwSeekEnd(walker);

	for(int i=0;i<entryCount;i++)
		rttwAppendPreorderedEntry(walker,entries+i, routingTable);

	//LOG(LOG_INFO,"Routing Table with %i entries used %i branches and %i leaves",
//		entryCount,walker->treeProxy->branchArrayProxy.newDataCount,walker->treeProxy->leafArrayProxy.newDataCount);


}


