#include "int.h"

#ifdef __cplusplus
extern "C" {
#endif
int char_isdigit(int c);
int char_isupper(int c);
int char_islower(int c);
int char_upper(int c);
int char_lower(int c);
#ifdef __cplusplus
}
#endif

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

int char_isdigit(int c) {
    return (jpr_uint8)(c - 48) < 10;
}

int char_isupper(int c) {
    return (jpr_uint8)(c - 65) < 26;
}

int char_islower(int c) {
    return (jpr_uint8)(c - 97) < 26;
}

int char_upper(int c) {
    if (char_islower(c)) {
        return c & 0x5F;
    }
    return c;
}

int char_lower(int c) {
    if (char_isupper(c)) {
        return c | 32;
    }
    return c;
}

#ifdef JPR_ALIAS_STDLIB
#if defined(__APPLE__) || ( defined(_WIN32) && !defined(_MSC_VER) && !defined(_WIN64))
#define MAKE_ALIAS(x,y) \
__asm__(".globl _"  #y); \
__asm__(".set _" #y ", _"  #x);
#elif !defined(_MSC_VER)
#define MAKE_ALIAS(x,y) \
__asm__(".globl " #y); \
__asm__(".set " #y  ", " #x);
#else
#define MAKE_ALIAS(x,y)
#endif

MAKE_ALIAS(char_isdigit,isdigit)
MAKE_ALIAS(char_isupper,isupper)
MAKE_ALIAS(char_islower,islower)
MAKE_ALIAS(char_upper,toupper)
MAKE_ALIAS(char_lower,tolower)

#ifdef _MSC_VER
#ifdef __cplusplus
extern "C" {
#endif

int isupper(int c);
int islower(int c);
int toupper(int c);
int tolower(int c);
#ifdef __cplusplus
}
#endif

void *isdigit = char_isdigit;
int isupper(int c) { return char_isupper(c); }
int islower(int c) { return char_islower(c); }
int toupper(int c) { return char_upper(c); }
int tolower(int c) { return char_lower(c); }
#endif

#endif

