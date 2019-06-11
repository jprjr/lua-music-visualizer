#include "str.h"
#include "char.h"
#include <stddef.h>

unsigned int str_nlower(char *dest, const char *src, unsigned int max) {
    char *d = dest;
    unsigned int n = 0;
    while(*src && n < max) {
        *d++ = char_lower(*src++);
        n++;
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
    char *t = strchr(s,c);
    if(t == NULL) return str_len(s);
    return t - s;
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

#ifdef _WIN32
unsigned int str_ncpy(char *d, const char *s, unsigned int max) {
    strncpy(d,s,max);
    unsigned int t = str_len(s);
    if(t < max) return t;
    return max;
}
#endif

unsigned int str_ncat(char *d,const char *s,unsigned int max) {
    unsigned int len = str_len(d);
    return str_len(strncat(d,s,max)) - len;
}

