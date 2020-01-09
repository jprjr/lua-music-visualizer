#ifndef JPR_UTIL_H
#define JPR_UTIL_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include "norm.h"
#include "str.h"



/*
 * utility methods:
 * LOG_ERROR(s) - write (s) to stderr
 * LOG_DEBUG(s) - write (s) to stderr iff NDEBUG is undef
 * JPR_EXIT(e)  - exit/ExitProcess with code e
*/

#ifdef JPR_WINDOWS
#define WRITE_STDERR(s) WriteFile(GetStdHandle(STD_ERROR_HANDLE),s,(DWORD)str_len(s),NULL,0)
#define JPR_EXIT(e) ExitProcess(e)
#else
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#define WRITE_STDERR(s) \
    while( (write(2,s,str_len(s)) == -1) && (errno == EINTR) ) {}
#define JPR_EXIT(e) exit(e)
#endif

#define LOG_ERROR(s) \
    WRITE_STDERR(s); \
    WRITE_STDERR("\n")

#ifdef NDEBUG
#define LOG_DEBUG(s)
#else
#define LOG_DEBUG(s) LOG_ERROR(s)
#endif

#endif
