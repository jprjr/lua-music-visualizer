#include "norm.h"
#include "mem.h"
#include "str.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef JPR_WINDOWS
#include <windows.h>
#else
#include <time.h>
static double ts_to_sec(struct timespec *ts) {
    return (double)ts->tv_sec + (double)ts->tv_nsec / 1000000000.0;
}
#endif

typedef struct benchmark_test_s benchmark_test;
struct benchmark_test_s {
    const char *name;
    void *(*setup)(void);
    void (*test)(void *);
    void (*cleanup)(void *);
};

static double benchmark(const benchmark_test *bench) {
    void *userdata;
    double ret;
#ifdef JPR_WINDOWS
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
#else
    struct timespec start;
    struct timespec end;
#endif

    userdata = bench->setup();

#ifdef JPR_WINDOWS
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    bench->test(userdata);
    QueryPerformanceCounter(&end);
    ret = (double)((end.QuadPart - start.QuadPart) / ((double)freq.QuadPart));
#else
    clock_gettime(CLOCK_MONOTONIC,&start);
    bench->test(userdata);
    clock_gettime(CLOCK_MONOTONIC,&end);
    ret = ts_to_sec(&end) - ts_to_sec(&start);
#endif
    bench->cleanup(userdata);
    return ret;
}

struct memcmp_bench {
    void *block1;
    void *block2;
};

static void *setup_mem_cmp(void) {
    unsigned int i;
    char *block1 = mem_alloc(10485760);
    char *block2 = mem_alloc(10485760);

    for(i=0; i<10485760;i++) {
        block1[i] = (uint8_t)(rand()/(RAND_MAX / 255));
    }
    memcpy(block2,block1,10485760);

    struct memcmp_bench *b = (struct memcmp_bench *)mem_alloc(sizeof(struct memcmp_bench));
    b->block1 = block1;
    b->block2 = block2;
    return b;
}

static void clean_mem_cmp(void *u) {
    struct memcmp_bench *b = (struct memcmp_bench *)u;
    mem_free(b->block1);
    mem_free(b->block2);
    mem_free(b);
}

static void test_mem_cmp(void *u) {
    unsigned int i = 0;
    struct memcmp_bench *b = (struct memcmp_bench *)u;
    for(;i<10000;i++) {
        mem_cmp(b->block1,b->block2,10485760);
    }
}

static void test_memcmp(void *u) {
    unsigned int i = 0;
    struct memcmp_bench *b = (struct memcmp_bench *)u;
    for(;i<10000;i++) {
        memcmp(b->block1,b->block2,10485760);
    }
}

static void test_memcpy(void *u) {
    unsigned int i = 0;
    struct memcmp_bench *b = (struct memcmp_bench *)u;
    for(;i<10000;i++) {
        memcpy(b->block1,b->block2,10485760);
    }
}

static void test_mem_cpy(void *u) {
    unsigned int i = 0;
    struct memcmp_bench *b = (struct memcmp_bench *)u;
    for(;i<10000;i++) {
        mem_cpy(b->block1,b->block2,10485760);
    }
}

static const benchmark_test tests[] = {
    { "memcmp ", setup_mem_cmp, test_memcmp, clean_mem_cmp },
    { "mem_cmp", setup_mem_cmp, test_mem_cmp, clean_mem_cmp },
    { "memcpy ", setup_mem_cmp, test_memcpy, clean_mem_cmp },
    { "mem_cpy", setup_mem_cmp, test_mem_cpy, clean_mem_cmp },
    { NULL, NULL, NULL, NULL },
};

int main(void) {
    unsigned int i = 0;
    double elapsed = 0.0;
    srand(time(NULL));
    while(tests[i].test != NULL) {
        elapsed = benchmark(&tests[i]);
        fprintf(stderr,"%s: %f\n",tests[i].name,elapsed);
        i++;
    }
    return 0;
}

