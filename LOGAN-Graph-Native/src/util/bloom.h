/*
 * bloom.h
 *
 *  Created on: Jun 6, 2014
 *      Author: tony
 */

#ifndef __BLOOM_H
#define __BLOOM_H

#include "../common.h"


typedef struct BloomStr
{
	u64 *data;
	u32 mask;
	u32 hashes;
} Bloom;


void initBloom(Bloom *bloom, u32 size, u32 hashes, u32 bitsPerEntry);
void freeBloom(Bloom *bloom);

void setBloom(Bloom *bloom, u64 value);
int testBloom(Bloom *bloom, u64 value);

#endif

