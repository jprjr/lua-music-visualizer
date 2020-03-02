#include <nsfplay/nsfplay.h>
#include <stdlib.h>

#include "jpr_nsf.h"
#include "attr.h"
#include "int.h"
#include "file.h"
#include "str.h"

struct nsfplay_private_s {
    xgm::NSF *nsf;
    xgm::NSFPlayerConfig *config;
    xgm::NSFPlayer *player;
    jpr_uint64 frames_rem;
    jpr_uint64 frames_fade;
};

typedef struct nsfplay_private_s nsfplay_private;

static void jprnsf_free(audio_plugin_ctx *ctx) {
    nsfplay_private *priv;
    if(ctx != nullptr) {
        if(ctx->priv != nullptr) {
            priv = (nsfplay_private *)ctx->priv;
            if(priv->player != nullptr) delete priv->player;
            if(priv->config != nullptr) delete priv->config;
            if(priv->nsf != nullptr) delete priv->nsf;
            free(priv);
        }
        free(ctx);
    }
}

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

static audio_info *jprnsf_probe(audio_decoder *decoder) {
    int i;
    int total;
    int nsfe_track;
    jpr_uint8 *nsf_data;
    size_t nsf_data_len;
    audio_info *info;
    xgm::NSF nsf;

    nsf_data = decoder->slurp(decoder,&nsf_data_len);
    if(nsf_data_len == 0) return NULL;

    if(!nsf.Load(nsf_data,nsf_data_len)) {
        free(nsf_data);
        return NULL;
    }

    info = (audio_info *)malloc(sizeof(audio_info));
    mem_set(info,0,sizeof(audio_info));

    if(nsf.nsfe_plst_size > 0) {
        total = nsf.nsfe_plst_size;
    } else {
        total = nsf.GetSongNum();
    }

    info->total = total;
    info->tracks = (track_info *)malloc(sizeof(track_info) * total);
    if(UNLIKELY(info->tracks == NULL)) {
        free(info);
        return NULL;
    }

    for(i=0;i<total;i++) {
        if(nsf.nsfe_plst_size > 0) {
            nsfe_track = nsf.nsfe_plst[i];
        } else {
            nsfe_track = i;
        }
        info->tracks[i].number = i+1;
        info->tracks[i].title = str_dup(nsf.nsfe_entry[nsfe_track].tlbl);
    }

    if(nsf.artist[0]) {
        info->artist = str_dup(nsf.artist);
    }
    if(nsf.title[0]) {
        info->album = str_dup(nsf.title);
    }

    free(nsf_data);
    return info;
}

static audio_plugin_ctx *jprnsf_open(audio_decoder *decoder) {
    audio_plugin_ctx *ctx;
    nsfplay_private *priv;
    jpr_uint8 *nsf_data;
    size_t nsf_data_len;
    unsigned int track;
    jpr_uint8 nsfe_track;

    ctx = nullptr;
    priv = nullptr;
    nsf_data = nullptr;
    nsf_data_len = 0;
    nsfe_track = 0;
    track = decoder->track - 1;

    ctx = (audio_plugin_ctx *)malloc(sizeof(audio_plugin_ctx));
    if(UNLIKELY(ctx == nullptr)) return nullptr;

    ctx->priv = nullptr;
    ctx->priv = malloc(sizeof(nsfplay_private));
    if(UNLIKELY(ctx->priv == nullptr)) goto jnsf_error;

    priv = (nsfplay_private *)ctx->priv;
    priv->player = nullptr;
    priv->config = nullptr;
    priv->nsf = nullptr;

    priv->player = new xgm::NSFPlayer();
    priv->config = new xgm::NSFPlayerConfig();
    priv->nsf    = new xgm::NSF();

    ctx->decoder = decoder;

    nsf_data = decoder->slurp(decoder,&nsf_data_len);
    if(nsf_data_len == 0) goto jnsf_error;

    if(!priv->nsf->Load(nsf_data,nsf_data_len)) {
        goto jnsf_error;
    }

    if(priv->nsf->nsfe_plst_size > 0) {
        if((int)track >= priv->nsf->nsfe_plst_size) {
            track = priv->nsf->nsfe_plst_size - 1;
        }
        nsfe_track = priv->nsf->nsfe_plst[track];
    } else {
        if((int)track > priv->nsf->GetSongNum()) {
            track = priv->nsf->GetSongNum() - 1;
        }
        nsfe_track = track;
    }
    priv->nsf->SetSong(track);

    (*(priv->config))["MASTER_VOLUME"] = 192;
    (*(priv->config))["RATE"] = 48000;
    (*(priv->config))["NCH"] = 2;

    priv->player->SetConfig(priv->config);
    priv->player->Load(priv->nsf);
    priv->player->SetChannels(2);
    priv->player->SetPlayFreq(48000);
    priv->player->SetInfinite(true);
    priv->player->Reset();

    priv->frames_rem = priv->nsf->GetLength();
    priv->frames_rem *= 48000;
    priv->frames_rem /= 1000;

    priv->frames_fade = priv->nsf->GetFadeTime();
    priv->frames_fade *= 48000;
    priv->frames_fade /= 1000;

    decoder->framecount = priv->frames_rem;
    decoder->samplerate = 48000;
    decoder->channels = 2;

    if(decoder->onmeta != nullptr) {
        if(priv->nsf->artist[0]) {
            decoder->onmeta(decoder->meta_ctx,"artist",priv->nsf->artist);
        }
        if(priv->nsf->title[0]) {
            decoder->onmeta(decoder->meta_ctx,"album",priv->nsf->title);
        }
        if(priv->nsf->nsfe_entry[nsfe_track].tlbl[0]) {
            decoder->onmeta(decoder->meta_ctx,"title",priv->nsf->nsfe_entry[nsfe_track].tlbl);
        }
    }

    goto jnsf_finish;
jnsf_error:
    jprnsf_free(ctx);
    ctx = nullptr;

jnsf_finish:
    if(nsf_data != nullptr) free(nsf_data);
    return ctx;
}

static jpr_uint64 jprnsf_decode(audio_plugin_ctx *ctx, jpr_uint64 framecount, jpr_int16 *buf) {
    nsfplay_private *priv;
    jpr_uint64 fc;
    priv = (nsfplay_private *)ctx->priv;

    if(priv->frames_rem == 0) return 0;

    fc = ( framecount > priv->frames_rem ? priv->frames_rem : framecount );
    priv->player->Render(buf,fc);
    fade_samples(priv->frames_rem, priv->frames_fade, fc, buf);
    priv->frames_rem -= fc;
    return fc;
}

static void jprnsf_close(audio_plugin_ctx *ctx) {
    jprnsf_free(ctx);
}

static const char *const extensions[] = {
    ".nsf",
    ".nsfe",
    nullptr
};

const audio_plugin jprnsf_plugin = {
    "nsfplay",
    jprnsf_open,
    jprnsf_decode,
    jprnsf_close,
    jprnsf_probe,
    extensions,
};
