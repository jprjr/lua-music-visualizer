#ifndef JPR_MEM_H
#define JPR_MEM_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include "norm.h"
#if !(defined(JPR_NO_STDLIB) && defined(JPR_WINDOWS))
#include <stdlib.h>
#define mem_alloc(x) malloc(x)
#define mem_realloc(x,y) realloc(x,y)
#define mem_free(x) free(x)
#else
#define mem_alloc(x) HeapAlloc(GetProcessHeap(),0,x)
#define mem_realloc(x,y) HeapReAlloc(GetProcessHeap(),0,x,y)
#define mem_free(x) HeapFree(GetProcessHeap(),0,x)
#endif


#endif

