#ifndef UNPACK_H
#define UNPACK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* methods for unpacking integers from bytes */

uint64_t unpack_uint64le(const uint8_t *b);
uint64_t unpack_uint64be(const uint8_t *b);
uint32_t unpack_uint32le(const uint8_t *b);
uint32_t unpack_uint32be(const uint8_t *b);
uint16_t unpack_uint16le(const uint8_t *b);
uint16_t unpack_uint16be(const uint8_t *b);

#define unpack_int64le(b) ((int64_t)unpack_uint64le(b))
#define unpack_int64be(b) ((int64_t)unpack_uint64be(b))
#define unpack_int32le(b) ((int32_t)unpack_uint32le(b))
#define unpack_int32be(b) ((int32_t)unpack_uint32be(b))
#define unpack_int16le(b) ((int16_t)unpack_uint16le(b))
#define unpack_int16be(b) ((int16_t)unpack_uint16be(b))

#ifdef __cplusplus
}
#endif

#endif
