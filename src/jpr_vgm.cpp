#include "jpr_vgm.h"
#include "attr.h"
#include "int.h"
#include "str.h"

#include <vgm/player/playerbase.hpp>
#include <vgm/player/vgmplayer.hpp>
#include <vgm/player/s98player.hpp>
#include <vgm/player/droplayer.hpp>
#include <vgm/utils/DataLoader.h>
#include <vgm/utils/MemoryLoader.h>
#include <cstring>
#include <cstdlib>

#define VGM_MAX_SAMPLE   8388607
#define VGM_MIN_SAMPLE  -8388608

struct vgm_private_s {
    PlayerBase *player;
    DATA_LOADER *loader;
    jpr_uint8 *vgm_data;
    jpr_uint64 frames_rem;
    jpr_uint64 frames_fade;
};

typedef struct vgm_private_s vgm_private;

static audio_info *jprvgm_probe(audio_decoder *decoder) {
    audio_info *info;
    PlayerBase *player;
    DATA_LOADER *loader;
    jpr_uint8 *vgm_data;
    size_t vgm_data_len;
	const char* const* tagList;
	const char* const *t;

    vgm_data = decoder->slurp(decoder,&vgm_data_len);
    if(vgm_data == nullptr) return nullptr;
    loader = MemoryLoader_Init(vgm_data,vgm_data_len);
    if(UNLIKELY(loader == nullptr)) {
        free(vgm_data);
        return nullptr;
    }
    DataLoader_SetPreloadBytes(loader,0x100);
    if(DataLoader_Load(loader)) {
        free(vgm_data);
        DataLoader_Deinit(loader);
        return nullptr;
    }

    if(!VGMPlayer::PlayerCanLoadFile(loader)) {
        player = new VGMPlayer();
    } else if(!S98Player::PlayerCanLoadFile(loader)) {
        player = new S98Player();
    } else if(!DROPlayer::PlayerCanLoadFile(loader)) {
        player = new DROPlayer();
    } else {
        free(vgm_data);
        DataLoader_Deinit(loader);
        return nullptr;
    }

    if(player->LoadFile(loader)) {
        free(vgm_data);
        DataLoader_Deinit(loader);
        delete player;
        return nullptr;
    }

    info = (audio_info *)malloc(sizeof(audio_info));
    if(UNLIKELY(info == nullptr)) {
        free(vgm_data);
        DataLoader_Deinit(loader);
        delete player;
        return nullptr;
    }
    mem_set(info,0,sizeof(audio_info));
    info->total = 1;

    info->tracks = (track_info *)malloc(sizeof(track_info) * 1);
    if(UNLIKELY(info->tracks == nullptr)) {
        free(vgm_data);
        DataLoader_Deinit(loader);
        delete player;
        free(info);
        return nullptr;
    }
    mem_set(info->tracks,0,sizeof(track_info) * 1);
    info->tracks[0].number = 1;
 
	tagList = player->GetTags();
	for(t = tagList; *t; t += 2) {
		if(str_equals(t[0],"TITLE"))
            info->tracks[0].title = str_dup(t[1]);
		else if(str_equals(t[0],"ARTIST"))
            info->artist = str_dup(t[1]);
		else if(str_equals(t[0],"GAME"))
            info->album = str_dup(t[1]);
	}

    free(vgm_data);
    DataLoader_Deinit(loader);
    delete player;

    return info;
}


static void jprvgm_free(audio_plugin_ctx *ctx) {
    vgm_private *priv;
    if(ctx != nullptr) {
        if(ctx->priv != nullptr) {
            priv = (vgm_private *)ctx->priv;
            if(priv->player != nullptr) delete priv->player;
            if(priv->loader != nullptr) DataLoader_Deinit(priv->loader);
            if(priv->vgm_data != nullptr) free(priv->vgm_data);
            free(priv);
        }
        free(ctx);
    }
}

static void
pack_samples(jpr_int16 *dest, WAVE_32BS *src, unsigned int count) {
	unsigned int i = 0;
	unsigned int j = 0;

	while(i<count) {

		/* check for clipping */
		if(src[i].L < VGM_MIN_SAMPLE) {
			src[i].L = VGM_MIN_SAMPLE;
		} else if(src[i].L > VGM_MAX_SAMPLE) {
			src[i].L = VGM_MAX_SAMPLE;
		}
		if(src[i].R < VGM_MIN_SAMPLE) {
			src[i].R = VGM_MIN_SAMPLE;
		} else if(src[i].R > VGM_MAX_SAMPLE) {
			src[i].R = VGM_MAX_SAMPLE;
		}

		dest[j]   = (jpr_int16)((jpr_uint32)src[i].L >> 8);
		dest[j+1] = (jpr_int16)((jpr_uint32)src[i].R >> 8);

		i++;
		j+=2;
	}
}

static void fade_samples(jpr_uint64 frames_rem, jpr_uint64 frames_fade, jpr_uint64 frames, WAVE_32BS *buf) {
    jpr_uint64 i = 0;
    jpr_uint64 f = frames_fade;
    jpr_uint64 fade_vol;
    (void)buf;

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
        buf[i].L = (jpr_int16)(((jpr_uint64)buf[i].L * fade_vol) >> 16);
        buf[i].R = (jpr_int16)(((jpr_uint64)buf[i].R * fade_vol) >> 16);
        i++;
    }

    return;
}

audio_plugin_ctx *jprvgm_open(audio_decoder *decoder, const char *filename) {
    audio_plugin_ctx *ctx;
    vgm_private *priv;
    jpr_uint8 *vgm_data;
    size_t vgm_data_len;

    (void)filename;

    ctx = nullptr;
    priv = nullptr;
    vgm_data = nullptr;
    vgm_data_len = 0;

    ctx = (audio_plugin_ctx *)malloc(sizeof(audio_plugin_ctx));
    if(UNLIKELY(ctx == nullptr)) return nullptr;

    ctx->decoder = decoder;
    ctx->priv = nullptr;
    ctx->priv = malloc(sizeof(vgm_private));
    if(UNLIKELY(ctx->priv == nullptr)) goto jprvgm_error;

    priv = (vgm_private *)ctx->priv;
    priv->player = nullptr;
    priv->loader = nullptr;
    priv->vgm_data = nullptr;
    priv->frames_fade = 0;
    priv->frames_rem = 0;

    vgm_data_len = (size_t)decoder->seek(decoder,0,JPR_FILE_END);
    if(vgm_data_len == 0) goto jprvgm_error;
    decoder->seek(decoder,0,JPR_FILE_SET);

    vgm_data = (jpr_uint8 *)malloc(vgm_data_len);
    if(UNLIKELY(vgm_data == nullptr)) {
        goto jprvgm_error;
    }

    if(decoder->read(decoder,vgm_data,vgm_data_len) != vgm_data_len) {
        goto jprvgm_error;
    }

    priv->vgm_data = vgm_data;
    priv->loader = MemoryLoader_Init(vgm_data,vgm_data_len);
    if(UNLIKELY(priv->loader == nullptr)) {
        goto jprvgm_error;
    }
    DataLoader_SetPreloadBytes(priv->loader,0x100);
    if(DataLoader_Load(priv->loader)) {
        goto jprvgm_error;
    }

    if(!VGMPlayer::PlayerCanLoadFile(priv->loader)) {
        priv->player = new VGMPlayer();
    } else if(!S98Player::PlayerCanLoadFile(priv->loader)) {
        priv->player = new S98Player();
    } else if(!DROPlayer::PlayerCanLoadFile(priv->loader)) {
        priv->player = new DROPlayer();
    } else {
        goto jprvgm_error;
    }
    if(priv->player->LoadFile(priv->loader)) {
        goto jprvgm_error;
    }

    priv->player->SetSampleRate(44100);
    priv->player->Start();

    priv->frames_rem = (jpr_uint64)priv->player->Tick2Sample(priv->player->GetTotalPlayTicks(2));
    if(priv->player->GetLoopTicks()) {
        priv->frames_fade = 44100 * 8;
    }
    priv->frames_rem += priv->frames_fade;

    decoder->framecount = priv->frames_rem;
    decoder->samplerate = 44100;
    decoder->channels = 2;

    if(decoder->onmeta != nullptr) {
	    const char* const* tagList = priv->player->GetTags();
	    for(const char* const *t = tagList; *t; t += 2) {
	    	if(!strcmp(t[0],"TITLE"))
                decoder->onmeta(decoder->meta_ctx,"title",t[1]);
	    	else if(!strcmp(t[0],"ARTIST"))
                decoder->onmeta(decoder->meta_ctx,"artist",t[1]);
	    	else if(!strcmp(t[0],"GAME"))
                decoder->onmeta(decoder->meta_ctx,"album",t[1]);
	    }
    }

    goto jprvgm_finish;
jprvgm_error:
    jprvgm_free(ctx);
    ctx = nullptr;

jprvgm_finish:
    return ctx;
}

static jpr_uint64 jprvgm_decode(audio_plugin_ctx *ctx, jpr_uint64 framecount, jpr_int16 *buf) {
    vgm_private *priv;
    jpr_uint64 fc;
    WAVE_32BS *frames;
    priv = (vgm_private *)ctx->priv;

    if(priv->frames_rem == 0) return 0;

    frames = (WAVE_32BS *)malloc(sizeof(WAVE_32BS) * framecount);
    if(frames == nullptr) return 0;
    memset(frames,0,sizeof(WAVE_32BS) * framecount);

    fc = ( framecount > priv->frames_rem ? priv->frames_rem : framecount );
    priv->player->Render(fc,frames);
    fade_samples(priv->frames_rem, priv->frames_fade, fc, frames);
    pack_samples(buf,frames,fc);
    priv->frames_rem -= fc;
    free(frames);
    return fc;
}

static void jprvgm_close(audio_plugin_ctx *ctx) {
    jprvgm_free(ctx);
}

static const char *const extensions[] = {
    ".vgm",
    ".vgz",
    ".s98",
    ".dro",
    NULL
};

const audio_plugin jprvgm_plugin = {
    "vgm",
    jprvgm_open,
    jprvgm_decode,
    jprvgm_close,
    jprvgm_probe,
    extensions,
};
