/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include "norm.h"
#include "mem.h"

#if !(defined(JPR_NO_STDLIB) && defined(JPR_WINDOWS))
#include <stdlib.h>
#endif

void *mem_alloc(size_t size) {
#if defined(JPR_NO_STDLIB) && defined(JPR_WINDOWS)
    return HeapAlloc(GetProcessHeap(),0,size);
#else
    return malloc(size);
#endif
}

void *mem_realloc(void *s, size_t size) {
#if defined(JPR_NO_STDLIB) && defined(JPR_WINDOWS)
    return HeapReAlloc(GetProcessHeap(),0,s,size);
#else
    return realloc(s,size);
#endif
}

void mem_free(void *s) {
#if defined(JPR_NO_STDLIB) && defined(JPR_WINDOWS)
    HeapFree(GetProcessHeap(),0,s);
#else
    free(s);
#endif
    return;
}
