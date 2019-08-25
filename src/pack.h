#ifndef PACK_H_
#define PACK_H_

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* methods for packing bytes from integers */

uint8_t pack_uint64le(uint8_t *d, uint64_t n);
uint8_t pack_uint64be(uint8_t *d, uint64_t n);
uint8_t pack_uint32le(uint8_t *d, uint32_t n);
uint8_t pack_uint32be(uint8_t *d, uint32_t n);
uint8_t pack_uint16le(uint8_t *d, uint16_t n);
uint8_t pack_uint16be(uint8_t *d, uint16_t n);

#define pack_int64le(d,n) pack_uint64le(d,(uint64_t)n)
#define pack_int64be(d,n) pack_uint64be(d,(uint64_t)n)
#define pack_int32le(d,n) pack_uint32le(d,(uint32_t)n)
#define pack_int32be(d,n) pack_uint32be(d,(uint32_t)n)
#define pack_int16le(d,n) pack_uint16le(d,(uint16_t)n)
#define pack_int16be(d,n) pack_uint16be(d,(uint16_t)n)

#ifdef __cplusplus
}
#endif

#endif
