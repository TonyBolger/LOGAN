
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
	arrayProxy->heapDataBlock=heapDataBlock;
	arrayProxy->arrayType=arrayType;

	rttBindBlockArrayProxy(arrayProxy, heapDataPtr, arrayProxy->heapDataBlock->headerSize);

	initBlockArrayProxy_scan(arrayProxy->ptrBlock, &arrayProxy->ptrAlloc, &arrayProxy->ptrCount);
	initBlockArrayProxy_scan(arrayProxy->dataBlock, &arrayProxy->dataAlloc, &arrayProxy->dataCount);

//	LOG(LOG_INFO,"Loaded array with %i of %i",arrayProxy->dataCount,arrayProxy->dataAlloc);

	arrayProxy->newData=NULL;
	arrayProxy->newDataAlloc=0;
	arrayProxy->newDataCount=0;

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

//static
u8 *getBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, s32 index)
{
	if(arrayProxy->ptrBlock!=NULL)
		{
		LOG(LOG_CRITICAL,"Two level arrays not implemented");
		}

	if(index>=0 && index<arrayProxy->newDataCount)
		{
		u8 *newData=arrayProxy->newData[index];

		if(newData!=NULL)
			return newData;
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
	if(arrayProxy->newData==NULL) // Haven't yet written anything - make writable
		{
		int newAlloc=arrayProxy->dataAlloc;

		if(arrayProxy->dataCount==newAlloc)
			newAlloc=MIN(newAlloc+ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK, ROUTE_TABLE_TREE_DATA_ARRAY_ENTRIES);

		u8 **newData=dAlloc(disp, sizeof(u8 *)*newAlloc);

		//if(newAlloc>arrayProxy->dataCount)

		memset(newData, 0, sizeof(u8 *)*newAlloc);

		arrayProxy->newData=newData;
		arrayProxy->newDataAlloc=newAlloc;
		arrayProxy->newDataCount=arrayProxy->dataCount;
		}

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
		arrayProxy->newData[index]=data;
	else
		LOG(LOG_CRITICAL,"Invalid array index %i - Size %i",index,arrayProxy->newDataCount);
}

//static
s32 appendBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, u8 *data, MemDispenser *disp)
{
	ensureBlockArrayWritable(arrayProxy, disp);

	if(arrayProxy->ptrBlock!=NULL)
		{
		LOG(LOG_CRITICAL,"Two level arrays not implemented");
		}

	if(arrayProxy->newDataCount>=arrayProxy->newDataAlloc)
		{
		int newAllocCount=arrayProxy->newDataAlloc+ROUTE_TABLE_TREE_ARRAY_ENTRIES_CHUNK; // TODO: Move to realloc function

		if(newAllocCount>ROUTE_TABLE_TREE_DATA_ARRAY_ENTRIES)
			{
			LOG(LOG_CRITICAL,"Need to allocate more than %i entries (%i)",ROUTE_TABLE_TREE_DATA_ARRAY_ENTRIES, newAllocCount);
			}

		u8 **newData=dAlloc(disp, sizeof(u8 *)*newAllocCount);
		memcpy(newData,arrayProxy->newData, sizeof(u8 *)*arrayProxy->newDataAlloc);
		memset(newData+arrayProxy->newDataAlloc, 0, sizeof(u8 *)*(newAllocCount-arrayProxy->newDataAlloc));

		arrayProxy->newData=newData;
		arrayProxy->newDataAlloc=newAllocCount;
		}

	s32 index=arrayProxy->newDataCount++;
	arrayProxy->newData[index]=data;

//	LOG(LOG_INFO,"appendBlockArrayEntry: Allocating Leaf %i",index);

	return index;
}

