
#include "common.h"



static int getRouteTableHeaderSize(int prefixBits, u32 suffixBits, u32 widthBits, u32 forwardEntries, u32 reverseEntries)
{
	prefixBits--;
	suffixBits--;
	widthBits--;

	if((forwardEntries>65535)||(reverseEntries>65535))
		return 10;

	if((prefixBits>7)||(suffixBits>7)||(widthBits>15)||(forwardEntries>63)||(reverseEntries>63))
		return 6;

	else if((prefixBits>3)||(suffixBits>3)||(widthBits>7)||(forwardEntries>15)||(reverseEntries>15))
		return 3;

	return 2;
}


static int encodeRouteTableHeader(u8 *packedData, u32 prefixBits, u32 suffixBits, u32 widthBits, u32 forwardEntries, u32 reverseEntries)
{
	prefixBits--;
	suffixBits--;
	widthBits--;

	if((prefixBits>15)||(suffixBits>15)||(widthBits>31))
		{
		LOG(LOG_CRITICAL,"Cannot encode header for %i %i %i %i %i",prefixBits,suffixBits,widthBits,forwardEntries,reverseEntries);
		}

	if((forwardEntries>65535)||(reverseEntries>65535))
		{
		packedData[0]=(widthBits&0x1F) | 0xE0;
		packedData[1]=((prefixBits&0xF)<<4)|(suffixBits&0xF);
		*((u32 *)(packedData+2))=forwardEntries;
		*((u32 *)(packedData+6))=reverseEntries;

		return 10;
		}

	if((prefixBits>7)||(suffixBits>7)||(widthBits>15)||(forwardEntries>63)||(reverseEntries>63))
		{
		packedData[0]=(widthBits&0x1F) | 0xC0;
		packedData[1]=((prefixBits&0xF)<<4)|(suffixBits&0xF);
		*((u16 *)(packedData+2))=forwardEntries;
		*((u16 *)(packedData+4))=reverseEntries;

		return 6;
		}

	else if((prefixBits>3)||(suffixBits>3)||(widthBits>7)||(forwardEntries>15)||(reverseEntries>15))
		{
		u32 tmp=0x800000 | ((widthBits&0xF)<<18) | ((prefixBits&0x7)<<15) | ((suffixBits&0x7)<<12) | ((forwardEntries&0x3F)<<6) | (reverseEntries&0x3F);
		packedData[2]=tmp & 0xFF;
		tmp>>=8;
		packedData[1]=tmp & 0xFF;
		tmp>>=8;
		packedData[0]=tmp & 0xFF;

		return 3;
		}
	else
		{
		packedData[0]=((widthBits&0x7)<<4) | ((prefixBits&0x3)<<2) | ((suffixBits&0x3));
		packedData[1]=((forwardEntries&0xF)<<4) | (reverseEntries&0xF);

		return 2;
		}
}


static int decodeHeader(u8 *packedData, u32 *prefixBits, u32 *suffixBits, u32 *widthBits, u32 *forwardEntries, u32 *reverseEntries)
{
	u8 sizeFlag=packedData[0]&0xE0;

	int w,p,s,f,r,size;

	if(sizeFlag == 0xE0)
		{
		u8 tmp1=packedData[0];
		u8 tmp2=packedData[1];

		w=(tmp1&0x1F)+1;
		p=((tmp2>>4)&0xF)+1;
		s=(tmp2&0xF)+1;
		f=*((u32 *)(packedData+2));
		r=*((u32 *)(packedData+6));

		size=10;
		}
	else if(sizeFlag == 0xC0)
		{
		u8 tmp1=packedData[0];
		u8 tmp2=packedData[1];

		w=(tmp1&0x1F)+1;
		p=((tmp2>>4)&0xF)+1;
		s=(tmp2&0xF)+1;
		f=*((u16 *)(packedData+2));
		r=*((u16 *)(packedData+4));

		size=6;
		}
	else if((sizeFlag&0xC0)==0x80)
		{
		u32 tmp=packedData[0]<<16 | packedData[1] << 8 | packedData[2];

		w=((tmp>>18)&0xF)+1;
		p=((tmp>>15)&0x7)+1;
		s=((tmp>>12)&0x7)+1;
		f=((tmp>>6)&0x3F);
		r=((tmp)&0x3F);

		size=3;
		}
	else
		{
		u8 tmp1=packedData[0];
		u8 tmp2=packedData[1];

		w=((tmp1>>4)&0x7)+1;
		p=((tmp1>>2)&0x3)+1;
		s=((tmp1)&0x3)+1;
		f=((tmp2>>4)&0xF);
		r=((tmp2)&0xF);

		size=2;
		}

	if(widthBits!=NULL)
		*widthBits=w;

	if(prefixBits!=NULL)
		*prefixBits=p;

	if(suffixBits!=NULL)
		*suffixBits=s;

	if(forwardEntries!=NULL)
		*forwardEntries=f;

	if(reverseEntries!=NULL)
		*reverseEntries=r;

	return size;
}


static void rtaUnpackRoutes(u8 *data, int prefixBits, int suffixBits, int widthBits,
		RouteTableEntry *forwardEntries, RouteTableEntry *reverseEntries, u32 forwardEntryCount, u32 reverseEntryCount,
		int *maxPrefixPtr, int *maxSuffixPtr, int *maxWidthPtr)
{
	int maxPrefix=0,maxSuffix=0,maxWidth=0;

	BitUnpacker unpacker;
	bpInitUnpacker(&unpacker, data, 0);

	for(int i=0;i<forwardEntryCount;i++)
		{
		s32 prefix=bpUnpackBits(&unpacker, prefixBits);
		s32 suffix=bpUnpackBits(&unpacker, suffixBits);
		s32 width=bpUnpackBits(&unpacker, widthBits)+1;

		if(prefix<0 || suffix<0 || width<0)
			{
			LOG(LOG_CRITICAL,"Negative entry in forward route: %i %i %i ",prefix,suffix,width);
			}

		maxPrefix=MAX(maxPrefix,prefix);
		maxSuffix=MAX(maxSuffix,suffix);
		maxWidth=MAX(maxWidth,width);

		forwardEntries[i].prefix=prefix;
		forwardEntries[i].suffix=suffix;
		forwardEntries[i].width=width;
		}

	for(int i=0;i<reverseEntryCount;i++)
		{
		s32 prefix=bpUnpackBits(&unpacker, prefixBits);
		s32 suffix=bpUnpackBits(&unpacker, suffixBits);
		s32 width=bpUnpackBits(&unpacker, widthBits)+1;

		if(prefix<0 || suffix<0 || width<0)
			{
			LOG(LOG_CRITICAL,"Negative entry in reverse route: %i %i %i ",prefix,suffix,width);
			}

		maxPrefix=MAX(maxPrefix,prefix);
		maxSuffix=MAX(maxSuffix,suffix);
		maxWidth=MAX(maxWidth,width);

		reverseEntries[i].prefix=prefix;
		reverseEntries[i].suffix=suffix;
		reverseEntries[i].width=width;
		}

	if(maxPrefixPtr!=NULL)
		*maxPrefixPtr=maxPrefix;

	if(maxSuffixPtr!=NULL)
		*maxSuffixPtr=maxSuffix;

	if(maxWidthPtr!=NULL)
		*maxWidthPtr=maxWidth;
}

static u8 *readRouteTableBuilderPackedData(RouteTableArrayBuilder *builder, u8 *data)
{
	int maxPrefix=0,maxSuffix=0,maxWidth=0;
	u32 prefixBits=0, suffixBits=0, widthBits=0, forwardEntryCount=0, reverseEntryCount=0;
	int headerSize=2;

	if(data!=NULL)
		{
		headerSize=decodeHeader(data, &prefixBits, &suffixBits, &widthBits, &forwardEntryCount, &reverseEntryCount);

		data+=headerSize;

		builder->oldForwardEntries=dAlloc(builder->disp, sizeof(RouteTableEntry)*forwardEntryCount);
		builder->oldReverseEntries=dAlloc(builder->disp, sizeof(RouteTableEntry)*reverseEntryCount);

		builder->oldForwardEntryCount=forwardEntryCount;
		builder->oldReverseEntryCount=reverseEntryCount;

		rtaUnpackRoutes(data, prefixBits, suffixBits, widthBits,
				builder->oldForwardEntries,builder->oldReverseEntries,forwardEntryCount,reverseEntryCount,
				&maxPrefix, &maxSuffix, &maxWidth);
		}
	else
		{
		builder->oldForwardEntries=NULL;
		builder->oldReverseEntries=NULL;

		builder->oldForwardEntryCount=0;
		builder->oldReverseEntryCount=0;
		}

	builder->maxPrefix=maxPrefix;
	builder->maxSuffix=maxSuffix;
	builder->maxWidth=maxWidth;

	int tableSize=PAD_1BITLENGTH_BYTE((prefixBits+suffixBits+widthBits)*(forwardEntryCount+reverseEntryCount));
	int totalSize=headerSize+tableSize;
	builder->totalPackedSize=totalSize;

	builder->oldDataSize=totalSize;

	return data+tableSize;
}


u8 *rtaScanRouteTableArray(u8 *data)
{
	if(data!=NULL)
		{
		u32 prefixBits=0, suffixBits=0, widthBits=0, forwardEntryCount=0, reverseEntryCount=0;

		int headerSize=decodeHeader(data, &prefixBits, &suffixBits, &widthBits, &forwardEntryCount, &reverseEntryCount);

		int tableSize=PAD_1BITLENGTH_BYTE((prefixBits+suffixBits+widthBits)*(forwardEntryCount+reverseEntryCount));
		int totalPackedSize=headerSize+tableSize;

		return data+totalPackedSize;
		}

	return NULL;
}

u8 *rtaInitRouteTableArrayBuilder(RouteTableArrayBuilder *builder, u8 *data, MemDispenser *disp)
{
	builder->disp=disp;

	//LOG(LOG_INFO,"RouteTable init from %p",data);
	data=readRouteTableBuilderPackedData(builder,data);

	builder->newForwardEntries=NULL;
	builder->newReverseEntries=NULL;

	builder->newForwardEntryCount=0;
	builder->newReverseEntryCount=0;

	builder->newForwardEntryAlloc=0;
	builder->newReverseEntryAlloc=0;

	return data;
}

s32 rtaGetRouteTableArrayBuilderDirty(RouteTableArrayBuilder *builder)
{
	return builder->newForwardEntryCount>0 || builder->newReverseEntryCount>0;
}

s32 rtaGetRouteTableArrayBuilderPackedSize(RouteTableArrayBuilder *builder)
{
	return builder->totalPackedSize;
}

static void rtaDumpRoutingTableArray_single(char *name, RouteTableEntry *entries, int entryCount)
{
	LOG(LOG_INFO,"%s Routes %i at %p",name, entryCount, entries);

	for(int i=0;i<entryCount;i++)
		LOG(LOG_INFO,"%i Route - P: %i S: %i W: %i", i, entries[i].prefix, entries[i].suffix, entries[i].width);

}

void rtaDumpRoutingTableArray(RouteTableArrayBuilder *builder)
{
	LOG(LOG_INFO,"Header: MaxPrefix: %i MaxSuffix: %i MaxWidth: %i", builder->maxPrefix, builder->maxSuffix, builder->maxWidth);

	rtaDumpRoutingTableArray_single("Old Forward", builder->oldForwardEntries, builder->oldForwardEntryCount);
	rtaDumpRoutingTableArray_single("New Forward", builder->newForwardEntries, builder->newForwardEntryCount);

	rtaDumpRoutingTableArray_single("Old Reverse", builder->oldReverseEntries, builder->oldReverseEntryCount);
	rtaDumpRoutingTableArray_single("New Reverse", builder->newReverseEntries, builder->newReverseEntryCount);
}

u8 *rtaWriteRouteTableArrayBuilderPackedData(RouteTableArrayBuilder *builder, u8 *data)
{
	u32 prefixBits=bpBitsRequired(builder->maxPrefix);
	u32 suffixBits=bpBitsRequired(builder->maxSuffix);
	u32 widthBits=bpBitsRequired(builder->maxWidth-1);

	if(prefixBits==15 || suffixBits==15 || widthBits==63)
		{
		LOG(LOG_INFO,"Header near full: %i %i %i", builder->maxPrefix, builder->maxSuffix, builder->maxWidth);
		}



//	LOG(LOG_INFO,"Route Header Max: %i %i %i", builder->maxPrefix, builder->maxSuffix, builder->maxWidth);

	RouteTableEntry *forwardEntries,*reverseEntries;
	u32 forwardEntryCount,reverseEntryCount;

	if(builder->newForwardEntryCount>0)
		{
		forwardEntries=builder->newForwardEntries;
		forwardEntryCount=builder->newForwardEntryCount;
		}
	else
		{
		forwardEntries=builder->oldForwardEntries;
		forwardEntryCount=builder->oldForwardEntryCount;
		}

	if(builder->newReverseEntryCount>0)
		{
		reverseEntries=builder->newReverseEntries;
		reverseEntryCount=builder->newReverseEntryCount;
		}
	else
		{
		reverseEntries=builder->oldReverseEntries;
		reverseEntryCount=builder->oldReverseEntryCount;
		}

//	LOG(LOG_INFO,"Route Header: %i %i %i %i %i", prefixBits,suffixBits,widthBits,forwardEntryCount,reverseEntryCount);

	int headerSize=encodeRouteTableHeader(data,prefixBits,suffixBits,widthBits,forwardEntryCount,reverseEntryCount);
	data+=headerSize;

	BitPacker packer;
	bpInitPacker(&packer, data, 0);

	for(int i=0;i<forwardEntryCount;i++)
		{
//		if(forwardEntryCount>1000)
//			LOG(LOG_INFO,"Forward Route - P: %i %i %i", forwardEntries[i].prefix, forwardEntries[i].suffix,  forwardEntries[i].width);

		bpPackBits(&packer, prefixBits, forwardEntries[i].prefix);
		bpPackBits(&packer, suffixBits, forwardEntries[i].suffix);
		bpPackBits(&packer, widthBits, forwardEntries[i].width-1);
		}
	for(int i=0;i<reverseEntryCount;i++)
		{
//		if(reverseEntryCount>1000)
//			LOG(LOG_INFO,"Reverse Route - P: %i %i %i", reverseEntries[i].prefix, reverseEntries[i].suffix,  reverseEntries[i].width);

		bpPackBits(&packer, prefixBits, reverseEntries[i].prefix);
		bpPackBits(&packer, suffixBits, reverseEntries[i].suffix);
		bpPackBits(&packer, widthBits, reverseEntries[i].width-1);
		}

	int tableSize=PAD_1BITLENGTH_BYTE((prefixBits+suffixBits+widthBits)*(forwardEntryCount+reverseEntryCount));

	int size=headerSize+tableSize;

	if(size!=builder->totalPackedSize)
	{
		LOG(LOG_CRITICAL,"Size %i did not match expected size %i",size,builder->totalPackedSize);
	}

	return data+tableSize;
}

static s32 arrayBufferPollInput(RouteTableArrayBuffer *buf)
{
	RouteTableEntry *oldEntryPtr=buf->oldEntryPtr;

	if(oldEntryPtr<buf->oldEntryPtrEnd)
		{
		buf->oldPrefix=oldEntryPtr->prefix;
		buf->oldSuffix=oldEntryPtr->suffix;
		buf->oldWidth=oldEntryPtr->width;

		buf->oldEntryPtr++;


		return buf->oldWidth;
		}
	else
		{
		buf->oldPrefix=0;
		buf->oldSuffix=0;
		buf->oldWidth=0;

		return 0;
		}
}


static void initArrayBuffer(RouteTableArrayBuffer *buf, RouteTableEntry *oldEntryPtr, RouteTableEntry *oldEntryPtrEnd, RouteTableEntry *newEntryPtr, s32 maxWidth)
{
	buf->oldEntryPtr=oldEntryPtr;
	buf->oldEntryPtrEnd=oldEntryPtrEnd;
	buf->newEntryPtr=newEntryPtr;

	arrayBufferPollInput(buf);

	buf->newPrefix=0;
	buf->newSuffix=0;
	buf->newWidth=0;

	buf->maxWidth=maxWidth;
}

static void arrayBufferFlushOutputEntry(RouteTableArrayBuffer *buf)
{
	RouteTableEntry *newEntryPtr=buf->newEntryPtr;

	if(buf->newWidth>0)
		{
		newEntryPtr->prefix=buf->newPrefix;
		newEntryPtr->suffix=buf->newSuffix;
		newEntryPtr->width=buf->newWidth;

		buf->maxWidth=MAX(buf->maxWidth,buf->newWidth);

		buf->newEntryPtr++;
		buf->newWidth=0;
		}
}

static s32 arrayBufferTransfer(RouteTableArrayBuffer *buf)
{
	s32 widthToTransfer=buf->oldWidth;

	if(buf->newWidth>0)
		{
		if(buf->newPrefix!=buf->oldPrefix || buf->newSuffix!=buf->oldSuffix)
			arrayBufferFlushOutputEntry(buf);
		else
			{
			buf->newWidth+=widthToTransfer;
			arrayBufferPollInput(buf);
			return widthToTransfer;
			}
		}

	buf->newPrefix=buf->oldPrefix;
	buf->newSuffix=buf->oldSuffix;
	buf->newWidth=widthToTransfer;

	arrayBufferPollInput(buf);
	return widthToTransfer;

}

static s32 arrayBufferPartialTransfer(RouteTableArrayBuffer *buf, s32 requestedTransferWidth)
{
	s32 widthToTransfer=MIN(requestedTransferWidth, buf->oldWidth);
	buf->oldWidth-=widthToTransfer;

	if(buf->newWidth>0)
		{
		if(buf->newPrefix!=buf->oldPrefix || buf->newSuffix!=buf->oldSuffix)
			arrayBufferFlushOutputEntry(buf);
		else
			{
			buf->newWidth+=widthToTransfer;
			if(buf->oldWidth==0)
				arrayBufferPollInput(buf);

			return widthToTransfer;
			}
		}

	buf->newPrefix=buf->oldPrefix;
	buf->newSuffix=buf->oldSuffix;
	buf->newWidth=widthToTransfer;

	if(buf->oldWidth==0)
		arrayBufferPollInput(buf);

	return widthToTransfer;
}


static void arrayBufferPushOutput(RouteTableArrayBuffer *buf, s32 prefix, s32 suffix, s32 width)
{
	if(buf->newWidth>0)
		{
		if(buf->newPrefix!=prefix || buf->newSuffix!=suffix)
			arrayBufferFlushOutputEntry(buf);
		else
			{
			buf->newWidth+=width;
			return;
			}
		}

	buf->newPrefix=prefix;
	buf->newSuffix=suffix;
	buf->newWidth=width;
}


/* Original version - no buffer

static RouteTableEntry *rtaMergeRoutes_ordered_forwardSingle(//RouteTableArrayBuilder *builder,
		RouteTableEntry *oldEntryPtr, RouteTableEntry *oldEntryEnd, RouteTableEntry *newEntryPtr, RoutePatch *patch, int *maxWidth)
{
	int targetPrefix=patch->prefixIndex;
	int targetSuffix=patch->suffixIndex;
	int minEdgePosition=(*(patch->rdiPtr))->minEdgePosition;
	int maxEdgePosition=(*(patch->rdiPtr))->maxEdgePosition;

	int upstreamEdgeOffset=0;
	int downstreamEdgeOffset=0;

//	LOG(LOG_INFO,"rtaMergeRoutes_ordered_forwardSingle %i %i with %i %i",targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

	while(oldEntryPtr<oldEntryEnd && oldEntryPtr->prefix<targetPrefix) // Skip lower upstream
		{
		if(oldEntryPtr->suffix==targetSuffix)
			downstreamEdgeOffset+=oldEntryPtr->width;

		*(newEntryPtr++)=*(oldEntryPtr++);
		}

	while(oldEntryPtr<oldEntryEnd && oldEntryPtr->prefix==targetPrefix &&
			((upstreamEdgeOffset+oldEntryPtr->width)<minEdgePosition ||
			((upstreamEdgeOffset+oldEntryPtr->width)==minEdgePosition && oldEntryPtr->suffix!=targetSuffix))) // Skip earlier upstream
		{
		upstreamEdgeOffset+=oldEntryPtr->width;
		if(oldEntryPtr->suffix==targetSuffix)
			downstreamEdgeOffset+=oldEntryPtr->width;

		*(newEntryPtr++)=*(oldEntryPtr++);
		}

	while(oldEntryPtr<oldEntryEnd && oldEntryPtr->prefix==targetPrefix &&
			(upstreamEdgeOffset+oldEntryPtr->width)<=maxEdgePosition && oldEntryPtr->suffix<targetSuffix) // Skip matching upstream with earlier downstream
		{
		if(oldEntryPtr->suffix==targetSuffix)
			downstreamEdgeOffset+=oldEntryPtr->width;

		upstreamEdgeOffset+=oldEntryPtr->width;
		*(newEntryPtr++)=*(oldEntryPtr++);
		}

	if(oldEntryPtr==oldEntryEnd || oldEntryPtr->prefix>targetPrefix || (oldEntryPtr->suffix!=targetSuffix && upstreamEdgeOffset>=minEdgePosition))
																								// No suitable existing entry, but can insert/append here
		{
		int minMargin=upstreamEdgeOffset-minEdgePosition; // Margin between current position and minimum position: Must be zero or positive
		int maxMargin=maxEdgePosition-upstreamEdgeOffset; // Margin between current position and maximum position: Must be zero or positive

		if(minMargin<0 || maxMargin<0)
			{
//			rtaDumpRoutingTableArray(builder);

			LOG(LOG_INFO,"Failed to add forward route for prefix %i suffix %i",targetPrefix,targetSuffix);
			LOG(LOG_INFO,"Current edge offset %i minEdgePosition %i maxEdgePosition %i",upstreamEdgeOffset,minEdgePosition,maxEdgePosition);
			LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets to new entry
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		newEntryPtr->prefix=targetPrefix;
		newEntryPtr->suffix=targetSuffix;
		newEntryPtr->width=1;
		newEntryPtr++;
		}
	else if(oldEntryPtr->prefix==targetPrefix && oldEntryPtr->suffix==targetSuffix) // Existing entry suitable, widen
		{
		int upstreamEdgeOffsetEnd=upstreamEdgeOffset+oldEntryPtr->width;

		// Adjust offsets
		if(minEdgePosition<upstreamEdgeOffset) // Trim upstream range to entry
			minEdgePosition=upstreamEdgeOffset;

		if(maxEdgePosition>upstreamEdgeOffsetEnd) // Trim upstream range to entry
			maxEdgePosition=upstreamEdgeOffsetEnd;

		int minOffset=minEdgePosition-upstreamEdgeOffset; // Offset of minimum position: zero or positive
		int maxOffset=maxEdgePosition-upstreamEdgeOffset; // Offset of maximum position: zero or positive

		if(minOffset<0 || maxOffset<0 || minOffset>maxOffset)
			{
			LOG(LOG_CRITICAL,"Invalid offsets or gap detected in route insert - Min: %i Max: %i",minOffset,maxOffset);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

		// Map offsets to new entry
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset+minOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset+maxOffset;

		newEntryPtr->prefix=targetPrefix;
		newEntryPtr->suffix=targetSuffix;

		int width=oldEntryPtr->width+1;
		newEntryPtr->width=width;
		*maxWidth=MAX(*maxWidth,width);

		newEntryPtr++;
		oldEntryPtr++;
		}
	else // Existing entry unsuitable, split and insert
		{
		int targetEdgePosition=oldEntryPtr->suffix>targetSuffix?minEdgePosition:maxEdgePosition; // Early or late split

		int splitWidth1=targetEdgePosition-upstreamEdgeOffset;
		int splitWidth2=oldEntryPtr->width-splitWidth1;

		if(splitWidth1<=0 || splitWidth2<=0)
			{
			LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i from %i",splitWidth1, splitWidth2);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		newEntryPtr->prefix=oldEntryPtr->prefix;	// Insert first part of split
		newEntryPtr->suffix=oldEntryPtr->suffix;
		newEntryPtr->width=splitWidth1;
		newEntryPtr++;

		newEntryPtr->prefix=targetPrefix;			// Insert new
		newEntryPtr->suffix=targetSuffix;
		newEntryPtr->width=1;
		newEntryPtr++;

		newEntryPtr->prefix=oldEntryPtr->prefix;	// Insert second part of split
		newEntryPtr->suffix=oldEntryPtr->suffix;
		newEntryPtr->width=splitWidth2;
		newEntryPtr++;

		oldEntryPtr++;
		}

	while(oldEntryPtr<oldEntryEnd) // Copy remaining old entries
		*(newEntryPtr++)=*(oldEntryPtr++);

	return newEntryPtr;
}


*/




static RouteTableEntry *rtaMergeRoutes_ordered_forwardSingle(RouteTableArrayBuilder *builder,
		RouteTableEntry *oldEntryPtr, RouteTableEntry *oldEntryEnd, RouteTableEntry *newEntryPtr, RoutePatch *patch, int *maxWidth)
{
	int targetPrefix=patch->prefixIndex;
	int targetSuffix=patch->suffixIndex;
	int minEdgePosition=patch->rdiPtr->minEdgePosition;
	int maxEdgePosition=patch->rdiPtr->maxEdgePosition;

	int upstreamEdgeOffset=0;
	int downstreamEdgeOffset=0;

//	LOG(LOG_INFO,"rtaMergeRoutes_ordered_forwardSingle %i %i with %i %i",targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

	RouteTableArrayBuffer buf;
	initArrayBuffer(&buf, oldEntryPtr, oldEntryEnd, newEntryPtr, *maxWidth);

//	LOG(LOG_INFO,"Ptrs %i %i", xoldEntryPtr, xoldEntryEnd);

//	LOG(LOG_INFO,"Phase 0 - offsets %i %i",upstreamEdgeOffset, downstreamEdgeOffset);

	while(buf.oldWidth && buf.oldPrefix<targetPrefix) // Skip lower upstream
		{
		if(buf.oldSuffix==targetSuffix)
			downstreamEdgeOffset+=buf.oldWidth;

		arrayBufferTransfer(&buf);
		}

//	LOG(LOG_INFO,"Phase 1 - offsets %i %i",upstreamEdgeOffset, downstreamEdgeOffset);

	while(buf.oldWidth && buf.oldPrefix==targetPrefix &&
			((upstreamEdgeOffset+buf.oldWidth)<minEdgePosition ||
			((upstreamEdgeOffset+buf.oldWidth)==minEdgePosition && buf.oldSuffix!=targetSuffix))) // Skip earlier upstream
		{
		upstreamEdgeOffset+=buf.oldWidth;
		if(buf.oldSuffix==targetSuffix)
			downstreamEdgeOffset+=buf.oldWidth;

		arrayBufferTransfer(&buf);
		}

//	LOG(LOG_INFO,"Phase 2 - offsets %i %i",upstreamEdgeOffset, downstreamEdgeOffset);

	while(buf.oldWidth && buf.oldPrefix==targetPrefix &&
			(upstreamEdgeOffset+buf.oldWidth)<=maxEdgePosition && buf.oldSuffix<targetSuffix) // Skip matching upstream with earlier downstream
		{
		if(buf.oldSuffix==targetSuffix)
			downstreamEdgeOffset+=buf.oldWidth;

		upstreamEdgeOffset+=buf.oldWidth;

		arrayBufferTransfer(&buf);
		}

//	LOG(LOG_INFO,"Phase 3 - offsets %i %i",upstreamEdgeOffset, downstreamEdgeOffset);

	if(buf.oldWidth==0 || buf.oldPrefix>targetPrefix || (buf.oldSuffix!=targetSuffix && upstreamEdgeOffset>=minEdgePosition))
																								// No suitable existing entry, but can insert/append here
		{
		int minMargin=upstreamEdgeOffset-minEdgePosition; // Margin between current position and minimum position: Must be zero or positive
		int maxMargin=maxEdgePosition-upstreamEdgeOffset; // Margin between current position and maximum position: Must be zero or positive

		if(minMargin<0 || maxMargin<0)
			{
			rtaDumpRoutingTableArray(builder);

			LOG(LOG_INFO,"Failed to add forward route for prefix %i suffix %i",targetPrefix,targetSuffix);
			LOG(LOG_INFO,"Current edge offset %i minEdgePosition %i maxEdgePosition %i",upstreamEdgeOffset,minEdgePosition,maxEdgePosition);
			LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets to new entry
		patch->rdiPtr->minEdgePosition=downstreamEdgeOffset;
		patch->rdiPtr->maxEdgePosition=downstreamEdgeOffset;

		arrayBufferPushOutput(&buf, targetPrefix, targetSuffix, 1);
		}
	else if(buf.oldPrefix==targetPrefix && buf.oldSuffix==targetSuffix) // Existing entry suitable, widen
		{
		int upstreamEdgeOffsetEnd=upstreamEdgeOffset+buf.oldWidth;

		// Adjust offsets
		if(minEdgePosition<upstreamEdgeOffset) // Trim upstream range to entry
			minEdgePosition=upstreamEdgeOffset;

		if(maxEdgePosition>upstreamEdgeOffsetEnd) // Trim upstream range to entry
			maxEdgePosition=upstreamEdgeOffsetEnd;

		int minOffset=minEdgePosition-upstreamEdgeOffset; // Offset of minimum position: zero or positive
		int maxOffset=maxEdgePosition-upstreamEdgeOffset; // Offset of maximum position: zero or positive

		if(minOffset<0 || maxOffset<0 || minOffset>maxOffset)
			{
			LOG(LOG_CRITICAL,"Invalid offsets or gap detected in route insert - Min: %i Max: %i",minOffset,maxOffset);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

		// Map offsets to new entry
		patch->rdiPtr->minEdgePosition=downstreamEdgeOffset+minOffset;
		patch->rdiPtr->maxEdgePosition=downstreamEdgeOffset+maxOffset;

		arrayBufferPushOutput(&buf, targetPrefix, targetSuffix, 1);

		s32 trans=arrayBufferPartialTransfer(&buf, maxOffset);
		if(trans!=maxOffset)
			LOG(LOG_CRITICAL,"Widening transfer size mismatch %i %i",maxOffset, trans);

		}
	else // Existing entry unsuitable, split and insert
		{
		int targetEdgePosition=buf.oldSuffix>targetSuffix?minEdgePosition:maxEdgePosition; // Early or late split

		int splitWidth1=targetEdgePosition-upstreamEdgeOffset;
		int splitWidth2=buf.oldWidth-splitWidth1;

		if(splitWidth1<=0 || splitWidth2<=0)
			{
			LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i from %i",splitWidth1, splitWidth2);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets
		patch->rdiPtr->minEdgePosition=downstreamEdgeOffset;
		patch->rdiPtr->maxEdgePosition=downstreamEdgeOffset;

		s32 transWidth1=arrayBufferPartialTransfer(&buf, splitWidth1);

		if(transWidth1!=splitWidth1)
			LOG(LOG_CRITICAL,"Split transfer size mismatch %i %i",splitWidth1, transWidth1);

		arrayBufferPushOutput(&buf, targetPrefix, targetSuffix, 1);
		}

	while(buf.oldWidth) // Copy remaining old entries
		arrayBufferTransfer(&buf);

	arrayBufferFlushOutputEntry(&buf);

	*maxWidth=MAX(*maxWidth,buf.maxWidth);

	return buf.newEntryPtr;
}




RouteTableEntry *rtaMergeRoutes_ordered_forwardMulti(RouteTableArrayBuilder *builder,
		RouteTableEntry *oldEntryPtr, RouteTableEntry *oldEntryEnd, RouteTableEntry *newEntryPtr, RoutePatch *patchPtr, RoutePatch *patchPtrEnd,
		s32 *upstreamOffsets, s32 *downstreamOffsets, int *maxWidth)
{
	//LOG(LOG_INFO,"rtaMergeRoutes_ordered_forwardMulti - %i",(patchPtrEnd-patchPtr));

	RouteTableArrayBuffer buf;
	initArrayBuffer(&buf, oldEntryPtr, oldEntryEnd, newEntryPtr, *maxWidth);

//	rtaDumpRoutingTableArray(builder);

	while(patchPtr<patchPtrEnd)
		{
		//LOG(LOG_INFO,"rtaMergeRoutes_ordered_forwardMulti");

		int targetPrefix=patchPtr->prefixIndex;
		int targetSuffix=patchPtr->suffixIndex;
		int minEdgePosition=patchPtr->rdiPtr->minEdgePosition;
		int maxEdgePosition=patchPtr->rdiPtr->maxEdgePosition;

		int expectedMaxEdgePosition=maxEdgePosition+1;
		RoutePatch *patchGroupPtr=patchPtr+1;  // Make groups of compatible inserts for combined processing

		if(minEdgePosition!=TR_INIT_MINEDGEPOSITION || maxEdgePosition!=TR_INIT_MAXEDGEPOSITION)
			{
			while(patchGroupPtr<patchPtrEnd && patchGroupPtr->prefixIndex==targetPrefix && patchGroupPtr->suffixIndex==targetSuffix &&
					patchGroupPtr->rdiPtr->minEdgePosition == minEdgePosition && patchGroupPtr->rdiPtr->maxEdgePosition == expectedMaxEdgePosition)
				{
				patchGroupPtr++;
				expectedMaxEdgePosition++;
				}
			}
		else
			{
			while(patchGroupPtr<patchPtrEnd && patchGroupPtr->prefixIndex==targetPrefix && patchGroupPtr->suffixIndex==targetSuffix &&
					patchGroupPtr->rdiPtr->minEdgePosition == TR_INIT_MINEDGEPOSITION && patchGroupPtr->rdiPtr->maxEdgePosition == TR_INIT_MAXEDGEPOSITION)
				patchGroupPtr++;
			}

		while(buf.oldWidth && buf.oldPrefix<targetPrefix) // Skip lower upstream
			{
			s32 width=buf.oldWidth;
			upstreamOffsets[buf.oldPrefix]+=width;
			downstreamOffsets[buf.oldSuffix]+=width;

			arrayBufferTransfer(&buf);
			}

		//s32 upstreamEdgeOffset=upstreamOffsets[oldEntryPtr->prefix];

		while(buf.oldWidth && buf.oldPrefix==targetPrefix &&
				((upstreamOffsets[buf.oldPrefix]+buf.oldWidth)<minEdgePosition ||
				((upstreamOffsets[buf.oldPrefix]+buf.oldWidth)==minEdgePosition && buf.oldSuffix!=targetSuffix))) // Skip earlier upstream
			{
			s32 width=buf.oldWidth;
			upstreamOffsets[buf.oldPrefix]+=width;
			downstreamOffsets[buf.oldSuffix]+=width;

			arrayBufferTransfer(&buf);
			}

		while(buf.oldWidth && buf.oldPrefix==targetPrefix &&
			(upstreamOffsets[buf.oldPrefix]+buf.oldWidth)<=maxEdgePosition && buf.oldSuffix<targetSuffix) // Skip matching upstream with earlier downstream
			{
			s32 width=buf.oldWidth;
			upstreamOffsets[buf.oldPrefix]+=width;
			downstreamOffsets[buf.oldSuffix]+=width;

			arrayBufferTransfer(&buf);
			}

		s32 upstreamEdgeOffset=upstreamOffsets[targetPrefix];
		s32 downstreamEdgeOffset=downstreamOffsets[targetSuffix];

		if(buf.oldWidth==0 || buf.oldPrefix>targetPrefix || (buf.oldSuffix!=targetSuffix && upstreamEdgeOffset>=minEdgePosition))
			{
			int minMargin=upstreamEdgeOffset-minEdgePosition; // Margin between current position and minimum position: Must be zero or positive
			int maxMargin=maxEdgePosition-upstreamEdgeOffset; // Margin between current position and maximum position: Must be zero or positive

			if(minMargin<0 || maxMargin<0)
				{
				rtaDumpRoutingTableArray(builder);

				LOG(LOG_INFO,"Failed to add forward route for prefix %i suffix %i",targetPrefix,targetSuffix);
				LOG(LOG_INFO,"Current edge offset %i minEdgePosition %i maxEdgePosition %i",upstreamEdgeOffset,minEdgePosition,maxEdgePosition);
				LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
				}

			// LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

			// Map offsets to new entry
			patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
			patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset;

			patchPtr++;
			int width=1;

			while(patchPtr<patchGroupPtr) // Remaining entries in group gain width
				{
				patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
				patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+width;

				patchPtr++;
				width++;
				}

			arrayBufferPushOutput(&buf, targetPrefix, targetSuffix, width);

			upstreamOffsets[targetPrefix]+=width;
			downstreamOffsets[targetSuffix]+=width;
			}
		else if(buf.oldPrefix==targetPrefix && buf.oldSuffix==targetSuffix) // Existing entry suitable, widen
			{
//			LOG(LOG_INFO,"Widening");

			int upstreamEdgeOffsetEnd=upstreamEdgeOffset+buf.oldWidth;

			// Adjust offsets
			if(minEdgePosition<upstreamEdgeOffset) // Trim upstream range to entry
				minEdgePosition=upstreamEdgeOffset;

			if(maxEdgePosition>upstreamEdgeOffsetEnd) // Trim upstream range to entry
				maxEdgePosition=upstreamEdgeOffsetEnd;

			int minOffset=minEdgePosition-upstreamEdgeOffset; // Offset of minimum position: zero or positive
			int maxOffset=maxEdgePosition-upstreamEdgeOffset; // Offset of maximum position: zero or positive

			if(minOffset<0 || maxOffset<0 || minOffset>maxOffset)
				{
				LOG(LOG_INFO,"OldEntry Offset Range: %i %i", upstreamEdgeOffset, upstreamEdgeOffsetEnd);
				LOG(LOG_INFO,"Min / Max position: %i %i", minEdgePosition, maxEdgePosition);

				LOG(LOG_CRITICAL,"Invalid offsets or gap detected in route insert - Min: %i Max: %i",minOffset,maxOffset);
				}

	//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

			// Map offsets to new entry
			patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset+minOffset;
			patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+maxOffset;

//			LOG(LOG_INFO,"Handoff %i %i to RDI: %p",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset, (*(patchPtr->rdiPtr)));

			patchPtr++;
			int width=1;

			while(patchPtr<patchGroupPtr) // Remaining entries in group gain width
				{
				patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset+minOffset;
				patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+maxOffset+width;

//				LOG(LOG_INFO,"Handoff %i %i to RDI: %p",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

				patchPtr++;
				width++;
				}

			arrayBufferPushOutput(&buf, targetPrefix, targetSuffix, width);
			s32 trans=arrayBufferPartialTransfer(&buf, maxOffset);
			if(trans!=maxOffset)
				LOG(LOG_CRITICAL,"Widening transfer size mismatch %i %i",maxOffset, trans);

//			LOG(LOG_INFO,"Post transfer: Min %i Max %i",minOffset, maxOffset);

			upstreamOffsets[targetPrefix]+=maxOffset+width;
			downstreamOffsets[targetSuffix]+=maxOffset+width;
			}
		else // Existing entry unsuitable, split and insert
			{
			int targetEdgePosition=buf.oldSuffix>targetSuffix?minEdgePosition:maxEdgePosition; // Early or late split
			int splitWidth1=targetEdgePosition-upstreamEdgeOffset;
			int splitWidth2=buf.oldWidth-splitWidth1;

			if(splitWidth1<=0 || splitWidth2<=0)
				{
				LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i from %i",splitWidth1, splitWidth2);
				}

			//LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

			// Map offsets
			patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
			patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset;

			patchPtr++;
			int width=1;

			while(patchPtr<patchGroupPtr) // Remaining entries in group gain width
				{
				patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
				patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+width;

				patchPtr++;
				width++;
				}

			upstreamOffsets[buf.oldPrefix]+=splitWidth1;
			downstreamOffsets[buf.oldSuffix]+=splitWidth1;

			s32 transWidth1=arrayBufferPartialTransfer(&buf, splitWidth1);

			if(transWidth1!=splitWidth1)
				LOG(LOG_CRITICAL,"Split transfer size mismatch %i %i",splitWidth1, transWidth1);

			arrayBufferPushOutput(&buf, targetPrefix, targetSuffix, width);

			upstreamOffsets[targetPrefix]+=width;
			downstreamOffsets[targetSuffix]+=width;
			}

		}

	while(buf.oldWidth) // Copy remaining old entries
		arrayBufferTransfer(&buf);

	arrayBufferFlushOutputEntry(&buf);

	*maxWidth=MAX(*maxWidth,buf.maxWidth);

	return buf.newEntryPtr;
}



/* Original version - no buffer

static RouteTableEntry *rtaMergeRoutes_ordered_reverseSingle(RouteTableEntry *oldEntryPtr, RouteTableEntry *oldEntryEnd, RouteTableEntry *newEntryPtr, RoutePatch *patch, int *maxWidth)
{
	int targetPrefix=patch->prefixIndex;
	int targetSuffix=patch->suffixIndex;
	int minEdgePosition=(*(patch->rdiPtr))->minEdgePosition;
	int maxEdgePosition=(*(patch->rdiPtr))->maxEdgePosition;

	int upstreamEdgeOffset=0;
	int downstreamEdgeOffset=0;

//	LOG(LOG_INFO,"rtaMergeRoutes_ordered_reverseSingle %i %i with %i %i",targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

	while(oldEntryPtr<oldEntryEnd && oldEntryPtr->suffix<targetSuffix) // Skip lower upstream
		{
		if(oldEntryPtr->prefix==targetPrefix)
			downstreamEdgeOffset+=oldEntryPtr->width;

		*(newEntryPtr++)=*(oldEntryPtr++);
		}

	while(oldEntryPtr<oldEntryEnd && oldEntryPtr->suffix==targetSuffix &&
			((upstreamEdgeOffset+oldEntryPtr->width)<minEdgePosition ||
			((upstreamEdgeOffset+oldEntryPtr->width)==minEdgePosition && oldEntryPtr->prefix!=targetPrefix))) // Skip earlier upstream
		{
		upstreamEdgeOffset+=oldEntryPtr->width;
		if(oldEntryPtr->prefix==targetPrefix)
			downstreamEdgeOffset+=oldEntryPtr->width;

		*(newEntryPtr++)=*(oldEntryPtr++);
		}

	while(oldEntryPtr<oldEntryEnd && oldEntryPtr->suffix==targetSuffix &&
			(upstreamEdgeOffset+oldEntryPtr->width)<=maxEdgePosition && oldEntryPtr->prefix<targetPrefix)  // Skip matching upstream with earlier downstream
		{
		if(oldEntryPtr->prefix==targetPrefix)
			downstreamEdgeOffset+=oldEntryPtr->width;

		upstreamEdgeOffset+=oldEntryPtr->width;
		*(newEntryPtr++)=*(oldEntryPtr++);
		}

	if(oldEntryPtr==oldEntryEnd || oldEntryPtr->suffix>targetSuffix || (oldEntryPtr->prefix!=targetPrefix && upstreamEdgeOffset>=minEdgePosition))
																							// No suitable existing entry, but can insert/append here
		{
		int minMargin=upstreamEdgeOffset-minEdgePosition; // Margin between current position and minimum position: Must be zero or positive
		int maxMargin=maxEdgePosition-upstreamEdgeOffset; // Margin between current position and maximum position: Must be zero or positive

		if(minMargin<0 || maxMargin<0)
			{
			LOG(LOG_INFO,"Current edge offset %i",upstreamEdgeOffset);
			LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets to new entry
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		newEntryPtr->prefix=targetPrefix;
		newEntryPtr->suffix=targetSuffix;
		newEntryPtr->width=1;
		newEntryPtr++;
		}
	else if(oldEntryPtr->prefix==targetPrefix) // Existing entry suitable, widen
		{
		int upstreamEdgeOffsetEnd=upstreamEdgeOffset+oldEntryPtr->width;
		// Adjust offsets

		if(minEdgePosition<upstreamEdgeOffset) // Trim upstream range to entry
			minEdgePosition=upstreamEdgeOffset;

		if(maxEdgePosition>upstreamEdgeOffsetEnd) // Trim upstream range to entry
			maxEdgePosition=upstreamEdgeOffsetEnd;

		int minOffset=minEdgePosition-upstreamEdgeOffset; // Offset of minimum position: zero or positive
		int maxOffset=maxEdgePosition-upstreamEdgeOffset; // Offset of maximum position: zero or positive

		if(minOffset<0 || maxOffset<0 || minOffset>maxOffset)
			{
			LOG(LOG_CRITICAL,"Invalid offsets or gap detected in route insert - Min: %i Max: %i",minOffset,maxOffset);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

		// Map offsets to new entry
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset+minOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset+maxOffset;

		newEntryPtr->prefix=targetPrefix;
		newEntryPtr->suffix=targetSuffix;

		int width=oldEntryPtr->width+1;
		newEntryPtr->width=width;
		*maxWidth=MAX(*maxWidth,width);

		newEntryPtr++;
		oldEntryPtr++;
		}
	else // Existing entry unsuitable, split and insert
		{
		int targetEdgePosition=oldEntryPtr->prefix>targetPrefix?minEdgePosition:maxEdgePosition; // Early or late split

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets
		(*(patch->rdiPtr))->minEdgePosition=downstreamEdgeOffset;
		(*(patch->rdiPtr))->maxEdgePosition=downstreamEdgeOffset;

		int splitWidth1=targetEdgePosition-upstreamEdgeOffset;
		int splitWidth2=oldEntryPtr->width-splitWidth1;

		if(splitWidth1<=0 || splitWidth2<=0)
			{
			LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i",splitWidth1, splitWidth2);
			}

		newEntryPtr->prefix=oldEntryPtr->prefix;	// Insert first part of split
		newEntryPtr->suffix=oldEntryPtr->suffix;
		newEntryPtr->width=splitWidth1;
		newEntryPtr++;

		newEntryPtr->prefix=targetPrefix;			// Insert new
		newEntryPtr->suffix=targetSuffix;
		newEntryPtr->width=1;
		newEntryPtr++;

		newEntryPtr->prefix=oldEntryPtr->prefix;	// Insert second part of split
		newEntryPtr->suffix=oldEntryPtr->suffix;
		newEntryPtr->width=splitWidth2;
		newEntryPtr++;

		oldEntryPtr++;
		}

	while(oldEntryPtr<oldEntryEnd) // Copy remaining old entries
		*(newEntryPtr++)=*(oldEntryPtr++);

	return newEntryPtr;
}


*/





static RouteTableEntry *rtaMergeRoutes_ordered_reverseSingle(RouteTableArrayBuilder *builder,
		RouteTableEntry *oldEntryPtr, RouteTableEntry *oldEntryEnd, RouteTableEntry *newEntryPtr, RoutePatch *patch, int *maxWidth)
{
	int targetPrefix=patch->prefixIndex;
	int targetSuffix=patch->suffixIndex;
	int minEdgePosition=patch->rdiPtr->minEdgePosition;
	int maxEdgePosition=patch->rdiPtr->maxEdgePosition;

	int upstreamEdgeOffset=0;
	int downstreamEdgeOffset=0;

//	LOG(LOG_INFO,"rtaMergeRoutes_ordered_reverseSingle %i %i with %i %i",targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

	RouteTableArrayBuffer buf;
	initArrayBuffer(&buf, oldEntryPtr, oldEntryEnd, newEntryPtr, *maxWidth);

//	LOG(LOG_INFO,"Ptrs %i %i", xoldEntryPtr, xoldEntryEnd);

//	LOG(LOG_INFO,"Phase 0 - offsets %i %i",upstreamEdgeOffset, downstreamEdgeOffset);

	while(buf.oldWidth && buf.oldSuffix<targetSuffix) // Skip lower upstream
		{
		if(buf.oldPrefix==targetPrefix)
			downstreamEdgeOffset+=buf.oldWidth;

		arrayBufferTransfer(&buf);
		}

//	LOG(LOG_INFO,"Phase 1 - offsets %i %i",upstreamEdgeOffset, downstreamEdgeOffset);

	while(buf.oldWidth && buf.oldSuffix==targetSuffix &&
			((upstreamEdgeOffset+buf.oldWidth)<minEdgePosition ||
			((upstreamEdgeOffset+buf.oldWidth)==minEdgePosition && buf.oldPrefix!=targetPrefix))) // Skip earlier upstream
		{
		upstreamEdgeOffset+=buf.oldWidth;
		if(buf.oldPrefix==targetPrefix)
			downstreamEdgeOffset+=buf.oldWidth;

		arrayBufferTransfer(&buf);
		}

//	LOG(LOG_INFO,"Phase 2 - offsets %i %i",upstreamEdgeOffset, downstreamEdgeOffset);

	while(buf.oldWidth && buf.oldSuffix==targetSuffix &&
			(upstreamEdgeOffset+buf.oldWidth)<=maxEdgePosition && buf.oldPrefix<targetPrefix) // Skip matching upstream with earlier downstream
		{
		if(buf.oldPrefix==targetPrefix)
			downstreamEdgeOffset+=buf.oldWidth;

		upstreamEdgeOffset+=buf.oldWidth;

		arrayBufferTransfer(&buf);
		}

//	LOG(LOG_INFO,"Phase 3 - offsets %i %i",upstreamEdgeOffset, downstreamEdgeOffset);

	if(buf.oldWidth==0 || buf.oldSuffix>targetSuffix || (buf.oldPrefix!=targetPrefix && upstreamEdgeOffset>=minEdgePosition))
																								// No suitable existing entry, but can insert/append here
		{
		int minMargin=upstreamEdgeOffset-minEdgePosition; // Margin between current position and minimum position: Must be zero or positive
		int maxMargin=maxEdgePosition-upstreamEdgeOffset; // Margin between current position and maximum position: Must be zero or positive

		if(minMargin<0 || maxMargin<0)
			{
			rtaDumpRoutingTableArray(builder);

			LOG(LOG_INFO,"Failed to add reverse route for prefix %i suffix %i",targetPrefix,targetSuffix);
			LOG(LOG_INFO,"Current edge offset %i minEdgePosition %i maxEdgePosition %i",upstreamEdgeOffset,minEdgePosition,maxEdgePosition);
			LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets to new entry
		patch->rdiPtr->minEdgePosition=downstreamEdgeOffset;
		patch->rdiPtr->maxEdgePosition=downstreamEdgeOffset;

		arrayBufferPushOutput(&buf, targetPrefix, targetSuffix, 1);
		}
	else if(buf.oldPrefix==targetPrefix && buf.oldSuffix==targetSuffix) // Existing entry suitable, widen
		{
		int upstreamEdgeOffsetEnd=upstreamEdgeOffset+buf.oldWidth;

		// Adjust offsets
		if(minEdgePosition<upstreamEdgeOffset) // Trim upstream range to entry
			minEdgePosition=upstreamEdgeOffset;

		if(maxEdgePosition>upstreamEdgeOffsetEnd) // Trim upstream range to entry
			maxEdgePosition=upstreamEdgeOffsetEnd;

		int minOffset=minEdgePosition-upstreamEdgeOffset; // Offset of minimum position: zero or positive
		int maxOffset=maxEdgePosition-upstreamEdgeOffset; // Offset of maximum position: zero or positive

		if(minOffset<0 || maxOffset<0 || minOffset>maxOffset)
			{
			LOG(LOG_CRITICAL,"Invalid offsets or gap detected in route insert - Min: %i Max: %i",minOffset,maxOffset);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

		// Map offsets to new entry
		patch->rdiPtr->minEdgePosition=downstreamEdgeOffset+minOffset;
		patch->rdiPtr->maxEdgePosition=downstreamEdgeOffset+maxOffset;

		arrayBufferPushOutput(&buf, targetPrefix, targetSuffix, 1);

		s32 trans=arrayBufferPartialTransfer(&buf, maxOffset);
		if(trans!=maxOffset)
			LOG(LOG_CRITICAL,"Widening transfer size mismatch %i %i",maxOffset, trans);

		}
	else // Existing entry unsuitable, split and insert
		{
		int targetEdgePosition=buf.oldPrefix>targetPrefix?minEdgePosition:maxEdgePosition; // Early or late split

		int splitWidth1=targetEdgePosition-upstreamEdgeOffset;
		int splitWidth2=buf.oldWidth-splitWidth1;

		if(splitWidth1<=0 || splitWidth2<=0)
			{
			LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i from %i",splitWidth1, splitWidth2);
			}

//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

		// Map offsets
		patch->rdiPtr->minEdgePosition=downstreamEdgeOffset;
		patch->rdiPtr->maxEdgePosition=downstreamEdgeOffset;

		s32 transWidth1=arrayBufferPartialTransfer(&buf, splitWidth1);

		if(transWidth1!=splitWidth1)
			LOG(LOG_CRITICAL,"Split transfer size mismatch %i %i",splitWidth1, transWidth1);

		arrayBufferPushOutput(&buf, targetPrefix, targetSuffix, 1);
		}

	while(buf.oldWidth) // Copy remaining old entries
		arrayBufferTransfer(&buf);

	arrayBufferFlushOutputEntry(&buf);

	*maxWidth=MAX(*maxWidth,buf.maxWidth);

	return buf.newEntryPtr;
}






RouteTableEntry *rtaMergeRoutes_ordered_reverseMulti(RouteTableArrayBuilder *builder,
		RouteTableEntry *oldEntryPtr, RouteTableEntry *oldEntryEnd, RouteTableEntry *newEntryPtr, RoutePatch *patchPtr, RoutePatch *patchPtrEnd,
		s32 *upstreamOffsets, s32 *downstreamOffsets, int *maxWidth)
{
	//LOG(LOG_INFO,"rtaMergeRoutes_ordered_reverseMulti - %i",(patchPtrEnd-patchPtr));

	RouteTableArrayBuffer buf;
	initArrayBuffer(&buf, oldEntryPtr, oldEntryEnd, newEntryPtr, *maxWidth);

//	rtaDumpRoutingTableArray(builder);

	while(patchPtr<patchPtrEnd)
		{
		int targetPrefix=patchPtr->prefixIndex;
		int targetSuffix=patchPtr->suffixIndex;
		int minEdgePosition=patchPtr->rdiPtr->minEdgePosition;
		int maxEdgePosition=patchPtr->rdiPtr->maxEdgePosition;

//		LOG(LOG_INFO,"Reverse Route: P: %i S: %i [%i %i]", targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

		int expectedMaxEdgePosition=maxEdgePosition+1;

		RoutePatch *patchGroupPtr=patchPtr+1;  // Make groups of compatible inserts for combined processing

		if(minEdgePosition!=TR_INIT_MINEDGEPOSITION || maxEdgePosition!=TR_INIT_MAXEDGEPOSITION)
			{
			while(patchGroupPtr<patchPtrEnd && patchGroupPtr->prefixIndex==targetPrefix && patchGroupPtr->suffixIndex==targetSuffix &&
					patchGroupPtr->rdiPtr->minEdgePosition == minEdgePosition && patchGroupPtr->rdiPtr->maxEdgePosition == expectedMaxEdgePosition)
				{
				patchGroupPtr++;
				expectedMaxEdgePosition++;
				}
			}
		else
			{
			while(patchGroupPtr<patchPtrEnd && patchGroupPtr->prefixIndex==targetPrefix && patchGroupPtr->suffixIndex==targetSuffix &&
					patchGroupPtr->rdiPtr->minEdgePosition == TR_INIT_MINEDGEPOSITION && patchGroupPtr->rdiPtr->maxEdgePosition == TR_INIT_MAXEDGEPOSITION)
				patchGroupPtr++;
			}

//		LOG(LOG_INFO,"Group Size: %i", (patchGroupPtr-patchPtr));
//		LOG(LOG_INFO,"Offsets now U: %i D: %i", upstreamOffsets[targetSuffix], downstreamOffsets[targetPrefix]);

		while(buf.oldWidth && buf.oldSuffix<targetSuffix) // Skip lower upstream
			{
			s32 width=buf.oldWidth;
			upstreamOffsets[buf.oldSuffix]+=width;
			downstreamOffsets[buf.oldPrefix]+=width;

			arrayBufferTransfer(&buf);
			}

		//s32 upstreamEdgeOffset=upstreamOffsets[oldEntryPtr->prefix];

//		LOG(LOG_INFO,"Offsets now U: %i D: %i", upstreamOffsets[targetSuffix], downstreamOffsets[targetPrefix]);

		while(buf.oldWidth && buf.oldSuffix==targetSuffix &&
				((upstreamOffsets[buf.oldSuffix]+buf.oldWidth)<minEdgePosition ||
				((upstreamOffsets[buf.oldSuffix]+buf.oldWidth)==minEdgePosition && buf.oldPrefix!=targetPrefix))) // Skip earlier upstream
			{
			s32 width=buf.oldWidth;
			upstreamOffsets[buf.oldSuffix]+=width;
			downstreamOffsets[buf.oldPrefix]+=width;

			arrayBufferTransfer(&buf);
			}

//		LOG(LOG_INFO,"Offsets now U: %i D: %i", upstreamOffsets[targetSuffix], downstreamOffsets[targetPrefix]);

		while(buf.oldWidth && buf.oldSuffix==targetSuffix &&
			(upstreamOffsets[buf.oldSuffix]+buf.oldWidth)<=maxEdgePosition && buf.oldPrefix<targetPrefix) // Skip matching upstream with earlier downstream
			{
			s32 width=buf.oldWidth;
			upstreamOffsets[buf.oldSuffix]+=width;
			downstreamOffsets[buf.oldPrefix]+=width;

			arrayBufferTransfer(&buf);
			}

//		LOG(LOG_INFO,"Offsets now U: %i D: %i", upstreamOffsets[targetSuffix], downstreamOffsets[targetPrefix]);

		s32 upstreamEdgeOffset=upstreamOffsets[targetSuffix];
		s32 downstreamEdgeOffset=downstreamOffsets[targetPrefix];

		if(buf.oldWidth==0 || buf.oldSuffix>targetSuffix || (buf.oldPrefix!=targetPrefix && upstreamEdgeOffset>=minEdgePosition))
			{
			int minMargin=upstreamEdgeOffset-minEdgePosition; // Margin between current position and minimum position: Must be zero or positive
			int maxMargin=maxEdgePosition-upstreamEdgeOffset; // Margin between current position and maximum position: Must be zero or positive

			if(minMargin<0 || maxMargin<0)
				{
				rtaDumpRoutingTableArray(builder);

				LOG(LOG_INFO,"Failed to add reverse route for prefix %i suffix %i",targetPrefix,targetSuffix);
				LOG(LOG_INFO,"Current edge offset %i minEdgePosition %i maxEdgePosition %i",upstreamEdgeOffset,minEdgePosition,maxEdgePosition);
				LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
				}

			//LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

			// Map offsets to new entry
			patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
			patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset;

			patchPtr++;
			int width=1;

			while(patchPtr<patchGroupPtr) // Remaining entries in group gain width
				{
				patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
				patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+width;

				patchPtr++;
				width++;
				}

			arrayBufferPushOutput(&buf, targetPrefix, targetSuffix, width);

			upstreamOffsets[targetSuffix]+=width;
			downstreamOffsets[targetPrefix]+=width;
			}
		else if(buf.oldPrefix==targetPrefix && buf.oldSuffix==targetSuffix) // Existing entry suitable, widen
			{
//			LOG(LOG_INFO,"Widening");

			int upstreamEdgeOffsetEnd=upstreamEdgeOffset+buf.oldWidth;

			// Adjust offsets
			if(minEdgePosition<upstreamEdgeOffset) // Trim upstream range to entry
				minEdgePosition=upstreamEdgeOffset;

			if(maxEdgePosition>upstreamEdgeOffsetEnd) // Trim upstream range to entry
				maxEdgePosition=upstreamEdgeOffsetEnd;

			int minOffset=minEdgePosition-upstreamEdgeOffset; // Offset of minimum position: zero or positive
			int maxOffset=maxEdgePosition-upstreamEdgeOffset; // Offset of maximum position: zero or positive

			if(minOffset<0 || maxOffset<0 || minOffset>maxOffset)
				{
				LOG(LOG_INFO,"OldEntry Offset Range: %i %i", upstreamEdgeOffset, upstreamEdgeOffsetEnd);
				LOG(LOG_INFO,"Min / Max position: %i %i", minEdgePosition, maxEdgePosition);

				LOG(LOG_CRITICAL,"Invalid offsets or gap detected in route insert - Min: %i Max: %i",minOffset,maxOffset);
				}

	//		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

			// Map offsets to new entry
			patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset+minOffset;
			patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+maxOffset;

//			LOG(LOG_INFO,"Handoff %i %i to RDI: %p",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset, (*(patchPtr->rdiPtr)));

			patchPtr++;
			int width=1;

			while(patchPtr<patchGroupPtr) // Remaining entries in group gain width
				{
				patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset+minOffset;
				//(*(patchPtr->rdiPtr))->maxEdgePosition=downstreamEdgeOffset+width;
				patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+maxOffset+width;

//				LOG(LOG_INFO,"Handoff %i %i to RDI: %p",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

				patchPtr++;
				width++;
				}

			arrayBufferPushOutput(&buf, targetPrefix, targetSuffix, width);
			s32 trans=arrayBufferPartialTransfer(&buf, maxOffset);
			if(trans!=maxOffset)
				LOG(LOG_CRITICAL,"Widening transfer size mismatch %i %i",maxOffset, trans);

//			LOG(LOG_INFO,"Post transfer: Min %i Max %i",minOffset, maxOffset);

			upstreamOffsets[targetSuffix]+=maxOffset+width;
			downstreamOffsets[targetPrefix]+=maxOffset+width;
			}
		else // Existing entry unsuitable, split and insert
			{
			int targetEdgePosition=buf.oldPrefix>targetPrefix?minEdgePosition:maxEdgePosition; // Early or late split
			int splitWidth1=targetEdgePosition-upstreamEdgeOffset;
			int splitWidth2=buf.oldWidth-splitWidth1;

			if(splitWidth1<=0 || splitWidth2<=0)
				{
				LOG(LOG_CRITICAL,"Non-positive split width detected in route insert - Width1: %i Width2: %i from %i",splitWidth1, splitWidth2);
				}

			//LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

			// Map offsets
			patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
			patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset;

			patchPtr++;
			int width=1;

			while(patchPtr<patchGroupPtr) // Remaining entries in group gain width
				{
				patchPtr->rdiPtr->minEdgePosition=downstreamEdgeOffset;
				patchPtr->rdiPtr->maxEdgePosition=downstreamEdgeOffset+width;

				patchPtr++;
				width++;
				}

			upstreamOffsets[buf.oldSuffix]+=splitWidth1;
			downstreamOffsets[buf.oldPrefix]+=splitWidth1;

			s32 transWidth1=arrayBufferPartialTransfer(&buf, splitWidth1);

			if(transWidth1!=splitWidth1)
				LOG(LOG_CRITICAL,"Split transfer size mismatch %i %i",splitWidth1, transWidth1);

			arrayBufferPushOutput(&buf, targetPrefix, targetSuffix, width);

			upstreamOffsets[targetSuffix]+=width;
			downstreamOffsets[targetPrefix]+=width;
			}

		}

	while(buf.oldWidth) // Copy remaining old entries
		arrayBufferTransfer(&buf);

	arrayBufferFlushOutputEntry(&buf);

	*maxWidth=MAX(*maxWidth,buf.maxWidth);

	return buf.newEntryPtr;
}






void rtaMergeRoutes(RouteTableArrayBuilder *builder, RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches,
		s32 forwardRoutePatchCount, s32 reverseRoutePatchCount, s32 forwardTagCount, s32 reverseTagCount,
		s32 prefixCount, s32 suffixCount, u32 *orderedDispatches, MemDispenser *disp)
{
	if(builder->newForwardEntryCount>0 || builder->newReverseEntryCount>0)
		{
		LOG(LOG_INFO,"RouteTableBuilder: Already contains new routes before merge");
		return;
		}

	/*
	builder->newForwardEntries=forwardEntries;
	builder->newForwardEntryCount=maxForwardEntryCount;

	builder->newReverseEntries=reverseEntries;
	builder->newReverseEntryCount=maxReverseEntryCount;
*/

	builder->maxPrefix=MAX(prefixCount, builder->maxPrefix);
	builder->maxSuffix=MAX(suffixCount, builder->maxSuffix);

	int maxWidth=MAX(1, builder->maxWidth);
/*
	s32 *prefixOffsets=NULL, *suffixOffsets=NULL;

	if(forwardRoutePatchCount > 1 || reverseRoutePatchCount > 1)
		{
		prefixOffsets=dAlloc(disp, sizeof(s32)*(prefixCount));
		suffixOffsets=dAlloc(disp, sizeof(s32)*(suffixCount));
		}
*/
	int maxForwardEntries=forwardRoutePatchCount*2+builder->oldForwardEntryCount;
	int maxReverseEntries=reverseRoutePatchCount*2+builder->oldReverseEntryCount;

	int maxEntries=MAX(maxForwardEntries,maxReverseEntries);

	RouteTableEntry *buffer1=dAlloc(disp, maxEntries*sizeof(RoutePatch));
//	RouteTableEntry *buffer2=dAlloc(disp, maxEntries*sizeof(RoutePatch));

	int forwardCount=builder->oldForwardEntryCount;
	int reverseCount=builder->oldReverseEntryCount;

	/*
	RouteTableEntry *multiForwardBuffer=dAlloc(disp, maxEntries*sizeof(RoutePatch));
	RoutePatch *multiForwardRoutePatch=NULL;

	if(forwardRoutePatchCount>1)
		multiForwardRoutePatch=rtCloneRoutePatches(disp, forwardRoutePatches, forwardRoutePatchCount);

	if(forwardRoutePatchCount>0)
		{
		RouteTableEntry *destBuffer=buffer1;
		RouteTableEntry *destBufferEnd=NULL;

		RouteTableEntry *nextBuffer=buffer2;

		RouteTableEntry *srcBuffer=builder->oldForwardEntries;
		RouteTableEntry *srcBufferEnd=builder->oldForwardEntries+builder->oldForwardEntryCount;

		RoutePatch *patchPtr=forwardRoutePatches;
		RoutePatch *patchEnd=patchPtr+forwardRoutePatchCount;
		RoutePatch *patchPeekPtr=NULL;

		while(patchPtr<patchEnd)
			{
			int targetUpstream=patchPtr->prefixIndex;
			patchPeekPtr=patchPtr+1;

			if(patchPeekPtr<patchEnd && patchPeekPtr->prefixIndex==targetUpstream)
				{
//				LOG(LOG_INFO,"MERGE: Scenario 0 - %p",destBuffer);
				// Evil case: Multiple patches in prefix (orderedReadset)

				while(patchPtr<patchEnd && patchPtr->prefixIndex==targetUpstream)
					{
					destBufferEnd=rtaMergeRoutes_ordered_forwardSingle(srcBuffer, srcBufferEnd, destBuffer, patchPtr, &maxWidth);

					srcBuffer=destBuffer;
					srcBufferEnd=destBufferEnd;
					destBuffer=nextBuffer;
					nextBuffer=srcBuffer;

					*(orderedDispatches++)=*(patchPtr->rdiPtr);
					patchPtr++;
					}
				}
			else
				{
				// Medium case: Single patch in prefix (patchPtr)
//				LOG(LOG_INFO,"MERGE: Scenario 1 - %p",destBuffer);


				destBufferEnd=rtaMergeRoutes_ordered_forwardSingle(srcBuffer, srcBufferEnd, destBuffer, patchPtr, &maxWidth);

				srcBuffer=destBuffer;
				srcBufferEnd=destBufferEnd;
				destBuffer=nextBuffer;
				nextBuffer=srcBuffer;

				*(orderedDispatches++)=*(patchPtr->rdiPtr);
				patchPtr++;
				}
			}

		forwardCount=srcBufferEnd-srcBuffer;
		builder->newForwardEntryCount=forwardCount;
		builder->newForwardEntries=dAlloc(disp, forwardCount*sizeof(RouteTableEntry));

	//	LOG(LOG_INFO,"Transferring %p to %p",srcBuffer, builder->newForwardEntries);

		memcpy(builder->newForwardEntries, srcBuffer, forwardCount*sizeof(RouteTableEntry));

		//LOG(LOG_INFO,"Was %i Now %i",builder->oldForwardEntryCount,forwardCount);

		//if(forwardCount>0)
//			LOG(LOG_INFO,"FirstRev: P: %i S: %i W: %i",builder->newForwardEntries[0].prefix,builder->newForwardEntries[0].suffix,builder->newForwardEntries[0].width);
		}

*/
	// Forward Routes: Multi

	if(forwardRoutePatchCount>0)
		{
		RouteTableEntry *srcBuffer=builder->oldForwardEntries;
		RouteTableEntry *srcBufferEnd=builder->oldForwardEntries+builder->oldForwardEntryCount;

		RouteTableEntry *destBuffer=buffer1;

		RoutePatch *patchPtr=forwardRoutePatches;

		RouteTableEntry *destBufferEnd=NULL;

		if(forwardRoutePatchCount>1)
			{
			s32 *prefixOffsets=NULL, *suffixOffsets=NULL;

			prefixOffsets=dAlloc(disp, sizeof(s32)*(prefixCount));
			suffixOffsets=dAlloc(disp, sizeof(s32)*(suffixCount));

			memset(prefixOffsets, 0, sizeof(s32)*(prefixCount));
			memset(suffixOffsets, 0, sizeof(s32)*(suffixCount));

			destBufferEnd=rtaMergeRoutes_ordered_forwardMulti(builder, srcBuffer, srcBufferEnd, destBuffer,
					patchPtr, patchPtr+forwardRoutePatchCount, prefixOffsets, suffixOffsets, &maxWidth);

			}
		else
			destBufferEnd=rtaMergeRoutes_ordered_forwardSingle(builder, srcBuffer, srcBufferEnd, destBuffer, patchPtr, &maxWidth);

		for(int i=0;i<forwardRoutePatchCount;i++)
			{
//			LOG(LOG_INFO,"OrderDispFwd: %i P: %i S: %i [%i %i]",patchPtr->dispatchLinkIndex, patchPtr->prefixIndex, patchPtr->suffixIndex, (*patchPtr->rdiPtr)->minEdgePosition, (*patchPtr->rdiPtr)->maxEdgePosition);
			*(orderedDispatches++)=patchPtr->dispatchLinkIndex;
			patchPtr++;
			}

		forwardCount=destBufferEnd-destBuffer;

		//LOG(LOG_INFO,"About to copy",builder->oldForwardEntryCount,forwardCount);

		builder->newForwardEntryCount=forwardCount;
		builder->newForwardEntries=dAlloc(disp, forwardCount*sizeof(RouteTableEntry));

		memcpy(builder->newForwardEntries, destBuffer, forwardCount*sizeof(RouteTableEntry));

//		LOG(LOG_INFO,"Was %i Now %i",builder->oldForwardEntryCount,forwardCount);

//		if(forwardCount>0)
//			LOG(LOG_INFO,"FirstRev: P: %i S: %i W: %i",builder->newForwardEntries[0].prefix,builder->newForwardEntries[0].suffix,builder->newForwardEntries[0].width);
		}




	// Reverse Routes
/*
	if(reverseRoutePatchCount>0)
		{
		RouteTableEntry *destBuffer=buffer1;
		RouteTableEntry *destBufferEnd=NULL;

		RouteTableEntry *nextBuffer=buffer2;

		RouteTableEntry *srcBuffer=builder->oldReverseEntries;
		RouteTableEntry *srcBufferEnd=builder->oldReverseEntries+builder->oldReverseEntryCount;

		RoutePatch *patchPtr=reverseRoutePatches;
		RoutePatch *patchEnd=patchPtr+reverseRoutePatchCount;
		RoutePatch *patchPeekPtr=NULL;

		while(patchPtr<patchEnd)
			{
			int targetUpstream=patchPtr->suffixIndex;
			patchPeekPtr=patchPtr+1;

			if(patchPeekPtr<patchEnd && patchPeekPtr->suffixIndex==targetUpstream)
				{
	//			LOG(LOG_INFO,"MERGE: Scenario 0");
				// Evil case: Multiple patches in prefix (orderedReadset)

				while(patchPtr<patchEnd && patchPtr->suffixIndex==targetUpstream)
					{
					destBufferEnd=rtaMergeRoutes_ordered_reverseSingle(srcBuffer, srcBufferEnd, destBuffer, patchPtr, &maxWidth);

					srcBuffer=destBuffer;
					srcBufferEnd=destBufferEnd;
					destBuffer=nextBuffer;
					nextBuffer=srcBuffer;

					*(orderedDispatches++)=*(patchPtr->rdiPtr);
					patchPtr++;
					}
				}
			else
				{
				// Medium case: Single patch in prefix (patchPtr)
//				LOG(LOG_INFO,"MERGE: Scenario 1");

				destBufferEnd=rtaMergeRoutes_ordered_reverseSingle(srcBuffer, srcBufferEnd, destBuffer, patchPtr, &maxWidth);

				srcBuffer=destBuffer;
				srcBufferEnd=destBufferEnd;
				destBuffer=nextBuffer;
				nextBuffer=srcBuffer;

				*(orderedDispatches++)=*(patchPtr->rdiPtr);
				patchPtr++;
				}
			}

		reverseCount=srcBufferEnd-srcBuffer;
		builder->newReverseEntryCount=reverseCount;
		builder->newReverseEntries=dAlloc(disp, reverseCount*sizeof(RouteTableEntry));
		memcpy(builder->newReverseEntries, srcBuffer, reverseCount*sizeof(RouteTableEntry));

		//LOG(LOG_INFO,"Was %i Now %i",builder->oldReverseEntryCount,reverseCount);

		//if(reverseCount>0)
//			LOG(LOG_INFO,"FirstRev: P: %i S: %i W: %i",builder->newReverseEntries[0].prefix,builder->newReverseEntries[0].suffix,builder->newReverseEntries[0].width);
		}
*/

	// Reverse Routes: Multi

	if(reverseRoutePatchCount>0)
		{
		RouteTableEntry *srcBuffer=builder->oldReverseEntries;
		RouteTableEntry *srcBufferEnd=builder->oldReverseEntries+builder->oldReverseEntryCount;

		RouteTableEntry *destBuffer=buffer1;

		RoutePatch *patchPtr=reverseRoutePatches;

		RouteTableEntry *destBufferEnd=NULL;

		if(reverseRoutePatchCount>1)
			{
			s32 *prefixOffsets=NULL, *suffixOffsets=NULL;

			prefixOffsets=dAlloc(disp, sizeof(s32)*(prefixCount));
			suffixOffsets=dAlloc(disp, sizeof(s32)*(suffixCount));

			memset(prefixOffsets, 0, sizeof(s32)*(prefixCount));
			memset(suffixOffsets, 0, sizeof(s32)*(suffixCount));

			destBufferEnd=rtaMergeRoutes_ordered_reverseMulti(builder, srcBuffer, srcBufferEnd, destBuffer,
					patchPtr, patchPtr+reverseRoutePatchCount, suffixOffsets, prefixOffsets, &maxWidth);

			}
		else
			destBufferEnd=rtaMergeRoutes_ordered_reverseSingle(builder, srcBuffer, srcBufferEnd, destBuffer, patchPtr, &maxWidth);

		for(int i=0;i<reverseRoutePatchCount;i++)
			{
//			LOG(LOG_INFO,"OrderDispRev: %i P: %i S: %i [%i %i]",patchPtr->dispatchLinkIndex, patchPtr->prefixIndex, patchPtr->suffixIndex, (*patchPtr->rdiPtr)->minEdgePosition, (*patchPtr->rdiPtr)->maxEdgePosition);
			*(orderedDispatches++)=patchPtr->dispatchLinkIndex;
			patchPtr++;
			}

		reverseCount=destBufferEnd-destBuffer;

		//LOG(LOG_INFO,"About to copy",builder->oldForwardEntryCount,forwardCount);

		builder->newReverseEntryCount=reverseCount;
		builder->newReverseEntries=dAlloc(disp, reverseCount*sizeof(RouteTableEntry));

		memcpy(builder->newReverseEntries, destBuffer, reverseCount*sizeof(RouteTableEntry));

//		LOG(LOG_INFO,"Was %i Now %i",builder->oldForwardEntryCount,forwardCount);

//		if(forwardCount>0)
//			LOG(LOG_INFO,"FirstRev: P: %i S: %i W: %i",builder->newForwardEntries[0].prefix,builder->newForwardEntries[0].suffix,builder->newForwardEntries[0].width);
		}




	builder->maxWidth=maxWidth;

	u32 prefixBits=bpBitsRequired(builder->maxPrefix);
	u32 suffixBits=bpBitsRequired(builder->maxSuffix);
	u32 widthBits=bpBitsRequired(builder->maxWidth-1);

	int headerSize=getRouteTableHeaderSize(prefixBits,suffixBits,widthBits,forwardCount,reverseCount);
	int tableSize=PAD_1BITLENGTH_BYTE((prefixBits+suffixBits+widthBits)*(forwardCount+reverseCount));

	builder->totalPackedSize=headerSize+tableSize;

}







void rtaUnpackRouteTableArrayForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp)
{
	u32 prefixBits=0, suffixBits=0, widthBits=0, forwardEntryCount=0, reverseEntryCount=0;

	if(data!=NULL)
		{
		data+=decodeHeader(data, &prefixBits, &suffixBits, &widthBits, &forwardEntryCount, &reverseEntryCount);

		smerLinked->forwardRouteEntries=dAlloc(disp, sizeof(RouteTableEntry)*forwardEntryCount);
		smerLinked->reverseRouteEntries=dAlloc(disp, sizeof(RouteTableEntry)*reverseEntryCount);

		smerLinked->forwardRouteCount=forwardEntryCount;
		smerLinked->reverseRouteCount=reverseEntryCount;

		rtaUnpackRoutes(data, prefixBits, suffixBits, widthBits,
				smerLinked->forwardRouteEntries, smerLinked->reverseRouteEntries, forwardEntryCount, reverseEntryCount,
				NULL,NULL,NULL);
		}
	else
		{
		smerLinked->forwardRouteEntries=NULL;
		smerLinked->reverseRouteEntries=NULL;

		smerLinked->forwardRouteCount=0;
		smerLinked->reverseRouteCount=0;
		}
}

void rtaGetStats(RouteTableArrayBuilder *builder,
		s64 *routeTableForwardRouteEntriesPtr, s64 *routeTableForwardRoutesPtr, s64 *routeTableReverseRouteEntriesPtr, s64 *routeTableReverseRoutesPtr,
		s64 *routeTableArrayBytesPtr)
{
	s64 forwardCount=0;
	s64 reverseCount=0;

	if(routeTableForwardRouteEntriesPtr!=NULL)
		*routeTableForwardRouteEntriesPtr=builder->oldForwardEntryCount;

	if(routeTableReverseRouteEntriesPtr!=NULL)
		*routeTableReverseRouteEntriesPtr=builder->oldReverseEntryCount;

	if(routeTableForwardRoutesPtr!=NULL)
		{
		for(int i=0;i<builder->oldForwardEntryCount;i++)
			forwardCount+=builder->oldForwardEntries[i].width;

		*routeTableForwardRoutesPtr=forwardCount;
		}

	if(routeTableReverseRoutesPtr!=NULL)
		{
		for(int i=0;i<builder->oldReverseEntryCount;i++)
			reverseCount+=builder->oldReverseEntries[i].width;

		*routeTableReverseRoutesPtr=reverseCount;
		}

	if(routeTableArrayBytesPtr!=NULL)
		{
		*routeTableArrayBytesPtr=rtaGetRouteTableArrayBuilderPackedSize(builder);
		}

}


