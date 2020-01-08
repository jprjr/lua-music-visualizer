#ifndef JPR_MAT_H
#define JPR_MAT_H

#include "attr.h"

#ifdef __cplusplus
extern "C" {
#endif

double d_round(double x) attr_const;
double d_sqrt(double x) attr_const;

double round(double x) attr_const;
double sqrt(double x) attr_const;

#ifdef __cplusplus
}
#endif
#endif