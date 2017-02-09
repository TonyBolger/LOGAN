
#include "common.h"


/*
RouteTableTreeArrayBlock *allocRouteTableTreeDataArrayBlock(MemDispenser *disp, s32 dataAlloc)
{
	if(dataAlloc>ROUTE_TABLE_TREE_DATA_ARRAY_ENTRIES)
			{
			LOG(LOG_CRITICAL,"Cannot allocate oversize data array with %i children",dataAlloc);
			}

	RouteTableTreeArrayBlock *block=dAlloc(disp, sizeof(RouteTableTreeArrayBlock)+dataAlloc*sizeof(u8 *));
	block->dataAlloc=dataAlloc;

	for(int i=0;i<dataAlloc;i++)
		block->data[i]=NULL;

	return block;
}

RouteTableTreeArrayBlock *allocRouteTableTreePtrArrayBlock(MemDispenser *disp, s32 dataAlloc)
{
	if(dataAlloc>ROUTE_TABLE_TREE_PTR_ARRAY_ENTRIES)
			{
			LOG(LOG_CRITICAL,"Cannot allocate oversize data array with %i children",dataAlloc);
			}

	RouteTableTreeArrayBlock *block=dAlloc(disp, sizeof(RouteTableTreeArrayBlock)+dataAlloc*sizeof(u8 *));
	block->dataAlloc=dataAlloc;

	for(int i=0;i<dataAlloc;i++)
		block->data[i]=NULL;

	return block;
}

*/


static void initBlockArrayProxy_scan(RouteTableTreeArrayBlock *arrayBlock, u16 *allocPtr, u16 *countPtr)
{
	if(arrayBlock==NULL)
		{
		*allocPtr=0;
		*countPtr=0;
		return;
		}

	u16 alloc=arrayBlock->dataAlloc;
	*allocPtr=alloc;
	u16 count=0;

	for(int i=alloc;i>0;i--)
		{
		if(arrayBlock->data[i-1]!=NULL)
			{
			count=i;
			break;
			}
		}

	*countPtr=count;
}

void rttBindBlockArrayProxy(RouteTableTreeArrayProxy *arrayProxy, u8 *heapDataPtr, u32 headerSize)
{
	switch(arrayProxy->arrayType)
		{
		case ARRAY_TYPE_SHALLOW_PTR:
			LOG(LOG_CRITICAL,"Not implemented");
			break;

		case ARRAY_TYPE_DEEP_PTR:
			LOG(LOG_CRITICAL,"Invalid DeepPtr format for top-level block");
			break;

		case ARRAY_TYPE_SHALLOW_DATA:
			arrayProxy->ptrBlock=NULL;
			if(heapDataPtr!=NULL)
				arrayProxy->dataBlock=(RouteTableTreeArrayBlock *)(heapDataPtr+headerSize);
			else
				arrayProxy->dataBlock=NULL;
			break;

		case ARRAY_TYPE_DEEP_DATA:
			LOG(LOG_CRITICAL,"Invalid DeepData format for top-level block");
			break;

		default:
			LOG(LOG_CRITICAL,"Invalid %i format for top-level block",arrayProxy->arrayType);
			break;
		}
}

void initBlockArrayProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeArrayProxy *arrayProxy, HeapDataBlock *heapDataBlock, u8 *heapDataPtr, u32 arrayType)
{
//	LOG(LOG_INFO,"Here");

	arrayProxy->heapDataBlock=heapDataBlock;
	arrayProxy->arrayType=arrayType;

	rttBindBlockArrayProxy(arrayProxy, heapDataPtr, arrayProxy->heapDataBlock->headerSize);

	initBlockArrayProxy_scan(arrayProxy->ptrBlock, &arrayProxy->ptrAlloc, &arrayProxy->ptrCount);
	initBlockArrayProxy_scan(arrayProxy->dataBlock, &arrayProxy->dataAlloc, &arrayProxy->dataCount);

//	LOG(LOG_INFO,"Loaded array with %i of %i",arrayProxy->dataCount,arrayProxy->dataAlloc);

	arrayProxy->newEntries=NULL;
	arrayProxy->newEntriesAlloc=0;
	arrayProxy->newEntriesCount=0;

	arrayProxy->newPtrAlloc=arrayProxy->ptrAlloc;
	arrayProxy->newPtrCount=arrayProxy->ptrCount;

	arrayProxy->newDataAlloc=arrayProxy->dataAlloc;
	arrayProxy->newDataCount=arrayProxy->dataCount;

	if(heapDataPtr!=NULL)
		heapDataBlock->dataSize=rttGetTopArraySize(arrayProxy);
	else
		heapDataBlock->dataSize=0;

}


//static
s32 getBlockArraySize(RouteTableTreeArrayProxy *arrayProxy)
{
	if(arrayProxy->newDataCount>0)
		return arrayProxy->newDataCount;

	return arrayProxy->dataCount;
}

/*
static int blockArrayEntryComparator(const void *key, const void *entry)
{
	s32 *keyPtr=(s32 *)key;
	RouteTableTreeArrayEntries **entryPtr=(RouteTableTreeArrayEntries **)entry;

	return (*keyPtr)-(*entryPtr)->entryIndex;
}
*/
/*
// Return the index of given entry, or the next entry immediately after (which could be past end)
static s32 findBlockArrayEntryIndex(s32 key, RouteTableTreeArrayEntry **base, size_t num)
{
	s32 start = 0, end = num;
	int result;

	while (start < end) {
		s32 mid = start + (end - start) / 2; // avoid wrap

		result = base[mid]->index-key;
			// - if key bigger, check [start,mid)
			// + if key smaller, check [mid+1, end)
		if (result < 0)
			end = mid;
		else if (result > 0)
			start = mid + 1;
		else
			return mid;
	}

	return end;
}
*/

static s32 findBlockArrayEntryIndex(s32 key, RouteTableTreeArrayEntry *base, size_t num)
{
	s32 start = 0, end = num;
	int result;

	while (start < end) {
		s32 mid = start + (end - start) / 2; // avoid wrap

//		LOG(LOG_INFO,"Search %i %i %i",start,mid,end);

		result = key - base[mid].index;
			// - if key smaller, check [start,mid)
			// + if key bigger, check [mid+1, end)
		if (result < 0)
			end = mid;
		else if (result > 0)
			start = mid + 1;
		else
			return mid;
	}

	return end;
}



//static
u8 *getBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, s32 index)
{
	if(arrayProxy->ptrBlock!=NULL)
		{
		LOG(LOG_CRITICAL,"Two level arrays not implemented");
		}

	if(arrayProxy->newEntries!=NULL)
		{
		s32 entryIndex=findBlockArrayEntryIndex(index, arrayProxy->newEntries, arrayProxy->newEntriesCount);

		if(entryIndex<arrayProxy->newEntriesCount && arrayProxy->newEntries[entryIndex].index==index)
			{
			return arrayProxy->newEntries[entryIndex].data;
			}
		}

	if(index>=0 && index<arrayProxy->dataCount)
		{
		u8 *data=arrayProxy->dataBlock->data[index];

		if(data!=NULL)
			data+=rtDecodeArrayBlockHeader(data,NULL,NULL,NULL,NULL,NULL,NULL);

		return data;
		}

	return NULL;

}

void ensureBlockArrayWritable(RouteTableTreeArrayProxy *arrayProxy, MemDispenser *disp)
{
	if(arrayProxy->newEntries==NULL) // Haven't yet written anything - make writable
		{
		arrayProxy->newEntriesAlloc=ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK;
		arrayProxy->newEntries=dAlloc(disp, sizeof(RouteTableTreeArrayEntry)*arrayProxy->newEntriesAlloc);
		memset(arrayProxy->newEntries, 0, sizeof(RouteTableTreeArrayEntry)*arrayProxy->newEntriesAlloc);

		arrayProxy->newDataAlloc=MIN(arrayProxy->dataAlloc+ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK, ROUTE_TABLE_TREE_DATA_ARRAY_ENTRIES);
		arrayProxy->newDataCount=arrayProxy->dataCount;
		}

}


static void insertBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, s32 entryIndex, s16 index, u8 *data, MemDispenser *disp)
{
	if(entryIndex<0 || entryIndex>arrayProxy->newEntriesCount)
		LOG(LOG_CRITICAL,"Attempt to insert at invalid entry index %i in array with %i elements",entryIndex, arrayProxy->newEntriesCount);

	if(index<0 || entryIndex>=arrayProxy->newDataCount)
		LOG(LOG_CRITICAL,"Attempt to insert at invalid index %i in array with %i elements",entryIndex, arrayProxy->newDataCount);

	// Need to expand or shuffle, then insert

	if(arrayProxy->newEntriesCount>=arrayProxy->newEntriesAlloc)
		{
		arrayProxy->newEntriesAlloc+=ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK;

		RouteTableTreeArrayEntry *newEntries=dAlloc(disp, sizeof(RouteTableTreeArrayEntry)*arrayProxy->newEntriesAlloc);
		memcpy(newEntries, arrayProxy->newEntries, sizeof(RouteTableTreeArrayEntry) * entryIndex);

		newEntries[entryIndex].index=index;
		newEntries[entryIndex].data=data;

		s32 remaining=arrayProxy->newEntriesCount-entryIndex;

		memcpy(newEntries+entryIndex+1, arrayProxy->newEntries+entryIndex, sizeof(RouteTableTreeArrayEntry) * remaining);

		arrayProxy->newEntries=newEntries;
		arrayProxy->newEntriesCount++;
		}
	else
		{
		s32 remaining=arrayProxy->newEntriesCount-entryIndex;

		memmove(arrayProxy->newEntries+entryIndex+1, arrayProxy->newEntries+entryIndex, sizeof(RouteTableTreeArrayEntry) * remaining);

		arrayProxy->newEntries[entryIndex].index=index;
		arrayProxy->newEntries[entryIndex].data=data;

		arrayProxy->newEntriesCount++;
		}

}


s32 appendBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, u8 *data, MemDispenser *disp)
{
	// Need to optionally expand, then append

	if(arrayProxy->newEntriesCount>=arrayProxy->newEntriesAlloc)
		{
		arrayProxy->newEntriesAlloc+=ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK;

		RouteTableTreeArrayEntry *newEntries=dAlloc(disp, sizeof(RouteTableTreeArrayEntry)*arrayProxy->newEntriesAlloc);
		memcpy(newEntries, arrayProxy->newEntries, sizeof(RouteTableTreeArrayEntry) * arrayProxy->newEntriesCount);

		arrayProxy->newEntries=newEntries;
		}

	if(arrayProxy->newDataCount>=arrayProxy->newDataAlloc)
		{
		arrayProxy->newDataAlloc=MIN(arrayProxy->newDataAlloc+ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK, ROUTE_TABLE_TREE_DATA_ARRAY_ENTRIES);
		if(arrayProxy->newDataCount>=arrayProxy->newDataAlloc)
			LOG(LOG_CRITICAL, "Cannot expand array beyond size limit %i",ROUTE_TABLE_TREE_DATA_ARRAY_ENTRIES);
		}

	s32 index=arrayProxy->newDataCount++;
	s32 entryIndex=arrayProxy->newEntriesCount;

	arrayProxy->newEntries[entryIndex].index=index;
	arrayProxy->newEntries[entryIndex].data=data;

	arrayProxy->newEntriesCount++;


	return index;
}



//static
void setBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, s32 index, u8 *data, MemDispenser *disp)
{
	ensureBlockArrayWritable(arrayProxy, disp);

	if(arrayProxy->ptrBlock!=NULL)
		{
		LOG(LOG_CRITICAL,"Two level arrays not implemented");
		}

	if(index>=0 && index<arrayProxy->newDataCount)
		{
		s32 entryIndex=findBlockArrayEntryIndex(index, arrayProxy->newEntries, arrayProxy->newEntriesCount);

		if(entryIndex<arrayProxy->newEntriesCount && arrayProxy->newEntries[entryIndex].index==index) // Found existing
			{
			arrayProxy->newEntries[entryIndex].data=data;
			return;
			}

		insertBlockArrayEntry(arrayProxy, entryIndex, index, data, disp);
		}
	else
		LOG(LOG_CRITICAL,"Invalid array index %i - Size %i",index,arrayProxy->newDataCount);
}

