#include "common.h"


void rtpDumpUnpackedSingleBlock(RouteTableUnpackedSingleBlock *block)
{
	LOG(LOG_INFO,"Begin UnpackedSingleBlock: %p",block);
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



void rtpRecalculateUnpackedBlockOffsets(RouteTableUnpackedSingleBlock *unpackedBlock)
{
	LOG(LOG_INFO,"rtpRecalculateUnpackedBlockOffsets");

	for(int i=0;i<unpackedBlock->upstreamOffsetAlloc;i++)
		unpackedBlock->upstreamOffsets[i]=0;

	for(int i=0;i<unpackedBlock->downstreamOffsetAlloc;i++)
		unpackedBlock->downstreamOffsets[i]=0;

	for(int i=0;i<unpackedBlock->entryArrayCount;i++)
		{
		RouteTableUnpackedEntryArray *array=unpackedBlock->entryArrays[i];

		s32 upstreamTotal=0;
		for(int j=0;j<array->entryCount;j++)
			{
			s32 width=array->entries[j].width;

			unpackedBlock->downstreamOffsets[array->entries[j].downstream]+=width;
			upstreamTotal+=width;
			}

		unpackedBlock->upstreamOffsets[array->upstream]+=upstreamTotal;
		}



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



RouteTableUnpackedEntryArray *rtpInsertNewDoubleEntry(RouteTableUnpackedSingleBlock *unpackedBlock, s32 arrayIndex, s32 entryIndex,
		s32 downstream1, s32 width1, s32 downstream2, s32 width2)
{
	if(arrayIndex<0 || arrayIndex>unpackedBlock->entryArrayCount)
		{
		LOG(LOG_CRITICAL,"ArrayIndex invalid %i",arrayIndex);
		}

	RouteTableUnpackedEntryArray *array=unpackedBlock->entryArrays[arrayIndex];

	if((array->entryCount+1)>=array->entryAlloc)
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
		memmove(&(array->entries[entryIndex+2]), &(array->entries[entryIndex]), sizeof(RouteTableUnpackedEntry)*itemsToMove);
		}

	array->entryCount+=2;
	array->entries[entryIndex].downstream=downstream1;
	array->entries[entryIndex].width=width1;
	entryIndex++;
	array->entries[entryIndex].downstream=downstream2;
	array->entries[entryIndex].width=width2;

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


RouteTableUnpackedEntryArray *rtpSplitArray(RouteTableUnpackedSingleBlock *block, s16 arrayIndex, s16 entryIndex, s16 *updatedArrayIndexPtr, s16 *updatedEntryIndexPtr)
{
	LOG(LOG_INFO,"rtpSplitArray");

	RouteTableUnpackedEntryArray *oldArray=block->entryArrays[arrayIndex];

	s32 toKeepHalfEntry=((1+oldArray->entryCount)/2);
	s32 toMoveHalfEntry=oldArray->entryCount-toKeepHalfEntry;

	s32 toMoveHalfEntryAlloc=toMoveHalfEntry+ROUTEPACKING_ENTRYS_CHUNK;

	RouteTableUnpackedEntryArray *newArray=rtpInsertNewEntryArray(block, arrayIndex+1, oldArray->upstream, toMoveHalfEntryAlloc);

	memcpy(newArray->entries, oldArray->entries+toKeepHalfEntry, sizeof(RouteTableUnpackedEntry)*toMoveHalfEntry);

	oldArray->entryCount=toKeepHalfEntry;
	newArray->entryCount=toMoveHalfEntry;

	for(int i=newArray->entryCount; i<newArray->entryAlloc; i++)
		{
		newArray->entries[i].downstream=-1;
		newArray->entries[i].width=0;
		}

	for(int i=oldArray->entryCount; i<oldArray->entryAlloc; i++)
		{
		oldArray->entries[i].downstream=-1;
		oldArray->entries[i].width=0;
		}

	if(entryIndex<toKeepHalfEntry)
		{
		*updatedArrayIndexPtr=arrayIndex;
		*updatedEntryIndexPtr=entryIndex;
		}
	else
		{
		*updatedArrayIndexPtr=arrayIndex+1;
		*updatedEntryIndexPtr=entryIndex-toKeepHalfEntry;
		}

	return newArray;

}



RouteTableUnpackedSingleBlock *rtpAllocUnpackedSingleBlock(MemDispenser *disp, s32 upstreamOffsetAlloc, s32 downstreamOffsetAlloc, s32 entryArrayAlloc)
{
	RouteTableUnpackedSingleBlock *block=dAlloc(disp, sizeof(RouteTableUnpackedSingleBlock));

	block->disp=disp;

	memset(&(block->packingInfo),0,sizeof(RouteTablePackingInfo));


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


	/*
	u8 *dataPtr=packedBlock->data+blockHeader&RTP_PACKEDHEADER_PAYLOADSIZE_MASK?2:1;

	s32 sizeUpstreamRange=blockHeader&RTP_PACKEDHEADER_UPSTREAMRANGESIZE_MASK?2:1;
	s32 sizeDownstreamRange=blockHeader&RTP_PACKEDHEADER_DOWNSTREAMRANGESIZE_MASK?2:1;
	s32 sizeOffset=((blockHeader&RTP_PACKEDHEADER_OFFSETSIZE_MASK)>>RTP_PACKEDHEADER_OFFSETSIZE_SHIFT)+1;
	s32 sizeArrayCount=((blockHeader&RTP_PACKEDHEADER_ARRAYCOUNTSIZE_MASK)>>RTP_PACKEDHEADER_ARRAYCOUNTSIZE_SHIFT)+1;

	s32 upstreamBits=blockHeader & RTP_PACKEDHEADER_UPSTREAMRANGESIZE_MASK?16:8;
	s32 downstreamBits=blockHeader & RTP_PACKEDHEADER_DOWNSTREAMRANGESIZE_MASK?16:8;
	s32 entryCountBits=(((blockHeader & RTP_PACKEDHEADER_ENTRYCOUNTSIZE_MASK)>>RTP_PACKEDHEADER_ENTRYCOUNTSIZE_SHIFT)+1)*4;
	s32 widthBits=(((blockHeader & RTP_PACKEDHEADER_WIDTHSIZE_MASK)>>RTP_PACKEDHEADER_WIDTHSIZE_SHIFT)+1);

	dataPtr+=sizeUpstreamRange;
	s32 upstreamAlloc=sizeUpstreamRange==1?(*dataPtr):(*((u16 *)(dataPtr)));
	dataPtr+=sizeUpstreamRange;

	dataPtr+=sizeDownstreamRange;
	s32 downstreamAlloc=sizeDownstreamRange==1?(*dataPtr):(*((u16 *)(dataPtr)));
	dataPtr+=sizeDownstreamRange;

	dataPtr+=upstreamAlloc*sizeUpstreamRange;
	dataPtr+=downstreamAlloc*sizestreamRange;
*/

	return NULL;
}


static void updatePackingInfoSizeAndHeader(RouteTablePackingInfo *packingInfo)
{
	int sizeUpstreamRange=packingInfo->packedUpstreamOffsetLast>U8MAX; 			// 0 = u8, 1 = u16
	int sizeDownstreamRange=packingInfo->packedDownstreamOffsetLast>U8MAX; 		// 0 = u8, 1 = u16

	int sizeOffset=packingInfo->maxOffset==0?0:(31-__builtin_clz(packingInfo->maxOffset))>>3;
																				// 0 = u8, 1 = u16, 2 = u24, 3 = u32
																				// leading 0s: 31 -> 0 -> 0
																				// leading 0s: 24 -> 7 -> 0
																				// leading 0s: 23 -> 8 -> 1
																				// leading 0s: 16 -> 15 -> 1
																				// leading 0s: 15 -> 16 -> 2
																				// leading 0s: 8 -> 23 -> 2
																				// leading 0s: 7 -> 24 -> 3

	int sizeArrayCount=packingInfo->arrayCount>U8MAX;							// 0 = u8, 1 = u16

	int sizeEntryCount=packingInfo->maxEntryCount==0?0:(31-__builtin_clz(packingInfo->maxEntryCount))>>2;
																				// 0 = u4, 7 = u32

	int sizeWidth=packingInfo->maxEntryWidth==0?0:(31-__builtin_clz(packingInfo->maxEntryWidth));
																				// 0 = u1, 31 = u32

	int payloadSize=1+																							// Minimum 1 byte for packedSize
		2+(sizeUpstreamRange<<1)+																				// 2 or 4 bytes for UpstreamRange
		2+(sizeDownstreamRange<<1)+ 																			// 2 or 4 bytes for DownstreamRange
		((sizeOffset+1)*(packingInfo->packedUpstreamOffsetAlloc+packingInfo->packedDownstreamOffsetAlloc))+		// 1 - 4 bytes for each offset index
		1+sizeArrayCount;																						// 1 or 2 bytes for ArrayCount

	int arrayBits=
		(packingInfo->arrayCount*((sizeUpstreamRange<<3)+(sizeEntryCount<<2)+12))+ // 8 or 16 per upstream, plus 4-32 bits per entryCount
		(packingInfo->totalEntryCount*((sizeDownstreamRange<<3)+sizeWidth+9));		   // 8 or 16 per downstream, plus 1-32 bits per width

	payloadSize+=PAD_1BITLENGTH_BYTE(arrayBits);

	int sizePayload=0;

	if(payloadSize>255)
		{
		payloadSize++;
		sizePayload=1;
		}

	packingInfo->payloadSize=payloadSize;
	packingInfo->packedSize=payloadSize+2; // Add space for 2-byte block header

	packingInfo->blockHeader=
			(sizeWidth<<RTP_PACKEDHEADER_WIDTHSIZE_SHIFT)|							// 0-4
			(sizeEntryCount<<RTP_PACKEDHEADER_ENTRYCOUNTSIZE_SHIFT)|				// 5-7
			(sizeArrayCount<<RTP_PACKEDHEADER_ARRAYCOUNTSIZE_SHIFT)|				// 8
			(sizeOffset<<RTP_PACKEDHEADER_OFFSETSIZE_SHIFT)|						// 9-10
			(sizeDownstreamRange<<RTP_PACKEDHEADER_DOWNSTREAMRANGESIZE_SHIFT)| 		// 11
			(sizeUpstreamRange<<RTP_PACKEDHEADER_UPSTREAMRANGESIZE_SHIFT)|			// 12
			(sizePayload<<RTP_PACKEDHEADER_PAYLOADSIZE_SHIFT);						// 13

	LOG(LOG_INFO,"PackedSizes: Payload %i: Up: %i Down: %i Offset: %i Array: %i Entry: %i Width: %i",
			payloadSize, packingInfo->packedUpstreamOffsetLast, packingInfo->packedDownstreamOffsetLast,
			packingInfo->maxOffset, packingInfo->arrayCount, packingInfo->maxEntryCount, packingInfo->maxEntryWidth);

	LOG(LOG_INFO,"PackedHeaderSizes: Payload(1x8): %i Up(1x8): %i Down(1x8): %i Offset(2x8): %i Array(1x8): %i Entry(3x4): %i Width(5x1): %i",
			sizePayload, sizeUpstreamRange, sizeDownstreamRange, sizeOffset, sizeArrayCount, sizeEntryCount, sizeWidth);
}

void rtpUpdateUnpackedSingleBlockPackingInfo(RouteTableUnpackedSingleBlock *block)
{
	s32 firstUpstream=INT32_MAX/2;
	s32 lastUpstream=0;

	s32 firstDownstream=INT32_MAX/2;
	s32 lastDownstream=0;

	s32 maxOffset=0;

	for(int i=0;i<block->upstreamOffsetAlloc;i++)
		{
		s32 offset=block->upstreamOffsets[i];

		if(offset)
			{
			firstUpstream=MIN(firstUpstream, i);
			lastUpstream=MAX(lastUpstream, i);
			maxOffset=MAX(maxOffset,offset);
			}
		}

	for(int i=0;i<block->downstreamOffsetAlloc;i++)
		{
		s32 offset=block->downstreamOffsets[i];

		if(offset)
			{
			firstDownstream=MIN(firstDownstream, i);
			lastDownstream=MAX(lastDownstream, i);
			maxOffset=MAX(maxOffset,offset);
			}
		}

	s32 maxEntryCount=0;
	s32 maxEntryWidth=0;

	s32 arrayCount=block->entryArrayCount;
	s32 totalEntryCount=0;

	for(int i=0;i<arrayCount;i++)
		{
		RouteTableUnpackedEntryArray *array=block->entryArrays[i];

		totalEntryCount+=array->entryCount;
		maxEntryCount=MAX(maxEntryCount, array->entryCount);

		for(int j=0;j<array->entryCount;j++)
			maxEntryWidth=MAX(maxEntryWidth, array->entries[j].width);
		}

	RouteTablePackingInfo *packingInfo=&(block->packingInfo);
	packingInfo->packedUpstreamOffsetFirst=firstUpstream;
	packingInfo->packedUpstreamOffsetLast=lastUpstream;
	packingInfo->packedUpstreamOffsetAlloc=firstUpstream<=lastUpstream?1+lastUpstream-firstUpstream:0;

	packingInfo->packedDownstreamOffsetFirst=firstDownstream;
	packingInfo->packedDownstreamOffsetLast=lastDownstream;
	packingInfo->packedDownstreamOffsetAlloc=firstDownstream<=lastDownstream?1+lastDownstream-firstDownstream:0;

	packingInfo->maxOffset=maxOffset;

	packingInfo->arrayCount=arrayCount;
	packingInfo->totalEntryCount=totalEntryCount;

	packingInfo->maxEntryCount=maxEntryCount;
	packingInfo->maxEntryWidth=maxEntryWidth;

	updatePackingInfoSizeAndHeader(packingInfo);

	LOG(LOG_INFO,"PackLeaf: rtpUpdateUnpackedSingleBlockSize Size: %i",packingInfo->packedSize);
}

void rtpPackSingleBlock(RouteTableUnpackedSingleBlock *unpackedBlock, RouteTablePackedSingleBlock *packedBlock)
{
	LOG(LOG_INFO,"PackLeaf: rtpPackSingleBlock start %p",packedBlock);

	RouteTablePackingInfo *packingInfo=&(unpackedBlock->packingInfo);

	u16 blockHeader=packingInfo->blockHeader;
	packedBlock->blockHeader=blockHeader;

	u8 *dataPtr=packedBlock->data;

	// Section 1: Payload Size

	u32 payloadSize=packingInfo->payloadSize;
	if(blockHeader & RTP_PACKEDHEADER_PAYLOADSIZE_MASK)	// 2 byte payload size
		{
		if(payloadSize<256 || payloadSize>65535)
			LOG(LOG_CRITICAL, "Invalid payload size %i for 2-byte format",payloadSize);

		*((u16 *)(dataPtr))=(u16)(payloadSize);
		dataPtr+=2;
		}
	else
		{
		if(payloadSize>255)
			LOG(LOG_CRITICAL, "Invalid payload size %i for 1-byte format",payloadSize);

		*(dataPtr++)=(u8)(payloadSize);
		}


	// Section 2: Upstream Ranges

	u32 upstreamFirst=packingInfo->packedUpstreamOffsetFirst;
	u32 upstreamAlloc=packingInfo->packedUpstreamOffsetAlloc;

	if(blockHeader & RTP_PACKEDHEADER_UPSTREAMRANGESIZE_MASK)	// 2 byte payload size
		{
		if(upstreamFirst>65535 || upstreamAlloc>65535)
			LOG(LOG_CRITICAL, "Invalid upstreamRange %i %i for 2-byte format",upstreamFirst,upstreamAlloc);

		*((u16 *)(dataPtr))=(u16)(upstreamFirst);
		dataPtr+=2;

		*((u16 *)(dataPtr))=(u16)(upstreamAlloc);
		dataPtr+=2;
		}
	else
		{
		if(upstreamFirst>255 || upstreamAlloc>255)
			LOG(LOG_CRITICAL, "Invalid upstreamRange %i %i for 1-byte format",upstreamFirst,upstreamAlloc);

		*(dataPtr++)=(u8)(upstreamFirst);
		*(dataPtr++)=(u8)(upstreamAlloc);
		}


	// Section 3: Downstream Ranges

	u32 downstreamFirst=packingInfo->packedDownstreamOffsetFirst;
	u32 downstreamAlloc=packingInfo->packedDownstreamOffsetAlloc;

	if(blockHeader & RTP_PACKEDHEADER_DOWNSTREAMRANGESIZE_MASK)	// 2 byte payload size
		{
		if(downstreamFirst>65535 || downstreamAlloc>65535)
			LOG(LOG_CRITICAL, "Invalid downstreamRange %i %i for 2-byte format",downstreamFirst,downstreamAlloc);

		*((u16 *)(dataPtr))=(u16)(downstreamFirst);
		dataPtr+=2;

		*((u16 *)(dataPtr))=(u16)(downstreamAlloc);
		dataPtr+=2;
		}
	else
		{
		if(downstreamFirst>255 || downstreamAlloc>255)
			LOG(LOG_CRITICAL, "Invalid downstreamRange %i %i for 1-byte format",downstreamFirst,downstreamAlloc);

		*(dataPtr++)=(u8)(downstreamFirst);
		*(dataPtr++)=(u8)(downstreamAlloc);
		}


	// Section 4: Offsets Arrays

	s32 *offsetsPtr=NULL;

	switch(blockHeader & RTP_PACKEDHEADER_OFFSETSIZE_MASK)
		{
		case RTP_PACKEDHEADER_OFFSETSIZE_8:
			offsetsPtr=unpackedBlock->upstreamOffsets+upstreamFirst;
			for(int i=0;i<upstreamAlloc;i++)
				{
				u32 offset=*(offsetsPtr++);
				*(dataPtr++)=(u8)(offset);
				}

			offsetsPtr=unpackedBlock->downstreamOffsets+downstreamFirst;
			for(int i=0;i<downstreamAlloc;i++)
				{
				u32 offset=*(offsetsPtr++);
				*(dataPtr++)=(u8)(offset);
				}

			break;

		case RTP_PACKEDHEADER_OFFSETSIZE_16:
			offsetsPtr=unpackedBlock->upstreamOffsets+upstreamFirst;
			for(int i=0;i<upstreamAlloc;i++)
				{
				u32 offset=*(offsetsPtr++);
				*((u16 *)(dataPtr))=(u16)(offset);
				dataPtr+=2;
				}

			offsetsPtr=unpackedBlock->downstreamOffsets+downstreamFirst;
			for(int i=0;i<downstreamAlloc;i++)
				{
				u32 offset=*(offsetsPtr++);
				*((u16 *)(dataPtr))=(u16)(offset);
				dataPtr+=2;
				}

			break;

		case RTP_PACKEDHEADER_OFFSETSIZE_24:
			offsetsPtr=unpackedBlock->upstreamOffsets+upstreamFirst;
			for(int i=0;i<upstreamAlloc;i++)
				{
				u32 offset=*(offsetsPtr++);
				*((u16 *)(dataPtr))=(u16)(offset);
				*(dataPtr+2)=(u8)(offset>>16);
				dataPtr+=3;
				}

			offsetsPtr=unpackedBlock->downstreamOffsets+downstreamFirst;
			for(int i=0;i<downstreamAlloc;i++)
				{
				u32 offset=*(offsetsPtr++);
				*((u16 *)(dataPtr))=(u16)(offset);
				*(dataPtr+2)=(u8)(offset>>16);
				dataPtr+=3;
				}

			break;

		case RTP_PACKEDHEADER_OFFSETSIZE_32:
			offsetsPtr=unpackedBlock->upstreamOffsets+upstreamFirst;
			for(int i=0;i<upstreamAlloc;i++)
				{
				u32 offset=*(offsetsPtr++);
				*((u32 *)(dataPtr))=(u32)(offset);
				dataPtr+=4;
				}

			offsetsPtr=unpackedBlock->downstreamOffsets+downstreamFirst;
			for(int i=0;i<downstreamAlloc;i++)
				{
				u32 offset=*(offsetsPtr++);
				*((u32 *)(dataPtr))=(u32)(offset);
				dataPtr+=4;
				}

			break;
		}


	// Section 5: Route Array Count

	s32 arrayCount=packingInfo->arrayCount;
	if(blockHeader & RTP_PACKEDHEADER_ARRAYCOUNTSIZE_MASK)	// 2 byte array count size
		{
		if(arrayCount>65535 || arrayCount>65535)
			LOG(LOG_CRITICAL, "Invalid arrayCount %i for 2-byte format",arrayCount);

		*((u16 *)(dataPtr))=(u16)(arrayCount);
		dataPtr+=2;
		}
	else
		{
		if(arrayCount>255)
			LOG(LOG_CRITICAL, "Invalid arrayCount %i for 1-byte format",arrayCount);

		*(dataPtr++)=(u8)(arrayCount);
		}

	// Section 6: Route Arrays

	//s32 upstreamBits=upstreamAlloc==0?0:(32-__builtin_clz(upstreamAlloc));
	//s32 downstreamBits=downstreamAlloc==0?0:(32-__builtin_clz(downstreamAlloc));

	s32 upstreamBits=blockHeader & RTP_PACKEDHEADER_UPSTREAMRANGESIZE_MASK?16:8;
	s32 downstreamBits=blockHeader & RTP_PACKEDHEADER_DOWNSTREAMRANGESIZE_MASK?16:8;

	s32 entryCountBits=(((blockHeader & RTP_PACKEDHEADER_ENTRYCOUNTSIZE_MASK)>>RTP_PACKEDHEADER_ENTRYCOUNTSIZE_SHIFT)+1)*4;
	s32 widthBits=(((blockHeader & RTP_PACKEDHEADER_WIDTHSIZE_MASK)>>RTP_PACKEDHEADER_WIDTHSIZE_SHIFT)+1);

	BitPacker bitPacker;

	bpInitPacker(&bitPacker, dataPtr, 0);

	for(int i=0;i<packingInfo->arrayCount;i++)
		{
		RouteTableUnpackedEntryArray *array=unpackedBlock->entryArrays[i];

		bpPackBits(&bitPacker, upstreamBits, array->upstream);
		bpPackBits(&bitPacker, entryCountBits, array->entryCount);

		for(int j=0;j<array->entryCount;j++)
			{
			bpPackBits(&bitPacker, downstreamBits, array->entries[j].downstream);
			bpPackBits(&bitPacker, widthBits, array->entries[j].width);
			}

		}

	dataPtr=bpPackerGetPaddedPtr(&bitPacker);
	u32 usedPayloadSize=dataPtr-packedBlock->data;

	if(packingInfo->payloadSize != usedPayloadSize)
		{
		LOG(LOG_CRITICAL,"Packed Block did not have payload side %i did not match expected %i",
				usedPayloadSize, packingInfo->payloadSize);
		}

	LOG(LOG_INFO,"PackLeaf: rtpPackSingleBlock end");
}

s32 rtpGetPackedSingleBlockSize(RouteTablePackedSingleBlock *packedBlock)
{
	LOG(LOG_INFO,"PackLeaf: rtpGetPackedSingleBlockSize start %p",packedBlock);

	u16 blockHeader=packedBlock->blockHeader;
	s32 payloadSize=(blockHeader&RTP_PACKEDHEADER_PAYLOADSIZE_MASK)?(*((u16 *)(packedBlock->data))):(*((u8 *)(packedBlock->data)));
	s32 totalSize=payloadSize+2;

	LOG(LOG_INFO,"PackLeaf: rtpGetPackedSingleBlockSize size %i",totalSize);

	return totalSize;
}
