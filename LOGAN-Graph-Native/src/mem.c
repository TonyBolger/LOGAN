
#include "common.h"


// Centralized Generic Alloc / Realloc / Free

void *gAlloc(size_t size)
{
	void *usrPtr=malloc(size);

	if(usrPtr==NULL)
		LOG(LOG_CRITICAL,"Failed to allocate %i bytes",size);

	return usrPtr;
}

void *gAllocC(size_t size)
{
	void *usrPtr=malloc(size);

	if(usrPtr==NULL)
		LOG(LOG_CRITICAL,"Failed to allocate %i bytes",size);

	memset(usrPtr,0,size);
	return usrPtr;
}

void *gRealloc(void *ptr, size_t size)
{
	void *usrPtr=realloc(ptr,size);

	if(usrPtr==NULL)
		LOG(LOG_CRITICAL,"Failed to re-allocate %i bytes",size);

	return usrPtr;
}


void gFree(void *ptr)
{
	if(ptr==NULL)
		return;

	free(ptr);
}


/*
 * Specific allocator for ParallelTaskConfig
 *
 * For now, it just delegates to the generic alloc/free
 *
 */

ParallelTaskConfig *ptParallelTaskConfigAlloc()
{
	return gAllocC(sizeof(ParallelTaskConfig));
}

void ptParallelTaskConfigFree(ParallelTaskConfig *ptc)
{
	gFree(ptc);
}


/*
 * Specific allocator for ParallelTask
 *
 * For now, it just delegates to the generic alloc/free
 *
 */

ParallelTask *ptParallelTaskAlloc()
{
	return gAllocC(sizeof(ParallelTask));
}

void ptParallelTaskFree(ParallelTask *pt)
{
	gFree(pt);
}


/*
 * Specific allocator for IndexingBuilder
 *
 * For now, it just delegates to the generic alloc/free
 *
 */


IndexingBuilder *tiIndexingBuilderAlloc()
{
	return gAllocC(sizeof(IndexingBuilder));

}

void tiIndexingBuilderFree(IndexingBuilder *ib)
{
	gFree(ib);
}


/*
 * Specific allocator for RoutingBuilder
 *
 * For now, it just delegates to the generic alloc/free
 *
 */


RoutingBuilder *tiRoutingBuilderAlloc()
{
	return gAllocC(sizeof(RoutingBuilder));
}

void tiRoutingBuilderFree(RoutingBuilder *rb)
{
	gFree(rb);
}



/*
 * Specific allocator for Graph
 *
 * For now, it just delegates to the generic alloc/free
 *
 */

Graph *grGraphAlloc()
{
	return gAllocC(sizeof(Graph));
}

void grGraphFree(SmerId *array)
{
	gFree(array);
}

