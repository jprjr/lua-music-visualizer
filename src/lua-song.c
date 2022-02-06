#include "lua-song.h"
#include <lauxlib.h>

static int song_table_ref = LUA_NOREF;

int luasong_init(lua_State *L) {
    if(song_table_ref == LUA_NOREF) {
        lua_newtable(L);
        song_table_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    return song_table_ref;
}

int luaopen_song(lua_State *L) {
    int ref = luasong_init(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    return 1;
}
