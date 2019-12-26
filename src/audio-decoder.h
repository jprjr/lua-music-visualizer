#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#ifndef DECODE_FLAC
#define DECODE_FLAC 1
#endif

#ifndef DECODE_MP3
#define DECODE_MP3 1
#endif

#ifndef DECODE_WAV
#define DECODE_WAV 1
#endif

#if DECODE_FLAC
#define DRFLAC_MALLOC(sz) mem_alloc((sz))
#define DRFLAC_REALLOC(p,sz) mem_realloc((p),(sz))
#define DRFLAC_FREE(p) mem_free((p))
#define DRFLAC_COPY_MEMORY(dst,src,sz) mem_cpy((dst),(src),(sz))
#define DRFLAC_ZERO_MEMORY(p,sz) mem_set((p),0,(sz))
#include "dr_flac.h"
#endif

#if DECODE_MP3
#define DRMP3_MALLOC(sz) mem_alloc((sz))
#define DRMP3_REALLOC(p,sz) mem_realloc((p),(sz))
#define DRMP3_FREE(p) mem_free((p))
#define DRMP3_COPY_MEMORY(dst,src,sz) mem_cpy((dst),(src),(sz))
#define DRMP3_MOVE_MEMORY(dst,src,sz) mem_move((dst),(src),(sz))
#define DRMP3_ZERO_MEMORY(p,sz) mem_set((p),0,(sz))
#include "dr_mp3.h"
#endif

#if DECODE_WAV
#define DRWAV_MALLOC(sz) mem_alloc((sz))
#define DRWAV_REALLOC(p,sz) mem_realloc((p),(sz))
#define DRWAV_FREE(p) mem_free((p))
#define DRWAV_COPY_MEMORY(dst,src,sz) mem_cpy((dst),(src),(sz))
#define DRWAV_ZERO_MEMORY(p,sz) mem_set((p),0,(sz))
#include "dr_wav.h"
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
