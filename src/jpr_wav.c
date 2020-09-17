#include <stdlib.h>
#include "attr.h"
#include "id3.h"
#include "str.h"
#include "unpack.h"
#include "jpr_wav.h"

#define DR_WAV_NO_STDIO
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

struct wav_probe_s {
    audio_info *info;
    meta_proc onmeta;
    void *meta_ctx;
};

typedef struct wav_probe_s wav_probe;

static void wav_probe_f(void *ctx, const char *key, const char *val) {
    wav_probe *probe = (wav_probe *)ctx;
    if(str_equals(key,"artist")) {
        probe->info->artist = str_dup(val);
    }
    else if(str_equals(key,"album")) {
        probe->info->album = str_dup(val);
    }
    else if(str_equals(key,"title")) {
        probe->info->tracks[0].number = 1;
        probe->info->tracks[0].title = str_dup(val);
    }
    if(probe->onmeta != NULL && probe->meta_ctx != NULL) {
        probe->onmeta(probe->meta_ctx,key,val);
    }
}

static size_t read_wrapper(void *userdata, void *buf, size_t bytes) {
    audio_decoder *a = (audio_decoder *)userdata;
    return (size_t)a->read(a,buf,(jpr_uint64)bytes);
}

static drwav_bool32 seek_wrapper(void *userdata, int offset, drwav_seek_origin origin) {
    audio_decoder *a = (audio_decoder *)userdata;
    enum JPR_FILE_POS w = JPR_FILE_SET;
    switch(origin) {
        case drwav_seek_origin_start: w = JPR_FILE_SET; break;
        case drwav_seek_origin_current: w = JPR_FILE_CUR; break;
    }
    return (drwav_bool32)(a->seek(a,(jpr_int64)offset,w) != -1);
}

static void wav_id3(audio_decoder *a) {
    jpr_uint32 bytes = 0;
    jpr_uint32 cbytes = 0;
    jpr_uint32 tbytes = 0;
    jpr_uint8 buffer[12];
    jpr_uint8 *buffer_tmp = NULL;
    jpr_uint8 *b = NULL;
    if(a->onmeta == NULL) return;

    if(a->read(a,buffer,12) != 12) goto closereturn;
    if(str_ncmp((char *)buffer,"RIFF",4)) goto closereturn;
    if(str_ncmp((char *)buffer+8,"WAVE",4)) goto closereturn;
    bytes = unpack_uint32le(buffer+4);
    bytes -= 4;

    while(bytes > 0) {
        if(a->read(a,buffer,8) != 8) goto closereturn;
        bytes -= 8;
        cbytes = unpack_uint32le(buffer+4);

        if(str_incmp((char *)buffer,"id3 ",4) == 0) {
            process_id3(a);
            goto closereturn;
        }

        if(str_incmp((char *)buffer,"LIST",4) == 0) {
            if(a->read(a,buffer,4) != 4) goto closereturn;
            cbytes -= 4;

            if(str_incmp((char *)buffer,"INFO",4) == 0) {
                bytes -= cbytes; /* going to finish out this chunk */
                buffer_tmp = malloc(cbytes);
                if(buffer_tmp == NULL) goto closereturn;
                if(a->read(a,buffer_tmp,cbytes) != cbytes) goto closereturn;
                b = buffer_tmp;

                while(b < buffer_tmp + cbytes) {
                    mem_cpy(buffer,b,4);
                    buffer[4] = 0;
                    b+= 4;
                    tbytes = unpack_uint32le(b);
                    b += 4;

                    if(str_icmp((const char *)buffer,"iart") == 0) {
                        a->onmeta(a->meta_ctx,"artist",(const char *)b);
                    }
                    else if(str_icmp((const char *)buffer,"inam") == 0) {
                        a->onmeta(a->meta_ctx,"title",(const char *)b);
                    }
                    else if(str_icmp((const char *)buffer,"iprd") == 0) {
                        a->onmeta(a->meta_ctx,"album",(const char *)b);
                    }

                    b += tbytes;
                    if(tbytes % 2 == 1) b++;

                }
            }
        }
        while(cbytes > 0x7FFFFFFF) {
            a->seek(a,0x7FFFFFFF,JPR_FILE_CUR);
            cbytes -= 0x7FFFFFFF;
        }
        a->seek(a,cbytes,JPR_FILE_CUR);
        bytes -= cbytes;
    }

closereturn:
    if(buffer_tmp != NULL) free(buffer_tmp);
    a->seek(a,0,0);
    return;
}


static audio_info *jprwav_probe(audio_decoder *decoder) {
    audio_info *info;
    wav_probe *metaprobe;

    info = (audio_info *)malloc(sizeof(audio_info));
    if(UNLIKELY(info == NULL)) {
        return NULL;
    }
    mem_set(info,0,sizeof(audio_info));
    info->total = 1;

    metaprobe = (wav_probe *)malloc(sizeof(wav_probe));
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

    decoder->onmeta = wav_probe_f;
    decoder->meta_ctx = metaprobe;

    wav_id3(decoder);

    decoder->onmeta = metaprobe->onmeta;
    decoder->meta_ctx = metaprobe->meta_ctx;

    free(metaprobe);
    return info;
}

static void jprwav_close(audio_plugin_ctx *ctx) {
    if(ctx->priv != NULL) {
        drwav_uninit((drwav *)ctx->priv);
        free(ctx->priv);
    }
    free(ctx);
}

static jpr_uint64 jprwav_decode(audio_plugin_ctx *ctx, jpr_uint64 framecount, jpr_int16 *buf) {
    return (jpr_uint64)drwav_read_pcm_frames_s16((drwav *)ctx->priv,(drwav_uint64)framecount,(drwav_int16*)buf);
}

static audio_plugin_ctx *jprwav_open(audio_decoder *decoder, const char *filename) {
    audio_plugin_ctx *ctx = NULL;
    drwav *pWav = NULL;

    (void)filename;

    ctx = (audio_plugin_ctx *)malloc(sizeof(audio_plugin_ctx));
    if(UNLIKELY(ctx == NULL)) {
        goto jprwav_error;
    }
    ctx->priv = NULL;

    pWav = (drwav *)malloc(sizeof(drwav));
    if(pWav == NULL) {
        goto jprwav_error;
    }

    // wav_id3(decoder);

    if(!drwav_init_ex(pWav,read_wrapper,seek_wrapper,NULL,decoder,NULL,DRWAV_SEQUENTIAL,NULL)) {
        goto jprwav_error;
    }

    ctx->priv = pWav;

    decoder->framecount = pWav->totalPCMFrameCount;
    decoder->samplerate = pWav->sampleRate;
    decoder->channels   = pWav->channels;

    goto jprwav_done;

    jprwav_error:
    jprwav_close(ctx);
    if(pWav != NULL) free(pWav);
    ctx = NULL;

    jprwav_done:
    return ctx;
}

static const char *const extensions[] = {
    ".wav",
    NULL
};

const audio_plugin jprwav_plugin = {
    "wav",
    jprwav_open,
    jprwav_decode,
    jprwav_close,
    jprwav_probe,
    extensions,
};
