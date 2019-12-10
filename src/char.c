#include "char.h"
#include <stdint.h>
/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifdef JPR_NO_STDLIB
int char_isdigit(char c) {
    return (uint8_t)(c - 48) < 10;
}
int char_isupper(char c) {
    return (uint8_t)(c - 65) < 26;
}
int char_islower(char c) {
    return (uint8_t)(c - 97) < 26;
}
char char_upper(char c) {
    if (char_islower(c))
        return c & 0x5F;
    return c;
}
char char_lower(char c) {
    if (char_isupper(c))
        return c | 32;
    return c;
}
#endif

