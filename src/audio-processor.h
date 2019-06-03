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
    int16_t buffer[8192]; /* stores incoming samples */
    kiss_fft_scalar mbuffer[4096]; /* stores a downmixed version of samples */
    kiss_fft_scalar wbuffer[4096]; /* stores window function factors */
    kiss_fft_cpx obuffer[2049]; /* n/2 + 1 output points */
    kiss_fftr_cfg plan;
    frange spectrum[20];
    int firstflag;
};

int audio_processor_init(audio_processor *, audio_decoder *);
void audio_processor_close(audio_processor *);
unsigned int audio_processor_process(audio_processor *, unsigned int framecount);

#endif
