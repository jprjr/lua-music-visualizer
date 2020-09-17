#include "norm.h"
#include "str.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    uint8_t *p1 = malloc(100);
    uint8_t *p2 = malloc(100);
    memset(p1,0xFF,100);
    memset(p2,0xFF,100);
    p2[99] = 0;
    fprintf(stderr,"memcmp: %d\n",memcmp(p1,p2,100));
    fprintf(stderr,"mem_cmp: %d\n",mem_cmp(p1,p2,100));
    return 0;
}
