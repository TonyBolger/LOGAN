#ifndef __VARIPACK_H
#define __VARIPACK_H

u32 varipackLength(u32 value);
u32 varipackEncode(u32 value, u8 *encBuffer);
u32 varipackDecode(u32 length, u8 *encBuffer);

#endif
