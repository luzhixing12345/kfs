#ifndef EXT4_BASIC_H
#define EXT4_BASIC_H

#include <stdint.h>

/* TODO: This is a good place to address endianness.  The defined types should
 * not leak outside the types directory anyway. */

#define __le64 uint64_t
#define __le32 uint32_t
#define __le16 uint16_t
#define __u64  uint64_t
#define __u32  uint32_t
#define __u16  uint16_t
#define __u8   uint8_t

#define MASK_16 0xFFFF
#define MASK_32 0xFFFFFFFF
#define MASK_64 0xFFFFFFFFFFFFFFFF

#define U16_MAX 0xFFFF
#define U32_MAX 0xFFFFFFFF
#define U64_MAX 0xFFFFFFFFFFFFFFFF

#endif
