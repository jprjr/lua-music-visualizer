#ifndef JPR_DATETIME_STRUCT_H
#define JPR_DATETIME_STRUCT_H

struct datetime {
  int year;
  int month;
  int day;
  int hour;
  int min;
  int sec;
  int yday;
  int wday;
  int isdst;
  int msec;
};

typedef struct datetime datetime_t;

#endif
