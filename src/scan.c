#include "scan.h"

unsigned int scan_uint(const char *str, unsigned int *num) {
    const char *s = str;
    *num = 0;
    while(*s) {
        if(*s < 48 || *s > 57) break;
        *num *= 10;
        *num += (*s - 48);
        s++;
    }

    return s - str;
}

unsigned int scan_int(const char *str, int *num) {
    int mult = 1;
    unsigned int r = 0;
    unsigned int t;
    if(str[0] == '-') {
        mult = -1;
        r = 1;
    }
    r += scan_uint(str + r, &t);
    *num = t * mult;
    return r;
}

