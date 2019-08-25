#include "unpack.h"

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

uint64_t unpack_uint64le(const uint8_t *b) {
    return (((uint64_t)b[7])<<56) |
           (((uint64_t)b[6])<<48) |
           (((uint64_t)b[5])<<40) |
           (((uint64_t)b[4])<<32) |
           (((uint64_t)b[3])<<24) |
           (((uint64_t)b[2])<<16) |
           (((uint64_t)b[1])<<8 ) |
           (((uint64_t)b[0])    );
}

uint64_t unpack_uint64be(const uint8_t *b) {
    return (((uint64_t)b[0])<<56) |
           (((uint64_t)b[1])<<48) |
           (((uint64_t)b[2])<<40) |
           (((uint64_t)b[3])<<32) |
           (((uint64_t)b[4])<<24) |
           (((uint64_t)b[5])<<16) |
           (((uint64_t)b[6])<<8 ) |
           (((uint64_t)b[7])    );
}

uint32_t unpack_uint32le(const uint8_t *b) {
    return (((uint32_t)b[3])<<24) |
           (((uint32_t)b[2])<<16) |
           (((uint32_t)b[1])<<8 ) |
           (((uint32_t)b[0])    );
}

uint32_t unpack_uint32be(const uint8_t *b) {
    return (((uint32_t)b[0])<<24) |
           (((uint32_t)b[1])<<16) |
           (((uint32_t)b[2])<<8 ) |
           (((uint32_t)b[3])    );
}

uint32_t unpack_uint24le(const uint8_t *b) {
    return (((uint32_t)b[2])<<16) |
           (((uint32_t)b[1])<< 8) |
           (((uint32_t)b[0])    );
}

uint32_t unpack_uint24be(const uint8_t *b) {
    return (((uint32_t)b[0])<<16) |
           (((uint32_t)b[1])<< 8) |
           (((uint32_t)b[2])    );
}
uint16_t unpack_uint16le(const uint8_t *b) {
    return (((uint16_t)b[1])<<8) |
           (((uint16_t)b[0])   );
}

uint16_t unpack_uint16be(const uint8_t *b) {
    return (((uint16_t)b[0])<<8) |
           (((uint16_t)b[1])   );
}

