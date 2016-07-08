
#include "../common.h"



// Large format:  1 1 W W W W W W  P P P P S S S S
//                F F F F F F F F  F F F F F F F F
// 				  R R R R R R R R  R R R R R R R R

// Medium format: 1 0 W W W W P P  P S S S F F F F
//                F F R R R R R R

// Small format:  0 W W W P P S S  F F F F R R R R


int getRouteTableHeaderSize(int prefixBits, u32 suffixBits, u32 widthBits, u32 forwardEntries, u32 reverseEntries)
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


int encodeRouteTableHeader(u8 *packedData, u32 prefixBits, u32 suffixBits, u32 widthBits, u32 forwardEntries, u32 reverseEntries)
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


int decodeHeader(u8 *packedData, u32 *prefixBits, u32 *suffixBits, u32 *widthBits, u32 *forwardEntries, u32 *reverseEntries)
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
		f=*((short *)(packedData+2));
		r=*((short *)(packedData+4));

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










u8 *initRouteTableBuilder(RouteTableBuilder *builder, u8 *data, MemDispenser *disp)
{
	return NULL;
}

s32 getRouteTableBuilderDirty(RouteTableBuilder *builder)
{
	return 0;
}

s32 getRouteTableBuilderPackedSize(RouteTableBuilder *builder)
{
	return 0;
}

u8 *writeRouteTableBuilderPackedData(RouteTableBuilder *builder, u8 *data)
{
	return data;
}

s32 mergeRoutes(RouteTableBuilder *builder, SmerId smer, s32 tailLength)
{
	return 0;
}

