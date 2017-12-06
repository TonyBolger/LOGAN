/*
 * pack.h
 *
 *  Created on: Jul 2, 2014
 *      Author: tony
 */

#ifndef __PACK_H
#define __PACK_H


/*
 * The COMP_BASE constants depend on the binary encoding in packChar
 *
 * 0x1 / 0x5 (e.g A-T-C-G binary encoding)
 * 0x2 / 0xA (e.g A-C-T-G binary encoding)
 * 0x3 / 0xF (e.g A-C-G-T binary encoding)
 *
 * 0x2 binary encoding enables (x&0x6)>>2 ASCII -> BIN mapping, so could be preferable
 */

#define COMP_BASE_SINGLE(x) ((x)^0x3)
#define COMP_BASE_QUAD(x) ((x)^0xFF)
#define COMP_BASE_HEX(x) ((x)^0xFFFFFFFF)
#define COMP_BASE_32(x)  ((x)^0xFFFFFFFFFFFFFFFFL)


u32 packChar(u8 ch);
void packSequence(char *seq, u8 *packedSeq, int length);
void unpackSequence(u8 *packedSeq, int length, char *seq);

void unpackSmer(SmerId smer, char *out);


#endif
