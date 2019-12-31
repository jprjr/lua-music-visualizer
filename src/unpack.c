#include "unpack.h"

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

jpr_uint64 unpack_uint64le(const jpr_uint8 *b) {
    return ((jpr_uint64 *)b)[0];
}

jpr_int64 unpack_int64le(const jpr_uint8 *b) {
    return ((jpr_int64 *)b)[0];
}

jpr_uint32 unpack_uint32le(const jpr_uint8 *b) {
    return ((jpr_uint32 *)b)[0];
}

jpr_int32 unpack_int32le(const jpr_uint8 *b) {
    return ((jpr_int32 *)b)[0];
}

jpr_uint16 unpack_uint16le(const jpr_uint8 *b) {
    return ((jpr_uint16 *)b)[0];
}

jpr_int16 unpack_int16le(const jpr_uint8 *b) {
    return ((jpr_int16 *)b)[0];
}

#else

jpr_uint64 unpack_uint64le(const jpr_uint8 *b) {
    return (((jpr_uint64)b[7])<<56) |
           (((jpr_uint64)b[6])<<48) |
           (((jpr_uint64)b[5])<<40) |
           (((jpr_uint64)b[4])<<32) |
           (((jpr_uint64)b[3])<<24) |
           (((jpr_uint64)b[2])<<16) |
           (((jpr_uint64)b[1])<<8 ) |
           (((jpr_uint64)b[0])    );
}

jpr_int64 unpack_int64le(const jpr_uint8 *b) {
    return
      (jpr_int64)(((jpr_int64)((  jpr_int8)b[7])) << 56) |
               (((jpr_uint64)b[6]) << 48) |
               (((jpr_uint64)b[5]) << 40) |
               (((jpr_uint64)b[4]) << 32) |
               (((jpr_uint64)b[3]) << 24) |
               (((jpr_uint64)b[2]) << 16) |
               (((jpr_uint64)b[1]) << 8 ) |
               (((jpr_uint64)b[0])      );
}

jpr_uint32 unpack_uint32le(const jpr_uint8 *b) {
    return (((jpr_uint32)b[3])<<24) |
           (((jpr_uint32)b[2])<<16) |
           (((jpr_uint32)b[1])<<8 ) |
           (((jpr_uint32)b[0])    );
}

jpr_int32 unpack_int32le(const jpr_uint8 *b) {
	jpr_int32 r;
	r  = (jpr_int32) ((jpr_int8)b[3])   << 24;
	r |= (jpr_int32) ((jpr_uint32)b[2]) << 16;
	r |= (jpr_int32) ((jpr_uint32)b[1]) <<  8;
	r |= (jpr_int32) ((jpr_uint32)b[0])      ;
	return r;

}

jpr_uint16 unpack_uint16le(const jpr_uint8 *b) {
    return (((jpr_uint16)b[1])<<8) |
           (((jpr_uint16)b[0])   );
}

jpr_int16 unpack_int16le(const jpr_uint8 *b) {
    return
      (jpr_int16)(((  jpr_int8)b[1]) << 8 ) |
               (((jpr_uint16)b[0])      );
}

#endif

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

jpr_uint64 unpack_uint64be(const jpr_uint8 *b) {
    return ((jpr_uint64 *)b)[0];
}

jpr_int64 unpack_int64be(const jpr_uint8 *b) {
    return ((jpr_int64 *)b)[0];
}

jpr_uint32 unpack_uint32be(const jpr_uint8 *b) {
    return ((jpr_uint32 *)b)[0];
}

jpr_int32 unpack_int32be(const jpr_uint8 *b) {
    return ((jpr_int32 *)b)[0];
}

jpr_uint16 unpack_uint16be(const jpr_uint8 *b) {
    return ((jpr_uint16 *)b)[0];
}

jpr_int16 unpack_int16be(const jpr_uint8 *b) {
    return ((jpr_int16 *)b)[0];
}

#else

jpr_uint64 unpack_uint64be(const jpr_uint8 *b) {
    return (((jpr_uint64)b[0])<<56) |
           (((jpr_uint64)b[1])<<48) |
           (((jpr_uint64)b[2])<<40) |
           (((jpr_uint64)b[3])<<32) |
           (((jpr_uint64)b[4])<<24) |
           (((jpr_uint64)b[5])<<16) |
           (((jpr_uint64)b[6])<<8 ) |
           (((jpr_uint64)b[7])    );
}

jpr_int64 unpack_int64be(const jpr_uint8 *b) {
	jpr_int64 r;
	r  = ((jpr_int64)((  jpr_int8)b[0])) << 56;
	r |= ((jpr_uint64)b[1]) << 48;
	r |= ((jpr_uint64)b[2]) << 40;
	r |= ((jpr_uint64)b[3]) << 32;
	r |= ((jpr_uint64)b[4]) << 24;
	r |= ((jpr_uint64)b[5]) << 16;
	r |= ((jpr_uint64)b[6]) <<  8;
	r |= ((jpr_uint64)b[7])      ;
	return r;

}

jpr_uint32 unpack_uint32be(const jpr_uint8 *b) {
    return (((jpr_uint32)b[0])<<24) |
           (((jpr_uint32)b[1])<<16) |
           (((jpr_uint32)b[2])<<8 ) |
           (((jpr_uint32)b[3])    );
}

jpr_int32 unpack_int32be(const jpr_uint8 *b) {
	jpr_int32 r;
	r = (jpr_int32)((jpr_int8)b[0]) << 24;
	r |= ((jpr_uint32)b[1]) << 16;
	r |= ((jpr_uint32)b[2]) <<  8;
	r |= ((jpr_uint32)b[3])      ;
	return r;
}

jpr_uint16 unpack_uint16be(const jpr_uint8 *b) {
    return (((jpr_uint16)b[0])<<8) |
           (((jpr_uint16)b[1])   );
}

jpr_int16 unpack_int16be(const jpr_uint8 *b) {
    return
      (jpr_int16)(((  jpr_int8)b[0]) << 8 ) |
               (((jpr_uint16)b[1])      );
}

#endif

jpr_uint32 unpack_uint24le(const jpr_uint8 *b) {
    return (((jpr_uint32)b[2])<<16) |
           (((jpr_uint32)b[1])<<8 ) |
           (((jpr_uint32)b[0])    );
}

jpr_uint32 unpack_uint24be(const jpr_uint8 *b) {
    return (((jpr_uint32)b[0])<<16) |
           (((jpr_uint32)b[1])<<8 ) |
           (((jpr_uint32)b[2])    );
}

jpr_int32 unpack_int24le(const jpr_uint8 *b) {
	jpr_int32 r;
	r = (jpr_int32)((jpr_int8)b[2]) << 16;
	r |= ((jpr_uint32)b[1]) << 8;
	r |= ((jpr_uint32)b[0])     ;
	return r;
}

jpr_int32 unpack_int24be(const jpr_uint8 *b) {
	jpr_int32 r;
	r = (jpr_int32)((jpr_int8)b[0]) << 16;
	r |= ((jpr_uint32)b[1]) << 8;
	r |= ((jpr_uint32)b[2])     ;
	return r;
}
