#ifndef CHAR_H
#define CHAR_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifndef NO_STDLIB
#include <ctype.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NO_STDLIB
int char_isdigit(char c);
int char_isupper(char c);
int char_islower(char c);
char char_upper(char c);
char char_lower(char c);
#else
#define char_isdigit(c) isdigit(c)
#define char_isupper(c) isupper(c)
#define char_islower(c) islower(c)
#define char_lower(c) tolower(c)
#define char_upper(c) toupper(c)
#endif

#ifdef __cplusplus
}
#endif

#endif
