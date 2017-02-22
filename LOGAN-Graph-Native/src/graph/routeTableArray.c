
#include "common.h"

//
//


// Idea for PackedArray format: store arrays of pointers, referencing a block of route entries grouped by upstream sequence. The blocks consist of packed entries
// PckArr format: 1 1 0 W W W W W  P P P P S S S S
//                F F F F F F F F  R R R R R R R R
//                Fptr*
//                Rptr*

// PA Block:      E E E E E E E E  E E E E E E E E (entry count)
//                E E E E E E E E  E E E E E E E E (entry count)
//			 	  U*, (D*, W*)*




// Huge format:   1 1 1 W W W W W  P P P P S S S S
//                F F F F F F F F  F F F F F F F F
//                F F F F F F F F  F F F F F F F F
// 				  R R R R R R R R  R R R R R R R R
// 				  R R R R R R R R  R R R R R R R R

// Large format:  1 1 0 W W W W W  P P P P S S S S
//                F F F F F F F F  F F F F F F F F
// 				  R R R R R R R R  R R R R R R R R

// Medium format: 1 0 W W W W P P  P S S S F F F F
//                F F R R R R R R

// Small format:  0 W W W P P S S  F F F F R R R R

// Max: W=2^31, P=2^15, S=2^15, F=2^16, R=2^16



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
	initUnpacker(&unpacker, data, 0);

	for(int i=0;i<forwardEntryCount;i++)
		{
		s32 prefix=unpackBits(&unpacker, prefixBits);
		s32 suffix=unpackBits(&unpacker, suffixBits);
		s32 width=unpackBits(&unpacker, widthBits)+1;

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
		s32 prefix=unpackBits(&unpacker, prefixBits);
		s32 suffix=unpackBits(&unpacker, suffixBits);
		s32 width=unpackBits(&unpacker, widthBits)+1;

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

void rtaDumpRoutingTableArray(RouteTableArrayBuilder *builder)
{
	LOG(LOG_INFO,"Header: %i %i %i", builder->maxPrefix, builder->maxSuffix, builder->maxWidth);

	LOG(LOG_INFO,"Old Forward Routes %i",builder->oldForwardEntryCount);
	for(int i=0;i<builder->oldForwardEntryCount;i++)
		LOG(LOG_INFO,"OldForward Route - P: %i S: %i W: %i", builder->oldForwardEntries[i].prefix, builder->oldForwardEntries[i].suffix,  builder->oldForwardEntries[i].width);

	LOG(LOG_INFO,"New Forward Routes %i",builder->newForwardEntryCount);
	for(int i=0;i<builder->newForwardEntryCount;i++)
		LOG(LOG_INFO,"NewForward Route - P: %i S: %i W: %i", builder->newForwardEntries[i].prefix, builder->newForwardEntries[i].suffix,  builder->newForwardEntries[i].width);


	LOG(LOG_INFO,"Old Reverse Routes %i",builder->oldReverseEntryCount);
	for(int i=0;i<builder->oldReverseEntryCount;i++)
		LOG(LOG_INFO,"OldReverse Route - P: %i S: %i W: %i", builder->oldReverseEntries[i].prefix, builder->oldReverseEntries[i].suffix,  builder->oldReverseEntries[i].width);

	LOG(LOG_INFO,"New Reverse Routes %i",builder->newReverseEntryCount);
	for(int i=0;i<builder->newReverseEntryCount;i++)
		LOG(LOG_INFO,"NewReverse Route - P: %i S: %i W: %i", builder->newReverseEntries[i].prefix, builder->newReverseEntries[i].suffix,  builder->newReverseEntries[i].width);


}

u8 *rtaWriteRouteTableArrayBuilderPackedData(RouteTableArrayBuilder *builder, u8 *data)
{
	u32 prefixBits=bitsRequired(builder->maxPrefix);
	u32 suffixBits=bitsRequired(builder->maxSuffix);
	u32 widthBits=bitsRequired(builder->maxWidth-1);

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
	initPacker(&packer, data, 0);

	for(int i=0;i<forwardEntryCount;i++)
		{
//		if(forwardEntryCount>1000)
//			LOG(LOG_INFO,"Forward Route - P: %i %i %i", forwardEntries[i].prefix, forwardEntries[i].suffix,  forwardEntries[i].width);

		packBits(&packer, prefixBits, forwardEntries[i].prefix);
		packBits(&packer, suffixBits, forwardEntries[i].suffix);
		packBits(&packer, widthBits, forwardEntries[i].width-1);
		}
	for(int i=0;i<reverseEntryCount;i++)
		{
//		if(reverseEntryCount>1000)
//			LOG(LOG_INFO,"Reverse Route - P: %i %i %i", reverseEntries[i].prefix, reverseEntries[i].suffix,  reverseEntries[i].width);

		packBits(&packer, prefixBits, reverseEntries[i].prefix);
		packBits(&packer, suffixBits, reverseEntries[i].suffix);
		packBits(&packer, widthBits, reverseEntries[i].width-1);
		}

	int tableSize=PAD_1BITLENGTH_BYTE((prefixBits+suffixBits+widthBits)*(forwardEntryCount+reverseEntryCount));

	int size=headerSize+tableSize;

	if(size!=builder->totalPackedSize)
	{
		LOG(LOG_CRITICAL,"Size %i did not match expected size %i",size,builder->totalPackedSize);
	}

	return data+tableSize;
}



static RouteTableEntry *rtaMergeRoutes_ordered_forwardSingle(RouteTableArrayBuilder *builder,
		RouteTableEntry *oldEntryPtr, RouteTableEntry *oldEntryEnd, RouteTableEntry *newEntryPtr, RoutePatch *patch, int *maxWidth)
{
	int targetPrefix=patch->prefixIndex;
	int targetSuffix=patch->suffixIndex;
	int minEdgePosition=(*(patch->rdiPtr))->minEdgePosition;
	int maxEdgePosition=(*(patch->rdiPtr))->maxEdgePosition;

	int upstreamEdgeOffset=0;
	int downstreamEdgeOffset=0;

	LOG(LOG_INFO,"rtaMergeRoutes_ordered_forwardSingle %i %i with %i %i",targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

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
			rtaDumpRoutingTableArray(builder);

			LOG(LOG_INFO,"Failed to add forward route for prefix %i suffix %i",targetPrefix,targetSuffix);
			LOG(LOG_INFO,"Current edge offset %i minEdgePosition %i maxEdgePosition %i",upstreamEdgeOffset,minEdgePosition,maxEdgePosition);
			LOG(LOG_CRITICAL,"Negative gap detected in route insert - Min: %i Max: %i",minMargin,maxMargin);
			}

		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);

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

		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

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

		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);
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



static RouteTableEntry *rtaMergeRoutes_ordered_reverseSingle(RouteTableEntry *oldEntryPtr, RouteTableEntry *oldEntryEnd, RouteTableEntry *newEntryPtr, RoutePatch *patch, int *maxWidth)
{
	int targetPrefix=patch->prefixIndex;
	int targetSuffix=patch->suffixIndex;
	int minEdgePosition=(*(patch->rdiPtr))->minEdgePosition;
	int maxEdgePosition=(*(patch->rdiPtr))->maxEdgePosition;

	int upstreamEdgeOffset=0;
	int downstreamEdgeOffset=0;

	LOG(LOG_INFO,"rtaMergeRoutes_ordered_reverseSingle %i %i with %i %i",targetPrefix, targetSuffix, minEdgePosition, maxEdgePosition);

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

		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);
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

		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset+minOffset,downstreamEdgeOffset+maxOffset);

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

		LOG(LOG_INFO,"Handoff %i %i",downstreamEdgeOffset,downstreamEdgeOffset);
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












void rtaMergeRoutes(RouteTableArrayBuilder *builder, RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 prefixCount, s32 suffixCount, RoutingReadData **orderedDispatches, MemDispenser *disp)
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
	int prefixOffsetSize=sizeof(int)*(maxPrefix+1);
	int suffixOffsetSize=sizeof(int)*(maxSuffix+1);

	s32 *prefixOffsets=alloca(prefixOffsetSize);
	s32 *suffixOffsets=alloca(suffixOffsetSize);

	memset(prefixOffsets,0,prefixOffsetSize);
	memset(suffixOffsets,0,suffixOffsetSize);
*/

	int maxForwardEntries=forwardRoutePatchCount*2+builder->oldForwardEntryCount;
	int maxReverseEntries=reverseRoutePatchCount*2+builder->oldReverseEntryCount;

	int maxEntries=MAX(maxForwardEntries,maxReverseEntries);

	RouteTableEntry *buffer1=dAlloc(disp, maxEntries*sizeof(RoutePatch));
	RouteTableEntry *buffer2=dAlloc(disp, maxEntries*sizeof(RoutePatch));

	int forwardCount=builder->oldForwardEntryCount;
	int reverseCount=builder->oldReverseEntryCount;

	// Forward Routes

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
//				LOG(LOG_INFO,"MERGE: Scenario 0");
				// Evil case: Multiple patches in prefix (orderedReadset)

				while(patchPtr<patchEnd && patchPtr->prefixIndex==targetUpstream)
					{
					destBufferEnd=rtaMergeRoutes_ordered_forwardSingle(builder, srcBuffer, srcBufferEnd, destBuffer, patchPtr, &maxWidth);

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

				destBufferEnd=rtaMergeRoutes_ordered_forwardSingle(builder, srcBuffer, srcBufferEnd, destBuffer, patchPtr, &maxWidth);

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
		memcpy(builder->newForwardEntries, srcBuffer, forwardCount*sizeof(RouteTableEntry));

//		LOG(LOG_INFO,"Was %i Now %i",builder->oldForwardEntryCount,forwardCount);

//		if(forwardCount>0)
//			LOG(LOG_INFO,"FirstRev: P: %i S: %i W: %i",builder->newForwardEntries[0].prefix,builder->newForwardEntries[0].suffix,builder->newForwardEntries[0].width);
		}


	// Reverse Routes

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

	builder->maxWidth=maxWidth;

	u32 prefixBits=bitsRequired(builder->maxPrefix);
	u32 suffixBits=bitsRequired(builder->maxSuffix);
	u32 widthBits=bitsRequired(builder->maxWidth-1);

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


