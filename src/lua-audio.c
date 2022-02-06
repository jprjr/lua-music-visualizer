#include "audio-processor.h"
#include "lua-audio.h"
#include "str.h"

#include <lua.h>
#include <lauxlib.h>
#include <assert.h>

#include <math.h>
#include <float.h>
#include <stdlib.h>

#ifdef USE_FFTW3
#include <complex.h>
#include <fftw3.h>
#define SCALAR_TYPE double
#define COMPLEX_TYPE fftw_complex
#define PLAN_TYPE fftw_plan
#define MALLOC fftw_malloc
#define FREE fftw_free
#else
#include "kiss_fftr.h"
#define SCALAR_TYPE kiss_fft_scalar
#define COMPLEX_TYPE kiss_fft_cpx
#define PLAN_TYPE kiss_fftr_cfg
#define MALLOC malloc
#define FREE free
#endif

#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

#ifndef INFINITY
#ifdef HUGE_VALF
#define INFINITY HUGE_VALF
#else
#define INFINITY (float)HUGE_VAL
#endif
#endif

#define AUDIO_MAX(a,b) (a > b ? a : b)

#define FREQ_MIN 50.0f
#define FREQ_MAX 10000.0f

#define SMOOTH_DOWN 0.2
#define SMOOTH_UP 0.8
#define AMP_MAX 70.0f
#define AMP_MIN 70.0f
#define AMP_BOOST 1.8f
#define MY_PI 3.14159265358979323846

#if __STDC_VERSION__ >= 199901L
#define my_log2(x) log2(x)
#else
#define my_log2(x) ((log(x) * 1.44269504088896340736))
#endif

typedef struct frange_s frange;
typedef struct analyzer_s analyzer;

struct frange_s {
    double freq;
    double amp;
    double prevamp;
    double boost;
    unsigned int first_bin;
    unsigned int last_bin;
};

struct analyzer_s {
    unsigned int bars;
    SCALAR_TYPE *mbuffer; /* stores a downmixed version of samples */
    SCALAR_TYPE *wbuffer; /* stores window function factors */
    COMPLEX_TYPE *obuffer; /* buffer_len/2 + 1 output points */
    PLAN_TYPE plan;
    frange *spectrum;
    int firstflag;
};

static audio_processor *processor = NULL;

attr_const static SCALAR_TYPE itur_468(double freq) {
    double h1;
    double h2;
    double r1;
    /* only calculate this for freqs > 1000 */
    if(freq >= 1000.0f) {
        return 0.0f;
    }
    h1 = (-4.737338981378384 * pow(10,-24) * pow(freq,6)) +
         ( 2.043828333606125 * pow(10,-15) * pow(freq,4)) -
         ( 1.363894795463638 * pow(10,-7)  * pow(freq,2)) +
         1;
    h2 = ( 1.306612257412824 * pow(10,-19) * pow(freq,5)) -
         ( 2.118150887518656 * pow(10,-11) * pow(freq,3)) +
         ( 5.559488023498642 * pow(10,-4)  * freq);
    r1 = ( 1.246332637532143 * pow(10,-4) * freq ) /
         sqrt((h1 * h1) + (h2 * h2));

    return (SCALAR_TYPE)(18.2f + (20.0f * log10(r1)));
}

#ifdef USE_FFTW3
#define cpx_abs(c) cabs(c)
#else
attr_const static SCALAR_TYPE cpx_abs(COMPLEX_TYPE c) {
    return (SCALAR_TYPE)(sqrt( (c.r * c.r) + (c.i * c.i)));
}
#endif

attr_const static SCALAR_TYPE cpx_amp(double buffer_len, COMPLEX_TYPE c) {
    return (SCALAR_TYPE)(20.0f * log10(2.0f * cpx_abs(c) /buffer_len));
}

attr_pure static SCALAR_TYPE find_amplitude_max(double buffer_len, COMPLEX_TYPE *out, unsigned int start, unsigned int end) {
    unsigned int i = 0;
    SCALAR_TYPE val = -INFINITY;
    SCALAR_TYPE tmp = 0.0f;
    for(i=start;i<=end;i++) {
        tmp = cpx_amp(buffer_len,out[i]);
        /* see https://groups.google.com/d/msg/comp.dsp/cZsS1ftN5oI/rEjHXKTxgv8J */
        val = AUDIO_MAX(tmp,val);
    }
    return val;
}

attr_const static SCALAR_TYPE window_blackman_harris(int i, int n) {
    SCALAR_TYPE a = (SCALAR_TYPE)((2.0f * MY_PI) / (n - 1));
    return (SCALAR_TYPE)(0.35875 - 0.48829*cos(a*i) + 0.14128*cos(2*a*i) - 0.01168*cos(3*a*i));
}

static int
luaanalyzer__gc(lua_State *L) {
    analyzer *a = lua_touserdata(L,1);

    if(a->plan != NULL) {
        FREE(a->plan);
        a->plan = NULL;
    }

    if(a->mbuffer != NULL) {
        FREE(a->mbuffer);
        a->mbuffer = NULL;
    }

    if(a->wbuffer != NULL) {
        FREE(a->wbuffer);
        a->wbuffer = NULL;
    }

    if(a->obuffer != NULL) {
        FREE(a->obuffer);
        a->obuffer = NULL;
    }

    if(a->spectrum != NULL) {
        FREE(a->spectrum);
        a->spectrum = NULL;
    }
    return 0;
}

static int
luaanalyzer_update(lua_State *L) {
    unsigned int i = 0;
    SCALAR_TYPE m = 0;
    analyzer *a = NULL;

    lua_getfield(L,1,"analyzer");
    a = lua_touserdata(L,-1);
    lua_getfield(L,1,"amps");

    while(i < processor->buffer_len) {
        if(processor->sampler->decoder->channels == 2) {
            m = (float)(processor->buffer[i * 2] + processor->buffer[(i*2)+1]);
            m /= 2.0f;
        } else {
            m = (float)(processor->buffer[i]);
        }
        m /= 32768.0f;
        a->mbuffer[i] = m * a->wbuffer[i];
        i++;
    }

#ifdef USE_FFTW3
    fftw_execute(a->plan);
#else
    kiss_fftr(a->plan,a->mbuffer,a->obuffer);
#endif

    for(i=0;i<a->bars;i++) {
        a->spectrum[i].amp = find_amplitude_max((double)processor->buffer_len,a->obuffer,a->spectrum[i].first_bin,a->spectrum[i].last_bin);

#ifdef _MSC_VER
		if(!_finite(a->spectrum[i].amp)) {
#else
        if(!isfinite(a->spectrum[i].amp)) {
#endif
            a->spectrum[i].amp = -999.0f;
        }

        a->spectrum[i].amp += a->spectrum[i].boost;

        if(a->spectrum[i].amp <= -AMP_MIN) {
            a->spectrum[i].amp = -AMP_MIN;
        }

        a->spectrum[i].amp += AMP_MIN;

        if(a->spectrum[i].amp > AMP_MAX) {
            a->spectrum[i].amp = AMP_MAX;
        }

        a->spectrum[i].amp /= AMP_MAX;

        a->spectrum[i].amp *= AMP_BOOST; /* i seem to rarely get results near 1.0, let's give this a boost */

        if(a->spectrum[i].amp > 1.0f) {
            a->spectrum[i].amp = 1.0f;
        }

        if(a->firstflag) {
            if(a->spectrum[i].amp < a->spectrum[i].prevamp) {
                a->spectrum[i].amp =
                  a->spectrum[i].amp * SMOOTH_DOWN +
                  a->spectrum[i].prevamp * ( 1 - SMOOTH_DOWN);
            }
            else {
                a->spectrum[i].amp =
                  a->spectrum[i].amp * SMOOTH_UP +
                  a->spectrum[i].prevamp * ( 1 - SMOOTH_UP);
            }
        }
        else {
            a->firstflag = 1;
        }
        a->spectrum[i].prevamp = a->spectrum[i].amp;

        lua_pushinteger(L,i+1);
        lua_pushnumber(L,a->spectrum[i].amp);
        lua_rawset(L,-3);
    }
    return 1;
}

static int
luaanalyzer_new(lua_State *L) {
    int bars;
    double octaves, interval, bin_size, upper_freq, lower_freq;
    analyzer *a;
    unsigned int i = 0;

    if(!lua_isnumber(L,1)) {
        lua_pushnil(L);
        lua_pushstring(L,"missing number of bars to calculate");
        return 2;
    }
    bars = (int)lua_tointeger(L,1);
    if(bars <= 0) {
        lua_pushnil(L);
        lua_pushstring(L,"invalid bar number value");
        return 2;
    }

    octaves = ceil(my_log2(FREQ_MAX / FREQ_MIN));
    interval = 1.0f / (octaves / (double)bars);
    bin_size = 0.0f;

    lua_newtable(L);
    a = (analyzer *)lua_newuserdata(L,sizeof(analyzer));
    if(a == NULL) {
        return luaL_error(L,"out of memory");
    }
    luaL_getmetatable(L,"lmv.analyzer_ud");
    lua_setmetatable(L,-2);

    lua_setfield(L,-2,"analyzer");

    lua_newtable(L);
    lua_setfield(L,-2,"freqs");

    lua_newtable(L);
    lua_setfield(L,-2,"amps");


    a->bars = (unsigned int)bars;
    a->plan = NULL;
    a->mbuffer = NULL;
    a->wbuffer = NULL;
    a->obuffer = NULL;
    a->spectrum = NULL;

    /* prefer to use newuserdata to allocate data, but depending
     * on library we may need custom malloc function */
    a->mbuffer = (SCALAR_TYPE *)MALLOC(sizeof(SCALAR_TYPE) * processor->buffer_len);
    if(UNLIKELY(a->mbuffer == NULL)) return 0;

    a->wbuffer = (SCALAR_TYPE *)MALLOC(sizeof(SCALAR_TYPE) * processor->buffer_len);
    if(UNLIKELY(a->wbuffer == NULL)) return 0;

    a->obuffer = (COMPLEX_TYPE *)MALLOC(sizeof(COMPLEX_TYPE) * ( (processor->buffer_len / 2) + 1));
    if(UNLIKELY(a->obuffer == NULL)) return 0;

    a->spectrum = (frange *)MALLOC(sizeof(frange) * (a->bars + 1));
    if(UNLIKELY(a->spectrum == NULL)) return 0;

    mem_set(a->mbuffer,0,sizeof(SCALAR_TYPE)*processor->buffer_len);
    mem_set(a->wbuffer,0,sizeof(SCALAR_TYPE)*processor->buffer_len);
    mem_set(a->obuffer,0,sizeof(COMPLEX_TYPE) * ((processor->buffer_len/2)+1) );

    bin_size = (double)processor->sampler->samplerate / ((double)processor->buffer_len);
#ifdef USE_FFTW3
    a->plan = fftw_plan_dft_r2c_1d(processor->buffer_len,a->mbuffer,a->obuffer,FFTW_ESTIMATE);
#else
    a->plan = kiss_fftr_alloc(processor->buffer_len, 0, NULL, NULL);
#endif
    if(UNLIKELY(a->plan == NULL)) return 0;

    while(i < processor->buffer_len) {
        a->wbuffer[i] = window_blackman_harris(i,processor->buffer_len);
        i++;
    }
    a->firstflag = 0;

    for(i=0;i<a->bars + 1;i++) {
        a->spectrum[i].amp = 0.0f;
        a->spectrum[i].prevamp = 0.0f;
        if(i==0) {
            a->spectrum[i].freq = FREQ_MIN;
        }
        else {
            /* see http://www.zytrax.com/tech/audio/calculator.html#centers_calc */
            a->spectrum[i].freq = a->spectrum[i-1].freq * pow(10, 3 / (10 * interval));
        }

        /* fudging this a bit to avoid overlap */
        upper_freq = a->spectrum[i].freq * pow(10, (3 * 1) / (10 * 2 * floor(interval)));
        lower_freq = a->spectrum[i].freq / pow(10, (3 * 1) / (10 * 2 * ceil(interval)));

        a->spectrum[i].first_bin = (unsigned int)floor(lower_freq / bin_size);
        a->spectrum[i].last_bin = (unsigned int)floor(upper_freq / bin_size);

        if(a->spectrum[i].last_bin > processor->buffer_len / 2) {
            a->spectrum[i].last_bin = processor->buffer_len / 2;
        }
        /* figure out the ITU-R 468 weighting to apply */
        a->spectrum[i].boost = itur_468(a->spectrum[i].freq);
    }

    lua_getfield(L,-1,"freqs");
    for(i=0;i<a->bars;i++) {
        lua_pushinteger(L,i+1);
        lua_pushnumber(L,a->spectrum[i].freq);
        lua_rawset(L,-3);
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"amps");
    for(i=0;i<a->bars;i++) {
        lua_pushinteger(L,i+1);
        lua_pushnumber(L,0.0f);
        lua_rawset(L,-3);
    }
    lua_pop(L,1);

    luaL_getmetatable(L,"lmv.analyzer");
    lua_setmetatable(L,-2);

    return 1;
}

void luaaudio_init(lua_State *L,audio_processor *a) {
#ifndef NDEBUG
    int lua_top = lua_gettop(L);
#endif
    processor = a;

    if(luaL_newmetatable(L,"lmv.analyzer_ud")) {
        lua_pushcclosure(L, luaanalyzer__gc, 0);
        lua_setfield(L,-2,"__gc");
    }
    lua_pop(L,1);

    if(luaL_newmetatable(L,"lmv.analyzer")) {
        lua_newtable(L);
        lua_pushcclosure(L, luaanalyzer_update, 0);
        lua_setfield(L,-2,"update");
        lua_setfield(L,-2,"__index");
    }
    lua_pop(L,1);

#ifndef NDEBUG
    assert(lua_top == lua_gettop(L));
#endif
}

int luaopen_audio(lua_State *L) {
    lua_newtable(L);
    lua_pushinteger(L,processor->sampler->samplerate);
    lua_setfield(L,-2,"samplerate");

    lua_pushinteger(L,processor->sampler->decoder->channels);
    lua_setfield(L,-2,"channels");

    lua_pushcclosure(L,luaanalyzer_new,0);
    lua_setfield(L,-2,"analyzer");

    return 1;
}



