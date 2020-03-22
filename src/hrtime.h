#ifndef JPR_TIME_H
#define JPR_TIME_H

#include "datetime-struct.h"
#include "hrtime-struct.h"

int get_hrtime(hrtime_t *);
int datetime_to_hrtime(datetime_t *, hrtime_t *);

#endif
