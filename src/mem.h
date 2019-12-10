#ifndef JPR_MEM_H
#define JPR_MEM_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *mem_alloc(size_t size);
void *mem_realloc(void *s, size_t size);
void mem_free(void *s);


#ifdef __cplusplus
}
#endif

#endif

