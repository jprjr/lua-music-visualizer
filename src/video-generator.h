#ifndef VIDEO_GENERATOR_H
#define VIDEO_GENERATOR_H

#include "audio-processor.h"
#include "audio-decoder.h"
#include "thread.h"
#include <lua.h>
#include "lua-image.h"

typedef struct video_generator_s video_generator;

struct video_generator_s {
    audio_processor *processor;
    audio_decoder *decoder;
    unsigned int width;
    unsigned int height;
    unsigned int fps;
    void *outHandle;
    uint8_t *framebuf;
    unsigned int framebuf_len;
    lua_State *L;
    int load_ref;
    int frame_ref;
    thread_queue_t image_queue;
    image_q images[100];
    unsigned int samples_per_frame;
    double ms_per_frame;
    double elapsed;
    uint8_t vid_header[8];
    uint8_t aud_header[8];
    void (*image_cb)(lua_State *L, intptr_t table_ref, unsigned int frames, uint8_t *image);
};

int video_generator_init(video_generator *, audio_processor *, audio_decoder *, const char *filename, const char *luascript, void *outHandle);
void video_generator_close(video_generator *);
int video_generator_loop(video_generator *);


#endif
