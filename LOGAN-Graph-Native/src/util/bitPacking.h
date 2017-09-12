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

void bpInitPacker(BitPacker *packer, u8 *ptr, int position);
void bpSeekPacker(BitPacker *packer, int position);
void bpPackBits(BitPacker *packer, int count, u32 data);
u8 *bpPackerGetPaddedPtr(BitPacker *packer);

void bpInitUnpacker(BitUnpacker *unpacker, u8 *ptr, int position);
void bpSeekUnpacker(BitUnpacker *unpacker, int position);
u32 bpUnpackBits(BitUnpacker *unpacker, int count);
u8 *bpUnpackerGetPaddedPtr(BitUnpacker *unpacker);

u32 bpBitsRequired(u32 value);

#endif
