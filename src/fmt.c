#include "fmt.h"
#include <stddef.h>

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

static const char numtab[11] = "0123456789";

unsigned int fmt_uint(char *d, unsigned int num) {
    unsigned int n = 1;
    unsigned int r;
    unsigned int val = num;

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

unsigned int fmt_int(char *d, int num) {
    unsigned int t = 0;
    unsigned int r = 0;
    if(num < 0) {
        r = 1;
        t = num * -1;
        num = 0;
        if(d != NULL) *d++ = '-';
    }
    t += num;
    r += fmt_uint(d,t);
    return r;
}

