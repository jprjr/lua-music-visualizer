#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

typedef struct gif_result_t {
    int delay;
    uint8_t *data;
    struct gif_result_t *next;
} gif_result;


#ifdef __cplusplus
extern "C" {
#endif

int
image_probe (const char *filename, unsigned int *width, unsigned int *height, unsigned int *channels);

void
image_blend(uint8_t *dst, uint8_t *src, unsigned int len, uint8_t a);

uint8_t *
image_load(
  const char *filename,
   unsigned int *width,
   unsigned int *height,
   unsigned int *channels,
   unsigned int *frames);

void draw_rectangle(uint8_t *image, unsigned int xstart, unsigned int xend, unsigned int ystart, unsigned int yend, unsigned int width, unsigned int channels, unsigned int r, unsigned int g, unsigned int b);
void draw_rectangle_alpha(uint8_t *image, unsigned int xstart, unsigned int xend, unsigned int ystart, unsigned int yend, unsigned int width, unsigned int channels, unsigned int r, unsigned int g, unsigned int b, unsigned int a);

#ifdef __cplusplus
}
#endif

#endif

