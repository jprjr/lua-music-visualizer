#include "attr.h"
#include "jpr_spc.h"
#include "int.h"
#include "str.h"

#include <snes_spc/spc.h>
#include <id666/id666.h>
#include <stdlib.h>

struct spc_private_s {
    snes_spc_t *spc;
    spc_filter_t *filter;
    id666 *id6;
    jpr_uint64 frames_rem;
    jpr_uint64 frames_fade;
};

typedef struct spc_private_s spc_private;

static void fade_samples(jpr_uint64 frames_rem, jpr_uint64 frames_fade, jpr_uint64 frames, jpr_int16 *buf) {
    jpr_uint64 i = 0;
    jpr_uint64 f = frames_fade;
    jpr_uint64 fade_vol;

    if(frames_rem - frames > frames_fade) return;

    if(frames_rem > frames_fade) {
        i = frames_rem - frames_fade;
        f += i;
    }
    else {
        f = frames_rem;
    }

    while(i<frames) {
        fade_vol = (jpr_uint64)((f - i)) * 0x10000;
        fade_vol /= frames_fade;
        fade_vol *= fade_vol;
        fade_vol = fade_vol * 0x10000 >> 32;
        buf[i * 2] = (jpr_int16)(((jpr_uint64)buf[i * 2] * fade_vol) >> 16);
        buf[(i * 2)+1] = (jpr_int16)(((jpr_uint64)buf[(i * 2)+1] * fade_vol) >> 16);
        i++;
    }

    return;
}

static void jprspc_free(audio_plugin_ctx *jspc) {
    spc_private *priv;
    if(jspc != NULL) {
        if(jspc->priv != NULL) {
            priv = (spc_private *)jspc->priv;
            if(priv->id6 != NULL) free(priv->id6);
            if(priv->filter != NULL) spc_filter_delete(priv->filter);
            if(priv->spc != NULL) spc_delete(priv->spc);
            free(jspc->priv);
        }
        free(jspc);
    }
}

static audio_info *jprspc_probe(audio_decoder *decoder) {
    audio_info *info;
    jpr_uint8 *spc_data;
    size_t spc_data_len;
    id666 *id6;

    spc_data = decoder->slurp(decoder,&spc_data_len);
    if(spc_data_len == 0) return NULL;

    id6 = (id666 *)malloc(sizeof(id666));
    if(UNLIKELY(id6 == NULL)) {
        free(spc_data);
        return NULL;
    }
    if(UNLIKELY(id666_parse(id6,spc_data,spc_data_len))) {
        free(spc_data);
        free(id6);
        return NULL;
    }

    info = (audio_info *)malloc(sizeof(audio_info));
    if(UNLIKELY(info == NULL)) {
        free(spc_data);
        free(id6);
        return NULL;
    }
    mem_set(info,0,sizeof(audio_info));
    info->total = 1;

    info->tracks = (track_info *)malloc(sizeof(track_info) * 1);
    if(UNLIKELY(info->tracks == NULL)) {
        free(spc_data);
        free(id6);
        free(info);
        return NULL;
    }
    mem_set(info->tracks,0,sizeof(track_info) * 1);

    info->tracks[0].number = 1;
    info->tracks[0].title = str_dup(id6->song);

    if(id6->game[0] != '\0') {
        info->album = str_dup(id6->game);
    }
    if(id6->artist[0] != '\0') {
        info->artist = str_dup(id6->artist);
    }

    free(spc_data);
    free(id6);

    return info;
}


static audio_plugin_ctx *jprspc_open(audio_decoder *decoder, const char *filename) {

    audio_plugin_ctx *jspc;
    spc_private *priv;
    jpr_uint8 *spc_data;
    size_t spc_data_len;

    (void)filename;

    jspc = NULL;
    priv = NULL;
    spc_data = NULL;
    spc_data_len = 0;

    jspc = (audio_plugin_ctx *)malloc(sizeof(audio_plugin_ctx));
    if(UNLIKELY(jspc == NULL)) return NULL;

    jspc->priv = NULL;

    jspc->priv = malloc(sizeof(spc_private));
    if(UNLIKELY(jspc->priv == NULL)) {
        goto jspc_error;
    }
    priv = (spc_private *)jspc->priv;
    priv->spc = NULL;
    priv->filter = NULL;

    priv->spc = spc_new();
    if(UNLIKELY(priv->spc == NULL)) {
        goto jspc_error;
    }
    priv->filter = spc_filter_new();
    if(UNLIKELY(priv->filter == NULL)) {
        goto jspc_error;
    }

    jspc->decoder = decoder;

    spc_data_len = (size_t)decoder->seek(jspc->decoder,0,2);
    if(spc_data_len == 0) goto jspc_error;
    decoder->seek(jspc->decoder,0,0);

    spc_data = (jpr_uint8 *)malloc(spc_data_len);
    if(UNLIKELY(spc_data == NULL)) {
        goto jspc_error;
    }

    if(decoder->read(jspc->decoder,spc_data,spc_data_len) != spc_data_len) {
        goto jspc_error;
    }

    priv->id6 = (id666 *)malloc(sizeof(id666));
    if(UNLIKELY(priv->id6 == NULL)) {
        goto jspc_error;
    }
    if(UNLIKELY(id666_parse(priv->id6,spc_data,spc_data_len))) {
        goto jspc_error;
    }

    if(spc_load_spc(priv->spc,spc_data,spc_data_len) != NULL) {
        goto jspc_error;
    }
    free(spc_data);
    spc_data = NULL;
    spc_clear_echo(priv->spc);
    spc_filter_clear(priv->filter);
    spc_filter_set_gain(priv->filter,0x180);

    priv->frames_rem  = priv->id6->total_len / 2;
    priv->frames_fade = priv->id6->fade / 2;

    decoder->framecount = priv->frames_rem;
    decoder->samplerate = 32000;
    decoder->channels = 2;

    if(decoder->onmeta != NULL) {
        if(priv->id6->song[0] != '\0') {
            decoder->onmeta(decoder->meta_ctx,"title",priv->id6->song);
        }
        if(priv->id6->game[0] != '\0') {
            decoder->onmeta(decoder->meta_ctx,"album",priv->id6->game);
        }
        if(priv->id6->artist[0] != '\0') {
            decoder->onmeta(decoder->meta_ctx,"artist",priv->id6->artist);
        }
    }

    goto jspc_finish;

jspc_error:
    jprspc_free(jspc);
    jspc = NULL;

jspc_finish:
    if(spc_data != NULL) free(spc_data);
    return jspc;
}

static void jprspc_close(audio_plugin_ctx *jspc) {
    jprspc_free(jspc);
}

static jpr_uint64 jprspc_decode(audio_plugin_ctx *spc, jpr_uint64 framecount, jpr_int16 *buf) {
    spc_private *priv;
    jpr_uint64 fc;
    priv = (spc_private *)spc->priv;

    if(priv->frames_rem == 0) return 0;

    fc = ( framecount > priv->frames_rem ? priv->frames_rem : framecount );
    spc_play(priv->spc,fc * 2, buf);
    spc_filter_run(priv->filter,buf,fc*2);
    fade_samples(priv->frames_rem, priv->frames_fade,fc,buf);
    priv->frames_rem -= fc;
    return fc;
}

static const char *const extensions[] = {
    ".spc",
    NULL
};

const audio_plugin jprspc_plugin = {
    "spc",
    jprspc_open,
    jprspc_decode,
    jprspc_close,
    jprspc_probe,
    extensions,
};
