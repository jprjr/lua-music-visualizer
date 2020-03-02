#include <stdlib.h>
#include "audio-decoder.h"
#include "id3.h"
#include "str.h"
#include "jpr_mp3.h"

#define DR_MP3_NO_STDIO
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

struct mp3_probe_s {
    audio_info *info;
    meta_proc onmeta;
    void *meta_ctx;
};

typedef struct mp3_probe_s mp3_probe;

static void mp3_probe_f(void *ctx, const char *key, const char *val) {
    mp3_probe *probe = (mp3_probe *)ctx;
    if(str_equals(key,"artist")) {
        probe->info->artist = str_dup(val);
    }
    else if(str_equals(key,"album")) {
        probe->info->album = str_dup(val);
    }
    else if(str_equals(key,"title")) {
        probe->info->tracks[0] = str_dup(val);
    }
    if(probe->onmeta != NULL) {
        probe->onmeta(probe->meta_ctx,key,val);
    }
}


static size_t read_wrapper(void *userdata, void *buf, size_t bytes) {
    audio_decoder *a = (audio_decoder *)userdata;
    return (size_t)a->read(a,buf,(jpr_uint64)bytes);
}

static drmp3_bool32 seek_wrapper(void *userdata, int offset, drmp3_seek_origin origin) {
    audio_decoder *a = (audio_decoder *)userdata;
    enum JPR_FILE_POS w = JPR_FILE_SET;
    switch(origin) {
        case drmp3_seek_origin_start: w = JPR_FILE_SET; break;
        case drmp3_seek_origin_current: w = JPR_FILE_CUR; break;
    }
    return (drwav_bool32)(a->seek(a,(jpr_int64)offset,w) != -1);
}

static void jprmp3_close(audio_plugin_ctx *ctx) {
    if(ctx->priv != NULL) {
        drmp3_uninit((drmp3 *)ctx->priv);
        free(ctx->priv);
    }
    free(ctx);
}

static jpr_uint64 jprmp3_decode(audio_plugin_ctx *ctx, jpr_uint64 framecount, jpr_int16 *buf) {
    return (jpr_uint64)drmp3_read_pcm_frames_s16((drmp3 *)ctx->priv,(drmp3_uint64)framecount,(drmp3_int16 *)buf);
}

static audio_info *jprmp3_probe(audio_decoder *decoder) {
    audio_info *info;
    mp3_probe *metaprobe;

    info = (audio_info *)malloc(sizeof(audio_info));
    if(UNLIKELY(info == NULL)) {
        return NULL;
    }

    metaprobe = (mp3_probe *)malloc(sizeof(mp3_probe));
    if(UNLIKELY(info == NULL)) {
        free(info);
        return NULL;
    }

    info->tracks = (char **)malloc(sizeof(char *) * 2);
    if(UNLIKELY(info->tracks == NULL)) {
        free(info);
        free(metaprobe);
        return NULL;
    }

    info->artist = NULL;
    info->album  = NULL;
    info->tracks[0] = NULL;
    info->tracks[1] = NULL;

    metaprobe->info = info;
    metaprobe->onmeta = decoder->onmeta;
    metaprobe->meta_ctx = decoder->meta_ctx;

    decoder->onmeta = mp3_probe_f;
    decoder->meta_ctx = metaprobe;

    process_id3(decoder);

    decoder->onmeta = metaprobe->onmeta;
    decoder->meta_ctx = metaprobe->meta_ctx;

    free(metaprobe);
    return info;
}

static audio_plugin_ctx *jprmp3_open(audio_decoder *decoder) {
    audio_plugin_ctx *ctx = NULL;
    drmp3 *pMp3 = NULL;

    ctx = (audio_plugin_ctx *)malloc(sizeof(audio_plugin_ctx));
    if(UNLIKELY(ctx == NULL)) {
        goto jprmp3_error;
    }
    ctx->priv = NULL;

    pMp3 = (drmp3 *)malloc(sizeof(drmp3));
    if(UNLIKELY(ctx == NULL)) {
        goto jprmp3_error;
    }
    mem_set(pMp3,0,sizeof(drmp3));

    process_id3(decoder);

    if(!drmp3_init(pMp3,read_wrapper,seek_wrapper,decoder,NULL,NULL)) {
        goto jprmp3_error;
    }

    decoder->framecount = drmp3_get_pcm_frame_count(pMp3);
    decoder->samplerate = pMp3->sampleRate;
    decoder->channels = pMp3->channels;
    ctx->decoder = decoder;
    ctx->priv = (void *)pMp3;

    goto jprmp3_done;

    jprmp3_error:
    free(pMp3);
    jprmp3_close(ctx);
    ctx = NULL;

    jprmp3_done:
    return ctx;
}

static const char *const extensions[] = {
    ".mp3",
    NULL
};

const audio_plugin jprmp3_plugin = {
    "mp3",
    jprmp3_open,
    jprmp3_decode,
    jprmp3_close,
    jprmp3_probe,
    extensions,
};