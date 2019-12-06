#ifndef STR_H
#define STR_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include <stdint.h>

#ifndef NO_STDLIB
#include <string.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NO_STDLIB
void *mem_chr(const uint8_t *src, uint8_t c, unsigned int n);
int mem_cmp(const uint8_t *p1, const uint8_t *p2, unsigned int n);
unsigned int str_len(const char *s);
unsigned int str_nlen(const char *s, unsigned int m);
int str_cmp(const char *s1, const char *s2);
int str_ncmp(const char *s1, const char *s2, unsigned int m);
int str_icmp(const char *s1, const char *s2);
int str_incmp(const char *s1, const char *s2, unsigned int m);
unsigned int str_cat(char *d, const char *s);
#else
#define mem_chr(src,c,n) memchr(src,c,n)
#define mem_cmp(p1,p2,n) memcmp(p1,p2,n)
#define str_len(s) strlen(s)
#define str_nlen(s,m) strnlen(s,m)
#define str_cmp(s,q) strcmp(s,q)
#define str_ncmp(s,q,max) strncmp(s,q,max)
#define str_icmp(s,q) strcasecmp(s,q)
#define str_incmp(s,q,max) strncasecmp(s,q,max)
#define str_cat(d,s) (strcat(d,s),strlen(s))
#endif

#if defined(NO_STDLIB) || defined(_WIN32)
unsigned int str_cpy(char *d, const char *p);
unsigned int str_ncpy(char *d, const char *s, unsigned int max);
#else
#define str_cpy(d,s) (stpcpy(d,s) - d)
#define str_ncpy(d,s,m) (stpncpy(d,s,m) - d)
#endif

#define str_equals(s,q) (str_cmp(s,q) == 0)

#define str_starts(s,q) (str_ncmp(s,q,str_len(q)) == 0)
#define str_istarts(s,q) (str_incmp(s,q,str_len(q)) == 0)

#define str_ends(s,q) (str_cmp(&s[str_len(s) - str_len(q)],q) == 0)
#define str_iends(s,q) (str_icmp(&s[str_len(s) - str_len(q)],q) == 0)

unsigned int str_ncat(char *d,const char *s,unsigned int max); /* returns number of characters cat'd */

unsigned int str_chr(const char *s, char c);
unsigned int str_nlower(char *d, const char *str, unsigned int max);
unsigned int str_lower(char *d, const char *str);


/* str_cat with escaping of characters in *e */
/* prepends encountered characters with 't' */
unsigned int str_necat(char *d, const char *s, unsigned int max, const char *e, char t);
unsigned int str_ecat(char *d, const char *s, const char *e, char t);

#ifdef __cplusplus
}
#endif

#endif
