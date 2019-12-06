#include "str.h"
#include "char.h"
#include <stdint.h>

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifdef NO_STDLIB
void *mem_cpy(uint8_t *dest, const uint8_t *src, unsigned int n) {
    uint8_t *d = dest;
    while(n--) {
        *d = *src;
        d++;
        src++;
    }
    return dest;
}

void *mem_chr(const uint8_t *src, uint8_t c, unsigned int n) {
    const uint8_t *s = src;
    while(n && *s != c) {
        s++;
        n--;
    }
    return n ? (void *)s : (void *)0;
}

int mem_cmp(const uint8_t *p1, const uint8_t *p2, unsigned int n) {
    const uint8_t *l = p1;
    const uint8_t *r = p2;
    while(n && *l == *r) {
        n--;
        l++;
        r++;
    }
    return n ? *l-*r : 0;
}

unsigned int str_len(const char *s) {
    const char *a = s;
    while(*s) s++;
    return s - a;
}

unsigned int str_nlen(const char *s, unsigned int m) {
    const char *p = mem_chr((const uint8_t *)s, 0, m);
    return (p ? (unsigned int)(p-s) : m);
}

int str_cmp(const char *s1, const char *s2) {
    while(*s1 == *s2 && *s1) {
        s1++;
        s2++;
    }
    return ((uint8_t *)s1)[0] - ((uint8_t *)s2)[0];
}

int str_ncmp(const char *s1, const char *s2, unsigned int m) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    if(!m--) return 0;
    while(*p1 && *p2 && m && *p1 == *p2) {
        p1++;
        p2++;
        m--;
    }
    return *p1 - *p2;
}

int str_icmp(const char *s1, const char *s2) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    while(*p1 && *p2 && (*p1 == *p2 || char_lower(*p1) == char_lower(*p2))) {
        p1++;
        p2++;
    }
    return char_lower(*p1) - char_lower(*p2);
}

int str_incmp(const char *s1, const char *s2, unsigned int m) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    if(!m--) return 0;
    while(*p1 && *p2 && m && (*p1 == *p2 || char_lower(*p1) == char_lower(*p2))) {
        p1++;
        p2++;
        m--;
    }
    return char_lower(*p1) - char_lower(*p2);
}

unsigned int str_cat(char *d, const char *s) {
    const char *src = s;
    d += str_len(d);
    while((*d = *s) != 0) {
        s++;
        d++;
    }
    return s - src;
}

#endif

#if defined(NO_STDLIB) || defined(_WIN32)
unsigned int str_cpy(char *d, const char *s) {
    const char *src = s;
    while((*d = *s) != 0) {
        s++;
        d++;
    }
    return s - src;
}

unsigned int str_ncpy(char *d, const char *s, unsigned int max) {
    const char *src = s;
    while(max && (*d = *s) != 0) {
        s++;
        d++;
        max--;
    }
    return s - src;
}
#endif

unsigned int str_ncat(char *d, const char *s, unsigned int max) {
    const char *src = s;
    d += str_len(d);
    while(max && (*d = *s) != 0) {
        s++;
        d++;
        max--;
    }
    return s - src;
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
    return d - dest;
}


unsigned int str_chr(const char *s, char c) {
#ifdef NO_STDLIB
    char *t = mem_chr((const uint8_t *)s,c,str_len(s));
#else
    char *t = strchr(s,c);
#endif
    if(t == (void *)0) return str_len(s);
    return t - s;
}

unsigned int str_necat(char *dest, const char *s, unsigned int max, const char *e, char t) {
    const char *f = s + max;
    char *d = dest;
    if(dest != (void *)0) d += str_len(dest);
    const char *q = e;
    char *p = d;
    while(*s && s < f) {
        q = e;
        while(*q) {
            if(*q == *s) {
                if(dest != (void *)0) *p = t;
                p++;
            }
            q++;
        }
        if(dest != (void *)0) *p = *s;
        s++;
        p++;
    }
    if(dest != (void *)0) *p = 0;
    return p - d;
}

unsigned int str_ecat(char *d, const char *s, const char *e, char t) {
    return str_necat(d,s,str_len(s),e,t);
}


