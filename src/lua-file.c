#include "mem.h"
#include "dir.h"
#include "path.h"
#include "lua-file.h"
#include "str.h"

#include <lua.h>
#include <lauxlib.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#ifndef _WIN32
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#endif

#ifdef __cplusplus
extern "C" {
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

static int
lua_file_exists(lua_State *L) {
    const char *filename = luaL_checkstring(L,1);
    lua_pushboolean(L,path_exists(filename));
    return 1;
}

static int
lua_file_realpath(lua_State *L) {
    char res[PATH_MAX];
    const char *path = luaL_checkstring(L,1);
#ifdef _WIN32
    if(_fullpath(res,path,PATH_MAX) == NULL) {
#else
    if(realpath(path,res) == NULL) {
#endif
        return 0;
    }

    lua_pushstring(L,res);
    return 1;
}

static int
lua_file_basename(lua_State *L) {
    const char *folder = luaL_checkstring(L,1);
    char *b = path_basename(folder);
    if(b != NULL) {
        lua_pushstring(L,b);
        mem_free(b);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int
lua_file_dirname(lua_State *L) {
    const char *folder = luaL_checkstring(L,1);
    char *b = path_dirname(folder);
    if(b != NULL) {
        lua_pushstring(L,b);
        mem_free(b);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int
lua_file_getcwd(lua_State *L) {
    char res[PATH_MAX];
#ifdef _WIN32
    if(_getcwd(res,PATH_MAX) == NULL) {
#else
    if(getcwd(res,PATH_MAX) == NULL) {
#endif
        return 0;
    }

    lua_pushstring(L,res);
    return 1;
}

static int
lua_file_list(lua_State *L) {
    const char *folder = luaL_checkstring(L,1);

    jpr_dir *dir;
    jpr_dire *entry;
    int j = 1;

    dir = dir_open(folder);
    if(dir == NULL) return 0;
    lua_newtable(L);

    while( (entry = dir_read(dir)) != NULL) {
        lua_pushinteger(L,j);
        lua_newtable(L);
        lua_pushstring(L,entry->path);
        lua_setfield(L,-2,"file");
        lua_pushinteger(L,entry->mtime);
        lua_setfield(L,-2,"mtime");
        lua_pushinteger(L,entry->size);
        lua_setfield(L,-2,"size");
        lua_settable(L,-3);
        j++;

        dire_free(entry);
    }

    dir_close(dir);


    return 1;
}


static const struct luaL_Reg lua_file_methods[] = {
    { "ls"       , lua_file_list     },
    { "exists"   , lua_file_exists   },
    { "dirname"  , lua_file_dirname  },
    { "basename" , lua_file_basename },
    { "realpath" , lua_file_realpath },
    { "getcwd"   , lua_file_getcwd   },
    { NULL       , NULL              },
};

int luaopen_file(lua_State *L) {
    lua_newtable(L);
    luaL_setfuncs(L,lua_file_methods,0);
    lua_setglobal(L,"file");

    return 0;
}

#ifdef __cplusplus
}
#endif
