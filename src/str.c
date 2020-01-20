#include "norm.h"
#include "char.h"
#include "int.h"
#include "attr.h"
#include <stdlib.h>

#ifndef JPR_NO_STDLIB
#include <string.h>
#ifndef JPR_WINDOWS
#include <strings.h>
#endif
#include <wchar.h>
#endif
#include <stdlib.h>

#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifdef __cplusplus
extern "C" {
#endif
attr_pure char *str_chr(const char *s, char c);
attr_pure char *str_rchr(const char *s, char c);
void *mem_cpy(void *dest, const void *src, size_t n);
void *mem_move(void *dest, const void *src, size_t n);
attr_pure void *mem_chr(const void *src, int c, size_t n);
attr_pure int mem_cmp(const void *p1, const void *p2, size_t n) attr_pure;
void *mem_set(void *s, int c, size_t n);
char *str_dup(const char *s);
attr_pure int str_cmp(const char *s1, const char *s2);
attr_pure char *str_str(const char *h, const char *n);
attr_pure size_t str_len(const char *s) attr_pure;
attr_pure size_t str_nlen(const char *s, size_t m);
attr_pure size_t wstr_len(const wchar_t *s);
attr_pure int str_ncmp(const char *s1, const char *s2, size_t m);
attr_pure int str_icmp(const char *s1, const char *s2);
attr_pure int str_incmp(const char *s1, const char *s2, size_t m);
char *str_cat(char *d, const char *s);
char *str_cpy(char *d, const char *p);
char *str_ncpy(char *d, const char *s, size_t max);
char *str_ncat(char *d, const char *s, size_t max);
#ifdef __cplusplus
}
#endif

void *mem_move(void *dest, const void *src, size_t n) {
    jpr_uint8 *d = dest;
    const jpr_uint8 *s = src;
    if(d == s) return d;
#ifdef JPR_NO_STDLIB
    if((uintptr_t)s-(uintptr_t)d-n <= -2*n) return mem_cpy(d,s,n);
#else
    if((uintptr_t)s-(uintptr_t)d-n <= -2*n) return memcpy(d,s,n);
#endif
    if(d < s) {
        for (; n; n--) *d++ = *s++;
    } else {
        while (n) n--, d[n] = s[n];
    }
    return dest;
}

void *mem_cpy(void *dest, const void *src, size_t n) {
#ifdef _MSC_VER
    void *d = dest;
    __movsb(d,src,n);
#elif defined(__i386__) || defined(__x86_64__)
    void *d = dest;
    __asm__ __volatile("cld ; rep ; movsb" : "=D"(d), "=S"(src), "=c"(n) : "S"(src), "0"(d), "1"(n) : "memory");
#else
    jpr_uint8 *d;
    const jpr_uint8 *s;
    size_t m = n / sizeof(size_t);

    size_t *ld = (size_t  *)dest;
    const size_t *ls = (const size_t  *)src;

    while(m) {
        *ld = *ls;
        ld++;
        ls++;
        m--;
        n-=sizeof(size_t);
    }

    d = (jpr_uint8 *)ld;
    s = (const jpr_uint8 *)ls;

    while(n--) {
        *d = *s;
        d++;
        s++;
    }
#endif
    return dest;
}

void *mem_chr(const void *src, int c, size_t n) {
    const jpr_uint8 *s = (const jpr_uint8 *)src;
    while(n && *s != (jpr_uint8)c) {
        s++;
        n--;
    }
    return n ? (void *)s : NULL;
}

int mem_cmp(const void *p1, const void *p2, size_t n) {
    const jpr_uint8 *l;
    const jpr_uint8 *r;

    size_t m = n / sizeof(size_t);

    const size_t *ll = (const size_t  *)p1;
    const size_t *lr = (const size_t  *)p2;

    while(m) {
        if(*ll != *lr) break;
        m--;
        ll++;
        lr++;
        n-=sizeof(size_t);
    }

    l = (const jpr_uint8 *)ll;
    r = (const jpr_uint8 *)lr;

    while(n && *l == *r) {
        n--;
        l++;
        r++;
    }
    return n ? *l - *r : 0;
}

void *mem_set(void *dest, int c, size_t n) {
#ifdef _MSC_VER
    void *d = dest;
    __stosb(d,(jpr_uint8)c,n);
#elif defined(__i386__) || defined(__x86_64__)
    void *d = dest;
    jpr_uint8 t = c;
    __asm__ __volatile("cld ; rep ; stosb" : "=D"(d), "=c"(n) : "a"(t), "0"(d),"1"(n) : "memory");
#else
    jpr_uint8 *d = (jpr_uint8 *)dest;
    while(n--) {
        *d = (jpr_uint8)c;
        d++;
    }
#endif
    return dest;
}

size_t str_len(const char *s) {
    const char *a = s;
    while(*s) s++;
    return s - a;
}

size_t wstr_len(const wchar_t *s) {
    size_t i = 0;
    while(s[i]) {
        i++;
    }
    return i;
}

size_t str_nlen(const char *s, size_t m) {
#ifdef JPR_NO_STDLIB
    const char *p = mem_chr((const jpr_uint8 *)s, 0, m);
#else
    const char *p = memchr((const jpr_uint8 *)s, 0, m);
#endif
    return (p ? (size_t)(p-s) : m);
}

int str_cmp(const char *s1, const char *s2) {
    while(*s1 == *s2 && *s1) {
        s1++;
        s2++;
    }
    return ((jpr_uint8 *)s1)[0] - ((jpr_uint8 *)s2)[0];
}

int str_ncmp(const char *s1, const char *s2, size_t m) {
    const jpr_uint8 *p1 = (const jpr_uint8 *)s1;
    const jpr_uint8 *p2 = (const jpr_uint8 *)s2;
    if(!m--) return 0;
    while(*p1 && *p2 && m && *p1 == *p2) {
        p1++;
        p2++;
        m--;
    }
    return *p1 - *p2;
}

int str_icmp(const char *s1, const char *s2) {
    const jpr_uint8 *p1 = (const jpr_uint8 *)s1;
    const jpr_uint8 *p2 = (const jpr_uint8 *)s2;
#ifdef JPR_NO_STDLIB
    while(*p1 && *p2 && (*p1 == *p2 || char_lower(*p1) == char_lower(*p2))) {
#else
    while(*p1 && *p2 && (*p1 == *p2 || tolower(*p1) == tolower(*p2))) {
#endif
        p1++;
        p2++;
    }
#ifdef JPR_NO_STDLIB
    return char_lower(*p1) - char_lower(*p2);
#else
    return tolower(*p1) - tolower(*p2);
#endif
}

int str_incmp(const char *s1, const char *s2, size_t m) {
    const jpr_uint8 *p1 = (const jpr_uint8 *)s1;
    const jpr_uint8 *p2 = (const jpr_uint8 *)s2;
    if(!m--) return 0;
    while(*p1 && *p2 && m && (*p1 == *p2 || char_lower(*p1) == char_lower(*p2))) {
        p1++;
        p2++;
        m--;
    }
#ifdef JPR_NO_STDLIB
    return char_lower(*p1) - char_lower(*p2);
#else
    return tolower(*p1) - tolower(*p2);
#endif
}

char *str_cat(char *d, const char *s) {
    char *dest = d;
#ifdef JPR_NO_STDLIB
    d += str_len(d);
#else
    d += strlen(d);
#endif
    while((*d = *s) != 0) {
        s++;
        d++;
    }
    return dest;
}

char *str_cpy(char *d, const char *s) {
    char *dest = d;
    while((*d = *s) != 0) {
        s++;
        d++;
    }
    *d = 0;
    return dest;
}

char *str_ncpy(char *d, const char *s, size_t max) {
    char *dest = d;
    while(max && (*d = *s) != 0) {
        s++;
        d++;
        max--;
    }
    return dest;
}

char *str_ncat(char *d, const char *s, size_t max) {
    char *dest = d;
#ifdef JPR_NO_STDLIB
    d += str_len(d);
#else
    d += strlen(d);
#endif
    while(max && (*d = *s) != 0) {
        s++;
        d++;
        max--;
    }
    return dest;
}

size_t str_nlower(char *dest, const char *src, size_t max) {
    char *d = dest;
    while(*src && max) {
#ifdef JPR_NO_STDLIB
        *d++ = char_lower(*src++);
#else
        *d++ = tolower(*src++);
#endif
        max--;
    }
    return d - dest;
}

size_t str_lower(char *dest, const char *src) {
    char *d = dest;
    while(*src) {
#ifdef JPR_NO_STDLIB
        *d++ = char_lower(*src++);
#else
        *d++ = tolower(*src++);
#endif
    }
    *d = 0;
    return d - dest;
}

char *str_rchr(const char *s, char c) {
    size_t len;
    size_t n;
#ifdef JPR_NO_STDLIB
    len = str_len(s);
#else
    len = strlen(s);
#endif
    n = len;
    while(n--) {
        if(s[n] == c) return (char *)&s[n];
    }
    return NULL;
}

char *str_nrchr(const char *s, char c, size_t n) {
    while(n--) {
        if(s[n] == c) return (char *)&s[n];
    }
    return NULL;
}

char *str_nchr(const char *s, char c, size_t n) {
    while(n--) {
        if(*s == c) return (char *)s;
        s++;
    }
    return NULL;
}

char *str_chr(const char *s, char c) {
#ifdef JPR_NO_STDLIB
    return mem_chr((const jpr_uint8 *)s,c,str_len(s));
#else
    return memchr((const jpr_uint8 *)s,c,str_len(s));
#endif
}

size_t str_necat(char *dest, const char *s, size_t max, const char *e, char t) {
    const char *f;
    char *d;
    const char *q;
    char *p;

    f = s + max;
    d = dest;
#ifdef JPR_NO_STDLIB
    if(dest != NULL) d += str_len(dest);
#else
    if(dest != NULL) d += strlen(dest);
#endif
    q = e;
    p = d;

    while(*s && s < f) {
        q = e;
        while(*q) {
            if(*q == *s) {
                if(dest != NULL) *p = t;
                p++;
            }
            q++;
        }
        if(dest != NULL) *p = *s;
        s++;
        p++;
    }
    if(dest != NULL) *p = 0;
    return p - d;
}

size_t str_ecat(char *d, const char *s, const char *e, char t) {
    return str_necat(d,s,str_len(s),e,t);
}

size_t str_ends(const char *s, const char *q) {
    size_t slen = str_len(s);
    size_t qlen = str_len(q);
    if(slen < qlen) return 0;
#ifdef JPR_NO_STDLIB
    return str_cmp(&s[slen - qlen],q) == 0;
#else
    return strcmp(&s[slen - qlen],q) == 0;
#endif
}

size_t str_iends(const char *s, const char *q) {
    size_t slen = str_len(s);
    size_t qlen = str_len(q);
    if(slen < qlen) return 0;
#ifdef JPR_NO_STDLIB
    return str_icmp(&s[slen - qlen],q) == 0;
#else
#ifdef _MSC_VER
    return _stricmp(&s[slen - qlen],q) == 0;
#else
    return strcasecmp(&s[slen - qlen],q) == 0;
#endif
#endif
}

char *str_str(const char *h, const char *n) {
    size_t nlen;
    const char *hp = h;
#ifdef JPR_NO_STDLIB
    nlen = str_len(n);
#else
    nlen = strlen(n);
#endif
    if(nlen == 0) return (char *)h;;
#ifdef JPR_NO_STDLIB
    while(*hp && (str_len(hp) >= nlen)) {
        if(str_ncmp(hp,n,nlen) == 0) return (char *)hp;
#else
    while(*hp && (strlen(hp) >= nlen)) {
        if(strncmp(hp,n,nlen) == 0) return (char *)hp;
#endif
        hp++;
    }
    return NULL;
}

char *str_dup(const char *s) {
#ifdef JPR_NO_STDLIB
    size_t len = str_len(s);
    char *t = malloc(len+1);
#else
    size_t len = strlen(s);
    char *t = malloc(len+1);
#endif
    if(t == NULL) return t;
#ifdef JPR_NO_STDLIB
    str_cpy(t,s);
#else
    strcpy(t,s);
#endif
    return t;
}


#ifdef JPR_ALIAS_STDLIB
#if defined(__APPLE__) || ( defined(_WIN32) && !defined(_MSC_VER) && !defined(_WIN64))
#define MAKE_ALIAS(x,y) \
__asm__(".globl _"  #y); \
__asm__(".set _" #y ", _"  #x);
#elif !defined(_MSC_VER)
#define MAKE_ALIAS(x,y) \
__asm__(".globl " #y); \
__asm__(".set " #y  ", " #x);
#else
#define MAKE_ALIAS(x,y)
#endif

MAKE_ALIAS(str_dup,strdup)
MAKE_ALIAS(str_cmp,strcmp)
MAKE_ALIAS(str_str,strstr)
MAKE_ALIAS(str_nlen,strnlen)
MAKE_ALIAS(str_len,strlen)
MAKE_ALIAS(wstr_len,wcslen)
MAKE_ALIAS(str_ncmp,strncmp)
MAKE_ALIAS(str_icmp,strcasecmp)
MAKE_ALIAS(str_incmp,strncasecmp)
MAKE_ALIAS(str_cat,strcat)
MAKE_ALIAS(str_cpy,strcpy)
MAKE_ALIAS(str_ncpy,strncpy)
MAKE_ALIAS(str_ncat,strncat)
MAKE_ALIAS(str_chr,strchr)
MAKE_ALIAS(str_rchr,strrchr)
MAKE_ALIAS(mem_cpy,memcpy)
MAKE_ALIAS(mem_move,memmove)
MAKE_ALIAS(mem_chr,memchr)
MAKE_ALIAS(mem_cmp,memcmp)
MAKE_ALIAS(mem_set,memset)

#if defined(_MSC_VER)
/* no good way to make an alias with MSVC
 * use wrapper functions instead */
#ifdef __cplusplus
extern "C" {
#endif
char *strdup(const char *s);
int strcmp(const char *s1, const char *s2);
char *strstr(const char *h, const char *n);
size_t strnlen(const char *s, size_t n);
size_t strlen(const char *s);
size_t wcslen(const wchar_t *s);
int strncmp(const char *s1, const char *s2, size_t n);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
char *strcat(char *s1, const char *s2);
char *strcpy(char *s1, const char *s2);
char *strncpy(char *s1, const char *s2, size_t n);
char *strncat(char *s1, const char *s2, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memchr(const void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memset(void *b, int c, size_t n);

#ifdef __cplusplus
}
#endif

char *strdup(const char *s) { return str_dup(s); }
int strcmp(const char *s1, const char *s2) { return str_cmp(s1, s2); }
char *strstr(const char *h, const char *n) { return str_str(h,n); }
size_t strnlen(const char *s, size_t n) { return str_nlen(s,n); }
size_t strlen(const char *s) { return str_len(s); }
size_t wcslen(const wchar_t *s) { return wstr_len(s); }
int strncmp(const char *s1, const char *s2, size_t n) { return str_ncmp(s1,s2,n); }
int strcasecmp(const char *s1, const char *s2) { return str_icmp(s1,s2); }
int strncasecmp(const char *s1, const char *s2, size_t n) { return str_incmp(s1,s2,n); }
char *strcat(char *s1, const char *s2) { return str_cat(s1,s2); }
char *strcpy(char *s1, const char *s2) { return str_cpy(s1,s2); }
char *strncpy(char *s1, const char *s2, size_t n) { return str_ncpy(s1,s2,n); }
char *strncat(char *s1, const char *s2, size_t n) { return str_ncat(s1,s2,n); }
char *strchr(const char *s, int c) { return str_chr(s,c); }
char *strrchr(const char *s, int c) { return str_rchr(s,c); }
void *memcpy(void *dst, const void *src, size_t n) { return mem_cpy(dst,src,n); }
void *memmove(void *dst, const void *src, size_t n) { return mem_move(dst,src,n); }
void *memchr(const void *s, int c, size_t n) { return mem_chr(s,c,n); }
int memcmp(const void *s1, const void *s2, size_t n) { return mem_cmp(s1,s2,n); }
void *memset(void *b, int c, size_t n) { return mem_set(b,c,n); }

#endif

#endif

