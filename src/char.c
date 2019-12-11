#include "char.h"
#include <stdint.h>

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifndef JPR_NO_STDLIB
#include <ctype.h>
#endif

int char_isdigit(char c) {
#ifdef JPR_NO_STDLIB
    return (uint8_t)(c - 48) < 10;
#else
    return isdigit(c);
#endif
}

int char_isupper(char c) {
#ifdef JPR_NO_STDLIB
    return (uint8_t)(c - 65) < 26;
#else
    return isupper(c);
#endif
}

int char_islower(char c) {
#ifdef JPR_NO_STDLIB
    return (uint8_t)(c - 97) < 26;
#else
    return islower(c);
#endif
}

char char_upper(char c) {
#ifdef JPR_NO_STDLIB
    if (char_islower(c))
        return c & 0x5F;
    return c;
#else
    return toupper(c);
#endif
}

char char_lower(char c) {
#ifdef JPR_NO_STDLIB
    if (char_isupper(c))
        return c | 32;
    return c;
#else
    return tolower(c);
#endif
}

