#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include "audio-decoder.h"

#ifdef USE_FFTW3
#include <complex.h>
#include <fftw3.h>
#define SCALAR_TYPE double
#define COMPLEX_TYPE fftw_complex
#define PLAN_TYPE fftw_plan
#define MALLOC fftw_malloc
#else
#include "kiss_fftr.h"
#define SCALAR_TYPE kiss_fft_scalar
#define COMPLEX_TYPE kiss_fft_cpx
#define PLAN_TYPE kiss_fftr_cfg
#define MALLOC mem_alloc
#endif

typedef struct audio_processor_s audio_processor;
typedef struct frange_s frange;

struct frange_s {
    double freq;
    double amp;
    double prevamp;
    double boost;
    unsigned int first_bin;
    unsigned int last_bin;
};

struct audio_processor_s {
    audio_decoder *decoder;
    unsigned int spectrum_bars;
    unsigned int buffer_len; /* buffer len does NOT account for channels */
    jpr_int16 *buffer; /* stores incoming samples - buffer_len * channels */
    SCALAR_TYPE *mbuffer; /* stores a downmixed version of samples */
    SCALAR_TYPE *wbuffer; /* stores window function factors */
    COMPLEX_TYPE *obuffer; /* buffer_len/2 + 1 output points */
    PLAN_TYPE plan;
    frange *spectrum;
    int firstflag;
};

int audio_processor_init(audio_processor *, audio_decoder *,unsigned int samples_per_frame);
void audio_processor_close(audio_processor *);
unsigned int audio_processor_process(audio_processor *, unsigned int framecount);

#endif
