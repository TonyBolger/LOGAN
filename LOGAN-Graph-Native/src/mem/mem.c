
#include "common.h"


// Centralized Generic Alloc / Realloc / Free

#ifdef FEATURE_ENABLE_MEMTRACK
void *gAlloc(size_t size, int memTrackerId)
{
	mtTrackAlloc(size, memTrackerId);

#else
	void *gAlloc(size_t size)
	{
#endif

	void *usrPtr=hbw_malloc(size);

	if(usrPtr==NULL)
		LOG(LOG_CRITICAL,"Failed to allocate %i bytes",size);

	return usrPtr;
}



#ifdef FEATURE_ENABLE_MEMTRACK
void *gAllocC(size_t size, int memTrackerId)
{
	mtTrackAlloc(size, memTrackerId);
#else
	void *gAllocC(size_t size)
	{
#endif
	void *usrPtr=hbw_calloc(1,size);

	if(usrPtr==NULL)
		LOG(LOG_CRITICAL,"Failed to allocate %i bytes",size);

	return usrPtr;
}


#ifdef FEATURE_ENABLE_MEMTRACK
void *gAllocAligned(size_t size, size_t alignment, int memTrackerId)
{
	mtTrackAlloc(size, memTrackerId);

#else
	void *gAllocAligned(size_t size, size_t alignment)
	{
#endif

	void *usrPtr=NULL;
	if(hbw_posix_memalign((void **)&usrPtr, alignment, size)!=0)
		LOG(LOG_CRITICAL,"Failed to allocate %i bytes with alignment %i",size, alignment);

	return usrPtr;
}



#ifdef FEATURE_ENABLE_MEMTRACK
void gFree(void *ptr, size_t size, int memTrackerId)
{
	mtTrackFree(size, memTrackerId);
#else
	void gFree(void *ptr)
	{
#endif
	if(ptr==NULL)
		return;

	hbw_free(ptr);
}



/*
 * Specific allocator for SmerId/SmerEntry/SmerData arrays
 *
 * For now, they just delegates to the malloc/free
 *
 */

SmerId *smSmerIdArrayAlloc(int length)
{
	SmerId *array = G_ALLOC(((long)length)*sizeof(SmerId), MEMTRACKID_SMER_ID);
	return array;
}

void smSmerIdArrayFree(SmerId *array, int length)
{
	G_FREE(array, ((long)length)*sizeof(SmerId), MEMTRACKID_SMER_ID);
}

SmerEntry *smSmerEntryArrayAlloc(int length)
{
	SmerEntry *array = G_ALLOC_C(((long)length)*sizeof(SmerEntry), MEMTRACKID_SMER_ENTRY);
	return array;
}

void smSmerEntryArrayFree(SmerEntry *array, int length)
{
	G_FREE(array, ((long)length)*sizeof(SmerEntry), MEMTRACKID_SMER_ENTRY);
}


u8 **smSmerDataArrayAlloc(int length)
{
	u8 **array = G_ALLOC_C(((long)length)*sizeof(u8 *), MEMTRACKID_SMER_DATA);
	return array;
}

void smSmerDataArrayFree(u8 **array, int length)
{
	G_FREE(array, ((long)length)*sizeof(u8 *), MEMTRACKID_SMER_DATA);
}






/*
 * Specific allocator for ParallelTaskConfig
 *
 * For now, it just delegates to the generic alloc/free
 *
 */

ParallelTaskConfig *ptParallelTaskConfigAlloc()
{
	return G_ALLOC_C(sizeof(ParallelTaskConfig), MEMTRACKID_TASK);
}

void ptParallelTaskConfigFree(ParallelTaskConfig *ptc)
{
	G_FREE(ptc, sizeof(ParallelTaskConfig), MEMTRACKID_TASK);
}


/*
 * Specific allocator for ParallelTask
 *
 * For now, it just delegates to the generic alloc/free
 *
 */

ParallelTask *ptParallelTaskAlloc()
{
	return G_ALLOC_C(sizeof(ParallelTask), MEMTRACKID_TASK);
}

void ptParallelTaskFree(ParallelTask *pt)
{
	G_FREE(pt, sizeof(ParallelTask), MEMTRACKID_TASK);
}


/*
 * Specific allocator for IndexingBuilder
 *
 * For now, it just delegates to the generic alloc/free
 *
 */


IndexingBuilder *tiIndexingBuilderAlloc()
{
	return G_ALLOC_C(sizeof(IndexingBuilder), MEMTRACKID_INDEXING_BUILDER);

}

void tiIndexingBuilderFree(IndexingBuilder *ib)
{
	G_FREE(ib, sizeof(IndexingBuilder), MEMTRACKID_INDEXING_BUILDER);
}


/*
 * Specific allocator for RoutingBuilder
 *
 * For now, it just delegates to the generic alloc/free
 *
 */


RoutingBuilder *tiRoutingBuilderAlloc()
{
	return G_ALLOC_C(sizeof(RoutingBuilder), MEMTRACKID_ROUTING_BUILDER);
}

void tiRoutingBuilderFree(RoutingBuilder *rb)
{
	G_FREE(rb, sizeof(RoutingBuilder), MEMTRACKID_ROUTING_BUILDER);
}



/*
 * Specific allocator for Graph
 *
 * For now, it just delegates to the generic alloc/free
 *
 */

Graph *grGraphAlloc()
{
	return G_ALLOC_C(sizeof(Graph), MEMTRACKID_GRAPH);
}

void grGraphFree(Graph *graph)
{
	G_FREE(graph, sizeof(Graph), MEMTRACKID_GRAPH);
}

