#ifndef JPR_STR_H
#define JPR_STR_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifndef JPR_NO_STDLIB
#include <stddef.h>
#include <string.h>
#ifdef _POSIX_C_SOURCE
#include <strings.h>
#endif
#include <wchar.h>
#ifdef _MSC_VER
#include <mbstring.h>
#endif
#else
#include "int.h"
#endif

#include "attr.h"

#ifdef __cplusplus
extern "C" {
#endif

attr_pure
char *str_nchr(const char *s, char c, size_t len);
attr_pure
char *str_nrchr(const char *s, char c, size_t len);

#ifdef JPR_NO_STDLIB
attr_pure char *str_chr(const char *s, char c);
attr_pure char *str_rchr(const char *s, char c);
void *mem_cpy(void *dest, const void *src, size_t n);
void *mem_move(void *dest, const void *src, size_t n);
attr_pure void *mem_chr(const void *src, int c, size_t n);
attr_pure int mem_cmp(const void *p1, const void *p2, size_t n);
void *mem_set(void *s, int c, size_t n);

attr_nonnull1
char *str_dup(const char *s);

attr_pure int str_cmp(const char *s1, const char *s2);
attr_pure char *str_str(const char *h, const char *n);
attr_pure size_t str_len(const char *s);
attr_pure size_t str_nlen(const char *s, size_t m);
attr_pure size_t wstr_len(const wchar_t *s);
attr_pure int str_ncmp(const char *s1, const char *s2, size_t m);
attr_pure int str_icmp(const char *s1, const char *s2);
attr_pure int str_incmp(const char *s1, const char *s2, size_t m);
char *str_cat(char *d, const char *s);
char *str_cpy(char *d, const char *p);
char *str_ncpy(char *d, const char *s, size_t max);
char *str_ncat(char *d, const char *s, size_t max);

#else

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#define str_chr strchr
#define str_rchr strrchr
#define mem_cpy memcpy
#define mem_move memmove
#define mem_chr memchr
#define mem_cmp memcmp
#define mem_set memset
#ifdef _MSC_VER
#define str_dup _mbsdup
#else
#define str_dup strdup
#endif
#define str_cmp strcmp
#define str_str strstr
#define str_nlen strnlen
#define str_len strlen
#define wstr_len wcslen
#define str_ncmp strncmp
#define str_icmp strcasecmp
#define str_incmp strncasecmp
#define str_cat strcat
#define str_cpy strcpy
#define str_ncpy strncpy
#define str_ncat strncat
#endif

size_t str_nlower(char *d, const char *str, size_t max);
size_t str_lower(char *d, const char *str);
attr_pure size_t str_ends(const char *s, const char *q);
attr_pure size_t str_iends(const char *s, const char *q);
attr_pure size_t str_begins(const char *s, const char *q);
attr_pure size_t str_ibegins(const char *s, const char *q);

#define str_equals(s,q) (str_cmp(s,q) == 0)
#define str_iequals(s,q) (str_icmp(s,q) == 0)
#define str_starts(s,q) (str_ncmp(s,q,str_len(q)) == 0)
#define str_istarts(s,q) (str_incmp(s,q,str_len(q)) == 0)

/* str_cat with escaping of characters in *e */
/* prepends encountered characters with 't' */
size_t str_necat(char *d, const char *s, size_t max, const char *e, char t);
size_t str_ecat(char *d, const char *s, const char *e, char t);

#ifdef __cplusplus
}
#endif

#endif
