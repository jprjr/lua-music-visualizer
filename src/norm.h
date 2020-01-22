#ifndef JPR_NORM_H
#define JPR_NORM_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#define JPR_WINDOWS

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#ifndef _UNICODE
#define _UNICODE 1
#endif

#ifndef UNICODE
#define UNICODE 1
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <windows.h>

#else

#define _POSIX_C_SOURCE 200809L

#if defined(__linux__)
#define JPR_LINUX
#define JPR_UNIX
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 1
#endif

#elif defined(__APPLE__)
#define JPR_APPLE
#define JPR_UNIX
#endif

#endif

#endif
