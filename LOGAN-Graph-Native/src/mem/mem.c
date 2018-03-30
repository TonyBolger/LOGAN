
#include "common.h"

#ifdef FEATURE_ENABLE_MEMKIND
#include <hbwmalloc.h>
#endif

// Centralized Generic Alloc / Realloc / Free: Delegates to either glibc allocator or libmemkind

#ifdef FEATURE_ENABLE_MEMTRACK
void *gAlloc(size_t size, int memTrackerId)
{
	mtTrackAlloc(size, memTrackerId);

#else
	void *gAlloc(size_t size)
	{
#endif

#ifdef FEATURE_ENABLE_MEMKIND
	void *usrPtr=hbw_malloc(size);
#else
	void *usrPtr=malloc(size);
#endif

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

#ifdef FEATURE_ENABLE_MEMKIND
	void *usrPtr=hbw_calloc(1,size);
#else
	void *usrPtr=calloc(1,size);
#endif

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

#ifdef FEATURE_ENABLE_MEMKIND
	int err=hbw_posix_memalign((void **)&usrPtr, alignment, size);
#else
	int err=posix_memalign((void **)&usrPtr, alignment, size);
#endif

	if(err)
		{
		char errBuf[ERRORBUF];
		strerror_r(err,errBuf,ERRORBUF);

		LOG(LOG_CRITICAL,"Failed to allocate size %li (alignment %i) with error %s",size, alignment, errBuf);
		}

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

#ifdef FEATURE_ENABLE_MEMKIND
	hbw_free(ptr);
#else
	free(ptr);
#endif
}



/*
 * Specific allocator for SmerId/SmerEntry/SmerData arrays
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


RoutingBuilder *trRoutingBuilderAlloc()
{
	return G_ALLOC_C(sizeof(RoutingBuilder), MEMTRACKID_ROUTING_BUILDER);
}

void trRoutingBuilderFree(RoutingBuilder *rb)
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


/*
 *
 * Specific allocator for SequenceFragment/Sequence/SequenceSource/SequenceIndex
 *
 */

SequenceFragment *siSequenceFragmentAlloc()
{
	return G_ALLOC_C(sizeof(SequenceFragment), MEMTRACKID_SEQINDEX);
}

void siSequenceFragmentFree(SequenceFragment *seqFrag)
{
	G_FREE(seqFrag, sizeof(SequenceFragment), MEMTRACKID_SEQINDEX);
}

Sequence *siSequenceAlloc()
{
	return G_ALLOC_C(sizeof(Sequence), MEMTRACKID_SEQINDEX);
}

void siSequenceFree(Sequence *seq)
{
	G_FREE(seq, sizeof(Sequence), MEMTRACKID_SEQINDEX);
}

SequenceSource *siSequenceSourceAlloc()
{
	return G_ALLOC_C(sizeof(SequenceSource), MEMTRACKID_SEQINDEX);
}

void siSequenceSourceFree(SequenceSource *seqSrc)
{
	G_FREE(seqSrc, sizeof(SequenceSource), MEMTRACKID_SEQINDEX);
}





