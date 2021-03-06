#ifndef IMAGE_H
#define IMAGE_H

#include "int.h"

typedef struct gif_result_t {
    int delay;
    jpr_uint8 *data;
    struct gif_result_t *next;
} gif_result;


#ifdef __cplusplus
extern "C" {
#endif

int
image_probe (const char *filename, unsigned int *width, unsigned int *height, unsigned int *channels);

void
image_blend(jpr_uint8 *dst, jpr_uint8 *src, unsigned int len, jpr_uint8 a);

jpr_uint8 *
image_load(
  const char *filename,
   unsigned int *width,
   unsigned int *height,
   unsigned int *channels,
   unsigned int *frames);

void draw_rectangle(jpr_uint8 *image, unsigned int xstart, unsigned int xend, unsigned int ystart, unsigned int yend, unsigned int width, unsigned int channels, unsigned int r, unsigned int g, unsigned int b);
void draw_rectangle_alpha(jpr_uint8 *image, unsigned int xstart, unsigned int xend, unsigned int ystart, unsigned int yend, unsigned int width, unsigned int channels, unsigned int r, unsigned int g, unsigned int b, unsigned int a);

#ifdef __cplusplus
}
#endif

#endif

