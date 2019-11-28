#include "image.h"
#include "lua-image.h"
#include "image.lua.lh"
#include "thread.h"
#include <lauxlib.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define rad(degrees) ( ((double)degrees) * M_PI / 180.0)

#if !defined(luaL_newlibtable) \
  && (!defined LUA_VERSION_NUM || LUA_VERSION_NUM==501)
static void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
  luaL_checkstack(L, nup+1, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    lua_pushlstring(L, l->name,strlen(l->name));
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
static int qsize;

void
wake_queue(void) {
    if(qsize > 0) {
        qsize = 0;
        thread_signal_raise(&t_signal);
    }
}

static int
lua_image_from_memory(lua_State *L, unsigned int width, unsigned int height, unsigned int channels, uint8_t *image) {
    uint8_t *l_image;
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
    memcpy(l_image,image,width*height*channels);

    luaL_getmetatable(L,"image");
    lua_setmetatable(L,idx);

#ifndef NDEBUG
    assert(idx == lua_gettop(L));
    assert(lua_top+1 == lua_gettop(L));
#endif

    return 1;
}

void
lua_load_image_cb(void *Lua, intptr_t table_ref, unsigned int frames, uint8_t *image) {
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

    uint8_t *b = NULL;
    lua_rawgeti(L,LUA_REGISTRYINDEX,table_ref);

    table_ind = lua_gettop(L);

    if(image == NULL) {
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
    width = lua_tointeger(L,-1);

    lua_getfield(L,table_ind,"height");
    height = lua_tointeger(L,-1);

    lua_getfield(L,table_ind,"channels");
    channels = lua_tointeger(L,-1);

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

    if(q == NULL) {
        fprintf(stderr,"error: unable to malloc memory for queue");
        exit(1);
    }

    q->filename = malloc(strlen(filename) + 1);
    strcpy(q->filename,filename);

    q->table_ref = table_ref;
    q->width = width;
    q->height = height;
    q->channels = channels;
    q->frames = 0;
    q->image = NULL;
    qsize++;

    thread_queue_produce(&thread_queue,q);

}

static int
lua_image_thread(void *userdata) {
    (void)userdata;
    image_q *q = NULL;

    while(1) {
        thread_signal_wait(&t_signal,THREAD_SIGNAL_WAIT_INFINITE);
        while(thread_queue_count(&thread_queue) > 0) {
            q = (image_q *)thread_queue_consume(&thread_queue);

            if(q == NULL) {
                continue;
            }

            if(q->table_ref < 0) {
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
#ifndef NDEBUG
    int lua_top = lua_gettop(L);
#endif

    if(lua_isstring(L,1)) {
      filename = lua_tostring(L,1);
    }
    unsigned int width    = (unsigned int)luaL_optinteger(L,2,0);
    unsigned int height   = (unsigned int)luaL_optinteger(L,3,0);
    unsigned int channels = (unsigned int)luaL_optinteger(L,4,0);

    if(filename == NULL && (width == 0 || height == 0 || channels == 0) ) {
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
        lua_pushlstring(L,filename,strlen(filename));
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

        uint8_t *image = lua_newuserdata(L,(sizeof(uint8_t) * width * height * channels));
        lua_setfield(L,image_ind,"image");
        memset(image,0,sizeof(uint8_t) * width * height * channels);

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
    state = lua_tointeger(L,-1);
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

    uint8_t *t = NULL;
    uint8_t *b = NULL;

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

    state = lua_tointeger(L,-1);
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
    width = lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = lua_tointeger(L,-1);

    lua_getfield(L,1,"filename");
    filename = lua_tostring(L,-1);

    if(!async) {
        t = image_load(filename,&width,&height,&channels,&frames);
        if(t == NULL) {
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
    uint8_t *image = NULL;
    unsigned int index = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    lua_Integer x = luaL_checkinteger(L,2);
    lua_Integer y = luaL_checkinteger(L,3);

    lua_getfield(L,1,"image");
    image = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = luaL_checkinteger(L,-1);

    lua_getfield(L,1,"height");
    height = luaL_checkinteger(L,-1);

    lua_getfield(L,1,"channels");
    channels = luaL_checkinteger(L,-1);

    if(x < 1 || y < 1 || x > width || y > height) {
        lua_pushboolean(L,0);
        return 1;
    }

    x = x - 1;
    y = height - y;

    index = (y * width * channels) + (x * channels);

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

static int
lua_image_draw_rectangle(lua_State *L) {
    uint8_t *image = NULL;
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

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    lua_Integer x1 = luaL_checkinteger(L,2);
    lua_Integer y1 = luaL_checkinteger(L,3);
    lua_Integer x2 = luaL_checkinteger(L,4);
    lua_Integer y2 = luaL_checkinteger(L,5);
    lua_Integer r = luaL_checkinteger(L,6);
    lua_Integer g = luaL_checkinteger(L,7);
    lua_Integer b = luaL_checkinteger(L,8);
    lua_Integer a = luaL_optinteger(L,9,255);

    if(a == 0) {
        lua_pushboolean(L,1);
        return 1;
    }

    lua_getfield(L,1,"image");
    image = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = lua_tointeger(L,-1);

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
        xstart = x1;
        xend = x2;
    }
    else {
        xstart = x2;
        xend = x1;
    }

    if(y1 <= y2) {
        ystart = y1;
        yend = y2;
    }
    else {
        ystart = y2;
        yend = y1;
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
    alpha = 1 + a;
    alpha_inv = 256 - a;

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
    uint8_t *image = NULL;

    unsigned int index = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    unsigned int alpha = 0;
    unsigned int alpha_inv = 0;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    lua_Integer x = luaL_checkinteger(L,2);
    lua_Integer y = luaL_checkinteger(L,3);
    lua_Integer r = luaL_checkinteger(L,4);
    lua_Integer g = luaL_checkinteger(L,5);
    lua_Integer b = luaL_checkinteger(L,6);
    lua_Integer a = luaL_optinteger(L,7,255);

    if(a == 0) {
        lua_pushboolean(L,1);
        return 1;
    }

    lua_getfield(L,1,"image");
    image = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = luaL_checkinteger(L,-1);

    lua_getfield(L,1,"height");
    height = luaL_checkinteger(L,-1);

    lua_getfield(L,1,"channels");
    channels = luaL_checkinteger(L,-1);

    lua_pop(L,4);

    if(x < 1 || y < 1 || x > width || y > height) {
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

    index = (y * width * channels) + (x * channels);

    if(a == 255) {
        image[index] = b;
        image[index+1] = g;
        image[index+2] = r;
        if(channels == 4) image[index+3] = 255;
        return 1;
    }

    alpha = 1 + a;
    alpha_inv = 256 - a;

    image[index] = ((image[index] * alpha_inv) + (b * alpha)) >> 8;
    image[index+1] = ((image[index+1] * alpha_inv) + (g * alpha)) >> 8;
    image[index+2] = ((image[index+2] * alpha_inv) + (r * alpha)) >> 8;
    if(channels == 4) image[index+3] = 255;

    return 1;
}

static int
lua_image_set(lua_State *L) {
    /* image:set(src) */
    uint8_t *image_one = NULL;
    uint8_t *image_two = NULL;
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
        memcpy(image_one,image_two,image_one_len);
        lua_pushboolean(L,1);
        return 1;
    }
    return 0;
}

static int
lua_image_blend(lua_State *L) {
    /* image:blend(src,alpha) */
    int i = 0;
    uint8_t *image_one = NULL;
    uint8_t *image_two = NULL;
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
            image_one[i] =   ((image_one[i] * alpha_inv) + (image_two[i] * alpha)) >> 8;
            image_one[i+1] = ((image_one[i+1] * alpha_inv) + (image_two[i+1] * alpha)) >> 8;
            image_one[i+2] = ((image_one[i+2] * alpha_inv) + (image_two[i+2] * alpha)) >> 8;
            image_one[i+3] = ((image_one[i+3] * alpha_inv) + (image_two[i+3] * alpha)) >> 8;
            image_one[i+4] = ((image_one[i+4] * alpha_inv) + (image_two[i+4] * alpha)) >> 8;
            image_one[i+5] = ((image_one[i+5] * alpha_inv) + (image_two[i+5] * alpha)) >> 8;
            image_one[i+6] = ((image_one[i+6] * alpha_inv) + (image_two[i+6] * alpha)) >> 8;
            image_one[i+7] = ((image_one[i+7] * alpha_inv) + (image_two[i+7] * alpha)) >> 8;
        }
        lua_pushboolean(L,1);
        return 1;
    }
    return 0;
}

static int
lua_image_stamp_image(lua_State *L) {
    uint8_t *image_one = NULL;
    uint8_t *image_two = NULL;

    lua_Integer x;
    lua_Integer y;

    int hflip = 0;
    int vflip = 0;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
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
        mask_left = luaL_optinteger(L,-1,0);
        lua_getfield(L,6,"right");
        mask_right = luaL_optinteger(L,-1,0);
        lua_getfield(L,6,"top");
        mask_top = luaL_optinteger(L,-1,0);
        lua_getfield(L,6,"bottom");
        mask_bottom = luaL_optinteger(L,-1,0);
    }

    if(lua_isnumber(L,7)) {
        aa = lua_tointeger(L,7);
    }

    lua_getfield(L,1,"width");
    width = lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = lua_tointeger(L,-1);

    lua_getfield(L,1,"image");
    image_one = lua_touserdata(L,-1);

    lua_getfield(L,2,"width");
    src_width = lua_tointeger(L,-1);

    lua_getfield(L,2,"height");
    src_height = lua_tointeger(L,-1);

    lua_getfield(L,2,"channels");
    src_channels = lua_tointeger(L,-1);

    lua_getfield(L,2,"image");
    image_two = lua_touserdata(L,-1);

    lua_pop(L,8);

    if(x < 1) {
        xi = xi + (x * -1) + 1;
    }
    if(y < 1) {
        yi = yi + (y * -1) + 1;
    }

    xm = src_width;
    ym = src_height;

    xi += mask_left;
    yi += mask_top;
    xm -= mask_right;
    ym -= mask_bottom;

    for(yii=yi;yii <= ym; yii++) {
        for(xii=xi;xii <= xm; xii++) {
            xt = xii;
            yt = yii;

            if(hflip) {
                xt = src_width - xii + 1;
            }
            if(vflip) {
                yt = src_height - yii + 1;
            }

            dxt = x - 1 + xii;
            dyt = y - 1 + yii;

            if(dxt > width || dyt > height) {
                continue;
            }

            xt = xt - 1;
            yt = src_height - yt;

            dxt = dxt - 1;
            dyt = height - dyt;

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

            b = image_two[byte];
            g = image_two[byte + 1];
            r = image_two[byte + 2];

            if(aa > -1 && a > 0) {
                a = aa;
            }

            byte = (dyt * width * channels) + (dxt * channels);

            if(a == 255) {
                image_one[byte] = b;
                image_one[byte+1] = g;
                image_one[byte+2] = r;
                continue;
            }
            alpha = 1 + a;
            alpha_inv = 256 - a;

            image_one[byte]   = ((image_one[byte]   * alpha_inv) + (b * alpha)) >> 8;
            image_one[byte+1] = ((image_one[byte+1] * alpha_inv) + (g * alpha)) >> 8;
            image_one[byte+2] = ((image_one[byte+2] * alpha_inv) + (r * alpha)) >> 8;
        }
    }

    return 0;

}

static int
lua_image_get_ref(lua_State *L) {
    lua_pushvalue(L,1);
    intptr_t r = luaL_ref(L,LUA_REGISTRYINDEX);
    lua_pushinteger(L,r);
    return 1;
}

static int
lua_image_from_ref(lua_State *L) {
    intptr_t t = lua_tointeger(L,1);
    lua_rawgeti(L,LUA_REGISTRYINDEX,t);
    return 1;
}

static int
lua_image_rotate(lua_State *L) {
    uint8_t *original;
    uint8_t *rotated;
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
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
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
    lua_rawgeti(L,-1,frameno); /* push */
    frame_ind = lua_gettop(L);

    lua_getfield(L,1,"width"); /* push */
    width = lua_tointeger(L,-1);

    lua_getfield(L,1,"height"); /* push */
    height = lua_tointeger(L,-1);

    lua_getfield(L,1,"channels"); /* push */
    channels = lua_tointeger(L,-1);

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
        diag = lua_tointeger(L,-1);
        lua_pop(L,3);
    }
    memset(rotated,0,diag*diag*4);

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
    { "set", lua_image_set },
    { "blend", lua_image_blend },
    { "stamp_image", lua_image_stamp_image },
    { NULL, NULL },
};

static const struct luaL_Reg lua_image_instance_methods[] = {
    { "unload", lua_image_unload },
    { "load", lua_image_load },
    { "get_ref", lua_image_get_ref },
    { "rotate", lua_image_rotate },
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
    qsize++;
    wake_queue();
    thread_join(thread);
    thread_destroy(thread);
    thread_queue_term(&thread_queue);
    thread_signal_term(&t_signal);
    return 0;
}

int
luaopen_image(lua_State *L) {
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
        fprintf(stderr,"error: %s\n",lua_tostring(L,-1));
        return 1;
    }

    if(lua_pcall(L,0,0,0)) {
        fprintf(stderr,"error: %s\n",lua_tostring(L,-1));
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

