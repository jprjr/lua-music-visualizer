#include <limits.h>
#include "norm.h"
#include "datetime.h"
#include "hrtime.h"

#ifndef JPR_WINDOWS
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

static void tm_to_dt(struct tm *tm, datetime_t *dt, int msec) {
    dt->year  = tm->tm_year + 1900;
    dt->month = tm->tm_mon + 1;
    dt->day   = tm->tm_mday;
    dt->hour  = tm->tm_hour;
    dt->min   = tm->tm_min;
    dt->sec   = tm->tm_sec;
    dt->wday  = tm->tm_wday + 1;
    dt->yday  = tm->tm_yday + 1;
    dt->isdst = tm->tm_isdst;
    dt->msec  = msec;
}

#else

BOOL WINAPI DllMainCRTStartup(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpReserved)
{
    (void)hinstDLL;
    (void)fdwReason;
    (void)lpReserved;
       return 1;
}

static int isleap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int get_yday(int year, int month, int day) {
    static const int days[2][13] = {
        {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
        {0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335},
    };
    return days[isleap(year)][month] + day;
}

static void st_to_dt(SYSTEMTIME *st, datetime_t *dt) {
    dt->year  = st->wYear;
    dt->month = st->wMonth;
    dt->day   = st->wDay;
    dt->hour  = st->wHour;
    dt->min   = st->wMinute;
    dt->sec   = st->wSecond;
    dt->msec  = st->wMilliseconds;
    dt->wday  = st->wDayOfWeek + 1;
    dt->yday  = get_yday(dt->year,dt->month,dt->day);
}

#endif


int get_datetime(datetime_t *dt, int utc) {
#ifdef JPR_WINDOWS
    SYSTEMTIME st;
    TIME_ZONE_INFORMATION tz;
    if(utc) {
        GetSystemTime(&st);
        dt->isdst = 0;
    }
    else {
        GetLocalTime(&st);
        dt->isdst = (GetTimeZoneInformation(&tz) == 2 ? 1 : 0);
    }

    st_to_dt(&st,dt);

#else
    struct tm tm;
    hrtime_t hr;
    get_hrtime(&hr);

    if(utc) {
        gmtime_r((time_t *)&hr.sec, &tm);
    }
    else {
        localtime_r((time_t *)&hr.sec,&tm);
    }

    tm_to_dt(&tm,dt,hr.msec);

#endif

    return 0;
}



