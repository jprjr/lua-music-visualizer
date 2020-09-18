#include <stdlib.h>
#include "audio-resampler.h"

#if ENABLE_LIBSAMPLERATE
static long resampler_load(void *cb_data, float **data) {
    jpr_uint64 t;
    audio_resampler *sampler;

    t = 0;
    sampler = (audio_resampler *)cb_data;
    *data = sampler->in;
    t = audio_decoder_decode(sampler->decoder,8192 / sampler->decoder->channels, sampler->buf);
    src_short_to_float_array(sampler->buf,sampler->in, t * sampler->decoder->channels);

    return (long)t;
}

static jpr_uint64 resampler_resample(audio_resampler *sampler, jpr_uint64 framecount, jpr_int16 *buf) {
    long t;
    t = src_callback_read(sampler->src,sampler->ratio,(long)framecount,sampler->out);
    src_float_to_short_array(sampler->out,buf,t * sampler->decoder->channels);

    return (jpr_uint64)t;
}
#endif


static jpr_uint64 resampler_passthrough(audio_resampler *sampler, jpr_uint64 framecount, jpr_int16 *buf) {
    return audio_decoder_decode(sampler->decoder,framecount,buf);
}

int audio_resampler_open(audio_resampler *sampler, audio_decoder  *decoder) {
#if !ENABLE_LIBSAMPLERATE
    sampler->resample = resampler_passthrough;
    sampler->samplerate = decoder->samplerate;
#else
    int src_error = 0;
    if(sampler->samplerate == 0 || sampler->samplerate == decoder->samplerate) {
        sampler->resample = resampler_passthrough;
        sampler->samplerate = decoder->samplerate;
    }
    else {
        sampler->buf = (jpr_int16 *)malloc(sizeof(jpr_int16) * 8192);
        if(sampler->buf == NULL) return 1;
        sampler->in  = (float *)malloc(sizeof(float) * 8192);
        if(sampler->in == NULL) {
            free(sampler->buf);
            return 1;
        }
        sampler->out = (float *)malloc(sizeof(float) * 8192);
        if(sampler->out == NULL) {
            free(sampler->buf);
            free(sampler->in);
            return 1;
        }
        sampler->src = src_callback_new(resampler_load,SRC_SINC_BEST_QUALITY,decoder->channels,&src_error,sampler);
        if(sampler->src == NULL) {
            free(sampler->buf);
            free(sampler->in);
            return 1;
        }
        sampler->resample = resampler_resample;
        sampler->ratio = ((double)sampler->samplerate) / ((double)decoder->samplerate);
    }
#endif
    sampler->decoder = decoder;
    return 0;
}

int audio_resampler_init(audio_resampler *sampler) {
    sampler->buf = NULL;
    sampler->in  = NULL;
    sampler->out = NULL;
    sampler->src = NULL;
    sampler->ratio = 0.0;
    return 0;
}

jpr_uint64 audio_resampler_decode(audio_resampler *sampler, jpr_uint64 framecount, jpr_int16 *buf) {
    return sampler->resample(sampler,framecount,buf);
}

void audio_resampler_close(audio_resampler *sampler) {
    if(sampler->buf != NULL) free(sampler->buf);
    if(sampler->in != NULL) free(sampler->in);
    if(sampler->out != NULL) free(sampler->out);
#if ENABLE_LIBSAMPLERATE
    if(sampler->src != NULL) src_delete(sampler->src);
#endif
    sampler->buf   = NULL;
    sampler->in    = NULL;
    sampler->out   = NULL;
    sampler->src   = NULL;
    sampler->ratio = 0.0;
    return;
}
