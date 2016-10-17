
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

u32 varipackDecode(u32 length, u8 *encBuffer)
{
	u32 value=0;

//	if(length>1)
		//LOG(LOG_INFO,"Varipack decode len %i",length);

	while(length-->0)
		value=(value<<8)|encBuffer[length];

	//LOG(LOG_INFO,"Varipack decode val %i",value);

	return value;
}


