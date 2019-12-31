#ifndef JPR_FMT_H
#define JPR_FMT_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */
#include "int.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t fmt_uint(char *d, jpr_uint64 num);
size_t fmt_int(char *d, jpr_int64 num);

#ifdef __cplusplus
}
#endif

#endif
