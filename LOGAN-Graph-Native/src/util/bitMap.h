/*
 * bitMap.h
 *
 *  Created on: Dec 5, 2011
 *      Author: tony
 */

#ifndef __BIT_MAP_H
#define __BIT_MAP_H

#include "../common.h"


#define BIT_MAP_MASK 0x3F;
#define BIT_MAP_SHIFT 6;


typedef struct hashedBitMapStr
{
	u64 *data;
	u32 hash;
} HashedBitMap;



void initHashedBitMap(HashedBitMap *bitMap, u64 *data, u32 hash);
void markHashedBitMapEntry(HashedBitMap *bitMap, s32 index);
int isHashedBitMapEntrySet(HashedBitMap *bitMap, s32 index);

int testAndMarkBitMapEntry(u64 *data, s32 index);

#endif
