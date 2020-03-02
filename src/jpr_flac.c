#include "attr.h"
#include "str.h"
#include "jpr_flac.h"

#define DR_FLAC_NO_STDIO
#define DR_FLAC_NO_OGG
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

struct flac_probe_s {
    audio_info *info;
    meta_proc onmeta;
    void *meta_ctx;
};

typedef struct flac_probe_s flac_probe;

static void flac_probe_f(void *ctx, const char *key, const char *val) {
    flac_probe *probe = (flac_probe *)ctx;
    if(str_equals(key,"artist")) {
        probe->info->artist = str_dup(val);
    }
    else if(str_equals(key,"album")) {
        probe->info->album = str_dup(val);
    }
    else if(str_equals(key,"title")) {
        probe->info->tracks[0].number = 1;
        probe->info->tracks[0].title  = str_dup(val);
    }
    if(probe->onmeta != NULL && probe->meta_ctx != NULL) {
        probe->onmeta(probe->meta_ctx,key,val);
    }
}

static size_t read_wrapper(void *userdata, void *buf, size_t bytes) {
    audio_decoder *a = (audio_decoder *)userdata;
    return (size_t)a->read(a,buf,(jpr_uint64)bytes);
}

static drflac_bool32 seek_wrapper(void *userdata, int offset, drflac_seek_origin origin) {
    audio_decoder *a = (audio_decoder *)userdata;
    enum JPR_FILE_POS w = JPR_FILE_SET;
    switch(origin) {
        case drflac_seek_origin_start: w = JPR_FILE_SET; break;
        case drflac_seek_origin_current: w = JPR_FILE_CUR; break;
    }
    return (drflac_bool32)(a->seek(a,(jpr_int64)offset,w) != -1);
}

static void flac_meta(void *ctx, drflac_metadata *pMetadata) {
    audio_decoder *a = ctx;
    drflac_vorbis_comment_iterator iter;
    char buf[4096];
    char *r;
    const char *comment = NULL;
    unsigned int commentLength = 0;

    if(pMetadata->type != DRFLAC_METADATA_BLOCK_TYPE_VORBIS_COMMENT) return;
    if(a->onmeta == NULL) return;

    drflac_init_vorbis_comment_iterator(&iter,pMetadata->data.vorbis_comment.commentCount, pMetadata->data.vorbis_comment.pComments);
    while( (comment = drflac_next_vorbis_comment(&iter,&commentLength)) != NULL) {
        if(commentLength > 4095) continue;
        str_ncpy(buf,comment,commentLength);
        buf[commentLength] = 0;
        r = str_chr(buf,'=');
        if(r != NULL) {
            *r = '\0';
            str_lower(buf,buf);
            a->onmeta(a->meta_ctx,buf,&r[1]);
        }
    }
}

static audio_info *jprflac_probe(audio_decoder *decoder) {
    audio_info *info;
    flac_probe *metaprobe;
    drflac *pFlac;

    info = (audio_info *)malloc(sizeof(audio_info));
    if(UNLIKELY(info == NULL)) {
        return NULL;
    }
    mem_set(info,0,sizeof(audio_info));
    info->total = 1;

    metaprobe = (flac_probe *)malloc(sizeof(flac_probe));
    if(UNLIKELY(info == NULL)) {
        free(info);
        return NULL;
    }

    info->tracks = (track_info *)malloc(sizeof(track_info) * 1);
    if(UNLIKELY(info->tracks == NULL)) {
        free(info);
        free(metaprobe);
        return NULL;
    }
    mem_set(info->tracks,0,sizeof(track_info) * 1);

    metaprobe->info = info;
    metaprobe->onmeta = decoder->onmeta;
    metaprobe->meta_ctx = decoder->meta_ctx;

    decoder->onmeta = flac_probe_f;
    decoder->meta_ctx = metaprobe;

    pFlac = drflac_open_with_metadata(read_wrapper,seek_wrapper,flac_meta,decoder,NULL);
    if(pFlac != NULL) {
        drflac_close(pFlac);
    }

    decoder->onmeta = metaprobe->onmeta;
    decoder->meta_ctx = metaprobe->meta_ctx;

    free(metaprobe);
    return info;
}

static void jprflac_close(audio_plugin_ctx *ctx) {
    if(ctx->priv != NULL) drflac_close((drflac *)ctx->priv);
    free(ctx);
}

static jpr_uint64 jprflac_decode(audio_plugin_ctx *ctx, jpr_uint64 framecount, jpr_int16 *buf) {
    return (jpr_uint64)drflac_read_pcm_frames_s16((drflac *)ctx->priv,(drflac_uint64)framecount,(drflac_int16 *)buf);
}

static audio_plugin_ctx *jprflac_open(audio_decoder *decoder) {
    audio_plugin_ctx *ctx = NULL;
    drflac *pFlac = NULL;
    ctx = (audio_plugin_ctx *)malloc(sizeof(audio_plugin_ctx));
    if(UNLIKELY(ctx == NULL)) {
        goto jprflac_error;
    }
    ctx->priv = NULL;

    pFlac = drflac_open_with_metadata(read_wrapper,seek_wrapper,flac_meta,decoder,NULL);
    if(pFlac == NULL) {
        goto jprflac_error;
    }
    ctx->priv = pFlac;

    decoder->framecount = pFlac->totalPCMFrameCount;
    decoder->samplerate = pFlac->sampleRate;
    decoder->channels   = pFlac->channels;

    goto jprflac_done;

    jprflac_error:
    jprflac_close(ctx);
    ctx = NULL;

    jprflac_done:
    return ctx;
}

static const char *const extensions[] = {
    ".flac",
    NULL
};

const audio_plugin jprflac_plugin = {
    "flac",
    jprflac_open,
    jprflac_decode,
    jprflac_close,
    jprflac_probe,
    extensions,
};
