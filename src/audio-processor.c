#include "audio-processor.h"
#include "audio-decoder.h"
#include <string.h>
#include <math.h>

#define AUDIO_MAX(a,b) (a > b ? a : b)

#define FREQ_MIN 50.0f
#define FREQ_MAX 10000.0f

#define SMOOTH_DOWN 0.2
#define SMOOTH_UP 0.8
#define AMP_MAX 70.0f
#define AMP_MIN 70.0f
#define AMP_BOOST 1.8f

static kiss_fft_scalar itur_468(double freq) {
    /* only calculate this for freqs > 1000 */
    if(freq >= 1000.0f) {
        return 0.0f;
    }
    double h1 = (-4.737338981378384 * pow(10,-24) * pow(freq,6)) +
                ( 2.043828333606125 * pow(10,-15) * pow(freq,4)) -
                ( 1.363894795463638 * pow(10,-7)  * pow(freq,2)) +
                1;
    double h2 = ( 1.306612257412824 * pow(10,-19) * pow(freq,5)) -
                ( 2.118150887518656 * pow(10,-11) * pow(freq,3)) +
                ( 5.559488023498642 * pow(10,-4)  * freq);
    double r1 = ( 1.246332637532143 * pow(10,-4) * freq ) /
                sqrt(pow(h1,2) + pow(h2,2));
    return 18.2f + (20.0f * log10(r1));
}

static kiss_fft_scalar cpx_abs(kiss_fft_cpx c) {
    return sqrt( (c.r * c.r) + (c.i * c.i));
}

static kiss_fft_scalar cpx_amp(double buffer_len, kiss_fft_cpx c) {
    return 20.0f * log10(2.0f * cpx_abs(c) /buffer_len);
}

static kiss_fft_scalar find_amplitude_max(double buffer_len, kiss_fft_cpx *out, unsigned int start, unsigned int end) {
    unsigned int i = 0;
    kiss_fft_scalar val = -INFINITY;
    kiss_fft_scalar tmp = 0.0f;
    for(i=start;i<=end;i++) {
        tmp = cpx_amp(buffer_len,out[i]);
        /* see https://groups.google.com/d/msg/comp.dsp/cZsS1ftN5oI/rEjHXKTxgv8J */
        val = AUDIO_MAX(tmp,val);
    }
    return val;
}

static kiss_fft_scalar window_blackman_harris(int i, int n) {
    kiss_fft_scalar a = (2.0f * M_PI) / (n - 1);
    return 0.35875 - 0.48829*cos(a*i) + 0.14128*cos(2*a*i) - 0.01168*cos(3*a*i);
}

int audio_processor_init(audio_processor *p, audio_decoder *a,unsigned int samples_per_frame) {
    unsigned int i = 0;
    double octaves = ceil(log2(FREQ_MAX / FREQ_MIN));
    double interval = 1.0f / (octaves / p->spectrum_bars);
    double bin_size = 0.0f;
    if(a->type == -1) return 1;
    if(a->samplerate == 0) return 1;
    if(a->channels == 0) return 1;
    if(a->framecount == 0) return 1;

    p->buffer_len = 4096;
    while(p->buffer_len < samples_per_frame) {
        p->buffer_len *= 2;
    }
    p->decoder = a;

    p->buffer = (int16_t *)malloc(sizeof(int16_t) * p->buffer_len * a->channels);
    if(p->buffer == NULL) return 1;
    memset(p->buffer,0,sizeof(int16_t)*(p->buffer_len * p->decoder->channels));

    if(p->spectrum_bars > 0) {
        p->mbuffer = (kiss_fft_scalar *)malloc(sizeof(kiss_fft_scalar) * p->buffer_len);
        if(p->mbuffer == NULL) return 1;

        p->wbuffer = (kiss_fft_scalar *)malloc(sizeof(kiss_fft_scalar) * p->buffer_len);
        if(p->wbuffer == NULL) return 1;

        p->obuffer = (kiss_fft_cpx *)malloc(sizeof(kiss_fft_cpx) * ( (p->buffer_len / 2) + 1));
        if(p->obuffer == NULL) return 1;

        p->spectrum = (frange *)malloc(sizeof(frange) * (p->spectrum_bars + 1));
        if(p->spectrum == NULL) return 1;

        memset(p->mbuffer,0,sizeof(kiss_fft_scalar)*p->buffer_len);
        memset(p->wbuffer,0,sizeof(kiss_fft_scalar)*p->buffer_len);
        memset(p->obuffer,0,sizeof(kiss_fft_cpx) * ((p->buffer_len/2)+1) );

        bin_size = (double)a->samplerate / ((double)p->buffer_len);
        p->plan = kiss_fftr_alloc(p->buffer_len, 0, NULL, NULL);
        if(p->plan == NULL) return 1;
        while(i<p->buffer_len) {
            p->wbuffer[i] = window_blackman_harris(i,p->buffer_len);
            i++;
        }
        p->firstflag = 0;

        for(i=0;i<p->spectrum_bars + 1;i++) {
            p->spectrum[i].amp = 0.0f;
            p->spectrum[i].prevamp = 0.0f;
            if(i==0) {
                p->spectrum[i].freq = FREQ_MIN;
            }
            else {
                /* see http://www.zytrax.com/tech/audio/calculator.html#centers_calc */
                p->spectrum[i].freq = p->spectrum[i-1].freq * pow(10, 3 / (10 * interval));
            }

            /* fudging this a bit to avoid overlap */
            double upper_freq = p->spectrum[i].freq * pow(10, (3 * 1) / (10 * 2 * floor(interval)));
            double lower_freq = p->spectrum[i].freq / pow(10, (3 * 1) / (10 * 2 * ceil(interval)));

            p->spectrum[i].first_bin = (unsigned int)floor(lower_freq / bin_size);
            p->spectrum[i].last_bin = (unsigned int)floor(upper_freq / bin_size);

            if(p->spectrum[i].last_bin > p->buffer_len / 2) {
                p->spectrum[i].last_bin = p->buffer_len / 2;
            }
            /* figure out the ITU-R 468 weighting to apply */
            p->spectrum[i].boost = itur_468(p->spectrum[i].freq);

        }
    }
    else {
        p->mbuffer = NULL;
        p->wbuffer = NULL;
        p->obuffer = NULL;
        p->spectrum = NULL;
    }


    return 0;
}

void audio_processor_close(audio_processor *p) {
    if(p->plan != NULL) {
        KISS_FFT_FREE(p->plan);
        p->plan = NULL;
    }
    if(p->buffer != NULL) {
        free(p->buffer);
        p->buffer = NULL;
    }

    if(p->mbuffer != NULL) {
        free(p->mbuffer);
        p->mbuffer = NULL;
    }

    if(p->wbuffer != NULL) {
        free(p->wbuffer);
        p->wbuffer = NULL;
    }

    if(p->obuffer != NULL) {
        free(p->obuffer);
        p->obuffer = NULL;
    }

    if(p->spectrum != NULL) {
        free(p->spectrum);
        p->spectrum = NULL;
    }
}

static void audio_processor_fft(audio_processor *p) {
    unsigned int i=0;
    kiss_fft_scalar m = 0;

    while(i < p->buffer_len) {
        if(p->decoder->channels ==2) {
            m = p->buffer[i * 2] + p->buffer[(i*2)+1];
            m /= 2.0f;
        } else {
            m = p->buffer[i];
        }
        m /= 32768.0f;
        p->mbuffer[i] = m * p->wbuffer[i];
        i++;
    }

    kiss_fftr(p->plan,p->mbuffer,p->obuffer);

    for(i=0;i<p->spectrum_bars;i++) {
        p->spectrum[i].amp = find_amplitude_max((double)p->buffer_len,p->obuffer,p->spectrum[i].first_bin,p->spectrum[i].last_bin);

        if(!isfinite(p->spectrum[i].amp)) {
            p->spectrum[i].amp = -999.0f;
        }

        p->spectrum[i].amp += p->spectrum[i].boost;

        if(p->spectrum[i].amp <= -AMP_MIN) {
            p->spectrum[i].amp = -AMP_MIN;
        }

        p->spectrum[i].amp += AMP_MIN;

        if(p->spectrum[i].amp > AMP_MAX) {
            p->spectrum[i].amp = AMP_MAX;
        }

        p->spectrum[i].amp /= AMP_MAX;

        p->spectrum[i].amp *= AMP_BOOST; /* i seem to rarely get results near 1.0, let's give this a boost */

        if(p->spectrum[i].amp > 1.0f) {
            p->spectrum[i].amp = 1.0f;
        }

        if(p->firstflag) {
            if(p->spectrum[i].amp < p->spectrum[i].prevamp) {
                p->spectrum[i].amp =
                  p->spectrum[i].amp * SMOOTH_DOWN +
                  p->spectrum[i].prevamp * ( 1 - SMOOTH_DOWN);
            }
            else {
                p->spectrum[i].amp =
                  p->spectrum[i].amp * SMOOTH_UP +
                  p->spectrum[i].prevamp * ( 1 - SMOOTH_UP);
            }
        }
        else {
            p->firstflag = 1;
        }

        p->spectrum[i].prevamp = p->spectrum[i].amp;

    }
}

unsigned int audio_processor_process(audio_processor *p, unsigned int framecount) {
    /* first shift everything in the buffer down */
    unsigned int i = 0;
    unsigned int r = 0;
    unsigned int o = (p->buffer_len * p->decoder->channels) - (framecount * p->decoder->channels);

    while(i+framecount < (p->buffer_len * p->decoder->channels)) {
        p->buffer[i] = p->buffer[framecount+i];
        i++;
    }

    r = audio_decoder_decode(p->decoder,framecount,&(p->buffer[o]));

    if(r < framecount) {
        i = r;
        while(i<framecount) {
            p->buffer[o+(i*p->decoder->channels)] = 0.0f;
            if(p->decoder->channels > 1) {
                p->buffer[o+(i*p->decoder->channels) + 1] = 0.0f;
            }
            i++;
        }
    }

    if(p->spectrum_bars > 0) audio_processor_fft(p);

    return r;
}
