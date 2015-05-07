#ifndef NUMBER_H
#define NUMBER_H

#include <stdint.h>

#ifdef __APPLE__

#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#else

#define _BSD_SOURCE
#include <endian.h>

#endif

/* Read signed or unsigned 16, 32 or 64 bit integers
 * from a buffer in big endian (aka Motorola format). */
static inline   int8_t  ld_i8_be(const void *x){return ((int8_t *) x)[0];}
static inline  int16_t ld_i16_be(const void *x){return be16toh(((int16_t *) x)[0]);}
static inline  int32_t ld_i32_be(const void *x){return be32toh(((int32_t *) x)[0]);}
static inline  int64_t ld_i64_be(const void *x){return be64toh(((int64_t *) x)[0]);}
static inline  uint8_t  ld_u8_be(const void *x){return ((uint8_t *) x)[0];}
static inline uint16_t ld_u16_be(const void *x){return be16toh(((uint16_t *) x)[0]);}
static inline uint32_t ld_u32_be(const void *x){return be32toh(((uint32_t *) x)[0]);}
static inline uint64_t ld_u64_be(const void *x){return be64toh(((uint64_t *) x)[0]);}

/* Read signed or unsigned 16, 32 or 64 bit integers
 * from a buffer in little endian (aka Intel format). */
static inline   int8_t  ld_i8_le(const void *x){return ((int8_t *) x)[0];}
static inline  int16_t ld_i16_le(const void *x){return le16toh(((int16_t *) x)[0]);}
static inline  int32_t ld_i32_le(const void *x){return le32toh(((int32_t *) x)[0]);}
static inline  int64_t ld_i64_le(const void *x){return le64toh(((int64_t *) x)[0]);}
static inline  uint8_t  ld_u8_le(const void *x){return ((uint8_t *) x)[0];}
static inline uint16_t ld_u16_le(const void *x){return le16toh(((uint16_t *) x)[0]);}
static inline uint32_t ld_u32_le(const void *x){return le32toh(((uint32_t *) x)[0]);}
static inline uint64_t ld_u64_le(const void *x){return le64toh(((uint64_t *) x)[0]);}

/* Write signed or unsigned 16, 32 or 64 bit integers
 * to a buffer in big endian. */
static inline void  st_i8_be(void *x,   int8_t i){((int8_t *) x)[0] = i;}
static inline void st_i16_be(void *x,  int16_t i){((int16_t *) x)[0] = htobe16(i);}
static inline void st_i32_be(void *x,  int32_t i){((int32_t *) x)[0] = htobe32(i);}
static inline void st_i64_be(void *x,  int64_t i){((int64_t *) x)[0] = htobe64(i);}
static inline void  st_u8_be(void *x,  uint8_t u){((uint8_t *) x)[0] = u;}
static inline void st_u16_be(void *x, uint16_t u){((uint16_t *) x)[0] = htobe16(u);}
static inline void st_u32_be(void *x, uint32_t u){((uint32_t *) x)[0] = htobe32(u);}
static inline void st_u64_be(void *x, uint64_t u){((uint64_t *) x)[0] = htobe64(u);}

/* Write signed or unsigned 16, 32 or 64 bit integers
 * to a buffer in little endian. */
static inline void  st_i8_le(void *x,   int8_t i){((int8_t *) x)[0] = i;}
static inline void st_i16_le(void *x,  int16_t i){((int16_t *) x)[0] = htole16(i);}
static inline void st_i32_le(void *x,  int32_t i){((int32_t *) x)[0] = htole32(i);}
static inline void st_i64_le(void *x,  int64_t i){((int64_t *) x)[0] = htole64(i);}
static inline void  st_u8_le(void *x,  uint8_t u){((uint8_t *) x)[0] = u;}
static inline void st_u16_le(void *x, uint16_t u){((uint16_t *) x)[0] = htole16(u);}
static inline void st_u32_le(void *x, uint32_t u){((uint32_t *) x)[0] = htole32(u);}
static inline void st_u64_le(void *x, uint64_t u){((uint64_t *) x)[0] = htole64(u);}

#endif
