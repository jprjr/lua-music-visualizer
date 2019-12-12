#include "str.h"
#include "mem.h"
#define JPRP_STRLEN(x) str_len(x)
#define JPRP_STRCAT(d,s) str_cat(d,s)
#define JPRP_STRCPY(d,s) str_cpy(d,s)
#define JPRP_MEMSET(d,v,n) mem_set(d,v,n)
#define JPRP_MALLOC(x) mem_alloc(x)
#define JPRP_FREE(x) mem_free(x)
#define JPR_PROC_IMPLEMENTATION
#include "jpr_proc.h"
