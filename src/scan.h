#ifndef JPR_SCAN_H
#define JPR_SCAN_H

#include "int.h"

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifdef __cplusplus
extern "C" {
#endif

size_t scan_uint(const char *s, unsigned int *num);
size_t scan_int(const char *s, int *num);
size_t scan_uint64(const char *s, jpr_uint64 *num);
size_t scan_int64(const char *s, jpr_int64 *num);
size_t scan_uint32(const char *s, jpr_uint32 *num);
size_t scan_int32(const char *s, jpr_int32 *num);
size_t scan_uint16(const char *s, jpr_uint16 *num);
size_t scan_int16(const char *s, jpr_int16 *num);
size_t scan_sizet(const char *s, size_t *num);
size_t scan_uint64_base16(const char *s, jpr_uint64 *num);
size_t scan_uint32_base16(const char *s, jpr_uint32 *num);

#ifdef __cplusplus
}
#endif

#endif
