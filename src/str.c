#include "str.h"
#include "char.h"

#define STR_MAX(a,b) ( a > b ? a : b)
#define STR_MIN(a,b) ( a < b ? a : b)

unsigned int str_nlower(char *dest, const char *str, unsigned int max) {
    char *d = dest;
    unsigned int n = 0;
    while(*str && n < max) {
        *d++ = char_lower(*str++);
        n++;
    }
    return d - dest;
}

unsigned int str_lower(char *dest, const char *str) {
    return str_nlower(dest,str,str_len(str));
}


unsigned int str_chr(const char *s, char c) {
    const unsigned char *p = (const unsigned char *)s;
    unsigned char d = c;

    do {
        if(*p == d) break;
    } while(*p++ != 0);

    return p - (const unsigned char *)s;
}

unsigned int str_ncpy(char *dest, const char *src, unsigned int max) {
    char *d = dest;
    const char *s = src;
    unsigned int n = 0;
    while(*s && n++ < max) {
        *d++ = *s++;
    }
    *d = 0;
    return d - dest;
}

unsigned int str_len(const char *s) {
    int n = 0;
    while(*s++ != 0) {
        n++;
    }
    return n;
}

unsigned int str_cpy(char *dest, const char *src) {
    return str_ncpy(dest,src,str_len(src));
}

int str_ncmp(const char *s, const char *q, unsigned int max) {
    unsigned int n = 0;
    const unsigned char *t = (const unsigned char *)s;
    const unsigned char *r = (const unsigned char *)q;
    while(t[n] && r[n] && n < max) {
        if(t[n] != r[n]) return t[n] - r[n];
        n++;
    }
    return (n == max ? 0 : t[n]-r[n]);
}

int str_incmp(const char *s, const char *q, unsigned int max) {
    unsigned int n = 0;
    unsigned char sc; unsigned char qc;
    const unsigned char *t = (const unsigned char *)s;
    const unsigned char *r = (const unsigned char *)q;
    while(t[n] && r[n] && n < max) {
        sc = (unsigned char)char_lower(t[n]);
        qc = (unsigned char)char_lower(r[n]);
        if(sc != qc) return sc - qc;
        n++;
    }
    return (n == max ? 0 : char_lower(t[n])-char_lower(r[n]));
}

int str_cmp(const char *s, const char *q) {
    return str_ncmp(s,q,STR_MAX(str_len(s),str_len(q)));
}

int str_icmp(const char *s, const char *q) {
    return str_incmp(s,q,STR_MAX(str_len(s),str_len(q)));
}

unsigned int str_starts(const char *s, const char *q) {
    return str_ncmp(s,q,str_len(q)) == 0;
}

unsigned int str_istarts(const char *s, const char *q) {
    return str_incmp(s,q,str_len(q)) == 0;
}

unsigned int str_ends(const char *s, const char *q) {
    int sn = str_len(s);
    int qn = str_len(q);
    if(qn > sn) return 0;

    return str_ncmp(s + sn - qn,q,qn) == 0;
}

unsigned int str_iends(const char *s, const char *q) {
    int sn = str_len(s);
    int qn = str_len(q);
    if(qn > sn) return 0;

    return str_incmp(s + sn - qn,q,qn) == 0;
}

unsigned int str_ncat(char *dest, const char *s, unsigned int max) {
    const char *f = s + max;
    char *d = dest + str_len(dest);
    char *p = d;
    while(*s && s < f) {
        *p++ = *s++;
    }
    *p = 0;
    return p - d;
}

unsigned int str_cat(char *dest, const char *s) {
    return str_ncat(dest,s,str_len(s));
}

unsigned int str_necat(char *dest, const char *s, unsigned int max, const char *e, char t) {
    const char *f = s + max;
    char *d = dest + str_len(dest);
    const char *q = e;
    char *p = d;
    while(*s && s < f) {
        q = e;
        while(*q) {
            if(*q == *s) *p++ = t;
            q++;
        }
        *p++ = *s++;
    }
    *p = 0;
    return p - d;
}

unsigned int str_ecat(char *d, const char *s, const char *e, char t) {
    return str_necat(d,s,str_len(s),e,t);
}
