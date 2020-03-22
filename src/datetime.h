#ifndef JPR_DATETIME_H
#define JPR_DATETIME_H

#include "datetime-struct.h"
#include "hrtime-struct.h"

/* fills the datetime_t struct with high-res time, local or utc */
int get_datetime(datetime_t *, int utc);

#endif
