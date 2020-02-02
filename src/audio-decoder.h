#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include "int.h"

#ifndef DECODE_FLAC
#define DECODE_FLAC 1
#endif

#ifndef DECODE_MP3
#define DECODE_MP3 1
#endif

#ifndef DECODE_WAV
#define DECODE_WAV 1
#endif

#ifndef DECODE_SPC
#define DECODE_SPC 1
#endif

#if DECODE_FLAC
#include "dr_flac.h"
#endif

#if DECODE_MP3
#include "dr_mp3.h"
#endif

#if DECODE_WAV
#include "dr_wav.h"
#endif

#if DECODE_SPC
#include "jpr_spc.h"
#endif

#include "jpr_pcm.h"

#include "file.h"

/* audio_decoder is responsible for pulling in (x) frames of audio
 * data (a pull-style API) */
typedef struct audio_decoder_s audio_decoder;
typedef union decoder_ctx_u decoder_ctx;

union decoder_ctx_u {
    void *p;
#if DECODE_FLAC
    drflac *pFlac;
#endif
#if DECODE_MP3
    drmp3 *pMp3;
#endif
#if DECODE_WAV
    drwav *pWav;
#endif
#if DECODE_SPC
    jprspc *pSpc;
#endif
    jprpcm *pPcm;
};


typedef void(*meta_proc)(void *, const char *key, const char *val);

struct audio_decoder_s {
    decoder_ctx ctx;
    void *meta_ctx;
    int type;
    unsigned int samplerate;
    unsigned int channels;
    jpr_uint64 framecount;
    meta_proc onmeta;
    jpr_file *file;
};


int audio_decoder_init(audio_decoder *);
int audio_decoder_open(audio_decoder *, const char *filename);
jpr_uint64 audio_decoder_decode(audio_decoder *a, jpr_uint64 framecount, jpr_int16 *buf);
void audio_decoder_close(audio_decoder *a);

#endif
