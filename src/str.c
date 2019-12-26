#include "norm.h"
#include "str.h"
#include "char.h"
#include "mem.h"
#include <stdint.h>
#include <stddef.h>

#ifndef JPR_NO_STDLIB
#include <string.h>
#include <strings.h>
#include <wchar.h>
#endif

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

void *mem_move(void *dest, const void *src, unsigned int n) {
#ifdef JPR_NO_STDLIB
    uint8_t *d = dest;
    const uint8_t *s = src;
    if(d == s) return d;
    if((uintptr_t)s-(uintptr_t)d-n <= -2*n) return mem_cpy(d,s,n);
    if(d < s) {
        for (; n; n--) *d++ = *s++;
    } else {
        while (n) n--, d[n] = s[n];
    }
    return dest;
#else
    return memmove(dest,src,n);
#endif
}

void *mem_cpy(void *dest, const void *src, unsigned int n) {
#ifdef JPR_NO_STDLIB
    uint8_t *d;
    const uint8_t *s;
    unsigned int m = n / sizeof(size_t);

    size_t *ld = (size_t  *)dest;
    const size_t *ls = (const size_t  *)src;

    while(m) {
        *ld = *ls;
        ld++;
        ls++;
        m--;
        n-=sizeof(size_t);
    }

    d = (uint8_t *)ld;
    s = (const uint8_t *)ls;

    while(n--) {
        *d = *s;
        d++;
        s++;
    }
    return dest;
#else
    return memcpy(dest,src,n);
#endif
}

void *mem_chr(const void *src, uint8_t c, unsigned int n) {
#ifdef JPR_NO_STDLIB
    const uint8_t *s = (const uint8_t *)src;
    while(n && *s != c) {
        s++;
        n--;
    }
    return n ? (void *)s : NULL;
#else
    return memchr(src,c,n);
#endif
}

int mem_cmp(const void *p1, const void *p2, unsigned int n) {
#ifdef JPR_NO_STDLIB
    const uint8_t *l;
    const uint8_t *r;

    unsigned int m = n / sizeof(size_t);

    const size_t *ll = (const size_t  *)p1;
    const size_t *lr = (const size_t  *)p2;

    while(m) {
        if(*ll != *lr) break;
        m--;
        ll++;
        lr++;
        n-=sizeof(size_t);
    }

    l = (const uint8_t *)ll;
    r = (const uint8_t *)lr;

    while(n && *l == *r) {
        n--;
        l++;
        r++;
    }
    return n ? *l - *r : 0;
#else
    return memcmp(p1,p2,n);
#endif
}

void *mem_set(void *dest, uint8_t c, unsigned int n) {
#ifdef JPR_NO_STDLIB
    uint8_t *d = (uint8_t *)dest;
    while(n--) {
        *d = c;
        d++;
    }
    return dest;
#else
    return memset(dest,c,n);
#endif
}

unsigned int str_len(const char *s) {
#ifdef JPR_NO_STDLIB
    const char *a = s;
    while(*s) s++;
    return s - a;
#else
    return strlen(s);
#endif
}

unsigned int wstr_len(const wchar_t *s) {
#ifdef JPR_NO_STDLIB
    unsigned int i = 0;
    while(s[i]) {
        i++;
    }
    return i;
#else
    return wcslen(s);
#endif
}

unsigned int str_nlen(const char *s, unsigned int m) {
#ifdef JPR_NO_STDLIB
    const char *p = mem_chr((const uint8_t *)s, 0, m);
    return (p ? (unsigned int)(p-s) : m);
#else
    return strnlen(s,m);
#endif
}

int str_cmp(const char *s1, const char *s2) {
#ifdef JPR_NO_STDLIB
    while(*s1 == *s2 && *s1) {
        s1++;
        s2++;
    }
    return ((uint8_t *)s1)[0] - ((uint8_t *)s2)[0];
#else
    return strcmp(s1,s2);
#endif
}

int str_ncmp(const char *s1, const char *s2, unsigned int m) {
#ifdef JPR_NO_STDLIB
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    if(!m--) return 0;
    while(*p1 && *p2 && m && *p1 == *p2) {
        p1++;
        p2++;
        m--;
    }
    return *p1 - *p2;
#else
    return strncmp(s1,s2,m);
#endif
}

int str_icmp(const char *s1, const char *s2) {
#ifdef JPR_NO_STDLIB
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    while(*p1 && *p2 && (*p1 == *p2 || char_lower(*p1) == char_lower(*p2))) {
        p1++;
        p2++;
    }
    return char_lower(*p1) - char_lower(*p2);
#else
    return strcasecmp(s1,s2);
#endif
}

int str_incmp(const char *s1, const char *s2, unsigned int m) {
#ifdef JPR_NO_STDLIB
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    if(!m--) return 0;
    while(*p1 && *p2 && m && (*p1 == *p2 || char_lower(*p1) == char_lower(*p2))) {
        p1++;
        p2++;
        m--;
    }
    return char_lower(*p1) - char_lower(*p2);
#else
    return strncasecmp(s1,s2,m);
#endif
}

unsigned int str_cat(char *d, const char *s) {
#ifdef JPR_NO_STDLIB
    const char *src = s;
    d += str_len(d);
    while((*d = *s) != 0) {
        s++;
        d++;
    }
    return s - src;
#else
    strcat(d,s);
    return strlen(s);
#endif
}


unsigned int str_cpy(char *d, const char *s) {
#ifdef JPR_NO_STDLIB
    const char *src = s;
    while((*d = *s) != 0) {
        s++;
        d++;
    }
    *d = 0;
    return s - src;
#else
#ifdef JPR_WINDOWS
    strcpy(d,s);
    return str_len(s);
#else
    return stpcpy(d,s) - d;
#endif
#endif
}

unsigned int str_ncpy(char *d, const char *s, unsigned int max) {
#ifdef JPR_NO_STDLIB
    const char *src = s;
    while(max && (*d = *s) != 0) {
        s++;
        d++;
        max--;
    }
    return s - src;
#else
#ifdef JPR_WINDOWS
    strncpy(d,s,max);
    return str_nlen(s,max);
#else
    return stpncpy(d,s,max) - d;
#endif
#endif
}

unsigned int str_ncat(char *d, const char *s, unsigned int max) {
#ifdef JPR_NO_STDLIB
    const char *src = s;
    d += str_len(d);
    while(max && (*d = *s) != 0) {
        s++;
        d++;
        max--;
    }
    return s - src;
#else
    strncat(d,s,max);
    return str_nlen(s,max);
#endif
}

unsigned int str_nlower(char *dest, const char *src, unsigned int max) {
    char *d = dest;
    while(*src && max) {
        *d++ = char_lower(*src++);
        max--;
    }
    return d - dest;
}

unsigned int str_lower(char *dest, const char *src) {
    char *d = dest;
    while(*src) {
        *d++ = char_lower(*src++);
    }
    *d = 0;
    return d - dest;
}

unsigned int str_rchr(const char *s, char c) {
#ifdef JPR_NO_STDLIB
    unsigned int len;
    unsigned int n;
    len = str_len(s);
    n = len;
    while(n--) {
        if(s[n] == c) return n;
    }
    return len;
#else
    char *t = strrchr(s,c);
    return ( t != NULL ? (unsigned int)(t - s) : str_len(s) );
#endif
}

unsigned int str_nrchr(const char *s, char c, unsigned int n) {
    unsigned int len = n;
    while(n--) {
        if(s[n] == c) return n;
    }
    return len;
}

unsigned int str_chr(const char *s, char c) {
#ifdef JPR_NO_STDLIB
    char *t = mem_chr((const uint8_t *)s,c,str_len(s));
    if(t == NULL) return str_len(s);
    return t - s;
#else
    char *t = strchr(s,c);
    return ( t != NULL ? (unsigned int)(t - s) : str_len(s) );
#endif
}

unsigned int str_necat(char *dest, const char *s, unsigned int max, const char *e, char t) {
    const char *f = s + max;
    char *d = dest;
    if(dest != NULL) d += str_len(dest);
    const char *q = e;
    char *p = d;
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

unsigned int str_ecat(char *d, const char *s, const char *e, char t) {
    return str_necat(d,s,str_len(s),e,t);
}


unsigned int str_ends(const char *s, const char *q) {
    unsigned int slen = str_len(s);
    unsigned int qlen = str_len(q);
    if(slen < qlen) return 0;
    return str_cmp(&s[slen - qlen],q) == 0;
}

unsigned int str_iends(const char *s, const char *q) {
    unsigned int slen = str_len(s);
    unsigned int qlen = str_len(q);
    if(slen < qlen) return 0;
    return str_icmp(&s[slen - qlen],q) == 0;
}

unsigned int str_str(const char *h, const char *n) {
#ifdef JPR_NO_STDLIB
    unsigned int nlen;
    const char *hp = h;
    nlen = str_len(n);
    if(nlen == 0) return 0;
    while(*hp && (str_len(hp) >= nlen)) {
        if(str_ncmp(hp,n,nlen) == 0) return (unsigned int)(hp - h);
        hp++;
    }
    return str_len(h);
#else
    char *r = strstr(h,n);
    return ( r == NULL ? str_len(h) : (unsigned int)(r - h));
#endif
}

char *str_dup(const char *s) {
    unsigned int len = str_len(s);
    char *t = mem_alloc(len+1);
    if(t == NULL) return t;
    str_cpy(t,s);
    return t;
}
