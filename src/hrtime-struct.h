#ifndef JPR_TIME_STRUCT_H
#define JPR_TIME_STRUCT_H

#include <stdint.h>

struct hrtime {
    int64_t sec;
    int64_t msec;
};

typedef struct hrtime hrtime_t;

#endif
