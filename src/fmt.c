#include "fmt.h"
#include <stddef.h>

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

static const char numtab[11] = "0123456789";

size_t fmt_uint(char *d, jpr_uint64 num) {
    size_t n = 1;
    size_t r;
    jpr_uint64 val = num;

    while( (val /= 10) > 0) {
        n++;
    }

    if(d == NULL) return n;

    r = n;
    while(n-- > 0) {
        d[n] = numtab[num % 10];
        num /= 10;
    }

    return r;
}

size_t fmt_int(char *d, jpr_int64 num) {
    jpr_uint64 t;
    jpr_int64 mask = num >> 63;
    size_t r = 0;
    if(num < 0) {
        r++;
        if(d != NULL) *d++ = '-';
    }
    t = (num ^ mask) - mask;
    r += fmt_uint(d,t);
    return r;
}

