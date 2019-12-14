#include "mem.h"

#include <stdint.h>

void *mem_alloc16(size_t n) {
    void *mem;
    void *ptr;
    mem = mem_alloc(n + 15 + sizeof(void *));
    if(mem == NULL) return mem;
    ptr = (void *)(((uintptr_t)mem+sizeof(void *)+15) & ~ (uintptr_t)0x0F);
    ((void**)ptr)[-1] = mem;
    return ptr;
}

void mem_set16(void *ptr, uint8_t v, size_t n) {
    uint8_t *p = ptr;
    while(n > 16) {
        p[0] = v;
        p[1] = v;
        p[2] = v;
        p[3] = v;
        p[4] = v;
        p[5] = v;
        p[6] = v;
        p[7] = v;
        p[8] = v;
        p[9] = v;
        p[10] = v;
        p[11] = v;
        p[12] = v;
        p[13] = v;
        p[14] = v;
        p[15] = v;
        p += 16;
        n -= 16;
    }
    switch(n) {
        case 16: p[15] = v; /* fall-through */
        case 15: p[14] = v; /* fall-through */
        case 14: p[13] = v; /* fall-through */
        case 13: p[12] = v; /* fall-through */
        case 12: p[11] = v; /* fall-through */
        case 11: p[10] = v; /* fall-through */
        case 10:  p[9] = v; /* fall-through */
        case  9:  p[8] = v; /* fall-through */
        case  8:  p[7] = v; /* fall-through */
        case  7:  p[6] = v; /* fall-through */
        case  6:  p[5] = v; /* fall-through */
        case  5:  p[4] = v; /* fall-through */
        case  4:  p[3] = v; /* fall-through */
        case  3:  p[2] = v; /* fall-through */
        case  2:  p[1] = v; /* fall-through */
        case  1:  p[0] = v; /* fall-through */
    }
}

void mem_free16(void *ptr) {
    mem_free( ((void**)ptr)[-1]);
}
