#include "video-generator.h"
#include "jpr_proc.h"
#include "stream.lua.lh"
#include "font.lua.lh"
#include "lua-audio.h"
#include "lua-image.h"
#include "lua-file.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "str.h"
#include "pack.h"
#include <limits.h>
#include <libgen.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <io.h>
#include <fcntl.h>
#endif

#define format_dword(buf,n) pack_int32le(buf,n)
#define format_long(buf,n) pack_int32le(buf,n)
#define format_word(buf,n) pack_int16le(buf,n)

static void video_generator_set_image_cb(void *ctx, void(*f)(void *, intptr_t , unsigned int, uint8_t *)) {
    video_generator *v = (video_generator *)ctx;
    v->image_cb = f;
}


static void onmeta(void *ctx, const char *key, const char *value) {
    video_generator *v = ctx;
    lua_getglobal(v->L,"song");
    lua_pushstring(v->L,value);
    lua_setfield(v->L,-2,key);
    lua_settop(v->L,0);
}

static int write_avi_header(video_generator *v) {
    uint8_t buf[327];
    uint8_t *b = buf;
    unsigned int r = 0;

    b += str_cpy((char *)b,"RIFF");
    b += format_dword(b,0);
    b += str_cpy((char *)b,"AVI ");
    b += str_cpy((char *)b,"LIST");
    b += format_dword(b,294);
    b += str_cpy((char *)b,"hdrl");
    b += str_cpy((char *)b,"avih");
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

    b += str_cpy((char *)b,"LIST");
    b += format_dword(b,116);
    b += str_cpy((char *)b,"strl");
    b += str_cpy((char *)b,"strh");
    b += format_dword(b,56);
    b += str_cpy((char *)b,"vids");
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
    b += str_cpy((char *)b,"strf");
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
    b += str_cpy((char *)b,"LIST");
    b += format_dword(b,94);
    b += str_cpy((char *)b,"strl");
    b += str_cpy((char *)b,"strh");
    b += format_dword(b,56);
    b += str_cpy((char *)b,"auds");
    b += format_dword(b,1);
    b += format_dword(b,0); /* flags */
    b += format_word(b,0); /* prio */
    b += format_word(b,0); /* lang */
    b += format_dword(b,0); /* frames */
    b += format_dword(b,1); /* scale */
    b += format_dword(b,v->processor->decoder->samplerate); /* rate */
    b += format_dword(b,0); /* start */
    b += format_dword(b,0); /* length */
    b += format_dword(b,v->framebuf_audio_len);
    b += format_dword(b,0); /* quality */
    b += format_dword(b,v->processor->decoder->channels * sizeof(int16_t)); /* samplesize */
    b += format_word(b,0); /* left top right bottom */
    b += format_word(b,0);
    b += format_word(b,0);
    b += format_word(b,0);
    b += str_cpy((char *)b,"strf");
    b += format_dword(b,18);
    b += format_word(b,1);
    b += format_word(b,v->processor->decoder->channels);
    b += format_dword(b,v->processor->decoder->samplerate);
    b += format_dword(b,v->framebuf_audio_len);
    b += format_word(b,v->processor->decoder->channels * sizeof(int16_t));
    b += format_word(b,16);
    b += format_word(b,0);
    b += str_cpy((char *)b,"LIST");
    b += format_dword(b,0);
    b += str_cpy((char *)b,"movi");

    if(jpr_proc_pipe_write(v->out,(const char *)buf,326,&r)) return 1;
    if (r != 326) return 1;
    return 0;
}


void video_generator_close(video_generator *v) {
    if(v->lua_ref != -1) {
      lua_rawgeti(v->L,LUA_REGISTRYINDEX,v->lua_ref);
      lua_getfield(v->L,-1,"onunload");
      if(lua_isfunction(v->L,-1)) {
          lua_pushvalue(v->L,-2);
          lua_pcall(v->L,1,0,0);

      } else {
          lua_pop(v->L,1);
      }
    }
    luaclose_image();
    lua_close(v->L);
    audio_decoder_close(v->processor->decoder);
    audio_processor_close(v->processor);
    thread_queue_term(&(v->image_queue));
    free(v->framebuf);
}

int video_generator_loop(video_generator *v) {
    unsigned int samps;
    int r = 0;
    unsigned int i = 0;
    image_q *q = NULL;

    int pro_offset = 8192 - v->samples_per_frame * v->processor->decoder->channels;

    samps = audio_processor_process(v->processor, v->samples_per_frame);
    r = samps < v->samples_per_frame;

    memset(v->framebuf+8,0,v->framebuf_video_len);

    while(thread_queue_count(&(v->image_queue)) > 0) {
        q = thread_queue_consume(&(v->image_queue));
        if(q != NULL) {
            v->image_cb((void *)v->L,q->table_ref,q->frames,q->image);
            luaL_unref(v->L,LUA_REGISTRYINDEX,q->table_ref);
            free(q->filename);
            free(q);
            q = NULL;
        }
    }


    if(v->lua_ref != -1) {
      lua_rawgeti(v->L,LUA_REGISTRYINDEX,v->lua_ref);
      lua_getfield(v->L,-1,"onframe");
      if(lua_isfunction(v->L,-1)) {
          lua_pushvalue(v->L,-2);
          lua_pcall(v->L,1,0,0);

      } else {
          lua_pop(v->L,1);
      }
    }

    lua_getglobal(v->L,"song");
    lua_pushnumber(v->L,v->elapsed / 1000.0f);
    lua_setfield(v->L,-2,"elapsed");
    v->elapsed += v->ms_per_frame;

    lua_gc(v->L,LUA_GCCOLLECT,0);

    wake_queue();

    memcpy(v->framebuf + 16 + v->framebuf_video_len,(uint8_t *)&(v->processor->buffer[pro_offset]),v->framebuf_audio_len);
    if(jpr_proc_pipe_write(v->out,(const char *)v->framebuf,v->framebuf_len,&i)) return 1;

    if(i != v->framebuf_len) return 1;

    return r;
}

int video_generator_init(video_generator *v, audio_processor *p, audio_decoder *d, const char *filename, const char *luascript, jpr_proc_pipe *out) {
    v->out = out;
    char *rpath;
    char *tmp;
    rpath = malloc(sizeof(char)*PATH_MAX);
    if(rpath == NULL) {
        fprintf(stderr,"out of memory\n");
        return 1;
    }

    tmp = malloc(sizeof(char)*PATH_MAX);
    if(tmp == NULL) {
        fprintf(stderr,"out of memory\n");
        return 1;
    }
    char *dir;
    rpath[0] = 0;
    tmp[0] = 0;

    if(audio_decoder_init(d)) {
        fprintf(stderr,"error with audio_decoder_init\n");
        return 1;
    }

    d->onmeta = onmeta;
    d->meta_ctx = (void *)v;
    v->lua_ref = -1;

    thread_queue_init(&(v->image_queue),100,(void **)&(v->images),0);

    v->L = luaL_newstate();
    if(!v->L) {
        fprintf(stderr,"error opening lua\n");
        return 1;
    }

    luaL_openlibs(v->L);

#ifdef _WIN32
    if(_fullpath(rpath,luascript,PATH_MAX) == NULL) {
#else
    if(realpath(luascript,rpath) == NULL) {
#endif
        fprintf(stderr,"error resolving the lua script path\n");
        return 1;
    }
    dir = dirname(rpath);
    strcpy(tmp,dir);

    strcpy(rpath,"package.path = '");
    str_ecat(rpath,tmp,"\\",'\\');

#ifdef _WIN32
    strcat(rpath,"\\\\?.lua;' .. package.path");
#else
    strcat(rpath,"/?.lua;' .. package.path");
#endif

    if(luaL_dostring(v->L,rpath)) {
        fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
        return 1;
    }

    lua_newtable(v->L);
    lua_setglobal(v->L,"song");
    lua_settop(v->L,0);

    /* open audio file and populate metadata */
    if(audio_decoder_open(d,filename)) {
        fprintf(stderr,"error opening audio decoder\n");
        return 1;
    }
    v->samples_per_frame = d->samplerate / v->fps;
    v->ms_per_frame = 1000.0f / ((double)v->fps);
    v->elapsed = 0.0f;

    v->framebuf_video_len = v->width * v->height * 3;
    v->framebuf_audio_len = v->samples_per_frame * d->channels * sizeof(int16_t);
    v->framebuf_len = v->framebuf_video_len + v->framebuf_audio_len + 16;

    v->framebuf = malloc(v->framebuf_len);
    if(v->framebuf == NULL) {
        fprintf(stderr,"out of memory\n");
        return 1;
    }


    lua_getglobal(v->L,"song");
    lua_pushnumber(v->L,0.0f);
    lua_setfield(v->L,-2,"elapsed");
    lua_pushnumber(v->L, ((double)d->framecount) / ((double)d->samplerate));
    lua_setfield(v->L,-2,"total");
    lua_pushstring(v->L,filename);
    lua_setfield(v->L,-2,"file");
    lua_settop(v->L,0);

    if(audio_processor_init(p,d,v->samples_per_frame)) {
        fprintf(stderr,"error with audio_processor_init\n");
        return 1;
    }

    v->decoder = d;
    v->processor = p;
    v->image_cb = lua_load_image_cb;

    luaopen_image(v->L,v,&video_generator_set_image_cb);
    luaimage_setup_threads(&(v->image_queue));
    luaopen_file(v->L);

    if(luaL_loadbuffer(v->L,font_lua,font_lua_length-1,"font.lua")) {
        fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
        return 1;
    }

    if(lua_pcall(v->L,0,1,0)) {
        fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
        return 1;
    }

    lua_setglobal(v->L,"font");
    lua_settop(v->L,0);

    lua_getglobal(v->L,"image");
    lua_getfield(v->L,-1,"new");
    lua_pushnil(v->L);
    lua_pushinteger(v->L,v->width);
    lua_pushinteger(v->L,v->height);
    lua_pushinteger(v->L,3);
    if(lua_pcall(v->L,4,1,0)) {
        fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
        return 1;
    }

    lua_getfield(v->L,-1,"frames");
    lua_rawgeti(v->L,-1,1);

    lua_pushinteger(v->L,v->fps);
    lua_setfield(v->L,-2,"framerate");

    lua_pushlightuserdata(v->L,v->framebuf+8);
    lua_setfield(v->L,-2,"image");

    lua_newtable(v->L);
    lua_pushvalue(v->L,-2);
    lua_setfield(v->L,-2,"video");

    luaopen_audio(v->L,p);
    lua_setfield(v->L,-2,"audio");

    lua_setglobal(v->L,"stream");
    lua_settop(v->L,0);

    if(luaL_loadbuffer(v->L,stream_lua,stream_lua_length - 1,"stream.lua")) {
        fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
        return 1;
    }

    if(lua_pcall(v->L,0,0,0)) {
        fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
        return 1;
    }

    if(luaL_loadfile(v->L,luascript)) {
        fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
        return 1;
    }

    if(lua_pcall(v->L,0,1,0)) {
        fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
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
                fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
                return 1;
            }
        } else {
            lua_pop(v->L,1);
        }

        lua_pop(v->L,1);
    }
    lua_settop(v->L,0);

    strcpy((char *)v->framebuf,"00db");
    format_dword(v->framebuf + 4, v->framebuf_video_len);

    strcpy((char *)v->framebuf + v->framebuf_video_len + 8,"01wb");
    format_dword(v->framebuf + v->framebuf_video_len + 12, v->framebuf_audio_len);

    if(write_avi_header(v)) return 1;

    free(rpath);
    free(tmp);
    return 0;
}

