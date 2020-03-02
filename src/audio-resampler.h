#ifndef AUDIO_RESAMPLER_H
#define AUDIO_RESAMPLER_H

#ifndef ENABLE_LIBSAMPLERATE
#define ENABLE_LIBSAMPLERATE 1
#endif

#include "audio-decoder.h"
#include "int.h"
#if ENABLE_LIBSAMPLERATE
#include <samplerate.h>
#endif

typedef struct audio_resampler_s audio_resampler;
typedef jpr_uint64 (*audio_resampler_func)(audio_resampler *, jpr_uint64 framecount, jpr_int16 *buf);

struct audio_resampler_s {
    unsigned int samplerate;
    audio_decoder *decoder;
    audio_resampler_func resample;
    double ratio;
    jpr_int16 *buf;
    float *in;
    float *out;
#if ENABLE_LIBSAMPLERATE
    SRC_STATE *src;
#else
    void *src;
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

int audio_resampler_init(audio_resampler *);
int audio_resampler_open(audio_resampler *, audio_decoder *);
void audio_resampler_close(audio_resampler *);

jpr_uint64 audio_resampler_decode(audio_resampler *, jpr_uint64 framecount, jpr_int16 *buf);

#ifdef __cplusplus
}
#endif

#endif
