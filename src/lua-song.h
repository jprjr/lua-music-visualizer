#ifndef LUA_SONG_H
#define LUA_SONG_H

#include <lua.h>

#ifdef __cplusplus
extern "C" {
#endif

/* creates the hidden "song" table to store in the registry,
 * returns the reference */
int luasong_init(lua_State *L);

int
luaopen_song(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif

