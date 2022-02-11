#include "norm.h"
#include "int.h"
#define THREAD_IMPLEMENTATION
#define THREAD_U64 jpr_uint64
#include <errno.h>
#include <string.h>
#ifdef JPR_LINUX
#define _GNU_SOURCE
#endif
#include "thread.h"
