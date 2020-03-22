#include "norm.h"
#include "datetime.h"
#include "hrtime.h"
#include <stdint.h>

#ifndef JPR_WINDOWS
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#endif

int get_hrtime(hrtime_t *hr) {
#ifndef JPR_WINDOWS
    struct timeval tv;
    gettimeofday(&tv,NULL);
    hr->sec = tv.tv_sec;
    hr->msec = tv.tv_usec / 1000;
    return 1;
#else
    datetime_t dt;

    get_datetime(&dt,1);
    datetime_to_hrtime(&dt,hr);
    return 1;

#endif
}

int datetime_to_hrtime(datetime_t *dt, hrtime_t *hr) {
    int64_t year;
    int64_t mon;
    int64_t day;

    year = dt->year;

    mon = dt->month - 1;
    if (mon >= 2) { mon -= 2; }
    else { mon += 10; --year; }

    day = (dt->day - 1) * 10 + 5 + 306 * mon;
    day /= 10;

    if (day == 365) { year -= 3; day = 1460; }
    else { day += 365 * (year % 4); }
    year /= 4;

    day += 1461 * (year % 25);
    year /= 25;

    if (day == 36524) { year -= 3; day = 146096; }
    else { day += 36524 * (year % 4); }
    year /= 4;

    day += 146097 * (year - 5);
    day += 11017;

    hr->sec = ((day * 24 + dt->hour) * 60 + dt->min) * 60 + dt->sec;
    hr->msec = dt->msec;

    return 1;

}
