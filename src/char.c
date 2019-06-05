#include "char.h"

unsigned char char_isdigit(char c) {
    return c > 0x2F && c < 0x3A;
}

unsigned char char_isupper(char c) {
    return c > 0x40 && c < 0x5B;
}

unsigned char char_islower(char c) {
    return c > 0x60 && c < 0x7B;
}

char char_lower(char c) {
    return (char_isupper(c) ? c + 0x20 : c);
}

char char_upper(char c) {
    return (char_islower(c) ? c - 0x20 : c);
}
