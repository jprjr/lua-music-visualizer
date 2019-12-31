#ifndef JPR_SCAN_H
#define JPR_SCAN_H

#include "int.h"

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifdef __cplusplus
extern "C" {
#endif

size_t scan_uint(const char *s, jpr_uint64 *num);
size_t scan_int(const char *s, jpr_int64 *num);


#ifdef __cplusplus
}
#endif

#endif
