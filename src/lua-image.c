#include "int.h"
#include "image.h"
#include "lua-frame.h"
#include "lua-image.h"
#include "lua-thread.h"
#include "thread.h"
#include "str.h"
#include "util.h"
#include "int.h"
#include <lauxlib.h>

#include <assert.h>
#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

enum IMAGE_STATE {
    IMAGE_ERR,
    IMAGE_UNLOADED,
    IMAGE_LOADING,
    IMAGE_LOADED,
    IMAGE_FIXED
};

typedef struct image_q {
    int table_ref;
    char *filename;
    unsigned int width;
    unsigned int height;
    unsigned int channels;
    unsigned int frames;
    jpr_uint8 *image;
} image_q;

static void image_q_free(lua_State *L, void *value) {
    image_q *q = (image_q *)value;
    if(q->table_ref != LUA_NOREF) {
        luaL_unref(L,LUA_REGISTRYINDEX,q->table_ref);
    }
    if(q->image != NULL) {
        free(q->image);
    }
    free(q->filename);
    free(value);
}

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
    

static void
lua_load_image_cb(void *Lua, unsigned int frames, jpr_uint8 *image) {
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
        if(luaframe_new(L,width,height,channels,b) == 1) {
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
        } else {
            /* top of the index is the error message */
            LOG_ERROR(lua_tostring(L,-1));
            lua_pop(L,2);
        }
    }

    lua_setfield(L,table_ind,"delays");
    lua_setfield(L,table_ind,"frames");

    lua_pushinteger(L,IMAGE_LOADED);
    lua_setfield(L,table_ind,"image_state");

    lua_pushliteral(L,"loaded");
    lua_setfield(L,table_ind,"state");

    lua_pushinteger(L,frames);
    lua_setfield(L,table_ind,"framecount");

#ifndef NDEBUG
    assert(lua_top == lua_gettop(L));
#endif

    return;
}


static int
bgimage_process(void *userdata, lmv_produce produce, void *queue) {
    image_q *q = userdata;
    q->image = image_load(q->filename,&(q->width),&(q->height),&(q->channels),&(q->frames));
    produce(queue,q);
    return 0;
}

static int
lua_image_new(lua_State *L) {
    const char *filename = NULL;
    int table_ind = 0;
    int frame_ind = 0;
    int delay_ind = 0;
    unsigned int width;
    unsigned int height;
    unsigned int channels;
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

        if(luaframe_new(L,width,height,channels,NULL) == 1) {
            lua_rawseti(L,frame_ind,1);
        } else {
            LOG_ERROR(lua_tostring(L,-1));
            lua_pop(L,2);
        }
        lua_setfield(L,table_ind,"frames");

        lua_newtable(L); /* image.delays */
        delay_ind = lua_gettop(L);
        lua_pushinteger(L,0);
        lua_rawseti(L,delay_ind,1);
        lua_setfield(L,table_ind,"delays");

        lua_pushinteger(L,1);
        lua_setfield(L,table_ind,"framecount");
    }

    luaL_getmetatable(L,"lmv.image");
    lua_setmetatable(L,table_ind);

#ifndef NDEBUG
    assert(table_ind == lua_gettop(L));
    assert(lua_top+1 == lua_gettop(L));
#endif

    return 1;
}

static int
lua_image_loader_load(lua_State *L) {
    const char *filename = NULL;
    image_q *q = NULL;

    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;

    int table_ref = 0;

    lua_getfield(L,1,"width");
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"filename");
    filename = lua_tostring(L,-1);

    lua_pop(L,4);

    q = (image_q *)malloc(sizeof(image_q));

    if(UNLIKELY(q == NULL)) {
        LOG_ERROR("error: out of memory");
        JPR_EXIT(1);
    }

    lua_pushvalue(L,1);
    table_ref = luaL_ref(L,LUA_REGISTRYINDEX);
    if(UNLIKELY(table_ref == LUA_NOREF)) {
        LOG_ERROR("error: unable to create reference");
        JPR_EXIT(1);
    }

    q->filename = malloc(str_len(filename) + 1);
    if(UNLIKELY(q->filename == NULL)) {
        LOG_ERROR("error: out of memory");
        JPR_EXIT(1);
    }
    str_cpy(q->filename,filename);

    q->table_ref = table_ref;
    q->width = width;
    q->height = height;
    q->channels = channels;
    q->frames = 0;
    q->image = NULL;

    lmv_thread_inject(lua_touserdata(L,lua_upvalueindex(1)),q);

    lua_pushboolean(L,1);
    return 1;
}

static int
lua_image_loader_update(lua_State *L) {
#ifndef NDEBUG
    int lua_top = lua_gettop(L);
#endif
    lmv_thread_t *t = (lmv_thread_t *)lua_touserdata(L,lua_upvalueindex(1));
    image_q *q = NULL;
    int res = 0;
    int total = 0;
    while( (q = (image_q *)lmv_thread_result(t,&res)) != NULL) {
        lua_rawgeti(L,LUA_REGISTRYINDEX,q->table_ref);
        lua_load_image_cb(L, q->frames, q->image);
        lua_pop(L,1);
        image_q_free(L,q);
        total++;
    }
#ifndef NDEBUG
    assert(lua_top == lua_gettop(L));
#endif
    lua_pushinteger(L,total);
    return 1;
}

static int
lua_image_loader_new(lua_State *L) {
    int qsize = 0;
    qsize = luaL_optinteger(L,1,100);
    if(qsize < 1) {
        return luaL_error(L,"invalid queue size for loader");
    }
    if(lmv_thread_new(L,bgimage_process,image_q_free,image_q_free,qsize,"image loader") != 1) {
        return luaL_error(L,"error creating loader");
    }
    lua_newtable(L);

    lua_pushvalue(L,-2);
    lua_pushcclosure(L,lua_image_loader_load,1);
    lua_setfield(L,-2,"load");

    lua_pushvalue(L,-2);
    lua_pushcclosure(L,lua_image_loader_update,1);
    lua_setfield(L,-2,"update");

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

/* the default synchronous loader */
static int
lua_image_load_default(lua_State *L) {
    const char *filename = NULL;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    unsigned int frames = 0;
    int delay = 0;

    int frame_ind = 0;
    int delay_ind = 0;
    unsigned int i = 0;

    jpr_uint8 *t = NULL;
    jpr_uint8 *b = NULL;

    lua_getfield(L,1,"width");
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"filename");
    filename = lua_tostring(L,-1);

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
        if(luaframe_new(L,width,height,channels,b) == 1) {
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
        } else {
            LOG_ERROR(lua_tostring(L,-1));
            lua_pop(L,2);
        }
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

static int
lua_image_load(lua_State *L) {
    int state = 0;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Bad argument for self");
        return 2;
    }

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

    if(lua_istable(L,2)) {
        lua_getfield(L,2,"load");
    } else {
        lua_pushnil(L);
    }

    if(!lua_isfunction(L,-1)) {
        lua_pushcfunction(L,lua_image_load_default);
    }
    lua_pushvalue(L,1);
    lua_call(L,1,1);
    return 1;
}

static const struct luaL_Reg lua_image_instance_methods[] = {
    { "unload", lua_image_unload },
    { "load", lua_image_load },
    { NULL, NULL },
};

static const struct luaL_Reg lua_image_methods[] = {
    { "new"    , lua_image_new },
    { "loader" , lua_image_loader_new },
    { NULL     , NULL                 },
};

static void luaimage_init(lua_State *L) {
#ifndef NDEBUG
    int lua_top = lua_gettop(L);
#endif
    lmv_thread_init(L);

    if(luaL_newmetatable(L,"lmv.image")) {
        lua_newtable(L);
        luaL_setfuncs(L,lua_image_instance_methods,0);
        lua_setfield(L,-2,"__index");
    }
    lua_pop(L,1);
#ifndef NDEBUG
    assert(lua_top == lua_gettop(L));
#endif
}

int
luaopen_image(lua_State *L) {
    luaimage_init(L);

    lua_newtable(L);
    luaL_setfuncs(L,lua_image_methods,0);

    return 1;
}


