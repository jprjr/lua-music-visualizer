#include "norm.h"
#include "str.h"
#include "char.h"
#include "mem.h"
#include "int.h"

#ifndef JPR_NO_STDLIB
#include <string.h>
#if __STDC_VERSION__ >= 199901L
#include <strings.h>
#endif
#include <wchar.h>
#endif

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifdef JPR_NO_STDLIB
void *mem_move(void *dest, const void *src, size_t n) {
    jpr_uint8 *d = dest;
    const jpr_uint8 *s = src;
    if(d == s) return d;
    if((uintptr_t)s-(uintptr_t)d-n <= -2*n) return mem_cpy(d,s,n);
    if(d < s) {
        for (; n; n--) *d++ = *s++;
    } else {
        while (n) n--, d[n] = s[n];
    }
    return dest;
}
#endif

#ifdef JPR_NO_STDLIB
void *mem_cpy(void *dest, const void *src, size_t n) {
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
    return dest;
}
#endif

#ifdef JPR_NO_STDLIB
void *mem_chr(const void *src, jpr_uint8 c, size_t n) {
    const jpr_uint8 *s = (const jpr_uint8 *)src;
    while(n && *s != c) {
        s++;
        n--;
    }
    return n ? (void *)s : NULL;
}
#endif

#ifdef JPR_NO_STDLIB
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
#endif

#ifdef JPR_NO_STDLIB
void *mem_set(void *dest, jpr_uint8 c, size_t n) {
    jpr_uint8 *d = (jpr_uint8 *)dest;
    while(n--) {
        *d = c;
        d++;
    }
    return dest;
}
#endif

#ifdef JPR_NO_STDLIB
size_t str_len(const char *s) {
    const char *a = s;
    while(*s) s++;
    return s - a;
}
#endif

#ifdef JPR_NO_STDLIB
size_t wstr_len(const wchar_t *s) {
    size_t i = 0;
    while(s[i]) {
        i++;
    }
    return i;
}
#endif

#ifdef JPR_NO_STDLIB
size_t str_nlen(const char *s, size_t m) {
    const char *p = mem_chr((const jpr_uint8 *)s, 0, m);
    return (p ? (size_t)(p-s) : m);
}
#endif

#ifdef JPR_NO_STDLIB
int str_cmp(const char *s1, const char *s2) {
    while(*s1 == *s2 && *s1) {
        s1++;
        s2++;
    }
    return ((jpr_uint8 *)s1)[0] - ((jpr_uint8 *)s2)[0];
}
#endif

#ifdef JPR_NO_STDLIB
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
#endif

#ifdef JPR_NO_STDLIB
int str_icmp(const char *s1, const char *s2) {
    const jpr_uint8 *p1 = (const jpr_uint8 *)s1;
    const jpr_uint8 *p2 = (const jpr_uint8 *)s2;
    while(*p1 && *p2 && (*p1 == *p2 || char_lower(*p1) == char_lower(*p2))) {
        p1++;
        p2++;
    }
    return char_lower(*p1) - char_lower(*p2);
}
#endif

#ifdef JPR_NO_STDLIB
int str_incmp(const char *s1, const char *s2, size_t m) {
    const jpr_uint8 *p1 = (const jpr_uint8 *)s1;
    const jpr_uint8 *p2 = (const jpr_uint8 *)s2;
    if(!m--) return 0;
    while(*p1 && *p2 && m && (*p1 == *p2 || char_lower(*p1) == char_lower(*p2))) {
        p1++;
        p2++;
        m--;
    }
    return char_lower(*p1) - char_lower(*p2);
}
#endif

#ifdef JPR_NO_STDLIB
char *str_cat(char *d, const char *s) {
    char *dest = d;
    d += str_len(d);
    while((*d = *s) != 0) {
        s++;
        d++;
    }
    return dest;
}
#endif


#ifdef JPR_NO_STDLIB
char *str_cpy(char *d, const char *s) {
    char *dest = d;
    while((*d = *s) != 0) {
        s++;
        d++;
    }
    *d = 0;
    return dest;
}
#endif

#ifdef JPR_NO_STDLIB
char *str_ncpy(char *d, const char *s, size_t max) {
    char *dest = d;
    while(max && (*d = *s) != 0) {
        s++;
        d++;
        max--;
    }
    return dest;
}
#endif

#ifdef JPR_NO_STDLIB
char *str_ncat(char *d, const char *s, size_t max) {
    char *dest = d;
    d += str_len(d);
    while(max && (*d = *s) != 0) {
        s++;
        d++;
        max--;
    }
    return dest;
}
#endif

size_t str_nlower(char *dest, const char *src, size_t max) {
    char *d = dest;
    while(*src && max) {
        *d++ = char_lower(*src++);
        max--;
    }
    return d - dest;
}

size_t str_lower(char *dest, const char *src) {
    char *d = dest;
    while(*src) {
        *d++ = char_lower(*src++);
    }
    *d = 0;
    return d - dest;
}

#ifdef JPR_NO_STDLIB
char *str_rchr(const char *s, char c) {
    size_t len;
    size_t n;
    len = str_len(s);
    n = len;
    while(n--) {
        if(s[n] == c) return (char *)&s[n];
    }
    return NULL;
}
#endif

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

#ifdef JPR_NO_STDLIB
char *str_chr(const char *s, char c) {
    return mem_chr((const jpr_uint8 *)s,c,str_len(s));
}
#endif

size_t str_necat(char *dest, const char *s, size_t max, const char *e, char t) {
    const char *f;
    char *d;
    const char *q;
    char *p;

    f = s + max;
    d = dest;
    if(dest != NULL) d += str_len(dest);
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
    return str_cmp(&s[slen - qlen],q) == 0;
}

size_t str_iends(const char *s, const char *q) {
    size_t slen = str_len(s);
    size_t qlen = str_len(q);
    if(slen < qlen) return 0;
    return str_icmp(&s[slen - qlen],q) == 0;
}

#ifdef JPR_NO_STDLIB
char *str_str(const char *h, const char *n) {
    size_t nlen;
    const char *hp = h;
    nlen = str_len(n);
    if(nlen == 0) return (char *)h;;
    while(*hp && (str_len(hp) >= nlen)) {
        if(str_ncmp(hp,n,nlen) == 0) return (char *)hp;
        hp++;
    }
    return NULL;
}
#endif

#ifdef JPR_NO_STDLIB
char *str_dup(const char *s) {
    size_t len = str_len(s);
    char *t = mem_alloc(len+1);
    if(t == NULL) return t;
    str_cpy(t,s);
    return t;
}
#endif
