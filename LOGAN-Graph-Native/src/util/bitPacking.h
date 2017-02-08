/*
 * bitPacking.h
 *
 *  Created on: Nov 10, 2011
 *      Author: tony
 */

#ifndef __BIT_PACKING_H
#define __BIT_PACKING_H


typedef struct bitPackerStr {
	//u8 *data;
	u32 *data;
	int position;
} BitPacker;


typedef struct bitUnpackerStr {
	u32 *data;
	int position;
} BitUnpacker;

void initPacker(BitPacker *packer, u8 *ptr, int position);
void seekPacker(BitPacker *packer, int position);
void packBits(BitPacker *packer, int count, u32 data);

void initUnpacker(BitUnpacker *unpacker, u8 *ptr, int position);
void seekUnpacker(BitUnpacker *unpacker, int position);
u32 unpackBits(BitUnpacker *unpacker, int count);

u32 bitsRequired(u32 value);

#endif
