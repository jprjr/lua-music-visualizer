#include "jpr_m3u.h"
#include "attr.h"
#include "int.h"
#include "str.h"
#include "scan.h"
#include "text.h"
#include "audio-resampler.h"
#include "audio-decoder.h"
#include "path.h"
#include <stdlib.h>

struct m3u_private_s {
    jpr_text *m3u_lines;
    char *dir;
    audio_resampler *resampler;
    audio_decoder *decoder;
    audio_decoder *parent;
};

typedef struct m3u_private_s m3u_private;

static void jprm3u_onchange(void *userdata, const char *type) {
    audio_decoder *d = (audio_decoder *)userdata;
    if(d->onchange != NULL && d->meta_ctx != NULL) {
        d->onchange(d->meta_ctx,type);
    }
}

static void jprm3u_onmeta(void *userdata, const char *key, const char *value) {
    audio_decoder *d = (audio_decoder *)userdata;
    if(d->onmeta != NULL && d->meta_ctx != NULL) {
        d->onmeta(d->meta_ctx,key,value);
    }
}

static void jprm3u_onmeta_double(void *userdata, const char *key, double value) {
    audio_decoder *d = (audio_decoder *)userdata;
    if(d->onmeta_double != NULL && d->meta_ctx != NULL) {
        d->onmeta_double(d->meta_ctx,key,value);
    }
}

static void jprm3u_free(audio_plugin_ctx *ctx) {
    m3u_private *priv = NULL;

    if(ctx->priv != NULL) {
        priv = (m3u_private *)ctx->priv;

        if(priv->m3u_lines != NULL) {
            jpr_text_close(priv->m3u_lines);
        }

        if(priv->resampler != NULL) {
            audio_resampler_close(priv->resampler);
            free(priv->resampler);
        }

        if(priv->decoder != NULL) {
            audio_decoder_close(priv->decoder);
            free(priv->decoder);
        }

        if(priv->dir != NULL) {
            free(priv->dir);
        }

        free(ctx->priv);
    }
    free(ctx);
}

static void jprm3u_close(audio_plugin_ctx *ctx) {
    jprm3u_free(ctx);
}

static audio_decoder *
jprm3u_nextinput(m3u_private *priv, jpr_text *m3u_lines, meta_proc onmeta, meta_proc_double onmeta_double, change_proc onchange, void *meta_ctx) {
    const char *line = NULL;
    char *tmp = NULL;
    char *c   = NULL;
    char *subfile = NULL;
    unsigned int tracknum = 0;
    audio_decoder *decoder = (audio_decoder *)malloc(sizeof(audio_decoder));

    while((line = jpr_text_line(m3u_lines)) != NULL) {
        if(str_len(line) == 0) continue;
        tmp = str_dup(line);
        if(tmp == NULL) {
            free(decoder);
            return NULL;
        }

        c = str_chr(tmp,'#');
        if(c != NULL) {
            *c = '\0';
            if(!scan_uint(&c[1],&tracknum)) {
                tracknum = 1;
            }
        }

        if(str_len(tmp) > 0) {
            subfile = realloc(subfile,strlen(priv->dir) + strlen(tmp) + 2);
            if(subfile == NULL) {
                free(tmp);
                free(decoder);
                return NULL;
            }
            str_cpy(subfile,priv->dir);
#ifdef JPR_WINDOWS
            str_cat(subfile,"\\");
#else
            str_cat(subfile,"/");
#endif
            str_cat(subfile,tmp);
            audio_decoder_init(decoder);
            decoder->onmeta          = onmeta;
            decoder->onmeta_double   = onmeta_double;
            decoder->onchange        = onchange;
            decoder->meta_ctx = meta_ctx;
            if(audio_decoder_open(decoder,subfile) == 0) {
                if(decoder->meta_ctx != NULL) {
                    decoder->onmeta(decoder->meta_ctx,"file",subfile);
                    decoder->onmeta_double(decoder->meta_ctx,"elapsed",0.0f);
                    decoder->onmeta_double(decoder->meta_ctx,"total",((double)decoder->framecount) / ((double)decoder->samplerate));
                    decoder->onchange(decoder->meta_ctx,"player");
                }
                free(subfile);
                free(tmp);
                return decoder;
            }
        }
        free(tmp);
    }
    free(subfile);
    free(decoder);
    return NULL;
}

static jpr_uint64 jprm3u_decode(audio_plugin_ctx *ctx, jpr_uint64 framecount, jpr_int16 *buf) {
    jpr_uint64 r;
    jpr_uint64 t;
    m3u_private *priv;

    r = 0;
    priv = (m3u_private *)ctx->priv;

    while(r < framecount) {
        t = audio_resampler_decode(priv->resampler,framecount,&buf[r * 2]);
        r += t;
        if(t != framecount) {
            audio_resampler_close(priv->resampler);
            audio_resampler_init(priv->resampler);
            audio_decoder_close(priv->decoder);
            free(priv->decoder);

            priv->decoder = jprm3u_nextinput(priv,priv->m3u_lines,jprm3u_onmeta,jprm3u_onmeta_double,jprm3u_onchange,priv->parent);
            if(priv->decoder == NULL) return r;
            audio_resampler_open(priv->resampler,priv->decoder);
        }
        framecount -= t;
    }
    return r;
}

static audio_plugin_ctx *jprm3u_open(audio_decoder *decoder, const char *filename) {
    audio_plugin_ctx *ctx;
    m3u_private *priv;
    jpr_uint8 *m3u_data;
    size_t m3u_data_len;
    jpr_text *m3u_lines;
    char *tmp;
    char *subfile;
    audio_decoder *tmpDecoder;

    ctx = NULL;
    priv = NULL;
    m3u_data = NULL;
    m3u_lines = NULL;
    tmpDecoder = NULL;
    tmp = NULL;
    subfile = NULL;

    m3u_data = decoder->slurp(decoder,&m3u_data_len);
    if(m3u_data_len == 0) goto m3u_error;

    m3u_lines = jpr_text_create(m3u_data, m3u_data_len);
    if(m3u_lines == NULL) goto m3u_error;

    ctx = (audio_plugin_ctx *)malloc(sizeof(audio_plugin_ctx));
    if(UNLIKELY(ctx == NULL)) goto m3u_error;
    ctx->priv = NULL;

    ctx->priv = malloc(sizeof(m3u_private));
    if(UNLIKELY(ctx->priv == NULL)) {
        goto m3u_error;
    }
    priv = (m3u_private *)ctx->priv;
    priv->resampler = NULL;
    priv->decoder = NULL;
    priv->m3u_lines = NULL;
    priv->dir = NULL;
    priv->parent = decoder;

    priv->dir = path_dirname(filename);
    if(priv->dir == NULL) goto m3u_error;

    decoder->framecount = 0;
    decoder->samplerate = 48000;
    decoder->channels   = 2;

    priv->m3u_lines = jpr_text_create(m3u_data,m3u_data_len);
    if(priv->m3u_lines == NULL) goto m3u_error;

    priv->resampler = (audio_resampler *)malloc(sizeof(audio_resampler));
    if(priv->resampler == NULL) goto m3u_error;
    priv->resampler->samplerate = 48000;

    while((tmpDecoder = jprm3u_nextinput(priv,m3u_lines,NULL,NULL,NULL,NULL)) != NULL) {
        audio_decoder_close(tmpDecoder);
        free(tmpDecoder);
    }

    priv->decoder = jprm3u_nextinput(priv,priv->m3u_lines,jprm3u_onmeta,jprm3u_onmeta_double,jprm3u_onchange,priv->parent);
    decoder->framecount = priv->decoder->framecount;
    decoder->framecount *= ((48000.0f / ((double)priv->decoder->samplerate)));
    audio_resampler_open(priv->resampler,priv->decoder);

    goto m3u_finish;

m3u_error:
    jprm3u_free(ctx);
    ctx = NULL;

m3u_finish:
    if(subfile != NULL) free(subfile);
    if(m3u_data != NULL) free(m3u_data);
    if(m3u_lines != NULL) jpr_text_close(m3u_lines);
    if(tmp != NULL) free(tmp);
    if(tmpDecoder != NULL) free(tmpDecoder);

    return ctx;

}

static const char *const extensions[] = {
    "m3u",
    NULL
};

const audio_plugin jprm3u_plugin = {
    "m3u",
    jprm3u_open, /* open */
    jprm3u_decode, /* decode */
    jprm3u_close, /* close */
    NULL, /* probe */
    extensions
};

