#ifndef VIDEO_GENERATOR_H
#define VIDEO_GENERATOR_H

#include "int.h"
#include "audio-processor.h"
#include "audio-resampler.h"
#include "audio-decoder.h"
#include "mpdc.h"
#include "jpr_proc.h"
#define THREAD_U64 jpr_uint64
#include "thread.h"
#include <lua.h>

typedef struct video_generator_s video_generator;

struct video_generator_s {
    audio_processor *processor;
    audio_resampler *sampler;
    audio_decoder *decoder;
    unsigned int width;
    unsigned int height;
    unsigned int fps;
    jpr_proc_pipe *out;
    jpr_uint8 *framebuf;
    unsigned int framebuf_len;
    unsigned int framebuf_video_len;
    unsigned int framebuf_audio_len;
    lua_State *L;
    int lua_ref;
    unsigned int samples_per_frame;
    double ms_per_frame;
    double elapsed;
    double duration;
    mpdc_connection *mpd;
    unsigned int mpd_tags;
    int song_table_ref;
};

int video_generator_init(video_generator *, audio_processor *, audio_resampler *, audio_decoder *, int jit, const char *modulename, const char *filename, const char *luascript, jpr_proc_pipe *out);
void video_generator_close(video_generator *);
int video_generator_loop(video_generator *, float *progress);
int video_generator_reload(video_generator *);

int video_generator_using_luajit(void);


#endif
