#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#ifdef __cplusplus
extern "C" {
#endif

static void
flip_and_bgr(uint8_t *image, unsigned int width, unsigned int height, unsigned int channels) {

    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int maxheight = height >> 1;
    unsigned int byte;
    unsigned int ibyte;
    unsigned int bytes_per_row = width * channels;
    uint8_t *temp_row = (uint8_t *)malloc(bytes_per_row);
    uint8_t temp;

    for(y=0; y<maxheight;y++) {
        byte = (y * width* channels);
        ibyte = (height - y - 1) * width * channels;
        memcpy(temp_row,image + ibyte,bytes_per_row);
        memcpy(image + ibyte,image + byte,bytes_per_row);
        memcpy(image + byte,temp_row,bytes_per_row);
        for(x=0;x<bytes_per_row;x+=channels) {

            temp = image[byte + x + 2];
            image[byte + x + 2] = image[byte + x];
            image[byte + x] = temp;

            temp = image[ibyte + x + 2];
            image[ibyte + x + 2] = image[ibyte + x];
            image[ibyte + x] = temp;
        }
    }

    if(height % 2 == 1) {
        byte = (y * width* channels);
        for(x=0;x<bytes_per_row;x+=channels) {
            temp = image[byte + x + 2];
            image[byte + x + 2] = image[byte + x];
            image[byte + x] = temp;
        }
    }

    free(temp_row);
}

void
image_blend(uint8_t *dst, uint8_t *src, unsigned int len, uint8_t a) {
    int alpha = 1 + a;
    int alpha_inv = 256 - a;

    unsigned int i = 0;
    for(i=0;i<len;i+=8) {
        dst[i] =   ((dst[i] * alpha_inv) + (src[i] * alpha)) >> 8;
        dst[i+1] = ((dst[i+1] * alpha_inv) + (src[i+1] * alpha)) >> 8;
        dst[i+2] = ((dst[i+2] * alpha_inv) + (src[i+2] * alpha)) >> 8;
        dst[i+3] = ((dst[i+3] * alpha_inv) + (src[i+3] * alpha)) >> 8;
        dst[i+4] = ((dst[i+4] * alpha_inv) + (src[i+4] * alpha)) >> 8;
        dst[i+5] = ((dst[i+5] * alpha_inv) + (src[i+5] * alpha)) >> 8;
        dst[i+6] = ((dst[i+6] * alpha_inv) + (src[i+6] * alpha)) >> 8;
        dst[i+7] = ((dst[i+7] * alpha_inv) + (src[i+7] * alpha)) >> 8;
    }
}

int
image_probe(const char *filename, unsigned int *width, unsigned int *height, unsigned int *channels) {
    int x = 0;
    int y = 0;
    int c = 0;

    if(stbi_info(filename,&x,&y,&c) == 0) {
        return 0;
    }
    if(c < 3 && *channels == 0) {
        c = 3;
    }

    if(*channels == 0) {
        *channels = c;
    }

    if(*width == 0 && *height == 0) {
        *width = x;
        *height = y;
    }
    else if(*width == 0) {
        *width = x * (*height) / y;
    }
    else if (*height == 0) {
        *height = y * (*width) / x;
    }

    return 1;
}

static uint8_t *
stbi_xload(
  const char *filename,
  unsigned int *width,
  unsigned int *height,
  unsigned int *channels,
  unsigned int *frames,
  unsigned int **delays) {

    FILE *f;
    stbi__context s;
    unsigned char *result = 0;
    stbi__result_info ri;
    memset(&ri,0,sizeof(stbi__result_info));
    int x;
    int y;
    int c;

    if (!(f = stbi__fopen(filename, "rb")))
        return stbi__errpuc("can't fopen", "Unable to open file");

    stbi__start_file(&s, f);

    if (stbi__gif_test(&s))
    {
        result = stbi__load_gif_main(&s, (int **)delays, &x, &y, (int *)frames, &c, *channels);
    }
    else
    {
        result = stbi__load_main(&s, &x,&y, &c, *channels, &ri, 8);
        *frames = !!result;
        *delays = 0;
    }

    if(result) {
        *width =  (unsigned int)x;
        *height = (unsigned int)y;
    }

    fclose(f);
    return result;
}

uint8_t *
image_load(
  const char *filename,
  unsigned int *width,
  unsigned int *height,
  unsigned int *channels,
  unsigned int *frames) {

    uint8_t *image = NULL;
    uint8_t *t = NULL;
    uint8_t *b = NULL;
    uint8_t *d = NULL;

    unsigned int ow = 0;
    unsigned int oh = 0;
    unsigned int oc = 0;
    unsigned int of = 0;
    unsigned int og_w = 0;
    unsigned int og_h = 0;
    unsigned int i = 0;
    unsigned int *delays = NULL;

    if(image_probe(filename,width,height,channels) == 0) {
        return NULL;
    }

    ow = *width;
    oh = *height;
    oc = *channels;

    t = stbi_xload(filename,&og_w,&og_h,channels,frames,&delays);

    if(!t) {
        return NULL;
    }

    of = *frames;

    if(of > 1) {
        image = (uint8_t *)malloc( ((ow * oh * oc) * of) + (2 * of));

    }
    else {
        image = (uint8_t *)malloc(ow * oh * oc);
    }
    b = t;
    d = image;

    for(i=0;i<of;i++) {
        if(ow  != og_w || oh != og_h) {
            stbir_resize_uint8(b,og_w,og_h,0,d,ow,oh,0,oc);
        }
        else {
            memcpy(d, b , ow * oh * oc);
        }

        flip_and_bgr(d,ow,oh,oc);

        if(of > 1) {
            d += (ow * oh * oc);
            *d++ = delays[i] & 0xFF;
            *d++ = (delays[i] & 0xFF00) >> 8;
            b += (og_w * og_h * oc);
        }
    }

    stbi_image_free(t);
    if(delays != NULL) free(delays);

    return image;
}


#ifdef __cplusplus
}
#endif

