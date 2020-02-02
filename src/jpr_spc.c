#include "attr.h"
#include "jpr_spc.h"
#include "int.h"

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

static void fade_samples(jpr_uint64 frames_rem, jpr_uint64 frames_fade, jpr_uint64 frames, unsigned int channels, jpr_int16 *buf) {
    jpr_uint64 i = 0;
    jpr_uint64 f = frames_fade;
    jpr_uint64 fade_vol;
    (void)buf;
    (void)channels;

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
        buf[i * channels] = (jpr_int16)(((jpr_uint64)buf[i * channels] * fade_vol) >> 16);
        if(channels > 1) {
            buf[(i * channels)+1] = (jpr_int16)(((jpr_uint64)buf[(i * channels)+1] * fade_vol) >> 16);
        }
        i++;
    }

    return;
}

static void jprspc_free(jprspc *jspc) {
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

jprspc *jprspc_open(jprspc_read_proc readProc, jprspc_seek_proc seekProc, jprspc_meta_proc metaProc, void *userdata, unsigned int samplerate, unsigned int channels) {
    jprspc *jspc;
    spc_private *priv;
    jpr_uint8 *spc_data;
    size_t spc_data_len;

    jspc = NULL;
    priv = NULL;
    spc_data = NULL;
    spc_data_len = 0;

    jspc = (jprspc *)malloc(sizeof(jprspc));
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


    jspc->readProc = readProc;
    jspc->seekProc = seekProc;
    jspc->userdata = userdata;

    spc_data_len = (size_t)jspc->seekProc(jspc->userdata,0,2);
    if(spc_data_len == 0) goto jspc_error;
    jspc->seekProc(jspc->userdata,0,0);

    spc_data = (jpr_uint8 *)malloc(spc_data_len);
    if(UNLIKELY(spc_data == NULL)) {
        goto jspc_error;
    }

    if(jspc->readProc(jspc->userdata,spc_data,spc_data_len) != spc_data_len) {
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

    (void)samplerate;
    (void)channels;

    jspc->samplerate = 32000;
    jspc->channels = 2;
    priv->frames_rem  = priv->id6->total_len / 2;
    priv->frames_fade = priv->id6->fade / 2;

    jspc->totalPCMFrameCount = priv->frames_rem;

    if(metaProc != NULL) {
        if(priv->id6->song[0] != '\0') {
            metaProc(jspc->userdata,"song",priv->id6->song);
        }
        if(priv->id6->game[0] != '\0') {
            metaProc(jspc->userdata,"game",priv->id6->game);
        }
        if(priv->id6->dumper[0] != '\0') {
            metaProc(jspc->userdata,"dumper",priv->id6->dumper);
        }
        if(priv->id6->comment[0] != '\0') {
            metaProc(jspc->userdata,"comment",priv->id6->comment);
        }
        if(priv->id6->artist[0] != '\0') {
            metaProc(jspc->userdata,"artist",priv->id6->artist);
        }
        if(priv->id6->publisher[0] != '\0') {
            metaProc(jspc->userdata,"publisher",priv->id6->publisher);
        }
        if(priv->id6->ost[0] != '\0') {
            metaProc(jspc->userdata,"ost",priv->id6->ost);
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

void jprspc_close(jprspc *jspc) {
    jprspc_free(jspc);
}

jpr_uint64 jprspc_read_pcm_frames_s16(jprspc *spc, jpr_uint64 framecount, jpr_int16 *buf) {
    (void)framecount;
    (void)buf;
    spc_private *priv;
    jpr_uint64 fc;
    priv = (spc_private *)spc->priv;

    if(priv->frames_rem == 0) return 0;

    fc = ( framecount > priv->frames_rem ? priv->frames_rem : framecount );
    spc_play(priv->spc,fc * 2, buf);
    spc_filter_run(priv->filter,buf,fc*2);
    fade_samples(priv->frames_rem, priv->frames_fade,fc,spc->channels,buf);
    priv->frames_rem -= fc;
    return fc;
}

