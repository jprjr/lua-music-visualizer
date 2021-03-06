#include "int.h"
#include "image.h"
#include "lua-image.h"
#include "image.lua.lh"
#include "thread.h"
#include "str.h"
#include "util.h"
#include "int.h"
#include <lauxlib.h>

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath.h"

#define MY_PI 3.14159265358979323846
#define rad(degrees) ( ((double)degrees) * MY_PI / 180.0)

#if !defined(luaL_newlibtable) \
  && (!defined LUA_VERSION_NUM || LUA_VERSION_NUM==501)
static void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
  luaL_checkstack(L, nup+1, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    lua_pushlstring(L, l->name,str_len(l->name));
    for (i = 0; i < nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -(nup+1));
    lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    lua_settable(L, -(nup + 3));
  }
  lua_pop(L, nup);  /* remove upvalues */
}
#endif

static thread_ptr_t thread;
static thread_signal_t t_signal;

static thread_queue_t thread_queue;
static image_q image_queue[100];

static thread_queue_t *ret_queue;

/* Handmade math uses floats everywhere, define a custom integer
 * vector */
typedef union jpr_vec2i {
    struct {
        int X, Y;
    };
    struct {
        int U, V;
    };
    struct {
        int Left, Right;
    };
    struct {
        int Width, Height;
    };
    int Elements[2];
} jpr_vec2i;

typedef union jpr_vec2ui {
    struct {
        unsigned int X, Y;
    };
    struct {
        unsigned int U, V;
    };
    struct {
        unsigned int Left, Right;
    };
    struct {
        unsigned int Width, Height;
    };
    unsigned int Elements[2];
} jpr_vec2ui;

HMM_INLINE jpr_vec2i JPR_Vec2i(int X, int Y) {
    jpr_vec2i r;
    r.X = X;
    r.Y = Y;

    return (r);
}

HMM_INLINE jpr_vec2ui JPR_Vec2ui(unsigned int X, unsigned int Y) {
    jpr_vec2ui r;
    r.X = X;
    r.Y = Y;

    return (r);
}

void
wake_queue(void) {
    thread_signal_raise(&t_signal);
}

static int
lua_image_from_memory(lua_State *L, unsigned int width, unsigned int height, unsigned int channels, jpr_uint8 *image) {
    jpr_uint8 *l_image;
    int idx;

#ifndef NDEBUG
    int lua_top = lua_gettop(L);
#endif

    lua_newtable(L);
    idx = lua_gettop(L);

    lua_pushinteger(L,width);
    lua_setfield(L,idx,"width");

    lua_pushinteger(L,height);
    lua_setfield(L,idx,"height");

    lua_pushinteger(L,channels);
    lua_setfield(L,idx,"channels");

    lua_pushinteger(L,width * height * channels);
    lua_setfield(L,idx,"image_len");

    lua_pushinteger(L,IMAGE_FIXED);
    lua_setfield(L,idx,"image_state");

    lua_pushliteral(L,"fixed");
    lua_setfield(L,idx,"state");

    l_image = lua_newuserdata(L,width*height*channels);
    lua_setfield(L,idx,"image");
    mem_cpy(l_image,image,width*height*channels);

    luaL_getmetatable(L,"image");
    lua_setmetatable(L,idx);

#ifndef NDEBUG
    assert(idx == lua_gettop(L));
    assert(lua_top+1 == lua_gettop(L));
#endif

    return 1;
}

void
lua_load_image_cb(void *Lua, intptr_t table_ref, unsigned int frames, jpr_uint8 *image) {
    lua_State *L = (lua_State *)Lua;
#ifndef NDEBUG
    int lua_top = lua_gettop(L);
#endif
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    int table_ind = 0;
    int frame_ind = 0;
    int delay_ind = 0;
    unsigned int i = 0;
    int delay = 0;

    jpr_uint8 *b = NULL;
    lua_rawgeti(L,LUA_REGISTRYINDEX,(int)table_ref);

    table_ind = lua_gettop(L);

    if(UNLIKELY(image == NULL)) {
      lua_pushnil(L);
      lua_setfield(L,table_ind,"frames");

      lua_pushnil(L);
      lua_setfield(L,table_ind,"delays");

      lua_pushinteger(L,IMAGE_ERR);
      lua_setfield(L,table_ind,"image_state");

      lua_pushliteral(L,"error");
      lua_setfield(L,table_ind,"state");
      lua_pop(L,1);

      #ifndef NDEBUG
          assert(lua_top == lua_gettop(L));
      #endif
      return;
    }
    b = image;

    lua_getfield(L,table_ind,"width");
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,table_ind,"height");
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,table_ind,"channels");
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_pop(L,3);

#ifndef NDEBUG
    assert(table_ind == lua_gettop(L));
#endif

    lua_newtable(L); /* image.frames */
    frame_ind = lua_gettop(L);

    lua_newtable(L); /* image.delays */
    delay_ind = lua_gettop(L);

    for(i=0;i<frames;i++) {
        lua_image_from_memory(L,width,height,channels,b);
        lua_rawseti(L,frame_ind,i+1);
        if(frames > 1) {
          b += width * height * channels;
          delay = 0 + b[0];
          delay += b[1] << 8;
        }
        else {
            delay = 0;
        }
        lua_pushinteger(L,delay);
        lua_rawseti(L,delay_ind,i+1);
        b += 2;
    }

    lua_setfield(L,table_ind,"delays");
    lua_setfield(L,table_ind,"frames");

    lua_pushinteger(L,IMAGE_LOADED);
    lua_setfield(L,table_ind,"image_state");

    lua_pushliteral(L,"loaded");
    lua_setfield(L,table_ind,"state");

    lua_pushinteger(L,frames);
    lua_setfield(L,table_ind,"framecount");

    free(image);
    lua_pop(L,1);

#ifndef NDEBUG
    assert(lua_top == lua_gettop(L));
#endif

    return;
}

void
queue_image_load(intptr_t table_ref,const char* filename, unsigned int width, unsigned int height, unsigned int channels) {
    image_q *q = (image_q *)malloc(sizeof(image_q));

    if(UNLIKELY(q == NULL)) {
        LOG_ERROR("error: out of memory");
        JPR_EXIT(1);
    }

    q->filename = malloc(str_len(filename) + 1);
    if(UNLIKELY(q->filename == NULL)) {
        LOG_ERROR("error: out of memory");
        JPR_EXIT(1);
    }
    str_cpy(q->filename,filename);

    q->table_ref = (int)table_ref;
    q->width = width;
    q->height = height;
    q->channels = channels;
    q->frames = 0;
    q->image = NULL;

    thread_queue_produce(&thread_queue,q);

}

static int
lua_image_thread(void *userdata) {
    image_q *q = NULL;
    (void)userdata;

    while(1) {
        thread_signal_wait(&t_signal,THREAD_SIGNAL_WAIT_INFINITE);
        while(thread_queue_count(&thread_queue) > 0) {
            q = (image_q *)thread_queue_consume(&thread_queue);

            if(UNLIKELY(q == NULL)) {
                continue;
            }

            if(UNLIKELY(q->table_ref < 0)) {
                thread_exit(0);
            }

            q->image = image_load(q->filename,&(q->width),&(q->height),&(q->channels),&(q->frames));
            thread_queue_produce(ret_queue,q);
            q = NULL;
        }
    }
    thread_exit(1);
    return 1;
}

static int
lua_image_new(lua_State *L) {
    const char *filename = NULL;
    int table_ind = 0;
    int image_ind = 0;
    int frame_ind = 0;
    int delay_ind = 0;
    unsigned int width;
    unsigned int height;
    unsigned int channels;
    jpr_uint8 *image;
#ifndef NDEBUG
    int lua_top = lua_gettop(L);
#endif

    if(lua_isstring(L,1)) {
      filename = lua_tostring(L,1);
    }

    width    = (unsigned int)luaL_optinteger(L,2,0);
    height   = (unsigned int)luaL_optinteger(L,3,0);
    channels = (unsigned int)luaL_optinteger(L,4,0);

    if(UNLIKELY(filename == NULL && (width == 0 || height == 0 || channels == 0) )) {
        lua_pushnil(L);
        lua_pushliteral(L,"Need either filename, or width/height/channels");
        return 2;
    }

    if(filename != NULL) {
        if(image_probe(filename,&width,&height,&channels) == 0) {
            lua_pushnil(L);
            lua_pushliteral(L,"unable to probe image");
            return 2;
        }
    }

    lua_newtable(L); /* push */
    table_ind = lua_gettop(L);

    lua_pushinteger(L,width);
    lua_setfield(L,table_ind,"width");

    lua_pushinteger(L,height);
    lua_setfield(L,table_ind,"height");

    lua_pushinteger(L,channels);
    lua_setfield(L,table_ind,"channels");

    if(filename != NULL) {
        lua_pushlstring(L,filename,str_len(filename));
        lua_setfield(L,table_ind,"filename");

        lua_pushinteger(L,IMAGE_UNLOADED);
        lua_setfield(L,table_ind,"image_state");

        lua_pushliteral(L,"unloaded");
        lua_setfield(L,table_ind,"state");
    }
    else {
        lua_newtable(L); /* image.frames  -- push */
        frame_ind = lua_gettop(L);

        lua_newtable(L);
        image_ind = lua_gettop(L); /* image.frames[1]  -- push */

        lua_pushinteger(L,IMAGE_FIXED);
        lua_setfield(L,table_ind,"image_state");

        lua_pushliteral(L,"fixed");
        lua_setfield(L,table_ind,"state");

        image = lua_newuserdata(L,(sizeof(jpr_uint8) * width * height * channels));
        lua_setfield(L,image_ind,"image");
        mem_set(image,0,sizeof(jpr_uint8) * width * height * channels);

        lua_pushinteger(L,IMAGE_FIXED);
        lua_setfield(L,image_ind,"image_state");

        lua_pushliteral(L,"fixed");
        lua_setfield(L,image_ind,"state");

        lua_pushinteger(L,width);
        lua_setfield(L,image_ind,"width");

        lua_pushinteger(L,height);
        lua_setfield(L,image_ind,"height");

        lua_pushinteger(L,channels);
        lua_setfield(L,image_ind,"channels");

        lua_pushinteger(L,width * height * channels);
        lua_setfield(L,image_ind,"image_len");

        luaL_getmetatable(L,"image");
        lua_setmetatable(L,image_ind);

        lua_rawseti(L,frame_ind,1);
        lua_setfield(L,table_ind,"frames");

        lua_newtable(L); /* image.delays */
        delay_ind = lua_gettop(L);
        lua_pushinteger(L,0);
        lua_rawseti(L,delay_ind,1);
        lua_setfield(L,table_ind,"delays");

        lua_pushinteger(L,1);
        lua_setfield(L,table_ind,"framecount");

    }

    luaL_getmetatable(L,"image_c");
    lua_setmetatable(L,table_ind);

#ifndef NDEBUG
    assert(table_ind == lua_gettop(L));
    assert(lua_top+1 == lua_gettop(L));
#endif

    return 1;
}

static int
lua_image_unload(lua_State *L) {
    int state = 0;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }
    lua_getfield(L,1,"image_state");

    if(!lua_isnumber(L,-1)) {
        lua_pop(L,1);
        lua_pushnil(L);
        lua_pushliteral(L,"Missing field image_state");
        return 2;
    }
    state = (int)lua_tointeger(L,-1);
    lua_pop(L,1);

    if(state != IMAGE_LOADED) {
        lua_pushboolean(L,0);
        return 1;
    }

    lua_pushnil(L);
    lua_setfield(L,1,"frames");

    lua_pushnil(L);
    lua_setfield(L,1,"delays");

    lua_pushnil(L);
    lua_setfield(L,1,"framecount");

    lua_pushinteger(L,IMAGE_UNLOADED);
    lua_setfield(L,1,"image_state");

    lua_pushliteral(L,"unloaded");
    lua_setfield(L,1,"state");

    lua_pushboolean(L,1);
    return 1;
}


static int
lua_image_load(lua_State *L) {
    int state = 0;
    int async = 0;

    const char *filename = NULL;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    unsigned int frames = 0;
    int delay = 0;

    int table_ref = 0;
    int frame_ind = 0;
    int delay_ind = 0;
    unsigned int i = 0;

    jpr_uint8 *t = NULL;
    jpr_uint8 *b = NULL;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Bad argument for self");
        return 2;
    }

    async = lua_toboolean(L,2);
    lua_settop(L,1);

    lua_getfield(L,1,"image_state");

    if(!lua_isnumber(L,-1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Bad argument for self");
        return 2;
    }

    state = (int)lua_tointeger(L,-1);
    lua_pop(L,1);

    if(state == IMAGE_ERR) {
        lua_pushnil(L);
        lua_pushliteral(L,"Error occured");
        return 2;
    }

    if(state == IMAGE_FIXED) {
        lua_pushnil(L);
        lua_pushliteral(L,"This is a fixed buffer");
        return 2;
    }

    if(state == IMAGE_LOADED) {
        lua_pushnil(L);
        lua_pushliteral(L,"Image already loaded");
        return 2;
    }

    if(state == IMAGE_LOADING) {
        lua_pushnil(L);
        lua_pushliteral(L,"Image already loading");
        return 2;
    }

    lua_pushinteger(L,IMAGE_LOADING);
    lua_setfield(L,1,"image_state");

    lua_pushliteral(L,"loading");
    lua_setfield(L,1,"state");

    lua_getfield(L,1,"width");
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"filename");
    filename = lua_tostring(L,-1);

    if(!async) {
        t = image_load(filename,&width,&height,&channels,&frames);
        if(UNLIKELY(t == NULL)) {
            lua_pushnil(L);
            lua_setfield(L,1,"frames");
            lua_pushnil(L);
            lua_setfield(L,1,"delays");
            lua_pushinteger(L,IMAGE_ERR);
            lua_setfield(L,1,"image_state");
            lua_pushliteral(L,"error");
            lua_setfield(L,1,"state");
            lua_pushboolean(L,0);
            return 1;
        }
        b = t;

        lua_newtable(L); /* image.frames */
        frame_ind = lua_gettop(L);

        lua_newtable(L); /* image.delays */
        delay_ind = lua_gettop(L);

        for(i=0;i<frames;i++) {
            lua_image_from_memory(L,width,height,channels,b);
            lua_rawseti(L,frame_ind,i+1);
            if(frames > 1) {
              b += width * height * channels;
              delay = 0 + b[0];
              delay += b[1] << 8;
            }
            else {
                delay = 0;
            }
            lua_pushinteger(L,delay);
            lua_rawseti(L,delay_ind,i+1);
            b += 2;
        }

        lua_setfield(L,1,"delays");
        lua_setfield(L,1,"frames");

        lua_pushinteger(L,IMAGE_LOADED);
        lua_setfield(L,1,"image_state");

        lua_pushliteral(L,"loaded");
        lua_setfield(L,1,"state");

        lua_pushinteger(L,frames);
        lua_setfield(L,1,"framecount");

        free(t);

        lua_pushboolean(L,1);
        return 1;
    }

    lua_pushvalue(L,1);
    table_ref = luaL_ref(L,LUA_REGISTRYINDEX);
    queue_image_load(table_ref,filename,width,height,channels);

    lua_pushboolean(L,1);
    return 1;
}


static int
lua_image_get_pixel(lua_State *L) {
    jpr_uint8 *image = NULL;
    unsigned int index = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    lua_Integer x;
    lua_Integer y;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    x = luaL_checkinteger(L,2);
    y = luaL_checkinteger(L,3);

    lua_getfield(L,1,"image");
    image = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = (unsigned int)luaL_checkinteger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)luaL_checkinteger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)luaL_checkinteger(L,-1);

    if(x < 1 || y < 1 || (unsigned int)x > width || (unsigned int)y > height) {
        lua_pushboolean(L,0);
        return 1;
    }

    x = x - 1;
    y = height - y;

    index = (unsigned int)((y * width * channels) + (x * channels));

    lua_pushinteger(L,image[index + 2]);
    lua_pushinteger(L,image[index + 1]);
    lua_pushinteger(L,image[index]);

    if(channels == 4) {
        lua_pushinteger(L,image[index + 3]);
    }
    else {
        lua_pushinteger(L,255);
    }

    return 4;
}

static void
local_draw_line_ver(jpr_uint8 *image, unsigned int width, unsigned int height, unsigned int channels, int x, int y1, int y2, jpr_uint8 r, jpr_uint8 g, jpr_uint8 b, jpr_uint8 a) {
    int s_x;
    int s_y1;
    int s_y2;
    int sy;
    int max_byte;
    unsigned int alpha;
    unsigned int alpha_inv;

    if(y1 <= y2) {
        sy = 1;
    }
    else {
        sy = -1;
    }

    max_byte = width * height * channels;

    s_x = x * channels;
    s_y1  = y1 * channels * width;
    s_y2  = y2 * channels * width;

    alpha = 1 + (unsigned int)a;
    alpha_inv = 256 - (unsigned int)a;
    sy *= width * channels;

    while(1) {
        if(s_x > 0 && s_y1 > 0 && (s_y1 + s_x < max_byte)) {
            image[s_y1+s_x+0] = ((image[s_y1+s_x+0] * alpha_inv) + (b * alpha)) >> 8;
            image[s_y1+s_x+1] = ((image[s_y1+s_x+1] * alpha_inv) + (g * alpha)) >> 8;
            image[s_y1+s_x+2] = ((image[s_y1+s_x+2] * alpha_inv) + (r * alpha)) >> 8;
        }
        if(s_y1 == s_y2) break;
        s_y1 += sy;
    }

}

static void
local_draw_line_hor(jpr_uint8 *image, unsigned int width, unsigned int height, unsigned int channels, int x1, int x2, int y, jpr_uint8 r, jpr_uint8 g, jpr_uint8 b, jpr_uint8 a) {
    int s_x1;
    int s_x2;
    int sx;
    int s_y;
    int max_byte;
    unsigned int alpha;
    unsigned int alpha_inv;

    if(x1 <= x2) {
        sx = 1;
    }
    else {
        sx = -1;
    }

    max_byte = width * height * channels;

    s_x1 = x1 * channels;
    s_x2 = x2 * channels;
    s_y  = y * channels * width;

    alpha = 1 + (unsigned int)a;
    alpha_inv = 256 - (unsigned int)a;
    sx *= channels;

    while(1) {
        if(s_y > 0 && s_x1 > 0 && (s_y + s_x1 < max_byte)) {
            image[s_y+s_x1+0] = ((image[s_y+s_x1+0] * alpha_inv) + (b * alpha)) >> 8;
            image[s_y+s_x1+1] = ((image[s_y+s_x1+1] * alpha_inv) + (g * alpha)) >> 8;
            image[s_y+s_x1+2] = ((image[s_y+s_x1+2] * alpha_inv) + (r * alpha)) >> 8;
        }
        if(s_x1 == s_x2) break;
        s_x1 += sx;
    }

}

static void
local_draw_line(jpr_uint8 *image, unsigned int width, unsigned int height, unsigned int channels, int x1, int y1, int x2, int y2, jpr_uint8 r, jpr_uint8 g, jpr_uint8 b, jpr_uint8 a) {
    int s_x1;
    int s_x2;
    int s_y1;
    int s_y2;
    int sx;
    int sy;
    int dx;
    int dy;
    int err;
    int e2;
    int max_byte;

    unsigned int alpha;
    unsigned int alpha_inv;

    if(x1 <= x2) {
        sx = 1;
    }
    else {
        sx = -1;
    }

    if(y1 <= y2) {
        sy = 1;
    }
    else {
        sy = -1;
    }

    max_byte = width * height * channels;

    dx = labs(x2 - x1);
    dy = -1 * labs(y2 - y1);
    err = dx + dy;

    s_x1 = x1 * channels;
    s_x2 = x2 * channels;
    s_y1 = y1 * channels * width;
    s_y2 = y2 * channels * width;

    alpha = 1 + (unsigned int)a;
    alpha_inv = 256 - (unsigned int)a;
    sx *= channels;
    sy *= channels * width;

    while(1) {
        if(s_y1 > 0 && s_x1 > 0 && (s_y1 + s_x1 < max_byte)) {
            image[s_y1+s_x1+0] = ((image[s_y1+s_x1+0] * alpha_inv) + (b * alpha)) >> 8;
            image[s_y1+s_x1+1] = ((image[s_y1+s_x1+1] * alpha_inv) + (g * alpha)) >> 8;
            image[s_y1+s_x1+2] = ((image[s_y1+s_x1+2] * alpha_inv) + (r * alpha)) >> 8;
        }
        if(s_x1 == s_x2 || s_y1 == s_y2) break;
        e2 = 2 * err;
        if(e2 >= dy) {
            err += dy;
            s_x1 += sx;
        }
        if(e2 <= dx) {
            err += dx;
            s_y1 += sy;
        }
    }
    if(s_x1 != s_x2) {
        local_draw_line_hor(image,width,height,channels,s_x1 / channels, s_x2 / channels, s_y1 / (width * channels), r, g, b, a);
    }
    else if(s_y1 != s_y2) {
        local_draw_line_ver(image,width,height,channels,s_x1 / channels, s_y1 / (width *channels), s_y2 / (width * channels), r, g, b, a);
    }
}

static int
lua_image_draw_line(lua_State *L) {
    jpr_uint8 *image = NULL;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    lua_Integer x1;
    lua_Integer y1;
    lua_Integer x2;
    lua_Integer y2;
    lua_Integer r;
    lua_Integer g;
    lua_Integer b;
    lua_Integer a;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    x1 = luaL_checkinteger(L,2);
    y1 = luaL_checkinteger(L,3);
    x2 = luaL_checkinteger(L,4);
    y2 = luaL_checkinteger(L,5);
    r  = luaL_checkinteger(L,6);
    g  = luaL_checkinteger(L,7);
    b  = luaL_checkinteger(L,8);
    a  = luaL_optinteger(L,9,255);

    if(a == 0) {
        lua_pushboolean(L,1);
        return 1;
    }

    lua_getfield(L,1,"image");
    image = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_pop(L,4);

    if(r > 255 || b > 255 || g > 255 || a > 255 ||
       r < 0   || b < 0   || g < 0 || a < 0 ) {
        lua_pushboolean(L,0);
        return 1;
    }

    x1--;
    x2--;
    y1 = height - y1;
    y2 = height - y2;

    local_draw_line(image,width,height,channels,x1,y1,x2,y2,(jpr_uint8)r,(jpr_uint8)g,(jpr_uint8)b,(jpr_uint8)a);

    lua_pushboolean(L,1);
    return 1;
}

static int
lua_image_set_alpha(lua_State *L) {
    jpr_uint8 *image = NULL;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;

    unsigned int xstart = 0;
    unsigned int xend = 0;
    unsigned int ystart = 0;
    unsigned int yend = 0;
    unsigned int ytmp = 0;

    unsigned int byte;
    unsigned int offset;
    unsigned int counter;

    lua_Integer x1;
    lua_Integer y1;
    lua_Integer x2;
    lua_Integer y2;
    lua_Integer a;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    x1 = luaL_checkinteger(L,2);
    y1 = luaL_checkinteger(L,3);
    x2 = luaL_checkinteger(L,4);
    y2 = luaL_checkinteger(L,5);
    a  = luaL_checkinteger(L,6);

    lua_getfield(L,1,"image");
    image = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_pop(L,4);

    if(channels != 4 || a > 255 ||  a < 0 ) {
        lua_pushboolean(L,0);
        return 1;
    }

    x1 = ( x1 < 0 ) ? 0 : x1;
    y1 = ( y1 < 0 ) ? 0 : y1;
    x2 = ( x2 < 0 ) ? 0 : x2;
    y2 = ( y2 < 0 ) ? 0 : y2;

    if(x1 <= x2) {
        xstart = (unsigned int)x1;
        xend = (unsigned int)x2;
    }
    else {
        xstart = (unsigned int)x2;
        xend = (unsigned int)x1;
    }

    if(y1 <= y2) {
        ystart = (unsigned int)y1;
        yend = (unsigned int)y2;
    }
    else {
        ystart = (unsigned int)y2;
        yend = (unsigned int)y1;
    }

    if(xend < 1) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"xend < 1");
        return 2;
    }

    if(yend < 1) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"yend < 1");
        return 2;
    }

    if(xstart > width) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"xstart > width");
        return 2;
    }

    if(ystart > height) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"ystart > height");
        return 2;
    }

    if(xstart < 1) {
        xstart = 1;
    }

    if(ystart < 1) {
        ystart = 1;
    }

    if(xend > width) {
        xend = width;
    }

    if(yend > height) {
        yend = height;
    }
    xstart--;
    ystart--;

    /* invert the ys */
    ytmp = ystart;
    ystart = yend;
    yend = ytmp;

    ystart = height - ystart;
    yend   = height - yend;

    xstart *= channels;
    xend *= channels;
    offset = xend - xstart;
    width *= channels;
    ystart *= width;
    yend *= width;

    while(ystart < yend) {
        byte = ystart + xstart;
        counter = 0;
        while(counter < offset) {
            image[byte+counter+3] = a;
            counter += channels;
        }
        ystart += width;
    }

    lua_pushboolean(L,1);
    return 1;
}

static int
lua_image_draw_rectangle(lua_State *L) {
    jpr_uint8 *image = NULL;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;

    unsigned int xstart = 0;
    unsigned int xend = 0;
    unsigned int ystart = 0;
    unsigned int yend = 0;
    unsigned int ytmp = 0;

    unsigned int byte;
    unsigned int offset;
    unsigned int counter;
    unsigned int alpha;
    unsigned int alpha_inv;

    lua_Integer x1;
    lua_Integer y1;
    lua_Integer x2;
    lua_Integer y2;
    lua_Integer r;
    lua_Integer g;
    lua_Integer b;
    lua_Integer a;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    x1 = luaL_checkinteger(L,2);
    y1 = luaL_checkinteger(L,3);
    x2 = luaL_checkinteger(L,4);
    y2 = luaL_checkinteger(L,5);
    r  = luaL_checkinteger(L,6);
    g  = luaL_checkinteger(L,7);
    b  = luaL_checkinteger(L,8);
    a  = luaL_optinteger(L,9,255);

    if(a == 0) {
        lua_pushboolean(L,1);
        return 1;
    }

    lua_getfield(L,1,"image");
    image = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_pop(L,4);

    if(r > 255 || b > 255 || g > 255 || a > 255 ||
       r < 0   || b < 0   || g < 0 || a < 0 ) {
        lua_pushboolean(L,0);
        return 1;
    }

    x1 = ( x1 < 0 ) ? 0 : x1;
    y1 = ( y1 < 0 ) ? 0 : y1;
    x2 = ( x2 < 0 ) ? 0 : x2;
    y2 = ( y2 < 0 ) ? 0 : y2;

    if(x1 <= x2) {
        xstart = (unsigned int)x1;
        xend = (unsigned int)x2;
    }
    else {
        xstart = (unsigned int)x2;
        xend = (unsigned int)x1;
    }

    if(y1 <= y2) {
        ystart = (unsigned int)y1;
        yend = (unsigned int)y2;
    }
    else {
        ystart = (unsigned int)y2;
        yend = (unsigned int)y1;
    }

    if(xend < 1) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"xend < 1");
        return 2;
    }

    if(yend < 1) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"yend < 1");
        return 2;
    }

    if(xstart > width) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"xstart > width");
        return 2;
    }

    if(ystart > height) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"ystart > height");
        return 2;
    }

    if(xstart < 1) {
        xstart = 1;
    }

    if(ystart < 1) {
        ystart = 1;
    }

    if(xend > width) {
        xend = width;
    }

    if(yend > height) {
        yend = height;
    }
    xstart--;
    ystart--;

    /* invert the ys */
    ytmp = ystart;
    ystart = yend;
    yend = ytmp;

    ystart = height - ystart;
    yend   = height - yend;

    xstart *= channels;
    xend *= channels;
    offset = xend - xstart;
    width *= channels;
    ystart *= width;
    yend *= width;
    alpha = 1 + (unsigned int)a;
    alpha_inv = 256 - (unsigned int)a;

    while(ystart < yend) {
        byte = ystart + xstart;
        counter = 0;
        while(counter < offset) {
            image[byte+counter+0] = ((image[byte+counter+0] * alpha_inv) + (b * alpha)) >> 8;
            image[byte+counter+1] = ((image[byte+counter+1] * alpha_inv) + (g * alpha)) >> 8;
            image[byte+counter+2] = ((image[byte+counter+2] * alpha_inv) + (r * alpha)) >> 8;
            counter += channels;
        }
        ystart += width;
    }

    lua_pushboolean(L,1);
    return 1;
}


static int lua_image_set_pixel(lua_State *L) {
    jpr_uint8 *image = NULL;

    unsigned int index = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    unsigned int alpha = 0;
    unsigned int alpha_inv = 0;

    lua_Integer x;
    lua_Integer y;
    lua_Integer r;
    lua_Integer g;
    lua_Integer b;
    lua_Integer a;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    x = luaL_checkinteger(L,2);
    y = luaL_checkinteger(L,3);
    r = luaL_checkinteger(L,4);
    g = luaL_checkinteger(L,5);
    b = luaL_checkinteger(L,6);
    a = luaL_optinteger(L,7,255);

    if(a == 0) {
        lua_pushboolean(L,1);
        return 1;
    }

    lua_getfield(L,1,"image");
    image = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = (unsigned int)luaL_checkinteger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)luaL_checkinteger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)luaL_checkinteger(L,-1);

    lua_pop(L,4);

    if(x < 1 || y < 1 || (unsigned int)x > width || (unsigned int)y > height) {
        lua_pushboolean(L,0);
        return 1;
    }

    if(r > 255 || b > 255 || g > 255 || a > 255 ||
       r < 0   || b < 0   || g < 0 || a < 0 ) {
        lua_pushboolean(L,0);
        return 1;
    }
    lua_pushboolean(L,1);

    x = x - 1;
    y = height - y;

    index = (unsigned int)((y * width * channels) + (x * channels));

    if(a == 255) {
        image[index] = (jpr_uint8)b;
        image[index+1] = (jpr_uint8)g;
        image[index+2] = (jpr_uint8)r;
        if(channels == 4) image[index+3] = 255;
        return 1;
    }

    alpha = 1 + (unsigned int)a;
    alpha_inv = 256 - (unsigned int)a;

    image[index]   = (jpr_uint8)(((image[index] * alpha_inv) + (b * alpha)) >> 8);
    image[index+1] = (jpr_uint8)(((image[index+1] * alpha_inv) + (g * alpha)) >> 8);
    image[index+2] = (jpr_uint8)(((image[index+2] * alpha_inv) + (r * alpha)) >> 8);
    if(channels == 4) image[index+3] = 255;

    return 1;
}

static int
lua_image_set(lua_State *L) {
    /* image:set(src) */
    jpr_uint8 *image_one = NULL;
    jpr_uint8 *image_two = NULL;
    lua_Integer image_one_len = 0;
    lua_Integer image_two_len = 0;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    if(!lua_istable(L,2)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument src");
        return 2;
    }

    lua_getfield(L,1,"image");
    image_one = lua_touserdata(L,-1);
    lua_getfield(L,1,"image_len");
    image_one_len = lua_tointeger(L,-1);

    lua_getfield(L,2,"image");
    image_two = lua_touserdata(L,-1);
    lua_getfield(L,2,"image_len");
    image_two_len = lua_tointeger(L,-1);

    if(image_one && image_two && (image_one_len == image_two_len)) {
        mem_cpy(image_one,image_two,image_one_len);
        lua_pushboolean(L,1);
        return 1;
    }
    return 0;
}

static int
lua_image_stamp_letter(lua_State *L) {
    /* image:stamp_letter(font,codepoint,scale,x,y,r,g,b,hloffset,hroffset,ytoffset,yboffset,hflip,vflip */
    /* returns width of letter rendered, in pixels */
    jpr_uint8 *image;
    unsigned int codepoint;
    unsigned int scale;
    unsigned int x;
    unsigned int y;
    jpr_uint8 r;
    jpr_uint8 g;
    jpr_uint8 b;
    unsigned int hloffset;
    unsigned int hroffset;
    unsigned int ytoffset;
    unsigned int yboffset;
    unsigned int pixel_hroffset;
    unsigned int pixel_yboffset;
    unsigned int width;
    unsigned int height;
    unsigned int xc;
    unsigned int yc;
    unsigned int xcc;
    unsigned int xccc;
    unsigned int ycc;
    unsigned int xi;
    unsigned int yi;
    unsigned int w;
    unsigned int cur;
    unsigned int index;
    unsigned int scaled_width;
    unsigned int scaled_height;

    unsigned int image_width;
    unsigned int image_height;
    unsigned int image_channels;

    int dest_x;
    int dest_y;

    int vflip;
    int hflip;

    /* self = 1 */
    /* font = 2 */

    lua_getfield(L,1,"image");
    image = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    image_width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    image_height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    image_channels = (unsigned int)lua_tointeger(L,-1);

    lua_pop(L,4);

    codepoint = (unsigned int)luaL_checkinteger(L,3);
    scale     = (unsigned int)luaL_checkinteger(L,4);
    x         = (unsigned int)luaL_checkinteger(L,5);
    y         = (unsigned int)luaL_checkinteger(L,6);
    r         = (jpr_uint8)luaL_checkinteger(L,7);
    g         = (jpr_uint8)luaL_checkinteger(L,8);
    b         = (jpr_uint8)luaL_checkinteger(L,9);
    hloffset  = (unsigned int)luaL_optinteger(L,10,0);
    hroffset  = (unsigned int)luaL_optinteger(L,11,0);
    ytoffset  = (unsigned int)luaL_optinteger(L,12,0);
    yboffset  = (unsigned int)luaL_optinteger(L,13,0);
    hflip     = (lua_isboolean(L,14) ? lua_toboolean(L,14) : 0);
    vflip     = (lua_isboolean(L,15) ? lua_toboolean(L,15) : 0);

    lua_getfield(L,2,"widths");
    lua_rawgeti(L,-1,codepoint);
    if(lua_isnil(L,-1)) {
        lua_getfield(L,2,"width");
        width = (unsigned int)lua_tointeger(L,-1);
        lua_pop(L,3);
        lua_pushinteger(L,width * scale);
        return 1;
    } else {
        width = (unsigned int)lua_tointeger(L,-1);
        lua_pop(L,2);
    }

    lua_getfield(L,2,"height");
    height = (unsigned int)lua_tointeger(L,-1);
    lua_pop(L,1);

    scaled_width = width * scale;
    scaled_height = height * scale;

    if(hroffset > scaled_width ||
       yboffset > scaled_height ||
       hloffset > scaled_width ||
       ytoffset > scaled_height) {
        lua_pushinteger(L,width * scale);
        return 1;
    }

    w = 1 << ( (width + 7) & ~0x07);

    pixel_hroffset = scaled_width  - hroffset;
    pixel_yboffset = scaled_height - yboffset;

    lua_getfield(L,2,"bitmaps");
    lua_rawgeti(L,-1,codepoint);

    /*for(yc=height;yc>=1;--yc) {*/
    for(yc=1;yc<=height;++yc) {
        lua_rawgeti(L,-1,(vflip ? height - yc + 1 : yc));
        cur = (unsigned int)lua_tointeger(L,-1);
        lua_pop(L,1);
        if(cur == 0) continue;
        for(xc=1;xc<=width;++xc) {
            xccc = (hflip ? width - xc + 1 : xc);
            if( cur & (w >> xccc)) {
                for(yi=0;yi<scale;++yi) {
                    ycc = (yc - 1) * scale + yi;
                    if(ycc >= ytoffset && ycc < pixel_yboffset) {
                        for(xi=0;xi<scale;xi++) {
                            xcc = (xc - 1) * scale + xi;
                            if(xcc >= hloffset && xcc < pixel_hroffset) {
                                /* self:set_pixel(x+xcc,y+((yc-1) * scale) + yi, r, g, b) */
                                /* inline this shit */
                                dest_y = (y + ycc);
                                dest_x = (x + xcc);
                                if(dest_x < 1 || dest_y < 1) continue;
                                if((unsigned int)dest_x > image_width || (unsigned int)dest_y > image_height) continue;
                                dest_x--;
                                dest_y = image_height - dest_y;
                                index = (dest_y * image_width * image_channels) + (dest_x * image_channels);
                                image[index] = b;
                                image[index+1] = g;
                                image[index+2] = r;

                            }
                        }

                    }
                }

            }
        }
    }

    lua_pushinteger(L,width * scale);
    return 1;
}


static int
lua_image_blend(lua_State *L) {
    /* image:blend(src,alpha) */
    int i = 0;
    jpr_uint8 *image_one = NULL;
    jpr_uint8 *image_two = NULL;
    lua_Integer a;
    lua_Integer alpha;
    lua_Integer alpha_inv;
    lua_Integer image_one_len = 0;
    lua_Integer image_two_len = 0;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    if(!lua_istable(L,2)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument src");
        return 2;
    }

    a = luaL_checkinteger(L,3);
    if(a == 0) {
        return 0;
    }

    alpha = 1 + a;
    alpha_inv = 256 - a;

    lua_getfield(L,1,"image");
    image_one = lua_touserdata(L,-1);
    lua_getfield(L,1,"image_len");
    image_one_len = lua_tointeger(L,-1);

    lua_getfield(L,2,"image");
    image_two = lua_touserdata(L,-1);
    lua_getfield(L,2,"image_len");
    image_two_len = lua_tointeger(L,-1);

    lua_pop(L,4);

    if(image_one && image_two && (image_one_len == image_two_len)) {
        for(i=0;i<image_one_len;i+=8) {
            image_one[i] =   (jpr_uint8)(((image_one[i+0] * alpha_inv) + (image_two[i+0] * alpha)) >> 8);
            image_one[i+1] = (jpr_uint8)(((image_one[i+1] * alpha_inv) + (image_two[i+1] * alpha)) >> 8);
            image_one[i+2] = (jpr_uint8)(((image_one[i+2] * alpha_inv) + (image_two[i+2] * alpha)) >> 8);
            image_one[i+3] = (jpr_uint8)(((image_one[i+3] * alpha_inv) + (image_two[i+3] * alpha)) >> 8);
            image_one[i+4] = (jpr_uint8)(((image_one[i+4] * alpha_inv) + (image_two[i+4] * alpha)) >> 8);
            image_one[i+5] = (jpr_uint8)(((image_one[i+5] * alpha_inv) + (image_two[i+5] * alpha)) >> 8);
            image_one[i+6] = (jpr_uint8)(((image_one[i+6] * alpha_inv) + (image_two[i+6] * alpha)) >> 8);
            image_one[i+7] = (jpr_uint8)(((image_one[i+7] * alpha_inv) + (image_two[i+7] * alpha)) >> 8);
        }
        lua_pushboolean(L,1);
        return 1;
    }
    return 0;
}

static int
lua_image_stamp_image(lua_State *L) {
    jpr_uint8 *image_one = NULL;
    jpr_uint8 *image_two = NULL;

    lua_Integer x;
    lua_Integer y;

    int hflip = 0;
    int vflip = 0;
    jpr_uint8 r;
    jpr_uint8 g;
    jpr_uint8 b;
    jpr_uint8 a;
    lua_Integer aa = -1;

    int mask_left = 0;
    int mask_right = 0;
    int mask_top = 0;
    int mask_bottom = 0;
    unsigned int src_width = 0;
    unsigned int src_height = 0;
    unsigned int src_channels = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    int xi = 1;
    int yi = 1;
    int xm;
    int ym;
    int yii;
    int xii;
    int xt;
    int yt;
    unsigned int dxt;
    unsigned int dyt;
    unsigned int y_transform;
    unsigned int x_transform;
    int y_mult;
    int x_mult;
    int byte;
    int alpha;
    int alpha_inv;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    if(!lua_istable(L,2)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument src");
        return 2;
    }

    x = luaL_optinteger(L,3,1);
    y = luaL_optinteger(L,4,1);

    if(lua_istable(L,5)) {
        lua_getfield(L,5,"vflip");
        vflip = lua_toboolean(L,-1);
        lua_getfield(L,5,"hflip");
        hflip = lua_toboolean(L,-1);
    }

    if(lua_istable(L,6)) {
        lua_getfield(L,6,"left");
        mask_left = (int)luaL_optinteger(L,-1,0);
        lua_getfield(L,6,"right");
        mask_right = (int)luaL_optinteger(L,-1,0);
        lua_getfield(L,6,"top");
        mask_top = (int)luaL_optinteger(L,-1,0);
        lua_getfield(L,6,"bottom");
        mask_bottom = (int)luaL_optinteger(L,-1,0);
    }

    if(lua_isnumber(L,7)) {
        aa = lua_tointeger(L,7);
    }

    lua_getfield(L,1,"width");
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"image");
    image_one = lua_touserdata(L,-1);

    lua_getfield(L,2,"width");
    src_width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,2,"height");
    src_height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,2,"channels");
    src_channels = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,2,"image");
    image_two = lua_touserdata(L,-1);

    lua_pop(L,8);

    if(x < 1) {
        xi = (int)(xi + (x * -1) + 1);
    }
    if(y < 1) {
        yi = (int)(yi + (y * -1) + 1);
    }

    xm = src_width;
    ym = src_height;

    xi += mask_left;
    yi += mask_top;
    xm -= mask_right;
    ym -= mask_bottom;

    while(y - 1 + ym > (int)height) {
        ym--;
    }

    while(x - 1 + xm > (int)width) {
        xm--;
    }

    if(vflip) {
        y_mult = -1;
        y_transform = src_height - 1;
    } else {
        y_mult = 1;
        y_transform = 0;
    }

    x_transform = 1;
    x_mult = 1;

    if(hflip) {
        x_transform = src_width;
        x_mult = -1;
    }

    for(yii=yi;yii <= ym; yii++) {
        dyt = (unsigned int)(y - 1 + yii);
        dyt = (unsigned int)(height - dyt);

        yt = (src_height - yii - y_transform) * y_mult;

        for(xii=xi;xii <= xm; xii++) {
            dxt = (unsigned int)(x - 1 + xii);
            dxt = dxt - 1;

            xt = (xii - x_transform) * x_mult;

            byte = (yt * src_width * src_channels) + (xt * src_channels);

            if(src_channels == 4) {
                a = image_two[byte + 3];
            }
            else {
                a = 255;
            }

            if(a == 0) {
                continue;
            }

            b = image_two[byte + 0];
            g = image_two[byte + 1];
            r = image_two[byte + 2];

            if(aa > -1) {
                a = (jpr_uint8)aa;
            }

            byte = (dyt * width * channels) + (dxt * channels);

            if(a == 255) {
                image_one[byte+0] = b;
                image_one[byte+1] = g;
                image_one[byte+2] = r;
                continue;
            }
            alpha = 1 + a;
            alpha_inv = 256 - a;

            image_one[byte+0] = ((image_one[byte+0] * alpha_inv) + (b * alpha)) >> 8;
            image_one[byte+1] = ((image_one[byte+1] * alpha_inv) + (g * alpha)) >> 8;
            image_one[byte+2] = ((image_one[byte+2] * alpha_inv) + (r * alpha)) >> 8;
        }
    }

    return 0;

}

static int
lua_image_get_ref(lua_State *L) {
    intptr_t r;
    lua_pushvalue(L,1);
    r = luaL_ref(L,LUA_REGISTRYINDEX);
    lua_pushinteger(L,r);
    return 1;
}

static int
lua_image_from_ref(lua_State *L) {
    intptr_t t = lua_tointeger(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,(int)t);
    return 1;
}

hmm_vec3 barycentric(jpr_vec2i *pts, jpr_vec2i P) {
    hmm_vec3 u = HMM_Cross(HMM_Vec3( pts[2].X - pts[0].X, pts[1].X - pts[0].X, pts[0].X - P.X  ), HMM_Vec3(pts[2].Y - pts[0].Y, pts[1].Y - pts[0].Y, pts[0].Y - P.Y));
    if(fabsf(u.Z) < 1) return HMM_Vec3(-1, 1, 1);
    return HMM_Vec3(1.f - (u.X + u.Y)/u.Z, u.Y/u.Z, u.X/u.Z);
}

static void
draw_triangle(jpr_uint8 *image, unsigned int *dimensions, jpr_vec2i *pts, jpr_uint8 *color) {
    /* assumptions:
     *   pts Y-axis has already been inverted */
    unsigned int width;
    unsigned int height;
    unsigned int channels;
    unsigned int byte;
    unsigned int offset;
    unsigned int max;
    unsigned int i;
    unsigned int alpha;
    unsigned int alpha_inv;

    jpr_vec2i bbox_min;
    jpr_vec2i bbox_max;
    jpr_vec2i clamp;
    jpr_vec2i P;
    hmm_vec3 bc_screen;

    width = dimensions[0];
    height = dimensions[1];
    channels = dimensions[2];

    max = width * height * channels;

    bbox_min.X = width - 1;
    bbox_min.Y = height - 1;
    bbox_max.X = 0;
    bbox_max.Y = 0;
    clamp.X = width - 1;
    clamp.Y = height - 1;

    alpha = 1 + (unsigned int)color[3];
    alpha_inv = 256 - (unsigned int)color[3];

    for(i=0;i<3;i++) {
        bbox_min.X = HMM_MAX(0, HMM_MIN((int)bbox_min.X,pts[i].X));
        bbox_min.Y = HMM_MAX(0, HMM_MIN((int)bbox_min.Y,pts[i].Y));
        bbox_max.X = HMM_MIN(clamp.X, HMM_MAX((int)bbox_max.X,pts[i].X));
        bbox_max.Y = HMM_MIN(clamp.Y, HMM_MAX((int)bbox_max.Y,pts[i].Y));
    }

    for(P.Y = bbox_min.Y; P.Y <= bbox_max.Y; P.Y++) {
        offset = P.Y * width;
        for(P.X = bbox_min.X; P.X <= bbox_max.X; P.X++) {
            bc_screen = barycentric(pts,P);
            if(bc_screen.X < 0 || bc_screen.Y < 0 || bc_screen.Z < 0) continue;
            byte = (offset + P.X) * channels;
            image[byte + 0] = ((image[byte + 0] * alpha_inv) + (color[0] * alpha)) >> 8;
            image[byte + 1] = ((image[byte + 1] * alpha_inv) + (color[1] * alpha)) >> 8;
            image[byte + 2] = ((image[byte + 2] * alpha_inv) + (color[2] * alpha)) >> 8;
        }
    }
}


static int
lua_frame_triangle_int(lua_State *L) {
    jpr_uint8 *image;
    jpr_uint8 color[4];
    unsigned int dimensions[3];
    jpr_vec2i pts[3]; /* 3 points of a triangle */
    unsigned int a;

#ifndef NDEBUG
    int lua_top = lua_gettop(L);
#endif

    a  = luaL_optinteger(L,9,255);
    if(a == 0) return 0;
    color[0] = lua_tointeger(L,5); /* b */
    color[1] = lua_tointeger(L,4); /* g */
    color[2] = lua_tointeger(L,3); /* r */
    color[3] = a;

    lua_getfield(L,1,"width"); /* push */
    dimensions[0] = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height"); /* push */
    dimensions[1] = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels"); /* push */
    dimensions[2] = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"image"); /* push */
    image = lua_touserdata(L,-1);

    lua_pop(L,4);

    lua_rawgeti(L,2,1);
    lua_rawgeti(L,-1,1);
    pts[0].X = lua_tointeger(L,-1) - 1;
    lua_rawgeti(L,-2,2);
    pts[0].Y = dimensions[1] - lua_tointeger(L,-1);

    lua_rawgeti(L,2,2);
    lua_rawgeti(L,-1,1);
    pts[1].X = lua_tointeger(L,-1) - 1;
    lua_rawgeti(L,-2,2);
    pts[1].Y = dimensions[1] - lua_tointeger(L,-1);

    lua_rawgeti(L,2,3);
    lua_rawgeti(L,-1,1);
    pts[2].X = lua_tointeger(L,-1) - 1;
    lua_rawgeti(L,-2,2);
    pts[2].Y = dimensions[1] - lua_tointeger(L,-1);

    lua_pop(L,9);

    draw_triangle(image, dimensions, pts, color);

#ifndef NDEBUG
    assert(lua_top == lua_gettop(L));
#endif

    return 0;
}

static int
lua_frame_tile_int(lua_State *L) {
    /* input: an image frame
     * returns a new 2d array, "tiled"
     */
    jpr_uint8 *original;
    jpr_uint8 *chunk;
    unsigned int width;
    unsigned int height;
    unsigned int channels;
    unsigned int x;
    unsigned int y;

    unsigned int x_tiles;
    unsigned int y_tiles;
    unsigned int x_ind;
    unsigned int y_ind;
    unsigned int y_cpy_ind;
    unsigned int tile_x;
    unsigned int tile_y;
    unsigned int y_ind_inv;
    unsigned int y_cpy_ind_inv;

    lua_getfield(L,1,"width"); /* push */
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height"); /* push */
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels"); /* push */
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"image"); /* push */
    original = lua_touserdata(L,-1);

    lua_pop(L,4);

    x = luaL_checkinteger(L,2);
    y = luaL_checkinteger(L,3);

    x_tiles = width / x;
    if(width % x != 0) {
        x_tiles++;
    }

    y_tiles = height / y;
    if(height % y != 0) {
        y_tiles++;
    }

    lua_newtable(L); /* "tiled" array */

    x_ind = 0;
    while(x_ind < x_tiles) {
        lua_newtable(L); /* tiled[x_ind] */
        tile_x = x;
        if(x_ind == x_tiles - 1 && width % x != 0) {
            tile_x = width % x;
        }

        y_ind = 0;
        while(y_ind < y_tiles) {
            lua_newtable(L); /* tiled[x_ind][y_ind] */
            tile_y = y;

            if(y_ind == y_tiles - 1 && height % y != 0) {
                tile_y = height % y;
            }

            lua_pushinteger(L,tile_x);
            lua_setfield(L,-2,"width");

            lua_pushinteger(L,tile_y);
            lua_setfield(L,-2,"height");

            lua_pushinteger(L,channels);
            lua_setfield(L,-2,"channels");

            lua_pushinteger(L,tile_x * tile_y * channels);
            lua_setfield(L,-2,"image_len");

            lua_pushinteger(L,IMAGE_FIXED);
            lua_setfield(L,-2,"image_state");

            lua_pushliteral(L,"fixed");
            lua_setfield(L,-2,"state");

            chunk = lua_newuserdata(L,tile_x * tile_y * channels);
            lua_setfield(L,-2,"image");

            luaL_getmetatable(L,"image");
            lua_setmetatable(L,-2);

            y_cpy_ind = 0;
            while(y_cpy_ind < tile_y) {
                y_ind_inv = height - (
                  (y_ind * y) + y_cpy_ind ) - 1;
                y_cpy_ind_inv = tile_y - y_cpy_ind - 1;
                memcpy(
                  &chunk[
                    (y_cpy_ind_inv * tile_x) * channels
                  ],
                  &original[
                    ((y_ind_inv * width) +
                    (x_ind * x)) * channels
                  ],
                  tile_x * channels);
                y_cpy_ind++;
            }
            lua_rawseti(L,-2,++y_ind);
        }
        lua_rawseti(L,-2,++x_ind);
    }

    return 1;
}

static int
lua_frame_tile(lua_State *L) {
    lua_frame_tile_int(L);
    lua_setfield(L,1,"tiled");
    return 0;
}

static int
lua_image_tile(lua_State *L) {
    lua_Integer frameno;
    lua_Integer x;
    lua_Integer y;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    frameno = luaL_checkinteger(L,2);
    x = luaL_checkinteger(L,3);
    y = luaL_checkinteger(L,4);

    lua_getfield(L,1,"frames"); /* push */
    lua_rawgeti(L,-1,(int)frameno); /* push */

    lua_pushcfunction(L,lua_frame_tile_int);
    lua_pushvalue(L,-2);
    lua_pushinteger(L,x);
    lua_pushinteger(L,y);
    lua_pcall(L,3,1,0);

    lua_setfield(L,1,"tiled");
    lua_pop(L,2);
    return 0;
}


static int
lua_image_rotate(lua_State *L) {
    jpr_uint8 *original;
    jpr_uint8 *rotated;
    int width;
    int height;
    int h_width;
    int h_height;
    int h_diag;
    int d_x;
    int d_y;
    int x;
    int y;
    int xs;
    int ys;
    int channels;
    int frame_ind;
    int diag;
    unsigned int index;
    jpr_uint8 r;
    jpr_uint8 g;
    jpr_uint8 b;
    jpr_uint8 a;
    double rads;

    double cos_rads;
    double sin_rads;
    double cosmax;
    double sinmax;
    double cosmaxt_p_hwidth_p_sinmahheight;
    double sinmaxt_p_hheight_m_cosmahheight;
#ifndef NDEBUG
    int top;
#endif

    lua_Integer frameno;
    lua_Integer rotation;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    frameno = luaL_checkinteger(L,2);
    rotation = luaL_checkinteger(L,3);

#ifndef NDEBUG
    top = lua_gettop(L);
#endif

    lua_getfield(L,1,"frames"); /* push */
    lua_rawgeti(L,-1,(int)frameno); /* push */
    frame_ind = lua_gettop(L);

    lua_getfield(L,1,"width"); /* push */
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height"); /* push */
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels"); /* push */
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,frame_ind,"image"); /* push */
    original = lua_touserdata(L,-1);

    lua_pop(L,6);

#ifndef NDEBUG
    assert(top == lua_gettop(L));
#endif

    lua_getfield(L,1,"rotated");
    if(lua_isnil(L,-1)) {
        lua_pop(L,1); /* remove the nil value */
        diag = (int)(ceil(sqrt((width*width) + (height*height))));

        lua_newtable(L); /* push */

        lua_pushinteger(L,4);
        lua_setfield(L,-2,"channels");

        lua_pushinteger(L,diag);
        lua_setfield(L,-2,"width");

        lua_pushinteger(L,diag);
        lua_setfield(L,-2,"height");

        lua_pushinteger(L, (diag >> 1) - (width >> 1));
        lua_setfield(L,-2,"width_offset");

        lua_pushinteger(L, (diag >> 1) - (height >> 1));
        lua_setfield(L,-2,"height_offset");

        rotated = lua_newuserdata(L,diag*diag*4);
        lua_setfield(L,-2,"image");

        lua_pushinteger(L,diag * diag * 4);
        lua_setfield(L,-2,"image_len");

        lua_pushinteger(L,IMAGE_FIXED);
        lua_setfield(L,-2,"image_state");

        lua_pushliteral(L,"fixed");
        lua_setfield(L,-2,"state");

        luaL_getmetatable(L,"image");
        lua_setmetatable(L,-2);
        lua_setfield(L,1,"rotated");

    } else {
        lua_getfield(L,-1,"image");
        rotated = lua_touserdata(L,-1);
        lua_getfield(L,-2,"width");
        diag = (int)lua_tointeger(L,-1);
        lua_pop(L,3);
    }
    mem_set(rotated,0,diag*diag*4);

    h_width = width >> 1;
    h_height = height >> 1;
    h_diag = diag >> 1;
    d_x = h_diag - h_width;
    d_y = h_diag - h_height;

    /* we have pointers to the original image and the rotated, time to rotate */


    if(rotation < 0) {
        rotation += 360;
    } else if(rotation > 359) {
        while(rotation > 359) rotation -= 360;
    }
    rads = rad(rotation);

    cos_rads = cos(rads);
    sin_rads = sin(rads);

    cosmax = -cos_rads * h_diag + h_diag + sin_rads * h_diag;
    sinmax = -sin_rads * h_diag + h_diag - cos_rads * h_diag;

    for(x=0;x<diag;++x) {
        cosmaxt_p_hwidth_p_sinmahheight = cosmax;
        sinmaxt_p_hheight_m_cosmahheight = sinmax;

        for(y=0;y<diag;++y) {
            xs = ((int)(round(cosmaxt_p_hwidth_p_sinmahheight))) - d_x;
            ys = ((int)(round(sinmaxt_p_hheight_m_cosmahheight))) - d_y;

            if(xs >= 0 && xs < width && ys >= 0 && ys < height) { /* {{{ */
                /* ysi = height - (ys + 1); */
                index = (ys * width * channels) + (xs * channels);
                b = original[index];
                g = original[index+1];
                r = original[index+2];

                if(channels == 4) {
                    a = original[index+3];
                } else {
                    a = 255;
                }

                /* ysi = diag - (y + 1); */
                index = (y * diag * 4) + (x * 4);

                rotated[index] = b;
                rotated[index+1] = g;
                rotated[index+2] = r;
                rotated[index+3] = a;
            } /* }}} */
            cosmaxt_p_hwidth_p_sinmahheight -= sin_rads;
            sinmaxt_p_hheight_m_cosmahheight += cos_rads;
        }
        cosmax += cos_rads;
        sinmax += sin_rads;
    }

    lua_pushboolean(L,1);
    return 1;
}


static const struct luaL_Reg lua_image_image_methods[] = {
    { "set_pixel", lua_image_set_pixel },
    { "get_pixel", lua_image_get_pixel },
    { "draw_rectangle", lua_image_draw_rectangle },
    { "set_alpha", lua_image_set_alpha },
    { "draw_line", lua_image_draw_line },
    { "set", lua_image_set },
    { "blend", lua_image_blend },
    { "stamp_image", lua_image_stamp_image },
    { "stamp_letter", lua_image_stamp_letter },
    { "tile", lua_frame_tile },
    { "draw_triangle", lua_frame_triangle_int },
    { NULL, NULL },
};

static const struct luaL_Reg lua_image_instance_methods[] = {
    { "unload", lua_image_unload },
    { "load", lua_image_load },
    { "get_ref", lua_image_get_ref },
    { "rotate", lua_image_rotate },
    { "tile", lua_image_tile },
    { NULL, NULL },
};

static const struct luaL_Reg lua_image_methods[] = {
    { "new"           , lua_image_new },
    { "from_ref"      , lua_image_from_ref },
    { NULL     , NULL                },
};

int luaimage_setup_threads(thread_queue_t *ret) {
    thread_signal_init(&t_signal);
    thread_queue_init(&thread_queue,100,(void **)&image_queue,0);
    ret_queue = ret;

    thread = thread_create( lua_image_thread, NULL, NULL, THREAD_STACK_SIZE_DEFAULT );
    return 0;
}

int luaimage_stop_threads(void) {
    image_q q;
    q.table_ref = -1;
    thread_queue_produce(&thread_queue,&q);
    wake_queue();
    thread_join(thread);
    thread_destroy(thread);
    thread_queue_term(&thread_queue);
    thread_signal_term(&t_signal);
    return 0;
}

int
luaopen_image(lua_State *L) {
    const char *s = NULL;
#ifndef NDEBUG
    int lua_top = lua_gettop(L);
#endif

    luaL_newmetatable(L,"image");
    lua_newtable(L);
    luaL_setfuncs(L,lua_image_image_methods,0);
    lua_setfield(L,-2,"__index");
    lua_pop(L,1);

    luaL_newmetatable(L,"image_c");
    lua_newtable(L);
    luaL_setfuncs(L,lua_image_instance_methods,0);
    lua_setfield(L,-2,"__index");
    lua_pop(L,1);

    lua_newtable(L);
    luaL_setfuncs(L,lua_image_methods,0);

    lua_setglobal(L,"image");

    if(luaL_loadbuffer(L,image_lua,image_lua_length-1,"image.lua")) {
        s = lua_tostring(L,-1);
        WRITE_STDERR("error: ");
        LOG_ERROR(s);
        return 1;
    }

    if(lua_pcall(L,0,0,0)) {
        s = lua_tostring(L,-1);
        WRITE_STDERR("error: ");
        LOG_ERROR(s);
        return 1;
    }

#ifndef NDEBUG
    assert(lua_top == lua_gettop(L));
#endif

    return 0;
}

int luaclose_image() {
    luaimage_stop_threads();
    return 0;
}


