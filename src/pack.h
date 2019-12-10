#ifndef JPR_PACK_H
#define JPR_PACK_H

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

uint8_t pack_int64le(uint8_t *d, int64_t n);
uint8_t pack_int64be(uint8_t *d, int64_t n);
uint8_t pack_int32le(uint8_t *d, int32_t n);
uint8_t pack_int32be(uint8_t *d, int32_t n);
uint8_t pack_int16le(uint8_t *d, int16_t n);
uint8_t pack_int16be(uint8_t *d, int16_t n);


#ifdef __cplusplus
}
#endif

#endif
