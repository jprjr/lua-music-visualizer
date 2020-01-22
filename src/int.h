#ifndef JPR_INT_H
#define JPR_INT_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include <stddef.h>

#if defined(_MSC_VER) && _MSC_VER < 1600
typedef   signed char    jpr_int8;
typedef unsigned char    jpr_uint8;
typedef   signed short   jpr_int16;
typedef unsigned short   jpr_uint16;
typedef   signed int     jpr_int32;
typedef size_t     jpr_uint32;
typedef   signed __int64 jpr_int64;
typedef unsigned __int64 jpr_uint64;
#else
#include <stdint.h>
typedef int8_t           jpr_int8;
typedef uint8_t          jpr_uint8;
typedef int16_t          jpr_int16;
typedef uint16_t         jpr_uint16;
typedef int32_t          jpr_int32;
typedef uint32_t         jpr_uint32;
typedef int64_t          jpr_int64;
typedef uint64_t         jpr_uint64;
#endif

#endif
