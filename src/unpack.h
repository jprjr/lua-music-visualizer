#ifndef UNPACK_H
#define UNPACK_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* methods for unpacking integers from bytes */

uint64_t unpack_uint64le(const uint8_t *b);
uint64_t unpack_uint64be(const uint8_t *b);
uint32_t unpack_uint32le(const uint8_t *b);
uint32_t unpack_uint32be(const uint8_t *b);
uint32_t unpack_uint24le(const uint8_t *b);
uint32_t unpack_uint24be(const uint8_t *b);
uint16_t unpack_uint16le(const uint8_t *b);
uint16_t unpack_uint16be(const uint8_t *b);

int64_t unpack_int64le(const uint8_t *b);
int64_t unpack_int64be(const uint8_t *b);
int32_t unpack_int32le(const uint8_t *b);
int32_t unpack_int32be(const uint8_t *b);
int32_t unpack_int24le(const uint8_t *b);
int32_t unpack_int24be(const uint8_t *b);
int16_t unpack_int16le(const uint8_t *b);
int16_t unpack_int16be(const uint8_t *b);


#ifdef __cplusplus
}
#endif

#endif
