#ifndef __VARIPACK_H
#define __VARIPACK_H

u32 vpExtCalcLength(u32 value);
u32 vpExtEncode(u32 value, u8 *encBuffer);
u32 vpExtDecode(u32 length, u8 *encBuffer);

u32 vpIntCalcLength(u32 value);
u32 vpIntEncode(u32 value, u8 *encBuffer);
u32 vpIntDecode(u8 *encBuffer, u32 *valuePtr);

#endif
