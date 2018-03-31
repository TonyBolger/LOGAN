#ifndef __MEM_H
#define __MEM_H


//#define lAlloc(size) alloca(size)

#define lAlloc(size) memset(alloca(size),0,size);


#ifdef FEATURE_ENABLE_MEMTRACK

void *gAlloc(size_t size, int memTrackerId);
void *gAllocC(size_t size, int memTrackerId);
void *gAllocAligned(size_t size, size_t alignment, int memTrackerId);
void gFree(void *ptr, size_t size, int memTrackerId);

#define G_ALLOC(S,M) gAlloc((S),(M))
#define G_ALLOC_C(S,M) gAllocC((S),(M))
#define G_ALLOC_ALIGNED(S,A,M) gAllocAligned((S),(A),(M))
#define G_FREE(P,S,M) gFree((P),(S),(M))

#else

void *gAlloc(size_t size);
void *gAllocC(size_t size);
void *gAllocAligned(size_t size, size_t alignment);
void gFree(void *ptr);

#define G_ALLOC(S,M) gAlloc((S))
#define G_ALLOC_C(S,M) gAllocC((S))
#define G_ALLOC_ALIGNED(S,A,M) gAllocAligned((S),(A))
#define G_FREE(P,S,M) gFree((P))

#endif

// SmerMap / SmerArray

SmerId *smSmerIdArrayAlloc(int length);
void smSmerIdArrayFree(SmerId *array, int length);

SmerEntry *smSmerEntryArrayAlloc(int length);
void smSmerEntryArrayFree(SmerEntry *array, int length);

u8 **smSmerDataArrayAlloc(int length);
void smSmerDataArrayFree(u8 **array, int length);

// ParallelTask

ParallelTaskConfig *ptParallelTaskConfigAlloc();
void ptParallelTaskConfigFree(ParallelTaskConfig *ptc);

ParallelTask *ptParallelTaskAlloc();
void ptParallelTaskFree(ParallelTask *pt);


IndexingBuilder *tiIndexingBuilderAlloc();
void tiIndexingBuilderFree(IndexingBuilder *ib);

RoutingBuilder *trRoutingBuilderAlloc();
void trRoutingBuilderFree(RoutingBuilder *rb);

// Graph

Graph *grGraphAlloc();
void grGraphFree(Graph *graph);

// SequenceIndex

IndexedSequenceFragment *siSequenceFragmentAlloc();
void siSequenceFragmentFree(IndexedSequenceFragment *seqFrag);

IndexedSequence *siSequenceAlloc();
void siSequenceFree(IndexedSequence *seq);

IndexedSequenceSource *siSequenceSourceAlloc();
void siSequenceSourceFree(IndexedSequenceSource *seqSrc);



#endif
