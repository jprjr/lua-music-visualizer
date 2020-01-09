#ifndef JPR_CHAR_H
#define JPR_CHAR_H

#include "attr.h"

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifndef JPR_NO_STDLIB
#include <ctype.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef JPR_NO_STDLIB
attr_pure int char_isdigit(int c);
attr_pure int char_isupper(int c);
attr_pure int char_islower(int c);
attr_pure int char_upper(int c);
attr_pure int char_lower(int c);
#else
#define char_isdigit isdigit
#define char_isupper isupper
#define char_islower islower
#define char_upper   toupper
#define char_lower   tolower
#endif

#ifdef __cplusplus
}
#endif

#endif
