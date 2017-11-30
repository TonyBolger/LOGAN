
#include "common.h"

/*
 * Variable length encoding, requiring length to be stored elsewhere
 */

u32 vpExtCalcLength(u32 value)
{
	int count=1;
	while(value>0xFF)
		{
		value>>=8;
		count++;
		}
	return count;
}

u32 vpExtEncode(u32 value, u8 *encBuffer)
{
	//LOG(LOG_INFO,"Varipack encode val %i",value);

	int count=1;
	while(value>0xFF)
		{
		*encBuffer++=value;
		value>>=8;
		count++;
		}

	*encBuffer++=value;

	//if(count>1)
//		LOG(LOG_INFO,"Varipack encode len %i",count);

	return count;
}

u32 vpExtDecode(u32 length, u8 *encBuffer)
{
	u32 value=0;

//	if(length>1)
		//LOG(LOG_INFO,"Varipack decode len %i",length);

	while(length-->0)
		value=(value<<8)|encBuffer[length];

	//LOG(LOG_INFO,"Varipack decode val %i",value);

	return value;
}


/*
 * Variable length encoding, using the top bits of the initial byte to encode the length
 *
 * 00000000-0000007F 	-> 0xxxxxxx									(0x00 <= x < 0x80)
 * 00000080-00003FFF   	-> 10xxxxxx xxxxxxxx 						(0x8000 <= x < 0xC000)
 * 00004000-001FFFFF    -> 110xxxxx xxxxxxxx xxxxxxxx 				(0xC00000 <= x < 0xE00000)
 * 00200000-0FFFFFFF    -> 1110xxxx xxxxxxxx xxxxxxxx xxxxxxx 		(0xE0000000 <= x < 0xF0000000)
 */

u32 vpIntCalcLength(u32 value)
{
	if(value<0x80)
		return 1;

	if(value<0x4000)
		return 2;

	if(value<0x200000)
		return 3;

	if(value<0x10000000)
		return 4;

	LOG(LOG_CRITICAL,"Cannot varipack encode value: %08x",value);
	return -1;
}

u32 vpIntEncode(u32 value, u8 *encBuffer)
{
	if(value<0x80)
		{
		*encBuffer=value;
		return 1;
		}

	if(value<0x4000)
		{
		*(encBuffer++)=0x80|(value>>8);
		*(encBuffer++)=value&0xFF;
		return 2;
		}

	if(value<0x200000)
		{
		*(encBuffer++)=0xC0|(value>>16);
		*(encBuffer++)=(value>>8)&0xFF;
		*(encBuffer++)=value&0xFF;
		return 3;
		}

	if(value<0x10000000)
		{
		*(encBuffer++)=0xE0|(value>>24);
		*(encBuffer++)=(value>>16)&0xFF;
		*(encBuffer++)=(value>>8)&0xFF;
		*(encBuffer++)=value&0xFF;
		return 4;
		}

	LOG(LOG_CRITICAL,"Cannot varipack encode value: %08x",value);
	return -1;
}

u32 vpIntDecode(u8 *encBuffer, u32 *valuePtr)
{
	u32 data=*(encBuffer++);

	if(data<0x80)
		{
		*valuePtr=data;
		return 1;
		}

	data=(data<<8)|*(encBuffer++);

	if(data<0xC000)
		{
		*valuePtr=(data&0x3FFF);
		return 2;
		}

	data=(data<<8)|*(encBuffer++);

	if(data<0xE00000)
		{
		*valuePtr=(data&0x1FFFFF);
		return 3;
		}

	data=(data<<8)|*(encBuffer++);

	if(data<0xF0000000)
		{
		*valuePtr=(data&0xFFFFFFF);
		return 4;
		}

	LOG(LOG_CRITICAL,"Cannot varipack decode value: %08x",data);
	return -1;

}


