#ifndef JPR_UNPACK_H
#define JPR_UNPACK_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include "int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* methods for unpacking integers from bytes */

jpr_uint64 unpack_uint64le(const jpr_uint8 *b);
jpr_uint64 unpack_uint64be(const jpr_uint8 *b);
jpr_uint32 unpack_uint32le(const jpr_uint8 *b);
jpr_uint32 unpack_uint32be(const jpr_uint8 *b);
jpr_uint32 unpack_uint24le(const jpr_uint8 *b);
jpr_uint32 unpack_uint24be(const jpr_uint8 *b);
jpr_uint16 unpack_uint16le(const jpr_uint8 *b);
jpr_uint16 unpack_uint16be(const jpr_uint8 *b);

jpr_int64 unpack_int64le(const jpr_uint8 *b);
jpr_int64 unpack_int64be(const jpr_uint8 *b);
jpr_int32 unpack_int32le(const jpr_uint8 *b);
jpr_int32 unpack_int32be(const jpr_uint8 *b);
jpr_int32 unpack_int24le(const jpr_uint8 *b);
jpr_int32 unpack_int24be(const jpr_uint8 *b);
jpr_int16 unpack_int16le(const jpr_uint8 *b);
jpr_int16 unpack_int16be(const jpr_uint8 *b);


#ifdef __cplusplus
}
#endif

#endif
