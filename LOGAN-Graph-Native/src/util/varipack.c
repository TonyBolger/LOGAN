
#include "common.h"


u32 varipackLength(u32 value)
{
	int count=1;
	while(value>0xFF)
		{
		value>>=8;
		count++;
		}
	return count;
}

u32 varipackEncode(u32 value, u8 *encBuffer)
{
	int count=1;
	while(value>0xFF)
		{
		*encBuffer++=value;
		value>>=8;
		count++;
		}

	*encBuffer++=value;
	return count;
}

u32 varipackDecode(u32 length, u8 *encBuffer)
{
	u32 value=0;

	while(length-->0)
		value=(value<<8)|*encBuffer++;


	return value;
}


