#ifndef __MEM_H
#define __MEM_H





void *gAlloc(size_t size);
void *gAllocC(size_t size);
void *gRealloc(void *ptr, size_t size);
void gFree(void *ptr);


// SmerMap / SmerArray

SmerId *smSmerIdArrayAlloc(int length);
void smSmerIdArrayFree(SmerId *array);

SmerEntry *smSmerEntryArrayAlloc(int length);
void smSmerEntryArrayFree(SmerEntry *array);

// ParallelTask

ParallelTaskConfig *ptParallelTaskConfigAlloc();
void ptParallelTaskConfigFree(ParallelTaskConfig *ptc);

ParallelTask *ptParallelTaskAlloc();
void ptParallelTaskFree(ParallelTask *pt);


IndexingBuilder *tiIndexingBuilderAlloc();
void tiIndexingBuilderFree(IndexingBuilder *ib);

RoutingBuilder *tiRoutingBuilderAlloc();
void tiRoutingBuilderFree(RoutingBuilder *rb);

// Graph

Graph *grGraphAlloc();
void grGraphFree(Graph *graph);


#endif
