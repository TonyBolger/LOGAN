#include "common.h"


void rtpDumpUnpackedSingleBlock(RouteTableUnpackedSingleBlock *block)
{
	LOG(LOG_INFO,"Begin UnpackedSingleBlock");
	// Skip packed stuff

	LOG(LOG_INFO,"Upstream Offsets: %i",block->upstreamOffsetAlloc);
	for(int i=0;i<block->upstreamOffsetAlloc;i++)
		LOG(LOG_INFO,"%i: %i  ",i,block->upstreamOffsets[i]);

	LOG(LOG_INFO,"Downstream Offsets: %i",block->downstreamOffsetAlloc);
	for(int i=0;i<block->downstreamOffsetAlloc;i++)
		LOG(LOG_INFO,"%i: %i  ",i,block->downstreamOffsets[i]);

	LOG(LOG_INFO,"Entry arrays: %i (%i)",block->entryArrayCount, block->entryArrayAlloc);
	for(int i=0;i<block->entryArrayCount;i++)
		{
		LOG(LOG_INFO,"  Entry Array: %i ",i);
		RouteTableUnpackedEntryArray *array=block->entryArrays[i];
		LOG(LOG_INFO,"  Upstream %i  Entries: %i (%i)",array->upstream, array->entryCount, array->entryAlloc);

		for(int j=0;j<array->entryCount;j++)
			LOG(LOG_INFO,"    D: %i  W: %i", array->entries[j].downstream, array->entries[j].width);
		}

	LOG(LOG_INFO,"End UnpackedSingleBlock");
}

RouteTableUnpackedEntryArray *rtpInsertNewEntry(RouteTableUnpackedSingleBlock *unpackedBlock, s32 arrayIndex, s32 entryIndex, s32 downstream, s32 width)
{
	if(arrayIndex<0 || arrayIndex>unpackedBlock->entryArrayCount)
		{
		LOG(LOG_CRITICAL,"ArrayIndex invalid %i",arrayIndex);
		}

	RouteTableUnpackedEntryArray *array=unpackedBlock->entryArrays[arrayIndex];

	if(array->entryCount>=array->entryAlloc)
		{
		int newEntryAlloc=MIN(array->entryAlloc+ROUTEPACKING_ENTRYS_CHUNK, ROUTEPACKING_ENTRYS_MAX);

		if(newEntryAlloc==array->entryAlloc)
			LOG(LOG_CRITICAL,"EntryArray Cannot Resize: Already at Max");

		RouteTableUnpackedEntryArray *newArray=dAlloc(unpackedBlock->disp, sizeof(RouteTableUnpackedEntryArray)+sizeof(RouteTableUnpackedEntry)*newEntryAlloc);

		newArray->upstream=array->upstream;
		newArray->entryAlloc=newEntryAlloc;
		newArray->entryCount=array->entryCount;

		memcpy(newArray->entries, array->entries, sizeof(RouteTableUnpackedEntry)*array->entryAlloc);

		//LOG(LOG_INFO,"Resize array to %p",newArray);
		unpackedBlock->entryArrays[arrayIndex]=newArray;
		array=newArray;
		}

	if(entryIndex<array->entryCount)
		{
		int itemsToMove=array->entryCount-entryIndex;
		memmove(&(array->entries[entryIndex+1]), &(array->entries[entryIndex]), sizeof(RouteTableUnpackedEntry)*itemsToMove);
		}

	array->entryCount++;
	array->entries[entryIndex].downstream=downstream;
	array->entries[entryIndex].width=width;

	return array;
}

RouteTableUnpackedEntryArray *rtpInsertNewEntryArray(RouteTableUnpackedSingleBlock *unpackedBlock, s32 arrayIndex, s32 upstream, s32 entryAlloc)
{
	if(arrayIndex<0 || arrayIndex>unpackedBlock->entryArrayCount)
		{
		LOG(LOG_CRITICAL,"ArrayIndex invalid: %i",arrayIndex);
		}

	if(unpackedBlock->entryArrayCount>=unpackedBlock->entryArrayAlloc)
		{
		int newEntryArrayAlloc=MIN(unpackedBlock->entryArrayAlloc+ROUTEPACKING_ENTRYARRAYS_CHUNK, ROUTEPACKING_ENTRYARRAYS_MAX);

		if(newEntryArrayAlloc==unpackedBlock->entryArrayAlloc)
			LOG(LOG_CRITICAL,"EntryArrays Cannot Resize: Already at Max");

		//LOG(LOG_INFO,"EntryArrays Resize");

		RouteTableUnpackedEntryArray **newEntryArrays=dAlloc(unpackedBlock->disp, sizeof(RouteTableUnpackedEntryArray *)*newEntryArrayAlloc);
		memcpy(newEntryArrays, unpackedBlock->entryArrays, sizeof(RouteTableUnpackedEntryArray *)*unpackedBlock->entryArrayAlloc);

		unpackedBlock->entryArrays=newEntryArrays;
		unpackedBlock->entryArrayAlloc=newEntryArrayAlloc;
		}

	if(arrayIndex<unpackedBlock->entryArrayCount)
		{
		int itemsToMove=unpackedBlock->entryArrayCount-arrayIndex;
		memmove(&(unpackedBlock->entryArrays[arrayIndex+1]), &(unpackedBlock->entryArrays[arrayIndex]), sizeof(RouteTableUnpackedEntryArray *)*itemsToMove);
		}

	RouteTableUnpackedEntryArray *array=dAlloc(unpackedBlock->disp, sizeof(RouteTableUnpackedEntryArray)+sizeof(RouteTableUnpackedEntry)*entryAlloc);
	//LOG(LOG_INFO,"New array to %p",array);

	unpackedBlock->entryArrays[arrayIndex]=array;
	unpackedBlock->entryArrayCount++;

	array->entryAlloc=entryAlloc;
	array->entryCount=0;
	array->upstream=upstream;

	return array;
}

RouteTableUnpackedSingleBlock *rtpAllocUnpackedSingleBlock(MemDispenser *disp, s32 upstreamOffsetAlloc, s32 downstreamOffsetAlloc, s32 entryArrayAlloc)
{
	RouteTableUnpackedSingleBlock *block=dAlloc(disp, sizeof(RouteTableUnpackedSingleBlock));

	block->disp=disp;

	block->packedDataSize=0;
	block->packedUpstreamOffsetFirst=0;
	block->packedUpstreamOffsetAlloc=0;
	block->packedDownstreamOffsetFirst=0;
	block->packedDownstreamOffsetAlloc=0;

	block->upstreamOffsetAlloc=upstreamOffsetAlloc;
	block->upstreamOffsets=dAlloc(disp, sizeof(s32)* upstreamOffsetAlloc);
	memset(block->upstreamOffsets, 0, sizeof(s32)*upstreamOffsetAlloc);

	block->downstreamOffsetAlloc=downstreamOffsetAlloc;
	block->downstreamOffsets=dAlloc(disp, sizeof(s32)* downstreamOffsetAlloc);
	memset(block->downstreamOffsets, 0, sizeof(s32)*downstreamOffsetAlloc);

	block->entryArrayAlloc=entryArrayAlloc;
	block->entryArrays=dAlloc(disp, sizeof(RouteTableUnpackedEntryArray *)* entryArrayAlloc);
	memset(block->entryArrays, 0, sizeof(RouteTableUnpackedEntryArray *)* entryArrayAlloc);

	block->entryArrayCount=0;

	return block;
}

RouteTableUnpackedSingleBlock *rtpUnpackSingleBlock(RouteTablePackedSingleBlock *packedBlock, MemDispenser *disp)
{
	LOG(LOG_CRITICAL,"PackLeaf: rtpUnpackSingleBlock TODO");

	return NULL;
}

void rtpUpdateUnpackedSingleBlockSize(RouteTableUnpackedSingleBlock *unpackedBlock)
{
	LOG(LOG_CRITICAL,"PackLeaf: rtpUpdateUnpackedSingleBlockSize TODO");
}

void rtpPackSingleBlock(RouteTableUnpackedSingleBlock *unpackedBlock, RouteTablePackedSingleBlock *packedBlock)
{
	LOG(LOG_CRITICAL,"PackLeaf: rtpPackSingleBlock TODO");
}

s32 rtpGetPackedSingleBlockSize(RouteTablePackedSingleBlock *packedBlock)
{
	LOG(LOG_CRITICAL,"PackLeaf: rtpGetPackedSingleBlockSize TODO");

	return 0;
}
