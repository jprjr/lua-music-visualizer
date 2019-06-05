#ifndef STR_H
#define STR_H

#ifdef __cplusplus
extern "C" {
#endif

int str_cmp(const char *s, const char *q);
int str_ncmp(const char *s, const char *q, unsigned int max);
int str_icmp(const char *s, const char *q);
int str_incmp(const char *s, const char *q, unsigned int max);

unsigned int str_cpy(char *d, const char *s);
unsigned int str_len(const char *s);
unsigned int str_nlower(char *d, const char *str, unsigned int max);
unsigned int str_lower(char *d, const char *str);
unsigned int str_starts(const char *s, const char *q);
unsigned int str_istarts(const char *s, const char *q);
unsigned int str_chr(const char *s, char c);
unsigned int str_ends(const char *s, const char *q);
unsigned int str_iends(const char *s, const char *q);
unsigned int str_ncpy(char *d, const char *s, unsigned int max);
unsigned int str_ncat(char *d, const char *s, unsigned int max);
unsigned int str_cat(char *d, const char *s);

/* str_cat with escaping of characters in *e */
/* prepends encountered characters with 't' */
unsigned int str_necat(char *d, const char *s, unsigned int max, const char *e, char t);
unsigned int str_ecat(char *d, const char *s, const char *e, char t);

#ifdef __cplusplus
}
#endif

#endif
