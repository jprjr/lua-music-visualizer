#ifndef JPR_MAT_H
#define JPR_MAT_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include "attr.h"

#ifdef __cplusplus
extern "C" {
#endif

attr_const double d_round(double x);
attr_const double d_sqrt(double x);

attr_const double round(double x);
attr_const double sqrt(double x);

#ifdef __cplusplus
}
#endif
#endif
