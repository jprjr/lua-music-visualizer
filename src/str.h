#ifndef STR_H
#define STR_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#

#define str_cmp(s,q) strcmp(s,q)
#define str_ncmp(s,q,max) strncmp(s,q,max)
#define str_icmp(s,q) strcasecmp(s,q)
#define str_incmp(s,q,max) strncasecmp(s,q,max)

#define str_len(s) strlen(s)
#define str_nlen(s,m) strnlen(s,m)


#ifdef _WIN32
char *stpcpy(char * dst, const char * src);
unsigned int str_ncpy(char *d, const char *s, unsigned int max);
#else
#define str_ncpy(d,s,m) (stpncpy(d,s,m) - d)
#endif
#define str_cpy(d,s) (stpcpy(d,s) - d)

#define str_starts(s,q) (str_ncmp(s,q,str_len(q)) == 0)
#define str_istarts(s,q) (str_incmp(s,q,str_len(q)) == 0)

#define str_ends(s,q) (str_cmp(s + str_len(s) - str_len(q),q) == 0)
#define str_iends(s,q) (str_icmp(s + str_len(s) - str_len(q),q) == 0)

#define str_cat(d,s) (strcat(d,s),strlen(s))
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
