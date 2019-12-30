#ifndef JPR_STR_H
#define JPR_STR_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include <stdint.h>
#include <stddef.h>

#ifndef JPR_NO_STDLIB
#include <string.h>
#include <strings.h>
#include <wchar.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


char *str_nrchr(const char *s, char c, unsigned int len);

#ifdef JPR_NO_STDLIB
char *str_chr(const char *s, char c);
char *str_rchr(const char *s, char c);
void *mem_cpy(void *dest, const void *src, unsigned int n);
void *mem_move(void *dest, const void *src, unsigned int n);
void *mem_chr(const void *src, uint8_t c, unsigned int n);
int mem_cmp(const void *p1, const void *p2, unsigned int n);
void *mem_set(void *s, uint8_t c, unsigned int n);
char *str_dup(const char *s);
int str_cmp(const char *s1, const char *s2);
char *str_str(const char *h, const char *n);
unsigned int str_len(const char *s);
unsigned int str_nlen(const char *s, unsigned int m);
unsigned int wstr_len(const wchar_t *s);
int str_ncmp(const char *s1, const char *s2, unsigned int m);
int str_icmp(const char *s1, const char *s2);
int str_incmp(const char *s1, const char *s2, unsigned int m);
char *str_cat(char *d, const char *s);
char *str_cpy(char *d, const char *p);
char *str_ncpy(char *d, const char *s, unsigned int max);
char *str_ncat(char *d, const char *s, unsigned int max);

#else
#define str_chr strchr
#define str_rchr strrchr
#define mem_cpy memcpy
#define mem_move memmove
#define mem_chr memchr
#define mem_cmp memcmp
#define mem_set memset
#define str_dup strdup
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

unsigned int str_nlower(char *d, const char *str, unsigned int max);
unsigned int str_lower(char *d, const char *str);
unsigned int str_ends(const char *s, const char *q);
unsigned int str_iends(const char *s, const char *q);

#define str_equals(s,q) (str_cmp(s,q) == 0)
#define str_starts(s,q) (str_ncmp(s,q,str_len(q)) == 0)
#define str_istarts(s,q) (str_incmp(s,q,str_len(q)) == 0)

/* str_cat with escaping of characters in *e */
/* prepends encountered characters with 't' */
unsigned int str_necat(char *d, const char *s, unsigned int max, const char *e, char t);
unsigned int str_ecat(char *d, const char *s, const char *e, char t);

#ifdef __cplusplus
}
#endif

#endif
