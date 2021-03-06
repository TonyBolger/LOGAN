
#include "common.h"



s32 rttaGetRouteTableTreeArraySize_Existing(RouteTableTreeArrayBlock *arrayBlock)
{
	return sizeof(RouteTableTreeArrayBlock)+sizeof(u8 *)*arrayBlock->dataAlloc;
}


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

static void initBlockArrayProxy_scanIndirect(RouteTableTreeArrayBlock *arrayBlock, u16 ptrCount, s32 sliceIndex, u16 *allocPtr, u16 *countPtr)
{
	int firstSubIndex=ptrCount-1;

	int nestedHeaderSize=rtGetIndexedBlockHeaderSize_1(vpExtCalcLength(sliceIndex));
	RouteTableTreeArrayBlock *midBlock=(RouteTableTreeArrayBlock *)(arrayBlock->data[firstSubIndex]+nestedHeaderSize);

	initBlockArrayProxy_scan(midBlock, allocPtr, countPtr);

	(*allocPtr)+=firstSubIndex*ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES;
	(*countPtr)+=firstSubIndex*ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES;
}


void rttaBindBlockArrayProxy(RouteTableTreeArrayProxy *arrayProxy, u8 *heapDataPtr, u32 headerSize, u32 isIndirect)
{
	RouteTableTreeArrayBlock *dataBlock=NULL;

	if(heapDataPtr!=NULL)
		dataBlock=(RouteTableTreeArrayBlock *)(heapDataPtr+headerSize);

	if(isIndirect)
		{
		arrayProxy->ptrBlock=dataBlock;
		arrayProxy->dataBlock=NULL;
		}
	else
		{
		arrayProxy->dataBlock=dataBlock;
		arrayProxy->ptrBlock=NULL;
		}

	//(RouteTableTreeArrayBlock *)(heapDataPtr+headerSize);

	//if(dataBlock->dataAlloc)



	/*
	switch(arrayProxy->arrayType)
		{
		case ARRAY_TYPE_SHALLOW_PTR:
			arrayProxy->ptrBlock=NULL;
			if(heapDataPtr!=NULL)
				arrayProxy->dataBlock=(RouteTableTreeArrayBlock *)(heapDataPtr+headerSize);
			else
				arrayProxy->dataBlock=NULL;
			break;

		case ARRAY_TYPE_DEEP_PTR:
			LOG(LOG_CRITICAL,"Invalid DeepPtr format for top-level block");
			break;

		case ARRAY_TYPE_SHALLOW_DATA:
			LOG(LOG_CRITICAL,"Invalid ShallowPtr format for top-level block");
			break;

		case ARRAY_TYPE_DEEP_DATA:
			LOG(LOG_CRITICAL,"Invalid DeepData format for top-level block");
			break;

		default:
			LOG(LOG_CRITICAL,"Invalid %i format for top-level block",arrayProxy->arrayType);
			break;
		}

		*/
}

void rttaInitBlockArrayProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeArrayProxy *arrayProxy, HeapDataBlock *heapDataBlock, u8 *heapDataPtr, u32 isIndirect)
{
	arrayProxy->heapDataBlock=heapDataBlock;

	rttaBindBlockArrayProxy(arrayProxy, heapDataPtr, arrayProxy->heapDataBlock->headerSize, isIndirect);

	if(arrayProxy->ptrBlock!=NULL) // Indirect mode
		{
		initBlockArrayProxy_scan(arrayProxy->ptrBlock, &arrayProxy->ptrAlloc, &arrayProxy->ptrCount);

		initBlockArrayProxy_scanIndirect(arrayProxy->ptrBlock, arrayProxy->ptrCount, heapDataBlock->sliceIndex,
				&arrayProxy->oldDataAlloc, &arrayProxy->oldDataCount);

		//LOG(LOG_INFO,"Indirect scan found %i of %i", arrayProxy->oldDataCount, arrayProxy->oldDataAlloc);
		}
	else
		{
		arrayProxy->ptrAlloc=0;
		arrayProxy->ptrCount=0;

		initBlockArrayProxy_scan(arrayProxy->dataBlock, &arrayProxy->oldDataAlloc, &arrayProxy->oldDataCount);
		}

	arrayProxy->newEntries=NULL;
	arrayProxy->newEntriesAlloc=0;
	arrayProxy->newEntriesCount=0;

	arrayProxy->newDataAlloc=arrayProxy->oldDataAlloc;
	arrayProxy->newDataCount=arrayProxy->oldDataCount;

	if(heapDataPtr!=NULL)
		heapDataBlock->dataSize=rttGetTopArraySize(arrayProxy);
	else
		heapDataBlock->dataSize=0;

	/*
	if(arrayProxy->ptrBlock!=NULL)
		{
		LOG(LOG_INFO,"INIT INDIRECT DUMP: Start");

		dumpBlockArrayProxy(arrayProxy);

		LOG(LOG_INFO,"INIT INDIRECT DUMP: End");
		}
*/
}


static void dumpBlockArrayEntry(u8 *rawPtr)
{
	s32 topIndex=0, indexSize=0, index=0, subindexSize=0, subindex=0;

	s32 headerSize=rtDecodeIndexedBlockHeader(rawPtr, &topIndex, &indexSize, &index, &subindexSize, &subindex);

	LOG(LOG_INFO,"Header %i TopIndex %i Index %i SubIndex %i",headerSize, topIndex, index, subindex);

	u8 *block=rawPtr+headerSize;

	switch(topIndex)
	{
		case ROUTE_TOPINDEX_FORWARD_BRANCH_ARRAY_0:
		case ROUTE_TOPINDEX_REVERSE_BRANCH_ARRAY_0:
			LOG(LOG_CRITICAL,"Unexpected BRANCH_ARRAY_0 at %p",rawPtr);
			break;

		case ROUTE_TOPINDEX_FORWARD_LEAF_ARRAY_0:
		case ROUTE_TOPINDEX_REVERSE_LEAF_ARRAY_0:
			LOG(LOG_CRITICAL,"Unexpected LEAF_ARRAY_0 at %p",rawPtr);
			break;

		case ROUTE_PSEUDO_INDEX_FORWARD_LEAF_ARRAY_1:
		case ROUTE_PSEUDO_INDEX_REVERSE_LEAF_ARRAY_1:
			LOG(LOG_CRITICAL,"Unexpected LEAF_ARRAY_1 at %p",rawPtr);
			break;

		case ROUTE_PSEUDO_INDEX_FORWARD_BRANCH_1:
		case ROUTE_PSEUDO_INDEX_REVERSE_BRANCH_1:
			rttbDumpBranchBlock((RouteTableTreeBranchBlock *) block);
			break;

		case ROUTE_PSEUDO_INDEX_FORWARD_LEAF_1:
		case ROUTE_PSEUDO_INDEX_REVERSE_LEAF_1:
		case ROUTE_PSEUDO_INDEX_FORWARD_LEAF_2:
		case ROUTE_PSEUDO_INDEX_REVERSE_LEAF_2:
			rttlDumpLeafBlock((RouteTableTreeLeafBlock *) block);
			break;

		default:
			LOG(LOG_CRITICAL,"Unexpected top index %i at %p",topIndex,rawPtr);

	}


}

void rttaDumpBlockArrayProxy(RouteTableTreeArrayProxy *arrayProxy)
{
	if(arrayProxy==NULL)
		{
		LOG(LOG_INFO,"ArrayProxy is NULL");
		return;
		}

	if(arrayProxy->ptrBlock!=NULL)
		{
		LOG(LOG_INFO,"Indirect Old Data: %i of %i in blocks: %i of %i (%i) at %p",arrayProxy->oldDataCount, arrayProxy->oldDataAlloc,
				arrayProxy->ptrCount, arrayProxy->ptrAlloc, arrayProxy->ptrBlock->dataAlloc, arrayProxy->ptrBlock);
		for(int i=0;i<arrayProxy->ptrAlloc;i++)
			{
			if(arrayProxy->ptrBlock->data[i]==NULL)
				{
				LOG(LOG_INFO,"Indirect Raw Block [%i] is NULL",i);
				}
			else
				{
				LOG(LOG_INFO,"Indirect Raw Block [%i] is %p", i, arrayProxy->ptrBlock->data[i]);

				int nestedHeaderSize=rtGetIndexedBlockHeaderSize_1(vpExtCalcLength(arrayProxy->heapDataBlock->sliceIndex));
				RouteTableTreeArrayBlock *level1Block=(RouteTableTreeArrayBlock *)(arrayProxy->ptrBlock->data[i]+nestedHeaderSize);

				LOG(LOG_INFO,"Indirect Block has %i",level1Block->dataAlloc);

				for(int j=0;j<level1Block->dataAlloc;j++)
					{
					if(level1Block->data[j]==NULL)
						LOG(LOG_INFO,"Indirect Block entry [%i][%i] is NULL",i,j);
					else
						{
						LOG(LOG_INFO,"Indirect Block entry [%i][%i] is %p", i,j,level1Block->data[j]);
						dumpBlockArrayEntry(level1Block->data[j]);
						}

					}
				}
			}
		}

	if(arrayProxy->dataBlock!=NULL)
		{
		LOG(LOG_INFO,"Direct Old Data: %i of %i (%i)",arrayProxy->oldDataCount, arrayProxy->oldDataAlloc, arrayProxy->dataBlock->dataAlloc);

		for(int i=0;i<arrayProxy->oldDataAlloc;i++)
			{
			if(arrayProxy->dataBlock->data[i]==NULL)
				LOG(LOG_INFO,"Direct Block is NULL");
			else
				{
				LOG(LOG_INFO,"Direct Block is %p", arrayProxy->dataBlock->data[i]);
				dumpBlockArrayEntry(arrayProxy->dataBlock->data[i]);
				}
			}
		}

	if(arrayProxy->newEntriesAlloc>0)
		{
		LOG(LOG_INFO,"NewEntry Blocks: %i of %i",arrayProxy->newEntriesCount, arrayProxy->newEntriesAlloc);

		for(int i=0;i<arrayProxy->newEntriesCount;i++)
			{
			LOG(LOG_INFO,"NewEntry Block: Index %i is %p", arrayProxy->newEntries[i].index, arrayProxy->newEntries[i].proxy);
			}
		}
}






/*
//static
s32 getBlockArraySize(RouteTableTreeArrayProxy *arrayProxy)
{
	if(arrayProxy->newDataCount>0)
		return arrayProxy->newDataCount;

	return arrayProxy->oldDataCount;
}
*/

u8 *rttaGetBlockArrayDataEntryRaw(RouteTableTreeArrayProxy *arrayProxy, s32 subindex)
{
	// Indirect Mode
	if(arrayProxy->ptrBlock!=NULL)
		{
		if(subindex>=0 && subindex<arrayProxy->oldDataAlloc)
			{
			int firstSubindex=subindex>>ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_SHIFT;
			int secondSubindex=subindex&ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_MASK;

			u8 *interRaw=arrayProxy->ptrBlock->data[firstSubindex];
			s32 headerSize=rtGetIndexedBlockHeaderSize_1(vpExtCalcLength(arrayProxy->heapDataBlock->sliceIndex));
			RouteTableTreeArrayBlock *interBlock=(RouteTableTreeArrayBlock *)(interRaw+headerSize);

			if(secondSubindex<interBlock->dataAlloc)
				{
				u8 *data=interBlock->data[secondSubindex];

				//LOG(LOG_INFO,"Indirect access at subindex %i gives %p", subindex, data);

				/*
				LOG(LOG_INFO,"Indirect access %p at subindex %i of %i / %i (%i of %i / %i (%p %i -> %p) and %i of %i)",
						data,
						subindex, arrayProxy->oldDataCount, arrayProxy->oldDataAlloc,
						firstSubindex, arrayProxy->ptrCount, arrayProxy->ptrAlloc,
						interRaw, headerSize, interBlock,
						secondSubindex, interBlock->dataAlloc);
*/

				return data;
				}
			else
				LOG(LOG_CRITICAL,"Indirect access out of bounds %i of %i, from %i of %i", secondSubindex, interBlock->dataAlloc, subindex, arrayProxy->oldDataAlloc);
			}
		LOG(LOG_CRITICAL,"Index out of range %i vs %i",subindex, arrayProxy->oldDataAlloc);
		}
	else if(arrayProxy->dataBlock!=NULL && subindex >=0 && subindex<arrayProxy->oldDataAlloc)
		{
		u8 *data=arrayProxy->dataBlock->data[subindex];
		//LOG(LOG_INFO,"Direct access at subindex %i gives %p", subindex, data);
		return data;
		}

	LOG(LOG_CRITICAL,"Index out of range %i vs %i",subindex, arrayProxy->oldDataAlloc);
	return NULL;
}

void rttaSetBlockArrayDataEntryRaw(RouteTableTreeArrayProxy *arrayProxy, s32 subindex, u8 *data)
{
//	LOG(LOG_INFO,"Setting array entry %i to %p",subindex, data);

	// Indirect Mode
	if(arrayProxy->ptrBlock!=NULL)
		{
		if(subindex>=0 && subindex<arrayProxy->oldDataAlloc)
			{
			int firstSubindex=subindex>>ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_SHIFT;
			int secondSubindex=subindex&ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_MASK;

			u8 *interRaw=arrayProxy->ptrBlock->data[firstSubindex];
			RouteTableTreeArrayBlock *interBlock=(RouteTableTreeArrayBlock *)(interRaw+rtGetIndexedBlockHeaderSize_1(vpExtCalcLength(arrayProxy->heapDataBlock->sliceIndex)));

			if(secondSubindex<interBlock->dataAlloc)
					interBlock->data[secondSubindex]=data;
			else
				LOG(LOG_CRITICAL,"Indirect access out of bounds %i of %i", secondSubindex, interBlock->dataAlloc);

			return;
			}

		LOG(LOG_CRITICAL,"Invalid datablock %p %p / index %i ",arrayProxy->ptrBlock, arrayProxy->dataBlock, subindex);
		}
	else if(arrayProxy->dataBlock!=NULL && subindex >=0 && subindex<arrayProxy->oldDataAlloc)
		{
		arrayProxy->dataBlock->data[subindex]=data;
		return;
		}

	LOG(LOG_CRITICAL,"Invalid datablock %p %p / index %i ",arrayProxy->ptrBlock, arrayProxy->dataBlock, subindex);
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


void *rttaGetBlockArrayNewEntryProxy(RouteTableTreeArrayProxy *arrayProxy, s32 index)
{
	if(arrayProxy->newEntries!=NULL)
		{
		s32 entryIndex=findBlockArrayEntryIndex(index, arrayProxy->newEntries, arrayProxy->newEntriesCount);

		if(entryIndex<arrayProxy->newEntriesCount && arrayProxy->newEntries[entryIndex].index==index)
			{
			return arrayProxy->newEntries[entryIndex].proxy;
			}
		}

	return NULL;
}


u8 *rttaGetBlockArrayExistingEntryData(RouteTableTreeArrayProxy *arrayProxy, s32 subindex)
{
	if(subindex<0 || subindex>arrayProxy->oldDataCount)
		{
		return NULL;
		}

	u8 *data=rttaGetBlockArrayDataEntryRaw(arrayProxy, subindex);

	if(data==NULL)
		{
		LOG(LOG_CRITICAL,"Failed to find %i",subindex);
		return data;
		}

	int headerSize=rtDecodeIndexedBlockHeaderSize(data);
	return data+headerSize;
}

/*
u8 *getBlockArrayExistingEntryData(RouteTableTreeArrayProxy *arrayProxy, s32 index)
{
	if(index>=0 && index<arrayProxy->dataCount)
		{
		u8 *data=arrayProxy->dataBlock->data[index];

		if(data!=NULL)
			data+=rtDecodeIndexedBlockHeader_1(data,NULL,NULL,NULL);

		return data;
		}

	return NULL;

}
*/
void rttaEnsureBlockArrayWritable(RouteTableTreeArrayProxy *arrayProxy, MemDispenser *disp)
{
	if(arrayProxy->newEntries==NULL) // Haven't yet written anything - make writable
		{
		arrayProxy->newEntriesAlloc=ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK;
		arrayProxy->newEntries=dAlloc(disp, sizeof(RouteTableTreeArrayEntry)*arrayProxy->newEntriesAlloc);
		memset(arrayProxy->newEntries, 0, sizeof(RouteTableTreeArrayEntry)*arrayProxy->newEntriesAlloc);

		if(arrayProxy->oldDataCount==arrayProxy->oldDataAlloc)
			arrayProxy->newDataAlloc=arrayProxy->oldDataAlloc;
		else
			arrayProxy->newDataAlloc=arrayProxy->oldDataAlloc;

		arrayProxy->newDataCount=arrayProxy->oldDataCount;
		}

}


static void insertBlockArrayEntryProxy(RouteTableTreeArrayProxy *arrayProxy, s32 entryIndex, s16 index, void *proxy, MemDispenser *disp)
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
		newEntries[entryIndex].proxy=proxy;

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
		arrayProxy->newEntries[entryIndex].proxy=proxy;

		arrayProxy->newEntriesCount++;
		}

}


s32 rttaAppendBlockArrayEntryProxy(RouteTableTreeArrayProxy *arrayProxy, void *proxy, MemDispenser *disp)
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
		arrayProxy->newDataAlloc=MIN(arrayProxy->newDataAlloc+ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK, ROUTE_TABLE_TREE_MAX_ARRAY_ENTRIES);
		if(arrayProxy->newDataCount>=arrayProxy->newDataAlloc)
			LOG(LOG_CRITICAL, "Cannot expand array beyond size limit %i",ROUTE_TABLE_TREE_MAX_ARRAY_ENTRIES);
		}

	s32 index=arrayProxy->newDataCount++;
	s32 entryIndex=arrayProxy->newEntriesCount;

	arrayProxy->newEntries[entryIndex].index=index;
	arrayProxy->newEntries[entryIndex].proxy=proxy;

	arrayProxy->newEntriesCount++;


	return index;
}



//static
void rttaSetBlockArrayEntryProxy(RouteTableTreeArrayProxy *arrayProxy, s32 index, void *proxy, MemDispenser *disp)
{
	rttaEnsureBlockArrayWritable(arrayProxy, disp);

	if(index>=0 && index<arrayProxy->newDataCount)
		{
		s32 entryIndex=findBlockArrayEntryIndex(index, arrayProxy->newEntries, arrayProxy->newEntriesCount);

		if(entryIndex<arrayProxy->newEntriesCount && arrayProxy->newEntries[entryIndex].index==index) // Found existing
			{
			arrayProxy->newEntries[entryIndex].proxy=proxy;
			return;
			}

		insertBlockArrayEntryProxy(arrayProxy, entryIndex, index, proxy, disp);
		}
	else
		LOG(LOG_CRITICAL,"Invalid array index %i - Size %i",index,arrayProxy->newDataCount);
}



s32 rttaGetArrayDirty(RouteTableTreeArrayProxy *arrayProxy)
{
	return arrayProxy->newEntries!=NULL;
}


s32 rttaCalcFirstLevelArrayEntries(s32 entries)
{
	if(entries<ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)
		LOG(LOG_CRITICAL,"Unnecessary use of indirect arrays for %i entries",entries);

	return ((entries+ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES-1) >> ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_SHIFT); // Round up
}

s32 rttaCalcFirstLevelArrayCompleteEntries(s32 entries)
{
	if(entries<ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)
		LOG(LOG_CRITICAL,"Unnecessary use of indirect arrays for %i entries",entries);

	return (entries >> ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_SHIFT); // No round up
}

s32 rttaCalcFirstLevelArrayAdditionalEntries(s32 entries)
{
	if(entries<ROUTE_TABLE_TREE_SHALLOW_DATA_ARRAY_ENTRIES)
		LOG(LOG_CRITICAL,"Unnecessary use of indirect arrays for %i entries",entries);

	return (entries & ROUTE_TABLE_TREE_DEEP_DATA_ARRAY_ENTRIES_MASK);
}




s32 rttaCalcArraySize(s32 entries)
{
	return sizeof(RouteTableTreeArrayBlock)+sizeof(u8 *)*entries;
}


s32 rttaGetArrayEntries(RouteTableTreeArrayProxy *arrayProxy)
{
	if(arrayProxy->newEntries!=NULL)
		return arrayProxy->newDataAlloc;
	else
		return arrayProxy->oldDataAlloc;
}


