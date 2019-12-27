#include "char.h"
#include <stdint.h>

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifdef JPR_NO_STDLIB
int char_isdigit(int c) {
    return (uint8_t)(c - 48) < 10;
}
#endif

#ifdef JPR_NO_STDLIB
int char_isupper(int c) {
    return (uint8_t)(c - 65) < 26;
}
#endif

#ifdef JPR_NO_STDLIB
int char_islower(int c) {
    return (uint8_t)(c - 97) < 26;
}
#endif

#ifdef JPR_NO_STDLIB
int char_upper(int c) {
    if (char_islower(c)) {
        return c & 0x5F;
    }
    return c;
}
#endif

#ifdef JPR_NO_STDLIB
int char_lower(int c) {
    if (char_isupper(c)) {
        return c | 32;
    }
    return c;
}
#endif

