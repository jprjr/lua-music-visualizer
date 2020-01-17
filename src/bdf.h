#ifndef BDF_H
#define BDF_H

#include "attr.h"
#include "int.h"

typedef struct bdf_s bdf;

struct bdf_s {
    unsigned int width;
    unsigned int height;
    unsigned int total_glyphs;
    unsigned int max_glyph;
    unsigned int *widths;
    jpr_uint32 **glyphs;
};

#ifdef __cplusplus
extern "C" {
#endif

bdf *bdf_load(const char * RESTRICT filename);
void bdf_free(bdf *font);

#ifdef __cplusplus
}
#endif

#endif

