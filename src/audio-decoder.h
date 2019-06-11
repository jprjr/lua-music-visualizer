#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include "dr_flac.h"
#include "dr_mp3.h"
#include "dr_wav.h"

/* audio_decoder is responsible for pulling in (x) frames of audio
 * data (a pull-style API) */
typedef struct audio_decoder_s audio_decoder;

typedef void(*meta_proc)(void *, const char *key, const char *val);

struct audio_decoder_s {
    void *meta_ctx;
    unsigned int samplerate;
    unsigned int channels;
    unsigned int framecount;
    meta_proc onmeta;
    int16_t *samples; /* holds all samples */
    unsigned int frame_pos;
};


int audio_decoder_init(audio_decoder *);
int audio_decoder_open(audio_decoder *, const char *filename);
unsigned int audio_decoder_decode(audio_decoder *a, unsigned int framecount, int16_t *buf);
void audio_decoder_close(audio_decoder *a);

#endif
