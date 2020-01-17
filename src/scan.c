#include "attr.h"
#include "scan.h"
#include "char.h"

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

size_t scan_uint_base16(const char *str, jpr_uint64 *num) {
    const char *s = str;
    jpr_uint8 t;
    *num = 0;
    while(*s) {
        switch(char_upper(*s)) {
            case '0': t = 0; break;
            case '1': t = 1; break;
            case '2': t = 2; break;
            case '3': t = 3; break;
            case '4': t = 4; break;
            case '5': t = 5; break;
            case '6': t = 6; break;
            case '7': t = 7; break;
            case '8': t = 8; break;
            case '9': t = 9; break;
            case 'A': t = 10; break;
            case 'B': t = 11; break;
            case 'C': t = 12; break;
            case 'D': t = 13; break;
            case 'E': t = 14; break;
            case 'F': t = 15; break;
            default: return s - str;
        }
        *num *= 16;
        *num += t;
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

size_t scan_uint32(const char *str, jpr_uint32 *num) {
    size_t r = 0;
    jpr_uint64 t;
    r = scan_uint(str, &t);
    *num = (jpr_uint32)t;
    return r;
}

size_t scan_int32(const char *str, jpr_int32 *num) {
    size_t r = 0;
    jpr_int64 t;
    r = scan_int(str, &t);
    *num = (jpr_int32)t;
    return r;
}

size_t scan_uint32_base16(const char *str, jpr_uint32 *num) {
    size_t r = 0;
    jpr_uint64 t;
    r = scan_uint_base16(str, &t);
    *num = (jpr_uint32)t;
    return r;
}
