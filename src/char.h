#ifndef CHAR_H
#define CHAR_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

#define char_isdigit(c) isdigit(c)
#define char_isupper(c) isupper(c)
#define char_islower(c) islower(c)
#define char_lower(c) tolower(c)
#define char_upper(c) toupper(c)

#ifdef __cplusplus
}
#endif

#endif
