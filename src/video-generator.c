#ifndef USE_OLD_FONT
#define USE_OLD_FONT 0
#endif

#include "int.h"
#include "norm.h"
#include "path.h"
#include "mpd_ez.h"
#include "video-generator.h"
#include "mpdc.h"
#include "util.h"
#include "jpr_proc.h"
#include "stream.lua.lh"
#include "lua-audio.h"
#include "lua-datetime.h"
#if USE_OLD_FONT
#include "font.lua.lh"
#else
#include "lua-bdf.h"
#endif
#include "lua-image.h"
#include "lua-file.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "str.h"
#include "pack.h"
#include "attr.h"
#include <limits.h>

#ifdef LUA_JITLIBNAME
#include <luajit.h>
#endif

#ifdef CHECK_PERFORMANCE
#include <stdio.h>
static double video_times[100];
static double audio_times[100];
static double frame_times[100];
static unsigned int framecounter;
#ifdef JPR_WINDOWS
#include <windows.h>
#define PERF_COUNTER LARGE_INTEGER
#define SAVE_COUNTER(c) QueryPerformanceCounter((c))
#define SAVE_COUNTER_DIFF(dest,start,end) dest =  (double)((end.QuadPart - start.QuadPart) / ((double)perf_freq.QuadPart))
static LARGE_INTEGER perf_freq;
#else
#include <time.h>
#define PERF_COUNTER struct timespec
#define SAVE_COUNTER(c) clock_gettime(CLOCK_MONOTONIC,(c))
#define SAVE_COUNTER_DIFF(dest,start,end) dest = ts_to_sec(&(end)) - ts_to_sec(&(start))
static attr_inline double ts_to_sec(struct timespec *ts) {
    return (double)ts->tv_sec + (double)ts->tv_nsec / 1000000000.0;
}
#endif
#else
#define fprintf(...)
#endif

#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>

#define format_dword(buf,n) pack_int32le(buf,n)
#define format_long(buf,n) pack_int32le(buf,n)
#define format_word(buf,n) pack_int16le(buf,n)

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

/* global lua_State variable for systemtap */
lua_State *globalL = NULL;

static int
lua_send_message_offline(lua_State *L) {
    (void)L;
    return 0;
}

static int
lua_send_message(lua_State *L) {
  mpdc_connection *ctx;
  const char *message;
  int r;

  ctx = lua_touserdata(L, lua_upvalueindex(1));
  message = lua_tostring(L,1);

  if(UNLIKELY(message == NULL)) {
      lua_pushboolean(L,0);
      return 1;
  }

  r = mpdc_sendmessage(ctx,"visualizer",message);

  if(UNLIKELY(r <= 0)) {
      lua_pushboolean(L,0);
  }
  else {
      lua_pushboolean(L,1);
  }
  return 1;
}

static void onchange(void *ctx, const char *type) {
    const char *err_str;
    video_generator *v = ctx;
    lua_rawgeti(v->L,LUA_REGISTRYINDEX,v->lua_ref);
    if(lua_isnil(v->L,-1)) {
        lua_pop(v->L,1);
        return;
    }

    lua_getfield(v->L,-1,"onchange");
    if(lua_isfunction(v->L,-1)) {
        lua_pushvalue(v->L,-2);
        lua_pushstring(v->L,type);
        if(lua_pcall(v->L,2,0,0)) {
            err_str = lua_tostring(v->L,-1);
            WRITE_STDERR("error: ");
            LOG_ERROR(err_str);
        }
    } else {
        lua_pop(v->L,1);
    }
    lua_pop(v->L,1);
}

static void onmeta(void *ctx, const char *key, const char *value) {
    video_generator *v = ctx;
    lua_getglobal(v->L,"song");
    lua_pushstring(v->L,value);
    lua_setfield(v->L,-2,key);
    lua_pop(v->L,1);
}

static void onmeta_double(void *ctx, const char *key, double value) {
    video_generator *v = ctx;
    lua_getglobal(v->L,"song");
    lua_pushnumber(v->L,value);
    lua_setfield(v->L,-2,key);
    lua_pop(v->L,1);
    if(str_icmp(key,"total") == 0) {
        v->duration = value;
    } else if(str_icmp(key,"elapsed") == 0) {
        v->elapsed = value;
    }
}

static int write_avi_header(video_generator *v) {
    jpr_uint8 buf[327];
    jpr_uint8 *b = buf;
    unsigned int r = 0;

    str_cpy((char *)b,"RIFF");
    b += 4;
    b += format_dword(b,0);
    str_cpy((char *)b,"AVI ");
    b += 4;
    str_cpy((char *)b,"LIST");
    b += 4;
    b += format_dword(b,294);
    str_cpy((char *)b,"hdrl");
    b += 4;
    str_cpy((char *)b,"avih");
    b += 4;
    b += format_dword(b,56);

    b += format_dword(b,1000000 / v->fps); /* dwMicroSecPerFrame */
    b += format_dword(b, (v->framebuf_video_len) + (v->framebuf_audio_len));
         /* ^ dwMaxBytesPerSec */
    b += format_dword(b,0); /* dwPaddingGranularity */
    b += format_dword(b,0); /* dwFlags */
    b += format_dword(b,0); /* dwFrames */
    b += format_dword(b,0); /* dwTotalFrames */
    b += format_dword(b,2); /* dwStreams */
    b += format_dword(b,(v->framebuf_video_len) + (v->framebuf_audio_len));
         /* ^ dwSuggestedBufferSize */
    b += format_dword(b,v->width); /* dwWidth*/
    b += format_dword(b,v->height); /* dwHeight */
    b += format_dword(b,0); /* dwReserverd[0] */
    b += format_dword(b,0); /* dwReserverd[1] */
    b += format_dword(b,0); /* dwReserverd[2] */
    b += format_dword(b,0); /* dwReserverd[3] */

    str_cpy((char *)b,"LIST");
    b += 4;
    b += format_dword(b,116);
    str_cpy((char *)b,"strl");
    b += 4;
    str_cpy((char *)b,"strh");
    b += 4;
    b += format_dword(b,56);
    str_cpy((char *)b,"vids");
    b += 4;
    b += format_dword(b,0); /* fcchandler*/
    b += format_dword(b,0); /* flags */
    b += format_word(b,0); /* priority */
    b += format_word(b,0); /* language */
    b += format_dword(b,0); /* initialframes */
    b += format_dword(b,1); /* scale */
    b += format_dword(b,v->fps); /* rate */
    b += format_dword(b,0); /* start */
    b += format_dword(b,0); /* length */
    b += format_dword(b,v->framebuf_video_len); /* bufferSize*/
    b += format_dword(b, 0); /* quality */
    b += format_dword(b, 0); /* sampleSize */
    b += format_word(b,0); /* top left right bottom (or whatever */
    b += format_word(b,0);
    b += format_word(b,0);
    b += format_word(b,0);
    str_cpy((char *)b,"strf");
    b += 4;
    b += format_dword(b,40); /* struct size */
    b += format_dword(b,40); /* struct size */
    b += format_dword(b,v->width);
    b += format_dword(b,v->height);
    b += format_word(b,1);
    b += format_word(b,24); /* bit count */
    b += format_dword(b,0); /* compression */
    b += format_dword(b,v->framebuf_video_len); /* image size */
    b += format_dword(b,0); /* pixels per meter stuff, next 4 */
    b += format_dword(b,0);
    b += format_dword(b,0);
    b += format_dword(b,0);
    str_cpy((char *)b,"LIST");
    b += 4;
    b += format_dword(b,94);
    str_cpy((char *)b,"strl");
    b += 4;
    str_cpy((char *)b,"strh");
    b += 4;
    b += format_dword(b,56);
    str_cpy((char *)b,"auds");
    b += 4;
    b += format_dword(b,1);
    b += format_dword(b,0); /* flags */
    b += format_word(b,0); /* prio */
    b += format_word(b,0); /* lang */
    b += format_dword(b,0); /* frames */
    b += format_dword(b,1); /* scale */
    b += format_dword(b,v->sampler->samplerate); /* rate */
    b += format_dword(b,0); /* start */
    b += format_dword(b,0); /* length */
    b += format_dword(b,v->framebuf_audio_len);
    b += format_dword(b,0); /* quality */
    b += format_dword(b,v->decoder->channels * sizeof(jpr_int16)); /* samplesize */
    b += format_word(b,0); /* left top right bottom */
    b += format_word(b,0);
    b += format_word(b,0);
    b += format_word(b,0);
    str_cpy((char *)b,"strf");
    b += 4;
    b += format_dword(b,18);
    b += format_word(b,1);
    b += format_word(b,v->decoder->channels);
    b += format_dword(b,v->sampler->samplerate);
    b += format_dword(b,v->framebuf_audio_len);
    b += format_word(b,v->decoder->channels * sizeof(jpr_int16));
    b += format_word(b,16);
    b += format_word(b,0);
    str_cpy((char *)b,"LIST");
    b += 4;
    b += format_dword(b,0);
    str_cpy((char *)b,"movi");
    b += 4;
    (void)b;

    if(UNLIKELY(jpr_proc_pipe_write(v->out,(const char *)buf,326,&r))) return 1;
    if(UNLIKELY(r != 326)) return 1;
    return 0;
}


void video_generator_close(video_generator *v) {
#ifndef NDEBUG
    int stack_top = lua_gettop(v->L);
#endif
    if(v->lua_ref != -1) {
      lua_rawgeti(v->L,LUA_REGISTRYINDEX,v->lua_ref);
      lua_getfield(v->L,-1,"onunload");
      if(lua_isfunction(v->L,-1)) {
          lua_pushvalue(v->L,-2);
          lua_pcall(v->L,1,0,0);

      } else {
          lua_pop(v->L,1);
      }
      lua_pop(v->L,1);
    }
#ifndef NDEBUG
    assert(stack_top == lua_gettop(v->L));
#endif
    if(v->mpd != NULL) {
        mpdc_disconnect(v->mpd);
        free(v->mpd->ctx);
        free(v->mpd);
    }
    luaclose_image();
    lua_close(v->L);
    audio_decoder_close(v->decoder);
    audio_resampler_close(v->sampler);
    audio_processor_close(v->processor);
    thread_queue_term(&(v->image_queue));
    free(v->framebuf);
}

int video_generator_loop(video_generator *v) {
#ifndef NDEBUG
    int lua_top;
#endif
#ifdef CHECK_PERFORMANCE
    double avg_frame_time;
    double avg_video_time;
    double avg_audio_time;
    PERF_COUNTER frame_start;
    PERF_COUNTER frame_end;
    PERF_COUNTER video_start;
    PERF_COUNTER video_end;
    PERF_COUNTER audio_start;
    PERF_COUNTER audio_end;
#endif
    unsigned int pro_offset;
    jpr_uint64 samps;
    int r = 0;
    unsigned int i = 0;
    image_q *q = NULL;

#ifdef CHECK_PERFORMANCE
    avg_frame_time = 0.0f;
    avg_video_time = 0.0f;
    avg_audio_time = 0.0f;

    if (framecounter == 100) {
        for(framecounter=0;framecounter < 100; framecounter++) {
            avg_audio_time += audio_times[framecounter];
            avg_video_time += video_times[framecounter];
            avg_frame_time += frame_times[framecounter];
        }
        avg_video_time /= 100.0;
        avg_video_time *= 1000.0;
        avg_audio_time /= 100.0;
        avg_audio_time *= 1000.0;
        avg_frame_time = 100.0 / avg_frame_time;
        fprintf(stderr,"fps: %f, audio: %f ms, video: %f ms\n",avg_frame_time, avg_audio_time,avg_video_time);
        framecounter = 0;
    }

    SAVE_COUNTER(&frame_start);
#endif

    if(v->mpd != NULL) {
        mpd_ez_loop(v);
    }

    pro_offset = (v->processor->buffer_len * sizeof(jpr_int16)) - (v->samples_per_frame * v->decoder->channels);

#ifdef CHECK_PERFORMANCE
    SAVE_COUNTER(&audio_start);
#endif
    samps = audio_processor_process(v->processor, v->samples_per_frame);
#ifdef CHECK_PERFORMANCE
    SAVE_COUNTER(&audio_end);
    SAVE_COUNTER_DIFF(audio_times[framecounter],audio_start,audio_end);
#endif
    r = samps < v->samples_per_frame;

    mem_set(v->framebuf+8,0,v->framebuf_video_len);

    while(thread_queue_count(&(v->image_queue)) > 0) {
        q = thread_queue_consume(&(v->image_queue));
        if(LIKELY(q != NULL)) {
            lua_load_image_cb(v->L,q->table_ref,q->frames,q->image);
            luaL_unref(v->L,LUA_REGISTRYINDEX,q->table_ref);
            free(q->filename);
            free(q);
            q = NULL;
        }
    }

    if(v->lua_ref != -1) {
#ifndef NDEBUG
      lua_top = lua_gettop(v->L);
#endif
      lua_rawgeti(v->L,LUA_REGISTRYINDEX,v->lua_ref);
      lua_getfield(v->L,-1,"onframe");
      if(LIKELY(lua_isfunction(v->L,-1))) {
          lua_pushvalue(v->L,-2);
#ifdef CHECK_PERFORMANCE
          SAVE_COUNTER(&video_start);
#endif
          if(lua_pcall(v->L,1,0,0)) {
              fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
          }
#ifdef CHECK_PERFORMANCE
          SAVE_COUNTER(&video_end);
          SAVE_COUNTER_DIFF(video_times[framecounter],video_start,video_end);
          framecounter++;
#endif

      } else {
          lua_pop(v->L,1);
      }
      lua_pop(v->L,1);
#ifndef NDEBUG
      assert(lua_top == lua_gettop(v->L));
#endif
    }

#ifndef NDEBUG
    lua_top = lua_gettop(v->L);
#endif
    lua_getglobal(v->L,"song");
    lua_pushnumber(v->L,v->elapsed / 1000.0f);
    lua_setfield(v->L,-2,"elapsed");
    lua_pop(v->L,1);

#ifndef NDEBUG
    assert(lua_top == lua_gettop(v->L));
#endif

    v->elapsed += v->ms_per_frame;

    lua_gc(v->L,LUA_GCSTEP,0);

    wake_queue();

    mem_cpy(v->framebuf + 16 + v->framebuf_video_len,(jpr_uint8 *)&(v->processor->buffer[pro_offset]),v->framebuf_audio_len);
    if(UNLIKELY(jpr_proc_pipe_write(v->out,(const char *)v->framebuf,v->framebuf_len,&i))) return 1;

    if(UNLIKELY(i != v->framebuf_len)) return 1;

#ifdef CHECK_PERFORMANCE
    SAVE_COUNTER(&frame_end);
    SAVE_COUNTER_DIFF(frame_times[framecounter],frame_start,frame_end);
#endif

    return r;
}

int video_generator_reload(video_generator *v) {
    lua_rawgeti(v->L,LUA_REGISTRYINDEX,v->lua_ref);
    lua_getfield(v->L,-1,"onreload");
    if(lua_isfunction(v->L,-1)) {
        lua_pushvalue(v->L,-2);
        lua_pcall(v->L,1,0,0);

    } else {
        lua_pop(v->L,1);
    }
    lua_pop(v->L,1);
    return 0;
}

int video_generator_init(video_generator *v, audio_processor *p, audio_resampler *r, audio_decoder *d, int jit, const char *modulename, const char *filename, const char *luascript, jpr_proc_pipe *out) {
    char *rpath;
    char *dir;
    char *script_path;
    const char *err_str;
    int lua_top;

    v->mpd = NULL;
    v->out = out;

    rpath = malloc(sizeof(char)*PATH_MAX);
    if(UNLIKELY(rpath == NULL)) {
        LOG_ERROR("out of memory");
        return 1;
    }
    rpath[0] = 0;

    if(audio_decoder_init(d)) {
        LOG_ERROR("init audio decoder failed");
        free(rpath);
        return 1;
    }
    if(audio_resampler_init(r)) {
        LOG_ERROR("init audio resampler failed");
        free(rpath);
    }

    mpd_ez_setup(v);

    d->onmeta        = onmeta;
    d->onmeta_double = onmeta_double;
    d->onchange      = onchange;
    d->meta_ctx = (void *)v;
    v->lua_ref = -1;

    thread_queue_init(&(v->image_queue),100,(void **)&(v->images),0);

    v->L = luaL_newstate();
    if(!v->L) {
        LOG_ERROR("init lua failed");
        free(rpath);
        return 1;
    }
    globalL = v->L;

    luaL_openlibs(v->L);
#ifdef LUA_JITLIBNAME
    if(jit == 0) {
        luaJIT_setmode(v->L,0,LUAJIT_MODE_OFF);
    }
#else
    (void)jit;
#endif

    script_path = path_absolute(luascript);
    if(UNLIKELY(script_path == NULL)) {
        WRITE_STDERR("error finding the full path for ");
        LOG_ERROR(luascript);
        free(rpath);
        lua_close(v->L);
        globalL = NULL;
        return 1;
     }

    dir = path_dirname(script_path);
    if(UNLIKELY(dir == NULL)) {
        WRITE_STDERR("error finding dirname for ");
        LOG_ERROR(luascript);
        free(rpath);
        free(script_path);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

    str_cpy(rpath,"package.path = '");
    str_ecat(rpath,dir,"\\",'\\');

#ifdef _WIN32
    str_cat(rpath,"\\\\?.lua;' .. package.path");
#else
    str_cat(rpath,"/?.lua;' .. package.path");
#endif

    if(luaL_loadbuffer(v->L,rpath,str_len(rpath),"package_path") || lua_pcall(v->L,0,LUA_MULTRET,0)) {
        err_str = lua_tostring(v->L,-1);
        WRITE_STDERR("error setting lua package path: ");
        LOG_ERROR(err_str);
        free(rpath);
        free(dir);
        free(script_path);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

    if(modulename != NULL) {
        lua_top = lua_gettop(v->L);
        str_cpy(rpath,"require('");
        str_cat(rpath,modulename);
        str_cat(rpath,"')");
        if(luaL_loadbuffer(v->L,rpath,str_len(rpath),"require_module") || lua_pcall(v->L,0,LUA_MULTRET,0)) {
            err_str = lua_tostring(v->L,-1);
            WRITE_STDERR("error loading lua module");
            LOG_ERROR(err_str);
            free(rpath);
            free(dir);
            free(script_path);
            lua_close(v->L);
            globalL = NULL;
            return 1;
        }
        lua_settop(v->L,lua_top);
    }


#ifndef NDEBUG
    lua_top = lua_gettop(v->L);
#endif

    lua_newtable(v->L);
    lua_setglobal(v->L,"song");

#ifndef NDEBUG
    assert(lua_top == lua_gettop(v->L));
#endif

    /* open audio file and populate metadata */
    if(audio_decoder_open(d,filename)) {
        LOG_ERROR("error opening audio decoder");
        free(rpath);
        free(dir);
        free(script_path);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

    if(audio_resampler_open(r,d)) {
        LOG_ERROR("error opening audio resampler");
        free(rpath);
        free(dir);
        free(script_path);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

    if(r->samplerate % v->fps != 0) {
        LOG_ERROR("error: fps does not divide cleanly into samplerate");
        free(rpath);
        free(dir);
        free(script_path);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

    v->samples_per_frame = r->samplerate / v->fps;
    v->ms_per_frame = 1000.0f / ((double)v->fps);
    v->elapsed = 0;
    v->mpd_tags = 0;
    v->duration = (((double)d->framecount) / ((double)d->samplerate));

    v->framebuf_video_len = v->width * v->height * 3;
    v->framebuf_audio_len = v->samples_per_frame * d->channels * sizeof(jpr_int16);
    v->framebuf_len = v->framebuf_video_len + v->framebuf_audio_len + 16;

    v->framebuf = malloc(v->framebuf_len);
    if(UNLIKELY(v->framebuf == NULL)) {
        LOG_ERROR("out of memory");
        free(rpath);
        free(dir);
        free(script_path);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

    lua_getglobal(v->L,"song");
    lua_pushnumber(v->L,0.0f);
    lua_setfield(v->L,-2,"elapsed");
    lua_pushnumber(v->L, v->duration);
    lua_setfield(v->L,-2,"total");
    lua_pushstring(v->L,filename);
    lua_setfield(v->L,-2,"file");
    if(v->mpd == NULL) {
        lua_pushcfunction(v->L, lua_send_message_offline);
        lua_setfield(v->L,-2,"sendmessage");
    }
    else {
        lua_pushlightuserdata(v->L,v->mpd);
        lua_pushcclosure(v->L, lua_send_message,1);
        lua_setfield(v->L,-2,"sendmessage");
    }

    lua_pop(v->L,1);

#ifndef NDEBUG
    assert(lua_top == lua_gettop(v->L));
#endif

    if(audio_processor_init(p,r,v->samples_per_frame)) {
        LOG_ERROR("init audio processor error");
        free(rpath);
        free(dir);
        free(script_path);
        free(v->framebuf);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

    v->decoder = d;
    v->sampler = r;
    v->processor = p;

    luaopen_datetime(v->L);

    luaopen_image(v->L);
#ifndef NDEBUG
    assert(lua_top == lua_gettop(v->L));
#endif

    luaimage_setup_threads(&(v->image_queue));
#ifndef NDEBUG
    assert(lua_top == lua_gettop(v->L));
#endif

    luaopen_file(v->L);
#ifndef NDEBUG
    assert(lua_top == lua_gettop(v->L));
#endif

#if USE_OLD_FONT
    if(luaL_loadbuffer(v->L,font_lua,font_lua_length-1,"font.lua")) {
        err_str = lua_tostring(v->L,-1);
        WRITE_STDERR("error: ");
        LOG_ERROR(err_str);
        free(rpath);
        free(dir);
        free(script_path);
        free(v->framebuf);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

    if(lua_pcall(v->L,0,1,0)) {
        err_str = lua_tostring(v->L,-1);
        WRITE_STDERR("error: ");
        LOG_ERROR(err_str);
        free(rpath);
        free(dir);
        free(script_path);
        free(v->framebuf);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

#else

    if(UNLIKELY(luaopen_bdf(v->L) == 0)) {
        free(rpath);
        free(dir);
        free(script_path);
        free(v->framebuf);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

#endif

    lua_setglobal(v->L,"font");

#ifndef NDEBUG
    assert(lua_top == lua_gettop(v->L));
#endif

    lua_getglobal(v->L,"image"); /* push */
    lua_getfield(v->L,-1,"new"); /* push */
    lua_pushnil(v->L);
    lua_pushinteger(v->L,v->width);
    lua_pushinteger(v->L,v->height);
    lua_pushinteger(v->L,3);
    if(lua_pcall(v->L,4,1,0)) { /* pop */
        err_str = lua_tostring(v->L,-1);
        WRITE_STDERR("error: ");
        LOG_ERROR(err_str);
        free(rpath);
        free(dir);
        free(script_path);
        free(v->framebuf);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }
    /* top of the stack has an image table */
    /* there's also the global "image" object still on the stack */

    lua_getfield(v->L,-1,"frames"); /* push */
    lua_rawgeti(v->L,-1,1); /* push */

    lua_pushinteger(v->L,v->fps);
    lua_setfield(v->L,-2,"framerate");

    lua_pushlightuserdata(v->L,v->framebuf+8);
    lua_setfield(v->L,-2,"image");

    lua_newtable(v->L); /* push */
    lua_pushvalue(v->L,-2);
    lua_setfield(v->L,-2,"video"); /* set frames[1] as "video" field on a new table */
    /* stack state (first is top) */
    /*  {table}
     *  frames[1]
     *  frames
     *  image */

    luaopen_audio(v->L,p);
    lua_setfield(v->L,-2,"audio");

    lua_setglobal(v->L,"stream");
    lua_pop(v->L,4);

#ifndef NDEBUG
    assert(lua_top == lua_gettop(v->L));
#endif

    if(luaL_loadbuffer(v->L,stream_lua,stream_lua_length - 1,"stream.lua")) {
        err_str = lua_tostring(v->L,-1);
        WRITE_STDERR("error: ");
        LOG_ERROR(err_str);
        free(rpath);
        free(dir);
        free(script_path);
        free(v->framebuf);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

    if(lua_pcall(v->L,0,0,0)) {
        err_str = lua_tostring(v->L,-1);
        WRITE_STDERR("error: ");
        LOG_ERROR(err_str);
        free(rpath);
        free(dir);
        free(script_path);
        free(v->framebuf);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

#ifndef NDEBUG
    assert(lua_top == lua_gettop(v->L));
#endif

    if(luaL_loadfile(v->L,luascript)) {
        err_str = lua_tostring(v->L,-1);
        WRITE_STDERR("error: ");
        LOG_ERROR(err_str);
        free(rpath);
        free(dir);
        free(script_path);
        free(v->framebuf);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

    if(lua_pcall(v->L,0,1,0)) {
        err_str = lua_tostring(v->L,-1);
        WRITE_STDERR("error: ");
        LOG_ERROR(err_str);
        free(rpath);
        free(dir);
        free(script_path);
        free(v->framebuf);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

    if(lua_isfunction(v->L,-1)) {
        lua_newtable(v->L);
        lua_pushvalue(v->L,-2);
        lua_setfield(v->L,-2,"onframe");

        v->lua_ref = luaL_ref(v->L,LUA_REGISTRYINDEX);
        lua_pop(v->L,1);
    }
    else if(lua_istable(v->L,-1)) {
        v->lua_ref = luaL_ref(v->L,LUA_REGISTRYINDEX);
    }
    else {
        lua_pop(v->L,1);
    }

    if(v->lua_ref != -1) {
        lua_rawgeti(v->L,LUA_REGISTRYINDEX,v->lua_ref);
        lua_getfield(v->L,-1,"onload");
        if(lua_isfunction(v->L,-1)) {
            lua_pushvalue(v->L,-2);
            if(lua_pcall(v->L,1,0,0)) {
                err_str = lua_tostring(v->L,-1);
                WRITE_STDERR("error: ");
                LOG_ERROR(err_str);
                free(rpath);
                free(dir);
                free(script_path);
                free(v->framebuf);
                lua_close(v->L);
                globalL = NULL;
                return 1;
            }
        } else {
            lua_pop(v->L,1);
        }

        lua_pop(v->L,1);
    }
#ifndef NDEBUG
    assert(lua_top == lua_gettop(v->L));
#endif

    mem_cpy(v->framebuf,"00db",4);
    format_dword(v->framebuf + 4, v->framebuf_video_len);

    mem_cpy(&v->framebuf[v->framebuf_video_len + 8],"01wb",4);
    format_dword(v->framebuf + v->framebuf_video_len + 12, v->framebuf_audio_len);

    if(write_avi_header(v)) {
        free(rpath);
        free(dir);
        free(script_path);
        free(v->framebuf);
        lua_close(v->L);
        globalL = NULL;
        return 1;
    }

    if(v->mpd != NULL) {
        mpd_ez_start(v);
    }

    free(rpath);
    free(dir);
    free(script_path);

#ifdef CHECK_PERFORMANCE
    framecounter = 0;
    memset(video_times,0,sizeof(double) * 100);
    memset(audio_times,0,sizeof(double) * 100);
    memset(frame_times,0,sizeof(double) * 100);
#ifdef JPR_WINDOWS
    QueryPerformanceFrequency(&perf_freq);
#endif
#endif
    return 0;
}

int video_generator_using_luajit(void) {
#ifdef LUA_JITLIBNAME
    return 1;
#else
    return 0;
#endif
}
