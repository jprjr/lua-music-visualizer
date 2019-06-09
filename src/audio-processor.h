#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include "audio-decoder.h"
#include "kiss_fftr.h"

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
    int16_t *buffer; /* stores incoming samples - buffer_len * channels */
    kiss_fft_scalar *mbuffer; /* stores a downmixed version of samples */
    kiss_fft_scalar *wbuffer; /* stores window function factors */
    kiss_fft_cpx *obuffer; /* buffer_len/2 + 1 output points */
    kiss_fftr_cfg plan;
    frange *spectrum;
    int firstflag;
};

int audio_processor_init(audio_processor *, audio_decoder *,unsigned int samples_per_frame);
void audio_processor_close(audio_processor *);
unsigned int audio_processor_process(audio_processor *, unsigned int framecount);

#endif
