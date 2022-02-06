#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include "audio-resampler.h"

typedef struct audio_processor_s audio_processor;

struct audio_processor_s {
    audio_resampler *sampler;
    unsigned int buffer_len; /* buffer len does NOT account for channels */
    jpr_int16 *buffer; /* stores incoming samples - buffer_len * channels */
};

int audio_processor_init(audio_processor *, audio_resampler *,unsigned int samples_per_frame);
void audio_processor_close(audio_processor *);
jpr_uint64 audio_processor_process(audio_processor *, jpr_uint64 framecount);

#endif
