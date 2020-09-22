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
#include <math.h>

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

static int
jprm3u_nextinput(m3u_private *priv) {
    const char *line = NULL;
    char *title  = NULL;
    char *album  = NULL;
    char *artist = NULL;
    char *tmp = NULL;
    char *c   = NULL;
    char *subfile = NULL;
    unsigned int tracknum = 1;

    if(priv->decoder != NULL) {
        audio_decoder_close(priv->decoder);
        free(priv->decoder);
    }

    priv->decoder = (audio_decoder *)malloc(sizeof(audio_decoder));

    while((line = jpr_text_line(priv->m3u_lines)) != NULL) {
        if(str_len(line) == 0) continue;
        tmp = str_dup(line);
        if(tmp == NULL) {
            free(priv->decoder);
            priv->decoder = NULL;
            return 1;
        }

        if(tmp[0] == '#') {
            /* try parsing for commands */
            c = tmp;
            while(*c && (*c == '\t' || *c == ' ' || *c == '#')) c++;
            if(str_ibegins(c,"message")) {
                c += 7;
                while(*c && (*c == ' ' || *c == '\t')) c++;
                if(str_len(c) > 0) {
                    jprm3u_onmeta(priv->parent,"message",c);
                    jprm3u_onchange(priv->parent,"message");
                }
            }
            else if(str_ibegins(c,"title")) {
                c += 5;
                while(*c && (*c == ' ' || *c == '\t')) c++;
                if(str_len(c) > 0) {
                    title = str_dup(c);
                    if(title == NULL) {
                        if(album != NULL) free(album);
                        if(artist != NULL) free(artist);
                        free(tmp);
                        free(priv->decoder);
                        priv->decoder = NULL;
                        return 1;
                    }
                }
            }
            else if(str_ibegins(c,"album")) {
                c += 5;
                while(*c && (*c == ' ' || *c == '\t')) c++;
                if(str_len(c) > 0) {
                    album = str_dup(c);
                    if(album == NULL) {
                        if(title != NULL) free(title);
                        if(artist != NULL) free(artist);
                        free(tmp);
                        free(priv->decoder);
                        priv->decoder = NULL;
                        return 1;
                    }
                }
            }
            else if(str_ibegins(c,"artist")) {
                c += 6;
                while(*c && (*c == ' ' || *c == '\t')) c++;
                if(str_len(c) > 0) {
                    artist = str_dup(c);
                    if(artist == NULL) {
                        if(album != NULL) free(album);
                        if(artist != NULL) free(artist);
                        free(tmp);
                        free(priv->decoder);
                        priv->decoder = NULL;
                        return 1;
                    }
                }
            }
            free(tmp);
            continue;
        }

        c = str_chr(tmp,'#');
        if(c != NULL) {
            *c = '\0';
            if(!scan_uint(&c[1],&tracknum)) {
                tracknum = 1;
            }
        }
        priv->decoder->track = tracknum;

        if(str_len(tmp) > 0) {
            /* some extra space will be allocated in the absolute path
             * case but whatever */
            subfile = realloc(subfile,strlen(priv->dir) + strlen(tmp) + 2);
            if(subfile == NULL) {
                free(tmp);
                free(priv->decoder);
                priv->decoder = NULL;
                return 1;
            }
            if(path_isabsolute(tmp)) {
                str_cpy(subfile,tmp);
            }
            else {
                str_cpy(subfile,priv->dir);
#ifdef JPR_WINDOWS
                str_cat(subfile,"\\");
#else
                str_cat(subfile,"/");
#endif
                str_cat(subfile,tmp);
            }

            audio_decoder_init(priv->decoder);
            priv->decoder->onmeta          = jprm3u_onmeta;
            priv->decoder->onmeta_double   = jprm3u_onmeta_double;
            priv->decoder->onchange        = jprm3u_onchange;
            priv->decoder->meta_ctx        = priv->parent;
            if(audio_decoder_open(priv->decoder,subfile) == 0) {
                /* apply title/artist/album overrides if they were given */
                if(title != NULL) {
                    priv->decoder->onmeta(priv->decoder->meta_ctx,"title",title);
                    free(title);
                    title = NULL;
                }
                if(artist != NULL) {
                    priv->decoder->onmeta(priv->decoder->meta_ctx,"artist",artist);
                    free(artist);
                    artist = NULL;
                }
                if(album != NULL) {
                    priv->decoder->onmeta(priv->decoder->meta_ctx,"album",album);
                    free(album);
                    album = NULL;
                }
                priv->decoder->onmeta(priv->decoder->meta_ctx,"file",subfile);
                priv->decoder->onmeta_double(priv->decoder->meta_ctx,"elapsed",0.0f);
                priv->decoder->onmeta_double(priv->decoder->meta_ctx,"total",((double)priv->decoder->framecount) / ((double)priv->decoder->samplerate));
                priv->decoder->onchange(priv->decoder->meta_ctx,"player");
                free(subfile);
                free(tmp);
                return 0;
            }
        }
        free(tmp);
    }
    if(title != NULL) free(title);
    if(artist != NULL) free(artist);
    if(album != NULL) free(album);
    free(subfile);
    free(priv->decoder);
    priv->decoder = NULL;
    return 1;
}

static audio_info *jprm3u_probe(audio_decoder *decoder, const char *filename) {
    char *c;
    audio_info *info;
    (void)decoder;

    info = (audio_info *)malloc(sizeof(audio_info));
    if(UNLIKELY(info == NULL)) {
        return NULL;
    }
    memset(info,0,sizeof(audio_info));

    info->total = 1;
    info->tracks = (track_info *)malloc(sizeof(track_info));
    if(UNLIKELY(info->tracks == NULL)) {
        free(info);
        return NULL;
    }
    memset(info->tracks,0,sizeof(track_info));

    info->tracks[0].number = 1;
    info->tracks[0].title = path_basename(filename);
    c = str_rchr(info->tracks[0].title,'.');
    if(c != NULL && str_ibegins(c,"m3u")) {
        *c = '\0';
    }

    return info;
}

static jpr_uint64 jprm3u_decode(audio_plugin_ctx *ctx, jpr_uint64 framecount, jpr_int16 *buf) {
    jpr_uint64 r;
    jpr_uint64 t;
    m3u_private *priv;

    r = 0;
    priv = (m3u_private *)ctx->priv;

    if(priv->decoder == NULL) {
        if(jprm3u_nextinput(priv)) return 0;
        audio_resampler_open(priv->resampler,priv->decoder);
    }

    while(r < framecount) {
        t = audio_resampler_decode(priv->resampler,framecount - r,&buf[r * 2]);
        r += t;
        if(r != framecount) {
            audio_resampler_close(priv->resampler);

            if(jprm3u_nextinput(priv)) {
                return r;
            }
            audio_resampler_open(priv->resampler,priv->decoder);
        }
    }
    return r;
}

static audio_plugin_ctx *jprm3u_open(audio_decoder *decoder, const char *filename) {
    audio_plugin_ctx *ctx;
    m3u_private *priv;
    jpr_uint8 *m3u_data;
    size_t m3u_data_len;
    char *tmp;
    char *subfile;
    audio_decoder *tmpDecoder;

    ctx = NULL;
    priv = NULL;
    m3u_data = NULL;
    tmpDecoder = NULL;
    tmp = NULL;
    subfile = NULL;

    m3u_data = decoder->slurp(decoder,&m3u_data_len);
    if(m3u_data_len == 0) goto m3u_error;

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
    audio_resampler_init(priv->resampler);

    /* go through and get all the durations */
    while(jprm3u_nextinput(priv) == 0) {
        jpr_uint64 fc = priv->decoder->framecount;
        decoder->framecount += ceil(((double)fc) * ((48000.0f / ((double)priv->decoder->samplerate))));
    }
    if(decoder->framecount == 0) goto m3u_error;

    jpr_text_close(priv->m3u_lines);
    priv->m3u_lines = jpr_text_create(m3u_data,m3u_data_len);

    goto m3u_finish;

m3u_error:
    jprm3u_free(ctx);
    ctx = NULL;

m3u_finish:
    if(subfile != NULL) free(subfile);
    if(m3u_data != NULL) free(m3u_data);
    if(tmp != NULL) free(tmp);
    if(tmpDecoder != NULL) free(tmpDecoder);

    return ctx;

}

static const char *const extensions[] = {
    "m3u",
    "m3u8",
    NULL
};

const audio_plugin jprm3u_plugin = {
    "m3u",
    jprm3u_open, /* open */
    jprm3u_decode, /* decode */
    jprm3u_close, /* close */
    jprm3u_probe, /* probe */
    extensions
};

