/*
 * bitMap.c
 *
 *  Created on: Dec 5, 2011
 *      Author: tony
 */

#include "bitMap.h"


void initHashedBitMap(HashedBitMap *bitMap, u64 *data, u32 hash)
{
	bitMap->data=data;
	bitMap->hash=hash;
}

void markHashedBitMapEntry(HashedBitMap *bitMap, s32 index)
{
	int location=index%bitMap->hash;
	int bitOffset=location&BIT_MAP_MASK;
	int ptrOffset=location>>BIT_MAP_SHIFT;

	*(bitMap->data+ptrOffset)|=1L<<bitOffset;
}

int isHashedBitMapEntrySet(HashedBitMap *bitMap, s32 index)
{
	int location=index%bitMap->hash;
	int bitOffset=location&BIT_MAP_MASK;
	int ptrOffset=location>>BIT_MAP_SHIFT;

	return (*(bitMap->data+ptrOffset)>>bitOffset)&0x1L;
}



int testAndMarkBitMapEntry(u64 *data, s32 index)
{
	int bitOffset=index&BIT_MAP_MASK;

	data+=index>>BIT_MAP_SHIFT;

	u64 val=1L<<bitOffset;
	if(val & *data)
		return 1;

	*data|=val;

	return 0;
}


