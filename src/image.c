#include "image.h"
#include "file.h"
#include "str.h"
#include "int.h"

#include <stdlib.h>

#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "stb_image_resize.h"

#if 0
typedef struct
{
   int      (*read)  (void *user,char *data,int size);   // fill 'data' with 'size' bytes.  return number of bytes actually read
   void     (*skip)  (void *user,int n);                 // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
   int      (*eof)   (void *user);                       // returns nonzero if we are at end of file/data
} stbi_io_callbacks;
#endif

static int io_image_read(void *user, char *data, int size) {
    jpr_file *f = (jpr_file *)user;
    return (int)file_read(f,data,(jpr_uint64)size);
}

static void io_image_skip(void *user, int n) {
    jpr_file *f = (jpr_file *)user;
    file_seek(f,n,JPR_FILE_CUR);
    return;
}

static int io_eof(void *user) {
    jpr_file *f = (jpr_file *)user;
    return file_eof(f);
}

static stbi_io_callbacks io_callbacks = {
    io_image_read,
    io_image_skip,
    io_eof,
};



static void
flip_and_bgr(jpr_uint8 *image, unsigned int width, unsigned int height, unsigned int channels) {

    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int maxheight = height >> 1;
    unsigned int byte;
    unsigned int ibyte;
    unsigned int bytes_per_row = width * channels;
    jpr_uint8 *temp_row = (jpr_uint8 *)malloc(bytes_per_row);
    jpr_uint8 temp;

    for(y=0; y<maxheight;y++) {
        byte = (y * width* channels);
        ibyte = (height - y - 1) * width * channels;
        mem_cpy(temp_row,image + ibyte,bytes_per_row);
        mem_cpy(image + ibyte,image + byte,bytes_per_row);
        mem_cpy(image + byte,temp_row,bytes_per_row);
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
image_blend(jpr_uint8 *dst, jpr_uint8 *src, unsigned int len, jpr_uint8 a) {
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
    jpr_file *f;
    f = file_open(filename,"r");
    if(f == NULL) return 0;

    if(stbi_info_from_callbacks(&io_callbacks,f,&x,&y,&c) == 0) {
        file_close(f);
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
    file_close(f);

    return 1;
}

static jpr_uint8 *
stbi_xload(
  const char *filename,
  unsigned int *width,
  unsigned int *height,
  unsigned int *channels,
  unsigned int *frames,
  unsigned int **delays) {

    jpr_file *f;
    stbi__context s;
    unsigned char *result = 0;
    int x;
    int y;
    int c;

    stbi__result_info ri;
    mem_set(&ri,0,sizeof(stbi__result_info));

    f = file_open(filename,"r");
    if(f == NULL) return NULL;

    stbi__start_callbacks(&s, &io_callbacks, f);

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

    file_close(f);
    return result;
}

jpr_uint8 *
image_load(
  const char *filename,
  unsigned int *width,
  unsigned int *height,
  unsigned int *channels,
  unsigned int *frames) {

    jpr_uint8 *image = NULL;
    jpr_uint8 *t = NULL;
    jpr_uint8 *b = NULL;
    jpr_uint8 *d = NULL;

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
        image = (jpr_uint8 *)malloc( ((ow * oh * oc) * of) + (2 * of));

    }
    else {
        image = (jpr_uint8 *)malloc(ow * oh * oc);
    }
    b = t;
    d = image;

    for(i=0;i<of;i++) {
        if(ow  != og_w || oh != og_h) {
            stbir_resize_uint8(b,og_w,og_h,0,d,ow,oh,0,oc);
        }
        else {
            mem_cpy(d, b , ow * oh * oc);
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



