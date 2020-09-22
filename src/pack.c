#include "pack.h"
#include <stddef.h>

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

jpr_uint8 pack_uint64le(jpr_uint8 *d, jpr_uint64 n) {
    if(d != NULL) {
        d[0] = (jpr_uint8)(n       );
        d[1] = (jpr_uint8)(n >> 8  );
        d[2] = (jpr_uint8)(n >> 16 );
        d[3] = (jpr_uint8)(n >> 24 );
        d[4] = (jpr_uint8)(n >> 32 );
        d[5] = (jpr_uint8)(n >> 40 );
        d[6] = (jpr_uint8)(n >> 48 );
        d[7] = (jpr_uint8)(n >> 56 );
    }
    return 8;
}

jpr_uint8 pack_int64le(jpr_uint8 *d, jpr_int64 n) {
    if(d != NULL) {
        d[0] = (jpr_uint8)(n       );
        d[1] = (jpr_uint8)(n >> 8  );
        d[2] = (jpr_uint8)(n >> 16 );
        d[3] = (jpr_uint8)(n >> 24 );
        d[4] = (jpr_uint8)(n >> 32 );
        d[5] = (jpr_uint8)(n >> 40 );
        d[6] = (jpr_uint8)(n >> 48 );
        d[7] = (jpr_uint8)(n >> 56 );
    }
    return 8;
}

jpr_uint8 pack_uint32le(jpr_uint8 *d, jpr_uint32 n) {
    if(d != NULL) {
        d[0] = (jpr_uint8)(n       );
        d[1] = (jpr_uint8)(n >> 8  );
        d[2] = (jpr_uint8)(n >> 16 );
        d[3] = (jpr_uint8)(n >> 24 );
    }
    return 4;
}

jpr_uint8 pack_int32le(jpr_uint8 *d, jpr_int32 n) {
    if(d != NULL) {
        d[0] = (jpr_uint8)(n       );
        d[1] = (jpr_uint8)(n >> 8  );
        d[2] = (jpr_uint8)(n >> 16 );
        d[3] = (jpr_uint8)(n >> 24 );
    }
    return 4;
}

jpr_uint8 pack_uint16le(jpr_uint8 *d, jpr_uint16 n) {
    if(d != NULL) {
        d[0] = (jpr_uint8)(n       );
        d[1] = (jpr_uint8)(n >> 8  );
    }
    return 2;
}

jpr_uint8 pack_int16le(jpr_uint8 *d, jpr_int16 n) {
    if(d != NULL) {
        d[0] = (jpr_uint8)(n       );
        d[1] = (jpr_uint8)(n >> 8  );
    }
    return 2;
}

jpr_uint8 pack_uint64be(jpr_uint8 *d, jpr_uint64 n) {
    if(d != NULL) {
        d[0] = (jpr_uint8)(n >> 56 );
        d[1] = (jpr_uint8)(n >> 48 );
        d[2] = (jpr_uint8)(n >> 40 );
        d[3] = (jpr_uint8)(n >> 32 );
        d[4] = (jpr_uint8)(n >> 24 );
        d[5] = (jpr_uint8)(n >> 16 );
        d[6] = (jpr_uint8)(n >> 8  );
        d[7] = (jpr_uint8)(n       );
    }
    return 8;
}

jpr_uint8 pack_int64be(jpr_uint8 *d, jpr_int64 n) {
    if(d != NULL) {
        d[0] = (jpr_uint8)(n >> 56 );
        d[1] = (jpr_uint8)(n >> 48 );
        d[2] = (jpr_uint8)(n >> 40 );
        d[3] = (jpr_uint8)(n >> 32 );
        d[4] = (jpr_uint8)(n >> 24 );
        d[5] = (jpr_uint8)(n >> 16 );
        d[6] = (jpr_uint8)(n >> 8  );
        d[7] = (jpr_uint8)(n       );
    }
    return 8;
}

jpr_uint8 pack_uint32be(jpr_uint8 *d, jpr_uint32 n) {
    if(d != NULL) {
        d[0] = (jpr_uint8)(n >> 24 );
        d[1] = (jpr_uint8)(n >> 16 );
        d[2] = (jpr_uint8)(n >> 8  );
        d[3] = (jpr_uint8)(n       );
    }
    return 4;
}

jpr_uint8 pack_int32be(jpr_uint8 *d, jpr_int32 n) {
    if(d != NULL) {
        d[0] = (jpr_uint8)(n >> 24 );
        d[1] = (jpr_uint8)(n >> 16 );
        d[2] = (jpr_uint8)(n >> 8  );
        d[3] = (jpr_uint8)(n       );
    }
    return 4;
}

jpr_uint8 pack_uint16be(jpr_uint8 *d, jpr_uint16 n) {
    if(d != NULL) {
        d[0] = (jpr_uint8)(n >> 8  );
        d[1] = (jpr_uint8)(n       );
    }
    return 2;
}

jpr_uint8 pack_int16be(jpr_uint8 *d, jpr_int16 n) {
    if(d != NULL) {
        d[0] = (jpr_uint8)(n >> 8  );
        d[1] = (jpr_uint8)(n       );
    }
    return 2;
}

