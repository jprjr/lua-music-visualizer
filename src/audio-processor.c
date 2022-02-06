#include "audio-processor.h"
#include "audio-resampler.h"
#include "str.h"
#include "attr.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>

#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

#ifdef USE_FFTW3
#include <complex.h>
#include <fftw3.h>
#define MALLOC fftw_malloc
#define FREE fftw_free
#else
#define MALLOC malloc
#define FREE free
#endif

int audio_processor_init(audio_processor *p, audio_resampler *r,unsigned int samples_per_frame) {
    if(r->samplerate == 0) return 1;
    if(r->decoder->channels == 0) return 1;

    p->buffer_len = 4096;
    while(p->buffer_len < samples_per_frame) {
        p->buffer_len *= 2;
    }
    p->sampler = r;

    p->buffer = (jpr_int16 *)MALLOC(sizeof(jpr_int16) * p->buffer_len * r->decoder->channels);
    if(UNLIKELY(p->buffer == NULL)) return 1;
    mem_set(p->buffer,0,sizeof(jpr_int16)*(p->buffer_len * p->sampler->decoder->channels));

    return 0;
}

void audio_processor_close(audio_processor *p) {
    if(p->buffer != NULL) {
        FREE(p->buffer);
        p->buffer = NULL;
    }
}

jpr_uint64 audio_processor_process(audio_processor *p, jpr_uint64 framecount) {
    /* first shift everything in the buffer down */
    jpr_uint64   i = 0;
    jpr_uint64   r = 0;
    jpr_uint64   o = (p->buffer_len * p->sampler->decoder->channels) - (framecount * p->sampler->decoder->channels);

    while(i+framecount < (p->buffer_len * p->sampler->decoder->channels)) {
        p->buffer[i] = p->buffer[framecount+i];
        i++;
    }

    r = audio_resampler_decode(p->sampler,framecount,&(p->buffer[o]));

    if(r < framecount) { /* didn't get enough samples, we're at the end, zero out the rest of the buffer */
        i = r;
        while(i<framecount) {
            p->buffer[o+(i*p->sampler->decoder->channels)] = 0;
            if(p->sampler->decoder->channels > 1) {
                p->buffer[o+(i*p->sampler->decoder->channels) + 1] = 0;
            }
            i++;
        }
    }

    return r;
}
