#include "video-generator.h"
#include "stream.lua.lh"
#include "font.lua.lh"
#include "lua-audio.h"
#include "lua-image.h"
#include "lua-file.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "str.h"
#include <limits.h>
#include <libgen.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <io.h>
#include <fcntl.h>
#endif

static void format_dword(uint8_t *buf, int32_t n) {
    *(buf+0) = n;
    *(buf+1) = n >> 8;
    *(buf+2) = n >> 16;
    *(buf+3) = n >> 24;
}

#define format_long(b,n) format_dword(b,n)

static void format_word(uint8_t *buf, int16_t n) {
    *(buf+0) = n;
    *(buf+1) = n >> 8;
}

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
    unsigned int r = 0;

    str_cpy((char *)buf,"RIFF");
    format_dword(buf+4,0);

    str_cpy((char *)buf+8,"AVI ");

    str_cpy((char *)buf+12,"LIST");

    format_dword(buf+16,294);

    str_cpy((char *)buf+20,"hdrl");

    str_cpy((char *)buf+24,"avih");
    format_dword(buf+28,56);

    format_dword(buf+32,1000000 / v->fps); /* dwMicroSecPerFrame */
    format_dword(buf+36, (v->framebuf_len) + (v->samples_per_frame * 2 * v->processor->decoder->channels));
    /* ^ dwMaxBytesPerSec */
    format_dword(buf+40,0); /* dwPaddingGranularity */
    format_dword(buf+44,0); /* dwFlags */
    format_dword(buf+48,0); /* dwFrames */
    format_dword(buf+52,0); /* dwTotalFrames */
    format_dword(buf+56,2); /* dwStreams */
    format_dword(buf+60,(v->framebuf_len) + (v->samples_per_frame * 2 * v->processor->decoder->channels));
    /* ^ dwSuggestedBufferSize */
    format_dword(buf+64,v->width); /* dwWidth*/
    format_dword(buf+68,v->height); /* dwHeight */
    format_dword(buf+72,0); /* dwReserverd[0] */
    format_dword(buf+76,0); /* dwReserverd[1] */
    format_dword(buf+80,0); /* dwReserverd[2] */
    format_dword(buf+84,0); /* dwReserverd[3] */

    str_cpy((char *)buf+88,"LIST");
    format_dword(buf+92,116);
    str_cpy((char *)buf+96,"strl");
    str_cpy((char *)buf+100,"strh");
    format_dword(buf+104,56);
    str_cpy((char *)buf+108,"vids");
    format_dword(buf+112,0); /* fcchandler*/
    format_dword(buf+116,0); /* flags */
    format_word(buf+120,0); /* priority */
    format_word(buf+122,0); /* language */
    format_dword(buf+124,0); /* initialframes */
    format_dword(buf+128,1); /* scale */
    format_dword(buf+132,v->fps); /* rate */
    format_dword(buf+136,0); /* start */
    format_dword(buf+140,0); /* length */
    format_dword(buf+144,v->framebuf_len); /* bufferSize*/
    format_dword(buf+148, 0); /* quality */
    format_dword(buf+152, 0); /* sampleSize */
    format_word(buf+156,0); /* top left right bottom (or whatever */
    format_word(buf+158,0);
    format_word(buf+160,0);
    format_word(buf+162,0);
    str_cpy((char *)buf+164,"strf");
    format_dword(buf+168,40); /* struct size */
    format_dword(buf+172,40); /* struct size */
    format_dword(buf+176,v->width);
    format_dword(buf+180,v->height);
    format_word(buf+184,1);
    format_word(buf+186,24); /* bit count */
    format_dword(buf+188,0); /* compression */
    format_dword(buf+192,v->framebuf_len); /* image size */
    format_dword(buf+196,0); /* pixels per meter stuff, next 4 */
    format_dword(buf+200,0);
    format_dword(buf+204,0);
    format_dword(buf+208,0);
    str_cpy((char *)buf+212,"LIST");
    format_dword(buf+216,94);
    str_cpy((char *)buf+220,"strl");
    str_cpy((char *)buf+224,"strh");
    format_dword(buf+228,56);
    str_cpy((char *)buf+232,"auds");
    format_dword(buf+236,1);
    format_dword(buf+240,0); /* flags */
    format_word(buf+244,0); /* prio */
    format_word(buf+246,0); /* lang */
    format_dword(buf+248,0); /* frames */
    format_dword(buf+252,1); /* scale */
    format_dword(buf+256,v->processor->decoder->samplerate); /* rate */
    format_dword(buf+260,0); /* start */
    format_dword(buf+264,0); /* length */
    format_dword(buf+268,v->processor->decoder->samplerate * v->processor->decoder->channels * 2);
    format_dword(buf+272,0); /* quality */
    format_dword(buf+276,v->processor->decoder->channels * 2); /* samplesize */
    format_word(buf+280,0); /* left top right bottom */
    format_word(buf+282,0);
    format_word(buf+284,0);
    format_word(buf+286,0);
    str_cpy((char *)buf+288,"strf");
    format_dword(buf+292,18);
    format_word(buf+296,1);
    format_word(buf+298,v->processor->decoder->channels);
    format_dword(buf+300,v->processor->decoder->samplerate);
    format_dword(buf+304,v->processor->decoder->samplerate * v->processor->decoder->channels * 2);
    format_word(buf+308,v->processor->decoder->channels * 2);
    format_word(buf+310,16);
    format_word(buf+312,0);
    str_cpy((char *)buf+314,"LIST");
    format_dword(buf+318,0);
    str_cpy((char *)buf+322,"movi");

#ifdef _WIN32
    WriteFile( (HANDLE)v->outHandle,buf,326,&r,NULL);
#else
    r = fwrite(buf,1,326,(FILE *)v->outHandle);
#endif
    if(r != 326) {
        fprintf(stderr,"avi_header: wanted to write 326 bytes, wrote: %u\n",r);
        return 1;
    }

    return 0;
}


void video_generator_close(video_generator *v) {
#ifdef _WIN32
    CloseHandle( (HANDLE)v->outHandle);
#endif
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

    do {

    samps = audio_processor_process(v->processor, v->samples_per_frame);
    r = samps < v->samples_per_frame;

    memset(v->framebuf,0,v->framebuf_len);

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


    lua_rawgeti(v->L,LUA_REGISTRYINDEX,v->frame_ref);
    if(lua_pcall(v->L,0,0,0)) {
        fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
    }

    lua_getglobal(v->L,"song");
    lua_pushnumber(v->L,v->elapsed / 1000.0f);
    lua_setfield(v->L,-2,"elapsed");
    v->elapsed += v->ms_per_frame;

    lua_gc(v->L,LUA_GCCOLLECT,0);

    wake_queue();

#ifdef _WIN32
    WriteFile( (HANDLE)v->outHandle,v->vid_header,8,&i,NULL);
#else
    i = fwrite(v->vid_header,1,8,(FILE *)v->outHandle);
#endif
    if(i != 8) {
        fprintf(stderr,"Error on vid_header - wanted to write %u bytes, only wrote: %u\n",8,i);
        return 1;
    }

#ifdef _WIN32
    WriteFile( (HANDLE)v->outHandle,v->framebuf,v->framebuf_len,&i,NULL);
#else
    i = fwrite(v->framebuf,1,v->framebuf_len,(FILE *)v->outHandle);
#endif
    if(i != v->framebuf_len) {
        fprintf(stderr,"Error on vid_frame - wanted to write %u bytes, only wrote: %u\n",v->framebuf_len,i);
        return 1;
    }

#ifdef _WIN32
    WriteFile( (HANDLE)v->outHandle,v->aud_header,8,&i,NULL);
#else
    i = fwrite(v->aud_header,1,8,(FILE *)v->outHandle);
#endif
    if(i != 8) {
        fprintf(stderr,"Error on aud_header - wanted to write %u bytes, only wrote: %u\n",8,i);
        return 1;
    }

#ifdef _WIN32
    WriteFile( (HANDLE)v->outHandle,&(v->processor->buffer[pro_offset]),v->samples_per_frame * v->processor->decoder->channels * 2, &i, NULL);
#else
    i = fwrite(&(v->processor->buffer[pro_offset]),1,v->samples_per_frame * v->processor->decoder->channels * 2,(FILE *)v->outHandle);
#endif
    if(i != v->samples_per_frame * v->processor->decoder->channels * 2) {
        fprintf(stderr,"Error on aud_data - wanted to write %u bytes, only wrote: %u\n",v->samples_per_frame * v->processor->decoder->channels * 2,i);
        return 1;
    }
    } while(r == 0);

    return r;
}

int video_generator_init(video_generator *v, audio_processor *p, audio_decoder *d, const char *filename, const char *luascript, void *outHandle) {
    v->outHandle = outHandle;
    char *rpath;
    char *tmp;
    v->framebuf_len = v->width * v->height * 3;

    v->framebuf = malloc(v->framebuf_len);
    if(v->framebuf == NULL) {
        fprintf(stderr,"out of memory\n");
        return 1;
    }

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
    v->frame_ref = -1;

    thread_queue_init(&(v->image_queue),100,(void **)&(v->images),0);

    v->L = luaL_newstate();
    if(!v->L) {
        fprintf(stderr,"error opening lua\n");
        return 1;
    }

    luaL_openlibs(v->L);

#ifdef _WIN32
    _fullpath(rpath,luascript,PATH_MAX);
#else
    realpath(luascript,rpath);
#endif
    dir = dirname(rpath);
    str_cpy(tmp,dir);

    str_cpy(rpath,"package.path = '");
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

    lua_pushlightuserdata(v->L,v->framebuf);
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
        v->load_ref = -1;
        v->frame_ref = luaL_ref(v->L,LUA_REGISTRYINDEX);
    }
    else if(lua_istable(v->L,-1)) {
        lua_getfield(v->L,-1,"onframe");
        if(lua_isfunction(v->L,-1)) {
            v->frame_ref = luaL_ref(v->L,LUA_REGISTRYINDEX);
        } else {
            lua_pop(v->L,1);
        }

        lua_getfield(v->L,-1,"onload");
        if(lua_isfunction(v->L,-1)) {
            if(lua_pcall(v->L,0,0,0)) {
                fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
                return 1;
            }
        } else {
            lua_pop(v->L,1);
        }
    }
    lua_settop(v->L,0);

    str_cpy((char *)v->vid_header,"00db");
    format_dword(v->vid_header + 4, v->width * v->height * 3);

    str_cpy((char *)v->aud_header,"01wb");
    format_dword(v->aud_header + 4, v->samples_per_frame * v->processor->decoder->channels * 2);

    if(write_avi_header(v)) return 1;

    free(rpath);
    free(tmp);
    return 0;
}

