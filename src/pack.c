#include "pack.h"
#include <stddef.h>

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

uint8_t pack_uint64le(uint8_t *d, uint64_t n) {
    if(d != NULL) {
        ((uint64_t *)d)[0] = n;
    }
    return 8;
}

uint8_t pack_int64le(uint8_t *d, int64_t n) {
    if(d != NULL) {
        ((int64_t *)d)[0] = n;
    }
    return 8;
}

uint8_t pack_uint32le(uint8_t *d, uint32_t n) {
    if(d != NULL) {
        ((uint32_t *)d)[0] = n;
    }
    return 4;
}

uint8_t pack_int32le(uint8_t *d, int32_t n) {
    if(d != NULL) {
        ((int32_t *)d)[0] = n;
    }
    return 4;
}

uint8_t pack_uint16le(uint8_t *d, uint16_t n) {
    if(d != NULL) {
        ((uint16_t *)d)[0] = n;
    }
    return 2;
}

uint8_t pack_int16le(uint8_t *d, int16_t n) {
    if(d != NULL) {
        ((int16_t *)d)[0] = n;
    }
    return 2;
}

#else

uint8_t pack_uint64le(uint8_t *d, uint64_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n       );
        d[1] = (uint8_t)(n >> 8  );
        d[2] = (uint8_t)(n >> 16 );
        d[3] = (uint8_t)(n >> 24 );
        d[4] = (uint8_t)(n >> 32 );
        d[5] = (uint8_t)(n >> 40 );
        d[6] = (uint8_t)(n >> 48 );
        d[7] = (uint8_t)(n >> 56 );
    }
    return 8;
}

uint8_t pack_int64le(uint8_t *d, int64_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n       );
        d[1] = (uint8_t)(n >> 8  );
        d[2] = (uint8_t)(n >> 16 );
        d[3] = (uint8_t)(n >> 24 );
        d[4] = (uint8_t)(n >> 32 );
        d[5] = (uint8_t)(n >> 40 );
        d[6] = (uint8_t)(n >> 48 );
        d[7] = (uint8_t)(n >> 56 );
    }
    return 8;
}

uint8_t pack_uint32le(uint8_t *d, uint32_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n       );
        d[1] = (uint8_t)(n >> 8  );
        d[2] = (uint8_t)(n >> 16 );
        d[3] = (uint8_t)(n >> 24 );
    }
    return 4;
}

uint8_t pack_int32le(uint8_t *d, int32_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n       );
        d[1] = (uint8_t)(n >> 8  );
        d[2] = (uint8_t)(n >> 16 );
        d[3] = (uint8_t)(n >> 24 );
    }
    return 4;
}

uint8_t pack_uint16le(uint8_t *d, uint16_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n       );
        d[1] = (uint8_t)(n >> 8  );
    }
    return 2;
}

uint8_t pack_int16le(uint8_t *d, int16_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n       );
        d[1] = (uint8_t)(n >> 8  );
    }
    return 2;
}

#endif

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

uint8_t pack_uint64be(uint8_t *d, uint64_t n) {
    if(d != NULL) {
        ((uint64_t *)d)[0] = n;
    }
    return 8;
}

uint8_t pack_int64be(uint8_t *d, int64_t n) {
    if(d != NULL) {
        ((int64_t *)d)[0] = n;
    }
    return 8;
}

uint8_t pack_uint32be(uint8_t *d, uint32_t n) {
    if(d != NULL) {
        ((uint32_t *)d)[0] = n;
    }
    return 4;
}

uint8_t pack_int32be(uint8_t *d, int32_t n) {
    if(d != NULL) {
        ((int32_t *)d)[0] = n;
    }
    return 4;
}

uint8_t pack_uint16be(uint8_t *d, uint16_t n) {
    if(d != NULL) {
        ((uint16_t *)d)[0] = n;
    }
    return 2;
}

uint8_t pack_int16be(uint8_t *d, int16_t n) {
    if(d != NULL) {
        ((int16_t *)d)[0] = n;
    }
    return 2;
}

#else

uint8_t pack_uint64be(uint8_t *d, uint64_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n >> 56 );
        d[1] = (uint8_t)(n >> 48 );
        d[2] = (uint8_t)(n >> 40 );
        d[3] = (uint8_t)(n >> 32 );
        d[4] = (uint8_t)(n >> 24 );
        d[5] = (uint8_t)(n >> 16 );
        d[6] = (uint8_t)(n >> 8  );
        d[7] = (uint8_t)(n       );
    }
    return 8;
}

uint8_t pack_int64be(uint8_t *d, int64_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n >> 56 );
        d[1] = (uint8_t)(n >> 48 );
        d[2] = (uint8_t)(n >> 40 );
        d[3] = (uint8_t)(n >> 32 );
        d[4] = (uint8_t)(n >> 24 );
        d[5] = (uint8_t)(n >> 16 );
        d[6] = (uint8_t)(n >> 8  );
        d[7] = (uint8_t)(n       );
    }
    return 8;
}

uint8_t pack_uint32be(uint8_t *d, uint32_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n >> 24 );
        d[1] = (uint8_t)(n >> 16 );
        d[2] = (uint8_t)(n >> 8  );
        d[3] = (uint8_t)(n       );
    }
    return 4;
}

uint8_t pack_int32be(uint8_t *d, int32_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n >> 24 );
        d[1] = (uint8_t)(n >> 16 );
        d[2] = (uint8_t)(n >> 8  );
        d[3] = (uint8_t)(n       );
    }
    return 4;
}

uint8_t pack_uint16be(uint8_t *d, uint16_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n >> 8  );
        d[1] = (uint8_t)(n       );
    }
    return 2;
}

uint8_t pack_int16be(uint8_t *d, int16_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n >> 8  );
        d[1] = (uint8_t)(n       );
    }
    return 2;
}

#endif
