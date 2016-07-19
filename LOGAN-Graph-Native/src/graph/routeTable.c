
#include "common.h"



// Large format:  1 1 W W W W W W  P P P P S S S S
//                F F F F F F F F  F F F F F F F F
// 				  R R R R R R R R  R R R R R R R R

// Medium format: 1 0 W W W W P P  P S S S F F F F
//                F F R R R R R R

// Small format:  0 W W W P P S S  F F F F R R R R

// Max: W=2^63, P=2^15, S=2^15, F=2^16, R=2^16

static int getRouteTableHeaderSize(int prefixBits, u32 suffixBits, u32 widthBits, u32 forwardEntries, u32 reverseEntries)
{
	prefixBits--;
	suffixBits--;
	widthBits--;

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

	if((prefixBits>15)||(suffixBits>15)||(widthBits>63)||(forwardEntries>65535)||(reverseEntries>65535))
		{
		LOG(LOG_ERROR,"Cannot encode header for %x %x %x %x %x",prefixBits,suffixBits,widthBits,forwardEntries,reverseEntries);
		LOG(LOG_ERROR,"This will end badly :(");
		}

	if((prefixBits>7)||(suffixBits>7)||(widthBits>15)||(forwardEntries>63)||(reverseEntries>63))
		{
		packedData[0]=(widthBits&0x3F) | 0xC0;
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
	u8 sizeFlag=packedData[0]&0xC0;

	int w,p,s,f,r,size;

	if(sizeFlag == 0xC0)
		{
		u8 tmp1=packedData[0];
		u8 tmp2=packedData[1];

		w=(tmp1&0x3F)+1;
		p=((tmp2>>4)&0xF)+1;
		s=(tmp2&0xF)+1;
		f=*((u16 *)(packedData+2));
		r=*((u16 *)(packedData+4));

		size=6;
		}
	else if(sizeFlag==0x80)
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



static u32 bitsRequired(u32 value)
{
	if(value==0)
		return 1;

	return 32-__builtin_clz(value);
}






/*
 * 	MemDispenser *disp;

	RouteTableEntry *oldForwardRoutes;
	RouteTableEntry *oldReverseRoutes;

	RouteTableEntry *newForwardRoutes;
	RouteTableEntry *newReverseRoutes;

	s32 oldForwardRouteCount;
	s32 oldReverseRouteCount;
	s32 newForwardRouteCount;
	s32 newReverseRouteCount;

	s32 newForwardRouteAlloc;
	s32 newReverseRouteAlloc;

	s32 maxPrefix;
	s32 maxSuffix;
	s32 maxWidth;

	s32 totalPackedSize;
 *
 */


static void unpackRoutes(u8 *data, int prefixBits, int suffixBits, int widthBits,
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

static u8 *readRouteTableBuilderPackedData(RouteTableBuilder *builder, u8 *data)
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

		unpackRoutes(data, prefixBits, suffixBits, widthBits,
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
	builder->totalPackedSize=headerSize+tableSize;

	return data;
}

u8 *initRouteTableBuilder(RouteTableBuilder *builder, u8 *data, MemDispenser *disp)
{
	builder->disp=disp;

//	LOG(LOG_INFO,"RouteTable init from %p",data);
	data=readRouteTableBuilderPackedData(builder,data);

	builder->newForwardEntries=NULL;
	builder->newReverseEntries=NULL;

	builder->newForwardEntryCount=0;
	builder->newReverseEntryCount=0;

	builder->newForwardEntryAlloc=0;
	builder->newReverseEntryAlloc=0;

	return data;
}

s32 getRouteTableBuilderDirty(RouteTableBuilder *builder)
{
	return builder->newForwardEntryCount>0 || builder->newReverseEntryCount>0;
}

s32 getRouteTableBuilderPackedSize(RouteTableBuilder *builder)
{
	return builder->totalPackedSize;
}

u8 *writeRouteTableBuilderPackedData(RouteTableBuilder *builder, u8 *data)
{
	u32 prefixBits=bitsRequired(builder->maxPrefix);
	u32 suffixBits=bitsRequired(builder->maxSuffix);
	u32 widthBits=bitsRequired(builder->maxWidth-1);

	if(prefixBits==15 || suffixBits==15 || widthBits==63)
		{
		LOG(LOG_INFO,"Header near full: %i %i %i", builder->maxPrefix, builder->maxSuffix, builder->maxWidth);
		}


	RouteTableEntry *forwardEntries,*reverseEntries;
	u32 forwardEntryCount,reverseEntryCount;

	if(builder->newForwardEntryCount>0 || builder->newReverseEntryCount>0)
		{
		forwardEntries=builder->newForwardEntries;
		reverseEntries=builder->newReverseEntries;
		forwardEntryCount=builder->newForwardEntryCount;
		reverseEntryCount=builder->newReverseEntryCount;
		}
	else
		{
		forwardEntries=builder->oldForwardEntries;
		reverseEntries=builder->oldReverseEntries;
		forwardEntryCount=builder->oldForwardEntryCount;
		reverseEntryCount=builder->oldReverseEntryCount;
		}

	int headerSize=encodeRouteTableHeader(data,prefixBits,suffixBits,widthBits,forwardEntryCount,reverseEntryCount);
	data+=headerSize;

	BitPacker packer;
	initPacker(&packer, data, 0);

	for(int i=0;i<forwardEntryCount;i++)
		{
		packBits(&packer, prefixBits, forwardEntries[i].prefix);
		packBits(&packer, suffixBits, forwardEntries[i].suffix);
		packBits(&packer, widthBits, forwardEntries[i].width-1);
		}
	for(int i=0;i<reverseEntryCount;i++)
		{
		packBits(&packer, prefixBits, reverseEntries[i].prefix);
		packBits(&packer, suffixBits, reverseEntries[i].suffix);
		packBits(&packer, widthBits, reverseEntries[i].width-1);
		}

	int tableSize=PAD_1BITLENGTH_BYTE((prefixBits+suffixBits+widthBits)*(forwardEntryCount+reverseEntryCount));

	return data+tableSize;
}

void mergeRoutes(RouteTableBuilder *builder, RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount)
{
	u32 oldForwardEntryCount=builder->oldForwardEntryCount;
	u32 oldReverseEntryCount=builder->oldReverseEntryCount;

	RouteTableEntry *oldForwardEntries=builder->oldForwardEntries;
	RouteTableEntry *oldReverseEntries=builder->oldReverseEntries;

	if(builder->newForwardEntryCount>0 || builder->newReverseEntryCount>0)
		{
		LOG(LOG_INFO,"RouteTableBuilder: Already contains new routes before merge");
		return;
		}

	int newForwardEntryCount=oldForwardEntryCount+forwardRoutePatchCount;
	int newReverseEntryCount=oldReverseEntryCount+reverseRoutePatchCount;

	//LOG(LOG_INFO,"Merging Route Table having %i %i with %i %i",oldForwardEntryCount, oldReverseEntryCount, forwardRoutePatchCount, reverseRoutePatchCount);

	RouteTableEntry *forwardEntries=dAlloc(builder->disp, sizeof(RouteTableEntry) * newForwardEntryCount);
	RouteTableEntry *reverseEntries=dAlloc(builder->disp, sizeof(RouteTableEntry) * newReverseEntryCount);

	builder->newForwardEntries=forwardEntries;
	builder->newForwardEntryCount=newForwardEntryCount;

	builder->newReverseEntries=reverseEntries;
	builder->newReverseEntryCount=newReverseEntryCount;

	int maxPrefix=0,maxSuffix=0,maxWidth=0;

	u32 prefix=-1;
	u32 suffix=-1;
	u32 width=-1;

	for(int i=0;i<oldForwardEntryCount;i++)
		{
		prefix=oldForwardEntries[i].prefix;
		suffix=oldForwardEntries[i].suffix;
		width=oldForwardEntries[i].width;

		maxPrefix=MAX(maxPrefix,prefix);
		maxSuffix=MAX(maxSuffix,suffix);
		maxWidth=MAX(maxWidth,width);

		forwardEntries[i].prefix=prefix;
		forwardEntries[i].suffix=suffix;
		forwardEntries[i].width=width;
		}

	forwardEntries+=oldForwardEntryCount;
	forwardEntries--;

	for(int i=0;i<forwardRoutePatchCount;i++)
		{
		u32 wantedPrefix=forwardRoutePatches[i].prefixIndex;
		u32 wantedSuffix=forwardRoutePatches[i].suffixIndex;

		if(prefix==wantedPrefix && suffix==wantedSuffix)
			{
			//LOG(LOG_INFO,"F merge");
			forwardEntries->width=++width;
			newForwardEntryCount--;
			}
		else
			{
			forwardEntries++;

			prefix=wantedPrefix;
			suffix=wantedSuffix;
			width=1;

			forwardEntries->prefix=prefix;
			forwardEntries->suffix=suffix;
			forwardEntries->width=width;
			}

		maxPrefix=MAX(maxPrefix,prefix);
		maxSuffix=MAX(maxSuffix,suffix);
		maxWidth=MAX(maxWidth,width);
		}

	prefix=-1;
	suffix=-1;
	width=-1;

	for(int i=0;i<oldReverseEntryCount;i++)
		{
		prefix=oldReverseEntries[i].prefix;
		suffix=oldReverseEntries[i].suffix;
		width=oldReverseEntries[i].width;

		maxPrefix=MAX(maxPrefix,prefix);
		maxSuffix=MAX(maxSuffix,suffix);
		maxWidth=MAX(maxWidth,width);

		reverseEntries[i].prefix=prefix;
		reverseEntries[i].suffix=suffix;
		reverseEntries[i].width=width;
		}

	reverseEntries+=oldReverseEntryCount;
	reverseEntries--;

	for(int i=0;i<reverseRoutePatchCount;i++)
		{
		u32 wantedPrefix=reverseRoutePatches[i].prefixIndex;
		u32 wantedSuffix=reverseRoutePatches[i].suffixIndex;

		if(prefix==wantedPrefix && suffix==wantedSuffix)
			{
			//LOG(LOG_INFO,"R merge");
			reverseEntries->width=++width;

			newReverseEntryCount--;
			}
		else
			{
			reverseEntries++;

			prefix=wantedPrefix;
			suffix=wantedSuffix;
			width=1;

			reverseEntries->prefix=prefix;
			reverseEntries->suffix=suffix;
			reverseEntries->width=width;
			}

		maxPrefix=MAX(maxPrefix,prefix);
		maxSuffix=MAX(maxSuffix,suffix);
		maxWidth=MAX(maxWidth,width);
		}


	builder->newForwardEntryCount=newForwardEntryCount;
	builder->newReverseEntryCount=newReverseEntryCount;

	builder->maxPrefix=maxPrefix;
	builder->maxSuffix=maxSuffix;
	builder->maxWidth=maxWidth;

//	LOG(LOG_INFO,"Merging Route Table resulted in %i %i",newForwardEntryCount, newReverseEntryCount);

	u32 prefixBits=bitsRequired(builder->maxPrefix);
	u32 suffixBits=bitsRequired(builder->maxSuffix);
	u32 widthBits=bitsRequired(builder->maxWidth-1);

	int headerSize=getRouteTableHeaderSize(prefixBits,suffixBits,widthBits,newForwardEntryCount,newReverseEntryCount);
	int tableSize=PAD_1BITLENGTH_BYTE((prefixBits+suffixBits+widthBits)*(newForwardEntryCount+newReverseEntryCount));

	builder->totalPackedSize=headerSize+tableSize;
}


void unpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp)
{
	u32 prefixBits=0, suffixBits=0, widthBits=0, forwardEntryCount=0, reverseEntryCount=0;

	if(data!=NULL)
		{
		data+=decodeHeader(data, &prefixBits, &suffixBits, &widthBits, &forwardEntryCount, &reverseEntryCount);

		smerLinked->forwardRouteEntries=dAlloc(disp, sizeof(RouteTableEntry)*forwardEntryCount);
		smerLinked->reverseRouteEntries=dAlloc(disp, sizeof(RouteTableEntry)*reverseEntryCount);

		smerLinked->forwardRouteCount=forwardEntryCount;
		smerLinked->reverseRouteCount=reverseEntryCount;

		unpackRoutes(data, prefixBits, suffixBits, widthBits,
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







