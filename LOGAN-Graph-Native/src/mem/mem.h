#ifndef __MEM_H
#define __MEM_H

#define MEM_ROUND_BYTE(L) (((L)+7)>>3)
#define MEM_ROUND_WORD(L) (((L)+15)>>4)
#define MEM_ROUND_DWORD(L) (((L)+31)>>5)
#define MEM_ROUND_QWORD(L) (((L)+63)>>6)



void *gAlloc(size_t size);
void *gAllocC(size_t size);
void *gRealloc(void *ptr, size_t size);
void gFree(void *ptr);


// SmerMap

SmerId *smSmerIdArrayAlloc(int length);
void smSmerIdArrayFree(SmerId *array);

SmerMapEntry *smSmerMapEntryArrayAlloc(int length);
void smSmerMapEntryArrayFree(SmerMapEntry *array);

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
void grGraphFree(SmerId *array);


#endif
