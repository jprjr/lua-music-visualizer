#include "scan.h"

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

size_t scan_uint(const char *str, jpr_uint64 *num) {
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

size_t scan_int(const char *str, jpr_int64 *num) {
    size_t r = 0;
    jpr_int64 mult = 1;
    jpr_uint64 t;

    if(str[0] == '-') {
        mult = -1;
        r = 1;
    }
    r += scan_uint(str + r, &t);
    *num = (jpr_int64)(t * mult);
    return r;
}

