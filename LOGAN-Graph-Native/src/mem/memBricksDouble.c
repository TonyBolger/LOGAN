#include "common.h"



// Called during init() (eventually also while holding pile->lock) but still needs to be thread-safe against regular uses

static void initDoubleBrickChunk(MemDoubleBrickPile *pile)
{
	s32 chunkCount=__atomic_load_n(&pile->chunkCount, __ATOMIC_SEQ_CST);

	if(chunkCount >=pile->chunkLimit)
		LOG(LOG_CRITICAL,"Cannot init another chunk: Pile at limit");

	MemDoubleBrickChunk *chunk=G_ALLOC_ALIGNED(sizeof(MemDoubleBrickChunk), DOUBLEBRICK_ALIGNMENT, pile->memTrackId);

	chunk->header.freeCount=DOUBLEBRICK_BRICKS_PER_CHUNK;
	chunk->header.flagPosition=DOUBLEBRICK_SKIP_FLAGS_PER_CHUNK;

	memset(chunk->freeFlag+DOUBLEBRICK_SKIP_FLAGS_PER_CHUNK,0xFF,sizeof(u64)*(DOUBLEBRICK_FLAGS_PER_CHUNK));

	pile->chunks[chunkCount]=chunk;

	__atomic_thread_fence(__ATOMIC_SEQ_CST);

	__atomic_fetch_add(&pile->chunkCount, 1, __ATOMIC_SEQ_CST);
	__atomic_fetch_add(&pile->freeCount, DOUBLEBRICK_BRICKS_PER_CHUNK, __ATOMIC_SEQ_CST);

}


void mbInitDoubleBrickPile(MemDoubleBrickPile *pile, s32 chunkCount, s32 chunkLimit, s32 memTrackId)
{
	if(sizeof(MemDoubleBrickChunk)!=DOUBLEBRICK_ALIGNMENT)
		LOG(LOG_CRITICAL,"Unexpected MemDoubleBrickChunk Size/Alignment mismatch: %i vs %i", sizeof(MemDoubleBrickChunk), DOUBLEBRICK_ALIGNMENT);

	pile->chunks=G_ALLOC(sizeof(MemDoubleBrickChunk *)*chunkLimit, memTrackId);
	memset(pile->chunks, 0, sizeof(MemDoubleBrickChunk *)*chunkLimit);

	pile->memTrackId=memTrackId;
	pile->chunkLimit=chunkLimit;
	pile->chunkCount=0;
	pile->lock=0;

	pile->freeCount=0;
	pile->scanChunk=0;

	for(int i=0;i<chunkCount;i++)
		initDoubleBrickChunk(pile);
}


void mbFreeDoubleBrickPile(MemDoubleBrickPile *pile, char *pileName)
{
	for(int i=0;i<pile->chunkCount;i++)
		{
		int alloc=DOUBLEBRICK_BRICKS_PER_CHUNK-pile->chunks[i]->header.freeCount;

		if(alloc>0)
			{
			LOG(LOG_INFO,"DoubleBrickPile %s Chunk %i has %i still allocated",pileName, i, alloc);

			u32 chunkBaseIndex=i<<16;

			for(int j=DOUBLEBRICK_SKIP_FLAGS_PER_CHUNK;j<DOUBLEBRICK_ALLOC_FLAGS_PER_CHUNK;j++)
				{
				u64 flags=pile->chunks[i]->freeFlag[j];
				if(flags!=0xFFFFFFFFFFFFFFFFl)
					{
					u32 flagBaseIndex=chunkBaseIndex+(j<<6);

					for(int k=0;k<64;k++)
						{
						if(!(flags & 0x1))
							LOG(LOG_INFO,"DoubleBrick %s %u", pileName, (flagBaseIndex+k));
						flags>>=1;
						}
					}
				}
			}

		G_FREE(pile->chunks[i], sizeof(MemDoubleBrickChunk), pile->memTrackId);
		}

	G_FREE(pile->chunks, sizeof(MemDoubleBrickChunk *)*pile->chunkLimit, pile->memTrackId);
}




s32 mbGetFreeDoubleBrickPile(MemDoubleBrickPile *pile)
{
	return __atomic_load_n(&(pile->freeCount),__ATOMIC_SEQ_CST);
}

s32 mbGetFreeDoubleBrickChunk(MemDoubleBrickChunk *chunk)
{
	s32 free=__atomic_load_n(&(chunk->header.freeCount),__ATOMIC_SEQ_CST);
	return free;
}


s32 mbCheckDoubleBrickAvailability(MemDoubleBrickPile *pile, s32 brickCount)
{
	s32 availableCount=mbGetFreeDoubleBrickPile(pile);
	s32 chunkCount=__atomic_load_n(&pile->chunkCount, __ATOMIC_SEQ_CST);

	s32 expandCount=(pile->chunkLimit-chunkCount)*(DOUBLEBRICK_BRICKS_PER_CHUNK-DOUBLEBRICK_PILE_MARGIN_PER_CHUNK);
	int needed=brickCount+chunkCount*DOUBLEBRICK_PILE_MARGIN_PER_CHUNK;

	return needed<(availableCount+expandCount);
}

s32 mbGetDoubleBrickPileCapacity(MemDoubleBrickPile *pile)
{
	return pile->chunkCount*DOUBLEBRICK_BRICKS_PER_CHUNK;
}


static s32 doubleBrickLockPile(MemDoubleBrickPile *pile)
{
	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		s32 lock=__atomic_load_n(&(pile->lock), __ATOMIC_SEQ_CST);

		if(lock)
			return 0;

		if(__atomic_compare_exchange_n(&(pile->lock), &lock, 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
				return 1;
		}

	__atomic_thread_fence(__ATOMIC_SEQ_CST);

	LOG(LOG_CRITICAL,"Failed to lock pile, should never happen"); // Can actually happen, just stupidly unlikely
	return 0;
}

static s32 doubleBrickUnlockPile(MemDoubleBrickPile *pile)
{
	__atomic_thread_fence(__ATOMIC_SEQ_CST);

	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		s32 lock=__atomic_load_n(&(pile->lock), __ATOMIC_SEQ_CST);

		if(!lock)
			return 0;

		if(__atomic_compare_exchange_n(&(pile->lock), &lock, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			return 1;
		}

	LOG(LOG_CRITICAL,"Failed to lock pile, should never happen"); // Can actually happen, just stupidly unlikely
	return 0;
}






static s32 doubleBrickAttemptPileReserve(MemDoubleBrickPile *pile, s32 resRequest)
{
	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		s32 freeCount=__atomic_load_n(&(pile->freeCount), __ATOMIC_SEQ_CST);
		s32 pileMargin=pile->chunkCount*DOUBLEBRICK_PILE_MARGIN_PER_CHUNK;

		if(freeCount<(resRequest+pileMargin))
			return 0;

		s32 newFreeCount=freeCount-resRequest;

		if(__atomic_compare_exchange_n(&(pile->freeCount), &freeCount, newFreeCount, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			return 1;
		}

	LOG(LOG_CRITICAL,"Failed to reserve Bricks, should never happen"); // Can actually happen, just stupidly unlikely
	return 0;
}


static s32 doubleBrickAttemptPileReserveOrExpand(MemDoubleBrickPile *pile, s32 resRequest)
{
	int ret=doubleBrickAttemptPileReserve(pile, resRequest);

	if(ret)
		return ret;

	s32 chunkCount=__atomic_load_n(&pile->chunkCount, __ATOMIC_SEQ_CST);

	if(chunkCount == pile->chunkLimit)
		return 0;

	for(int loopCount=0;loopCount<1000;loopCount++)
		{
		if(doubleBrickLockPile(pile))
			{
			s32 freeCount=__atomic_load_n(&(pile->freeCount), __ATOMIC_RELAXED);
			s32 pileMargin=pile->chunkCount*DOUBLEBRICK_PILE_MARGIN_PER_CHUNK;

			chunkCount=__atomic_load_n(&pile->chunkCount, __ATOMIC_SEQ_CST);

			while(freeCount < (resRequest+pileMargin) && chunkCount < pile->chunkLimit)
				{
				LOG(LOG_INFO,"Expand DoubleBrick pile");
				initDoubleBrickChunk(pile);
				freeCount=__atomic_load_n(&(pile->freeCount), __ATOMIC_RELAXED);
				}

			doubleBrickUnlockPile(pile);
			}

		ret=doubleBrickAttemptPileReserve(pile, resRequest);

		if(ret)
			return ret;

		chunkCount=__atomic_load_n(&pile->chunkCount, __ATOMIC_SEQ_CST);
		if(chunkCount == pile->chunkLimit)
			return 0;

		struct timespec req;

		req.tv_sec=DEFAULT_SLEEP_SECS;
		req.tv_nsec=DEFAULT_SLEEP_NANOS;

		nanosleep(&req, NULL);
		}

	return 0;
}




static s32 doubleBrickPileUnreserve(MemDoubleBrickPile *pile, s32 resRequest)
{
	__atomic_fetch_add(&(pile->freeCount), resRequest, __ATOMIC_RELAXED);

	return 0;
}





static s32 doubleBrickPileScanForUsableChunk(MemDoubleBrickPile *pile, s32 startChunk)
{
	s32 chunkCount=__atomic_load_n(&pile->chunkCount, __ATOMIC_SEQ_CST);

	for(int i=startChunk;i<chunkCount;i++)
		{
		if(mbGetFreeDoubleBrickChunk(pile->chunks[i])>DOUBLEBRICK_CHUNK_MARGIN_PER_CHUNK && __atomic_load_n(&(pile->chunks[i]->header.flagPosition),__ATOMIC_RELAXED)>=0)
			return i;
		}

	for(int i=0;i<startChunk;i++)
		{
		if(mbGetFreeDoubleBrickChunk(pile->chunks[i])>DOUBLEBRICK_CHUNK_MARGIN_PER_CHUNK && __atomic_load_n(&(pile->chunks[i]->header.flagPosition),__ATOMIC_RELAXED)>=0)
			return i;
		}

	return -1;
}



static s32 doubleBrickLockChunkForAllocation(MemDoubleBrickChunk *chunk)
{
	s32 flagPosition=__atomic_load_n(&(chunk->header.flagPosition), __ATOMIC_RELAXED);

	if(flagPosition==DOUBLEBRICK_SCANPOSITION_LOCKED)
		return flagPosition;

	s32 newScanPosition=DOUBLEBRICK_SCANPOSITION_LOCKED;

	if(__atomic_compare_exchange_n(&(chunk->header.flagPosition), &flagPosition, newScanPosition, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return flagPosition;

	return DOUBLEBRICK_SCANPOSITION_LOCKED;
}


static s32 doubleBrickUnlockChunk(MemDoubleBrickChunk *chunk, u16 flagPosition)
{
	s32 currentFlagPosition=__atomic_load_n(&(chunk->header.flagPosition), __ATOMIC_RELAXED);

	if(currentFlagPosition!=DOUBLEBRICK_SCANPOSITION_LOCKED)
		{
		LOG(LOG_CRITICAL,"Cannot unlock chunk not currently locked");
		return DOUBLEBRICK_SCANPOSITION_LOCKED;
		}

	if(__atomic_compare_exchange_n(&(chunk->header.flagPosition), &currentFlagPosition, flagPosition, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return flagPosition;

	LOG(LOG_CRITICAL,"Failed to unlock chunk");
	return DOUBLEBRICK_SCANPOSITION_LOCKED;
}



static s32 doubleBrickPileNextChunk(MemDoubleBrickPile *pile, s32 chunk)
{
	chunk++;
	if(chunk>=pile->chunkCount)
		return 0;
	return chunk;
}


static s32 doubleBrickAllocatorScanForChunkAndLock(MemDoubleBrickAllocator *alloc)
{
	MemDoubleBrickPile *pile=alloc->pile;
	s32 startChunk=pile->scanChunk;

	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		s32 scanChunkResult=doubleBrickPileScanForUsableChunk(pile, startChunk);
		if(scanChunkResult>=0)
			{
			int flagPosition=doubleBrickLockChunkForAllocation(pile->chunks[scanChunkResult]);

			if(flagPosition!=DOUBLEBRICK_SCANPOSITION_LOCKED)
				{
				alloc->chunk=pile->chunks[scanChunkResult];
				alloc->chunkIndex=(((u32)scanChunkResult)<<16);
				alloc->scanChunk=scanChunkResult;

				pile->scanChunk=doubleBrickPileNextChunk(pile, scanChunkResult);

				return flagPosition;
				}
			}

		startChunk=doubleBrickPileNextChunk(pile,startChunk);
		}

	LOG(LOG_CRITICAL,"Loop Stuck");
	return 0;
}

static u32 doubleBrickPileNextFlag(u32 flag)
{
	flag++;
	if(flag>=DOUBLEBRICK_ALLOC_FLAGS_PER_CHUNK)
		return DOUBLEBRICK_SKIP_FLAGS_PER_CHUNK;
	return flag;
}





static s32 doubleBlockAllocateGroup(MemDoubleBrickAllocator *alloc)
{
	MemDoubleBrickChunk *chunk=alloc->chunk;

	if(mbGetFreeDoubleBrickChunk(alloc->chunk)<DOUBLEBRICK_CHUNK_MARGIN_PER_CHUNK)
		return 0;

	for(int i=alloc->scanFlag; i<DOUBLEBRICK_ALLOC_FLAGS_PER_CHUNK; i++)
		{
		u64 allocMask=__atomic_load_n(chunk->freeFlag+i, __ATOMIC_RELAXED);

		if(allocMask!=0)
			{
			if(__atomic_compare_exchange_n(chunk->freeFlag+i, &allocMask, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
				{
				alloc->scanFlag=i;
				alloc->allocMask=allocMask;

				s32 allocCount=__builtin_popcountl(allocMask);
				alloc->allocCount=allocCount;
				alloc->allocIndex=alloc->scanFlag<<6;

				__atomic_fetch_sub(&(chunk->header.freeCount), allocCount, __ATOMIC_RELAXED);

				return allocCount;
				}
			}
		}

	for(int i=DOUBLEBRICK_SKIP_FLAGS_PER_CHUNK; i<alloc->scanFlag; i++)
		{
		u64 allocMask=__atomic_load_n(chunk->freeFlag+i, __ATOMIC_RELAXED);

		if(allocMask!=0)
			{
			if(__atomic_compare_exchange_n(chunk->freeFlag+i, &allocMask, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
				{
				alloc->scanFlag=i;
				alloc->allocMask=allocMask;

				s32 allocCount=__builtin_popcountl(allocMask);
				alloc->allocCount=allocCount;
				alloc->allocIndex=alloc->scanFlag<<6;

				__atomic_fetch_sub(&(chunk->header.freeCount), allocCount, __ATOMIC_RELAXED);

				return allocCount;
				}

			}
		}

	return 0;
}

static s32 doubleBlockUnallocateGroup(MemDoubleBrickAllocator *alloc)
{
	if(!alloc->allocCount)
		return 0;

	MemDoubleBrickChunk *chunk=alloc->chunk;
	u32 scanFlag=alloc->scanFlag;

	int loopCount=10000;

	do
		{
		u64 allocMask=__atomic_load_n(chunk->freeFlag+scanFlag, __ATOMIC_SEQ_CST);
		u64 newAllocMask=allocMask|alloc->allocMask;

		if(__atomic_compare_exchange_n(chunk->freeFlag+scanFlag, &allocMask, newAllocMask, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			{
			int tmp=alloc->allocCount;
			alloc->allocCount=0;

			__atomic_fetch_add(&(chunk->header.freeCount), tmp, __ATOMIC_RELAXED);

			return tmp;
			}

		}
	while(--loopCount>0);

	LOG(LOG_CRITICAL,"Failed to unallocate group");
	return 0;

}


static s32 doubleBlockUnallocateEntry(MemDoubleBrickChunk *chunk, u32 scanFlag, u32 allocIndex)
{
	int loopCount=10000;

	u64 freeAllocMask=(1L<<allocIndex);

	do
		{
		u64 allocMask=__atomic_load_n(chunk->freeFlag+scanFlag, __ATOMIC_SEQ_CST);

		if(allocMask & freeAllocMask)
			{
			LOG(LOG_CRITICAL,"Entry is already free (%i)", loopCount);
			return 0;
			}

		u64 newAllocMask=allocMask|freeAllocMask;

		if(__atomic_compare_exchange_n(chunk->freeFlag+scanFlag, &allocMask, newAllocMask, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			{
			__atomic_fetch_add(&(chunk->header.freeCount), 1, __ATOMIC_SEQ_CST);

			return 1;
			}

		}
	while(--loopCount>0);

	LOG(LOG_CRITICAL,"Failed to unallocate entry");
	return 0;

}







s32 mbInitDoubleBrickAllocator(MemDoubleBrickAllocator *alloc, MemDoubleBrickPile *pile, s32 pileResRequest)
{
	alloc->pile=pile;
	alloc->pileResRequest=pileResRequest+DOUBLEBRICK_DEFAULT_RESERVATION;

	if(!doubleBrickAttemptPileReserveOrExpand(pile, alloc->pileResRequest))
		return 0;

	alloc->pileResLeft=alloc->pileResRequest;

	s32 allocCount=0,loopCount=0;

	while(!allocCount && loopCount++<1000)
		{
		int scanFlag=doubleBrickAllocatorScanForChunkAndLock(alloc);

		if(scanFlag<0)
			{
			LOG(LOG_INFO,"Chunk Scan/Lock Failed");
			return 0;
			}

		alloc->scanFlag=scanFlag;

		allocCount=doubleBlockAllocateGroup(alloc);
		}

	if(loopCount>=1000)
		LOG(LOG_CRITICAL,"Loop stuck");

	return allocCount;
}

void *mbDoubleBrickAllocate(MemDoubleBrickAllocator *alloc, u32 *brickIndexPtr)
{
	if(alloc->allocCount==0)
		{
		alloc->scanFlag=doubleBrickPileNextFlag(alloc->scanFlag);

		if(alloc->pileResLeft < DOUBLEBRICK_DEFAULT_RESERVATION)
			{
			int additionalReserve=DOUBLEBRICK_DEFAULT_RESERVATION-alloc->pileResLeft;

			if(!doubleBrickAttemptPileReserveOrExpand(alloc->pile, additionalReserve))
				{
				LOG(LOG_INFO,"Pile Reserve Failed");
				return NULL;
				}

			alloc->pileResLeft=DOUBLEBRICK_DEFAULT_RESERVATION;
			}

		s32 allocCount=doubleBlockAllocateGroup(alloc);
		s32 loopCount=0;

		while(allocCount==0 && loopCount++<1000)
			{
			doubleBrickUnlockChunk(alloc->chunk, doubleBrickPileNextFlag(alloc->scanFlag));
			int scanFlag=doubleBrickAllocatorScanForChunkAndLock(alloc);

			if(scanFlag<0)
				{
				LOG(LOG_INFO,"Chunk Scan/Lock Failed");
				return NULL;
				}

			alloc->scanFlag=scanFlag;

			allocCount=doubleBlockAllocateGroup(alloc);
			}

		if(loopCount>=1000)
			LOG(LOG_CRITICAL,"Loop stuck");
		}

	if(alloc->allocMask==0)
		{
		LOG(LOG_CRITICAL,"Positive count but no bits");
		return NULL;
		}

	s32 index=alloc->allocIndex+__builtin_ctzl(alloc->allocMask);
	alloc->allocMask&=alloc->allocMask-1; // Clear lowest bit
	alloc->allocCount--;

	alloc->pileResLeft--;

	if(brickIndexPtr!=NULL)
		*brickIndexPtr=alloc->chunkIndex+index;

	return alloc->chunk->bricks[index].data;
}

void mbDoubleBrickAllocatorCleanup(MemDoubleBrickAllocator *alloc)
{
	//LOG(LOG_INFO,"Should return %i",alloc->allocCount);

	doubleBlockUnallocateGroup(alloc);
	doubleBrickUnlockChunk(alloc->chunk, alloc->scanFlag);
	doubleBrickPileUnreserve(alloc->pile, alloc->pileResLeft);
}

void *mbDoubleBrickFindByIndex(MemDoubleBrickPile *pile, u32 brickIndex)
{
	if(brickIndex==LINK_INDEX_DUMMY)
		{
		LOG(LOG_CRITICAL,"Invalid Index %i",brickIndex);
		}

	u32 chunkIndex=brickIndex>>16;
	u32 index=(brickIndex &0xFFFF);

	if(chunkIndex>pile->chunkCount)
		LOG(LOG_CRITICAL,"Index beyond end of pile");

	return pile->chunks[chunkIndex]->bricks+index;
}

void mbDoubleBrickFreeByIndex(MemDoubleBrickPile *pile, u32 brickIndex)
{
	u32 chunkIndex=brickIndex>>16;
	u32 flagIndex=(brickIndex &0xFFFF)>>6;
	u32 allocIndex=(brickIndex & 0x3F);

	if(chunkIndex>pile->chunkCount)
		LOG(LOG_CRITICAL,"Index beyond end of pile");

	if(!doubleBlockUnallocateEntry(pile->chunks[chunkIndex], flagIndex, allocIndex))
		LOG(LOG_CRITICAL,"Failed to free %i -> %i %i %i", brickIndex, chunkIndex, flagIndex, allocIndex);

	//__atomic_fetch_add(&pile->freeCount, 1, __ATOMIC_RELAXED);
	__atomic_fetch_add(&pile->freeCount, 1, __ATOMIC_SEQ_CST);
}



