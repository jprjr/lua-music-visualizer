#include "tinydir.h"
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
    lua_pushlstring(L, l->name,strlen(l->name));
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

#ifdef _WIN32
    lua_pushboolean(L,PathFileExists(filename));
    return 1;
#else
    if(access(filename,F_OK) != -1) {
        lua_pushboolean(L,1);
        return 1;
    }
    return 0;
#endif
}

static int
lua_file_realpath(lua_State *L) {
    char res[PATH_MAX];
    const char *path = luaL_checkstring(L,1);
#ifdef _WIN32
    _fullpath(res,path,PATH_MAX);
#else
    realpath(path,res);
#endif

    lua_pushstring(L,res);
    return 1;
}

static int
lua_file_basename(lua_State *L) {
    char res[PATH_MAX];
    const char *folder = luaL_checkstring(L,1);
    strcpy(res,folder);
    char *b = basename(res);

    lua_pushstring(L,b);
    return 1;
}

static int
lua_file_dirname(lua_State *L) {
    char res[PATH_MAX];
    const char *folder = luaL_checkstring(L,1);
    strcpy(res,folder);
    char *b = dirname(res);

    lua_pushstring(L,b);
    return 1;
}

static int
lua_file_getcwd(lua_State *L) {
    char res[PATH_MAX];
#ifdef _WIN32
    _getcwd(res,PATH_MAX);
#else
    getcwd(res,PATH_MAX);
#endif

    lua_pushstring(L,res);
    return 1;
}

static int
lua_file_list(lua_State *L) {
    const char *folder = luaL_checkstring(L,1);

    tinydir_dir dir;
    unsigned long int i;
    int j = 1;
    if(tinydir_open_sorted(&dir,folder) == -1) {
        return 0;
    }
    lua_newtable(L);

    for(i=0;i<dir.n_files;i++) {
        tinydir_file file;
        struct stat st;
        tinydir_readfile_n(&dir,&file,i);
        if(file.is_dir) {
            continue;
        }
        stat(file.path,&st);
        lua_pushinteger(L,j);
        lua_newtable(L);
        lua_pushlstring(L,file.path,strlen(file.path));
        lua_setfield(L,-2,"file");
        lua_pushinteger(L,st.st_mtime);
        lua_setfield(L,-2,"mtime");
        lua_settable(L,-3);
        j++;
    }

    tinydir_close(&dir);

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
