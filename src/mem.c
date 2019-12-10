/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#else
#include <stdlib.h>
#endif

#include "mem.h"

void *mem_alloc(size_t size) {
#if defined(_WIN32) || defined(_WIN64)
    return HeapAlloc(GetProcessHeap(),0,size);
#else
    return malloc(size);
#endif
}

void *mem_realloc(void *s, size_t size) {
#if defined(_WIN32) || defined(_WIN64)
    return HeapReAlloc(GetProcessHeap(),0,s,size);
#else
    return realloc(s,size);
#endif
}

void mem_free(void *s) {
#if defined(_WIN32) || defined(_WIN64)
    HeapFree(GetProcessHeap(),0,s);
#else
    free(s);
#endif
    return;
}
