#include "int.h"
#include "image.h"
#include "lua-frame.h"
#include "lua-image.h"
#include "image.lua.lh"
#include "thread.h"
#include "str.h"
#include "util.h"
#include "int.h"
#include <lauxlib.h>

#include <assert.h>
#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif


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

void
wake_queue(void) {
    thread_signal_raise(&t_signal);
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
        luaframe_new(L,width,height,channels,b);
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
        lua_pushinteger(L,IMAGE_FIXED);
        lua_setfield(L,table_ind,"image_state");

        lua_pushliteral(L,"fixed");
        lua_setfield(L,table_ind,"state");

        lua_newtable(L); /* image.frames  -- push */
        frame_ind = lua_gettop(L);

        luaframe_new(L,width,height,channels,NULL);

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

    luaL_getmetatable(L,"image");
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
            luaframe_new(L,width,height,channels,b);
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

static const struct luaL_Reg lua_image_instance_methods[] = {
    { "unload", lua_image_unload },
    { "load", lua_image_load },
    { "get_ref", lua_image_get_ref },
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


