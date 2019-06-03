#ifndef STR_H
#define STR_H

#include <string.h>
#include <strings.h>
#include <ctype.h>

#define str_cpy(d,s) strcpy(d,s)
#define str_len(s) strlen(s)
#define str_cmp(d,s) strcmp(d,s)
#define str_icmp(d,s) strcasecmp(d,s)
#define str_ncmp(d,s,n) strncmp(d,s,n)
#define str_incmp(d,s,n) strncasecmp(d,s,n)
#define str_lower(c) tolower(c)

unsigned int str_strlower(char *str);
unsigned int str_starts(const char *s, const char *q);
unsigned int str_istarts(const char *s, const char *q);
unsigned int str_chr(const char *s, char c);
unsigned int str_ends(const char *s, const char *q);
unsigned int str_iends(const char *s, const char *q);
unsigned int str_ncpy(char *d, const char *s, unsigned int max);
unsigned int str_cat(char *d, const char *s);
unsigned int str_cat_escape(char *d, const char *s);

#endif
