#include <nezplug/nezplug.h>
#include <stdlib.h>

#include "attr.h"
#include "jpr_nez.h"
#include "int.h"
#include "str.h"


struct nezplug_private_s {
    NEZ_PLAY *player;
    jpr_uint64 frames_rem;
    jpr_uint64 frames_fade;
};

typedef struct nezplug_private_s nezplug_private;

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

static audio_info *jprnez_probe(audio_decoder *decoder) {
    unsigned int i;
    unsigned int offset;
    jpr_uint8 *nez_data;
    size_t nez_data_len;
    NEZ_PLAY *player;
    audio_info *info;

    nez_data = decoder->slurp(decoder,&nez_data_len);
    if(nez_data_len == 0) return NULL;

    player = NEZNew();
    if(UNLIKELY(player == NULL)) {
        free(nez_data);
        return NULL;
    }

    if(NEZLoad(player,nez_data,nez_data_len)) {
        free(nez_data);
        NEZDelete(player);
        return NULL;
    }

    info = (audio_info *)malloc(sizeof(audio_info));
    if(UNLIKELY(info == NULL)) {
        free(nez_data);
        NEZDelete(player);
        return NULL;
    }
    mem_set(info,0,sizeof(audio_info));

    info->total = NEZGetSongMax(player) - NEZGetSongStart(player) + 1;
    info->tracks = (track_info *)malloc(sizeof(track_info) * info->total);
    if(UNLIKELY(info->tracks == NULL)) {
        free(nez_data);
        NEZDelete(player);
        free(info);
        return NULL;
    }
    mem_set(info->tracks,0,sizeof(track_info) * info->total);

    offset = NEZGetSongStart(player);

    for(i=NEZGetSongStart(player);i<=NEZGetSongMax(player);i++) {
        info->tracks[i - offset].number = i;
        if(NEZGetTrackTitle(player,i)) {
            info->tracks[i - offset].title = str_dup(NEZGetTrackTitle(player,i));
        } else {
            info->tracks[i - offset].title = str_dup("");
        }
    }

    if(NEZGetGameArtist(player)) {
        info->artist = str_dup(NEZGetGameArtist(player));
    }
    if(NEZGetGameTitle(player)) {
        info->album = str_dup(NEZGetGameTitle(player));
    }

    free(nez_data);
    NEZDelete(player);

    return info;
}


static void jprnez_free(audio_plugin_ctx *ctx) {
    nezplug_private *priv;
    if(ctx != NULL) {
        if(ctx->priv != NULL) {
            priv = (nezplug_private *)ctx->priv;
            if(priv->player != NULL) NEZDelete(priv->player);
            free(priv);
        }
        free(ctx);
    }
}


static audio_plugin_ctx *jprnez_open(audio_decoder *decoder) {
    audio_plugin_ctx *ctx;
    nezplug_private *priv;
    jpr_uint8 *nez_data;
    size_t nez_data_len;

    ctx = NULL;
    priv = NULL;
    nez_data = NULL;
    nez_data_len = 0;

    ctx = (audio_plugin_ctx *)malloc(sizeof(audio_plugin_ctx));
    if(UNLIKELY(ctx == NULL)) return NULL;

    ctx->priv = NULL;
    ctx->priv = malloc(sizeof(nezplug_private));
    if(UNLIKELY(ctx->priv == NULL)) goto jnez_error;

    priv = (nezplug_private *)ctx->priv;
    priv->player = NULL;

    priv->player = NEZNew();
    if(UNLIKELY(priv->player == NULL)) goto jnez_error;

    ctx->decoder  = decoder;

    nez_data_len = (size_t)decoder->seek(decoder,0,2);
    if(nez_data_len == 0) goto jnez_error;
    decoder->seek(decoder,0,0);

    nez_data = (jpr_uint8 *)malloc(nez_data_len);
    if(UNLIKELY(nez_data == NULL)) {
        goto jnez_error;
    }

    if(decoder->read(decoder,nez_data,nez_data_len) != nez_data_len) {
        goto jnez_error;
    }

    if(NEZLoad(priv->player,nez_data,nez_data_len)) goto jnez_error;
    NEZSetFrequency(priv->player,48000);
    NEZSetChannel(priv->player,2);
    NEZSetSongNo(priv->player,decoder->track);
    NEZReset(priv->player);

    decoder->samplerate = 48000;
    decoder->channels   = 2;

    priv->frames_fade = NEZGetTrackFade(priv->player,decoder->track); /* ms */
    priv->frames_fade *= 48000;
    priv->frames_fade /= 1000;

    priv->frames_rem = NEZGetTrackLength(priv->player,decoder->track);
    priv->frames_rem *= 48000;
    priv->frames_rem /= 1000;
    priv->frames_rem += priv->frames_fade;

    decoder->framecount = priv->frames_rem;

    if(decoder->onmeta != NULL) {
        if(NEZGetTrackTitle(priv->player,decoder->track))
            decoder->onmeta(decoder->meta_ctx,"title",NEZGetTrackTitle(priv->player,decoder->track));
        if(NEZGetGameArtist(priv->player))
            decoder->onmeta(decoder->meta_ctx,"artist",NEZGetGameArtist(priv->player));
        if(NEZGetGameTitle(priv->player))
            decoder->onmeta(decoder->meta_ctx,"album",NEZGetGameTitle(priv->player));
    }

    goto jnez_finish;
jnez_error:
    jprnez_free(ctx);
    ctx = NULL;

jnez_finish:
    if(nez_data != NULL) free(nez_data);
    return ctx;
}

static jpr_uint64 jprnez_decode(audio_plugin_ctx *ctx, jpr_uint64 framecount, jpr_int16 *buf) {
    nezplug_private *priv;
    jpr_uint64 fc;
    priv = (nezplug_private *)ctx->priv;

    if(priv->frames_rem == 0) return 0;

    fc = ( framecount > priv->frames_rem ? priv->frames_rem : framecount );
    NEZRender(priv->player,buf,fc);
    fade_samples(priv->frames_rem, priv->frames_fade, fc, buf);
    priv->frames_rem -= fc;
    return fc;
}

static void jprnez_close(audio_plugin_ctx *ctx) {
    jprnez_free(ctx);
}

static const char *const extensions[] = {
    ".kss",
    ".hes",
    ".nsf",
    ".nsfe",
    ".ay",
    ".gbr",
    ".gbs",
    ".nsd",
    ".sgc",
    NULL
};

const audio_plugin jprnez_plugin = {
    "nezplug",
    jprnez_open,
    jprnez_decode,
    jprnez_close,
    jprnez_probe,
    extensions,
};
