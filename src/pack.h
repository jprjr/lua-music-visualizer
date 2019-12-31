#ifndef JPR_PACK_H
#define JPR_PACK_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include "int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* methods for packing bytes from integers */

jpr_uint8 pack_uint64le(jpr_uint8 *d, jpr_uint64 n);
jpr_uint8 pack_uint64be(jpr_uint8 *d, jpr_uint64 n);
jpr_uint8 pack_uint32le(jpr_uint8 *d, jpr_uint32 n);
jpr_uint8 pack_uint32be(jpr_uint8 *d, jpr_uint32 n);
jpr_uint8 pack_uint16le(jpr_uint8 *d, jpr_uint16 n);
jpr_uint8 pack_uint16be(jpr_uint8 *d, jpr_uint16 n);

jpr_uint8 pack_int64le(jpr_uint8 *d, jpr_int64 n);
jpr_uint8 pack_int64be(jpr_uint8 *d, jpr_int64 n);
jpr_uint8 pack_int32le(jpr_uint8 *d, jpr_int32 n);
jpr_uint8 pack_int32be(jpr_uint8 *d, jpr_int32 n);
jpr_uint8 pack_int16le(jpr_uint8 *d, jpr_int16 n);
jpr_uint8 pack_int16be(jpr_uint8 *d, jpr_int16 n);


#ifdef __cplusplus
}
#endif

#endif
