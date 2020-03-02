#include "jpr_pcm.h"
#include "attr.h"
#include <stdlib.h>

static audio_plugin_ctx *jprpcm_open(audio_decoder *decoder) {
    audio_plugin_ctx *ctx;
    jpr_uint32 bytes;
    jpr_uint64 framecount;

    if(decoder->samplerate == 0 || decoder->channels == 0) return NULL;

    ctx = (audio_plugin_ctx *)malloc(sizeof(audio_plugin_ctx));
    if(UNLIKELY(ctx == NULL)) {
        return NULL;
    }

    ctx->decoder = decoder;
    ctx->priv = NULL;
    bytes = decoder->seek(decoder,0,JPR_FILE_END);
    if(bytes > 0) {
        framecount = (jpr_uint64)bytes;
        framecount /= decoder->samplerate;
        framecount /= decoder->channels;
        framecount /= sizeof(jpr_int16);
        decoder->framecount = framecount;
        decoder->seek(decoder,0,JPR_FILE_SET);
    }

    return ctx;
}

static jpr_uint64 jprpcm_decode(audio_plugin_ctx *ctx, jpr_uint64 framecount, jpr_int16 *buf) {
    jpr_uint64 r;
	jpr_uint64 req;
    req = sizeof(jpr_int16) * ctx->decoder->channels * (size_t)framecount;
    r = ctx->decoder->read(ctx->decoder,buf,req);
    return r;
}

static void jprpcm_close(audio_plugin_ctx *ctx) {
    free(ctx);
}

static const char *const extensions[] = {
    "",
    NULL
};

const audio_plugin jprpcm_plugin = {
    "pcm",
    jprpcm_open,
    jprpcm_decode,
    jprpcm_close,
    NULL,
    extensions,
};

