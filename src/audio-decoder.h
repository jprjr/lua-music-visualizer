#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include "file.h"
#include "dr_flac.h"
#include "dr_mp3.h"
#include "dr_wav.h"
#include "jpr_pcm.h"

/* audio_decoder is responsible for pulling in (x) frames of audio
 * data (a pull-style API) */
typedef struct audio_decoder_s audio_decoder;
typedef union decoder_ctx_u decoder_ctx;

union decoder_ctx_u {
    void *p;
    drflac *pFlac;
    drmp3 *pMp3;
    drwav *pWav;
    jprpcm *pPcm;
};


typedef void(*meta_proc)(void *, const char *key, const char *val);

struct audio_decoder_s {
    decoder_ctx ctx;
    void *meta_ctx;
    int type;
    unsigned int samplerate;
    unsigned int channels;
    unsigned int framecount;
    meta_proc onmeta;
    jpr_file *file;
};


int audio_decoder_init(audio_decoder *);
int audio_decoder_open(audio_decoder *, const char *filename);
unsigned int audio_decoder_decode(audio_decoder *a, unsigned int framecount, int16_t *buf);
void audio_decoder_close(audio_decoder *a);

#endif
