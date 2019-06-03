#include "str.h"
#include <stdio.h>

#define STR_MAX(a,b) ( a > b ? a : b)
#define STR_MIN(a,b) ( a < b ? a : b)

unsigned int str_strlower(char *str) {
    char *s = str;
    while(*s) {
        *s = str_lower(*s);
        s++;
    }
    return s - str;
}


unsigned int str_chr(const char *s, char c) {
    const char *p = s;

    do {
        if(*p == c) break;
    } while(*p++ != 0);

    return p - s;
}

unsigned int str_ncpy(char *dest, const char *src, unsigned int max) {
    char *d = dest;
    const char *s = src;
    unsigned int n = 0;
    while(*s && n++ < max) {
        *d++ = *s++;
    }
    *d = 0;
    return n;
}

#if 0
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
    while(s[n] && q[n] && n < max) {
        if(s[n] > q[n]) return 1;
        if(q[n] > s[n]) return -1;
        n++;
    }
    return 0;
}

int str_incmp(const char *s, const char *q, unsigned int max) {
    unsigned int n = 0;
    char sc; char qc;
    while(s[n] && q[n] && n < max) {
        sc = str_lower(s[n]);
        qc = str_lower(s[n]);
        if(sc > qc) return 1;
        if(qc > sc) return -1;
        n++;
    }
    return 0;
}

int str_cmp(const char *s, const char *q) {
    return str_ncmp(s,q,STR_MIN(str_len(s),str_len(q)));
}

int str_icmp(const char *s, const char *q) {
    return str_incmp(s,q,STR_MIN(str_len(s),str_len(q)));
}
#endif

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

unsigned int str_cat(char *d, const char *s) {
    char *p = d + str_len(d);
    while(*s) {
        *p++ = *s++;
    }
    *p = 0;
    return p - d;
}

unsigned int str_cat_escape(char *d, const char *s) {
    char *p = d + str_len(d);
    while(*s) {
        if(*s == '\\') {
            *p++ = '\\';
        }
        *p++ = *s++;
    }
    *p = 0;
    return p - d;
}
