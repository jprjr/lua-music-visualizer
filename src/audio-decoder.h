#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include "int.h"
#include "file.h"

/* audio_decoder is responsible for pulling in (x) frames of audio
 * data (a pull-style API) */
typedef struct audio_decoder_s audio_decoder;
typedef struct audio_plugin_ctx_s audio_plugin_ctx;

struct audio_plugin_ctx_s {
    audio_decoder *decoder;
    void *priv;
};

struct track_info_s {
    unsigned int number;
    char *title;
};

typedef struct track_info_s track_info;

struct audio_info_s {
    char *artist;
    char *album;
    unsigned int total;
    track_info* tracks;
};

typedef struct audio_info_s audio_info;

typedef jpr_uint64 (*read_cb)(audio_decoder *decoder, void *buf, jpr_uint64 bytes);
typedef jpr_int64 (*seek_cb)(audio_decoder *decoder, jpr_int64 offset, enum JPR_FILE_POS whence);
typedef jpr_int64 (*tell_cb)(audio_decoder *decoder);
typedef jpr_uint8 *(*slurp_cb)(audio_decoder *decoder, size_t *size);

typedef audio_plugin_ctx *(*plugin_open_func)(audio_decoder *decoder, const char *filename);
typedef jpr_uint64 (*plugin_decode_func)(audio_plugin_ctx *ctx, jpr_uint64 framecount, jpr_int16 *buf);
typedef void (*plugin_close_func)(audio_plugin_ctx *ctx);
typedef audio_info * (*plugin_probe_func)(audio_decoder *decoder);

typedef struct audio_plugin_s audio_plugin;

struct audio_plugin_s {
    const char *name;
    plugin_open_func open;
    plugin_decode_func decode;
    plugin_close_func close;
    plugin_probe_func probe;
    const char * const *extensions;
};

extern const audio_plugin* const plugin_list[];

#ifndef DECODE_PCM
#define DECODE_PCM 1
#endif

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
#define DECODE_SPC 0
#endif

#ifndef DECODE_NEZ
#define DECODE_NEZ 0
#endif

#ifndef DECODE_NSF
#define DECODE_NSF 0
#endif

#ifndef DECODE_VGM
#define DECODE_VGM 0
#endif

#if DECODE_PCM
#include "jpr_pcm.h"
#endif

#if DECODE_FLAC
#include "jpr_flac.h"
#endif

#if DECODE_MP3
#include "jpr_mp3.h"
#endif

#if DECODE_WAV
#include "dr_wav.h"
#endif

#if DECODE_SPC
#include "jpr_spc.h"
#endif

#if DECODE_NEZ
#include "jpr_nez.h"
#endif

#if DECODE_NSF
#include "jpr_nsf.h"
#endif

#if DECODE_VGM
#include "jpr_vgm.h"
#endif

#include "file.h"


typedef void(*meta_proc)(void *, const char *key, const char *val);
typedef void(*change_proc)(void *, const char *type);
typedef void(*meta_proc_double)(void *, const char *key, double val);

struct audio_decoder_s {
    const audio_plugin *plugin;
    audio_plugin_ctx *plugin_ctx;
    void *meta_ctx;
    unsigned int samplerate;
    unsigned int channels;
    unsigned int track;
    jpr_uint32 framecount;
    meta_proc onmeta;
    meta_proc_double onmeta_double;
    change_proc onchange;
    jpr_file *file;
    read_cb read;
    seek_cb seek;
    tell_cb tell;
    slurp_cb slurp;
};


int audio_decoder_init(audio_decoder *);
audio_info * audio_decoder_probe(audio_decoder *, const char *filename);
int audio_decoder_open(audio_decoder *, const char *filename);
jpr_uint32 audio_decoder_decode(audio_decoder *a, jpr_uint32 framecount, jpr_int16 *buf);
void audio_decoder_free_info(audio_info *i);
void audio_decoder_close(audio_decoder *a);

#endif
