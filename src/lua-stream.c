#include "lua-stream.h"
#include "lua-frame.h"
#include "stream.lua.lh"

#include <lauxlib.h>

static video_generator *video = NULL;

void luastream_init(lua_State *L, video_generator *v) {
    (void)L;
    video = v;
}

int luaopen_stream(lua_State *L) {
    if(luaL_loadbuffer(L,stream_lua,stream_lua_length - 1,"stream.lua")) {
        return lua_error(L);
    }

    if(lua_pcall(L,0,1,0)) {
        return lua_error(L);
    }

    luaframe_from(L,video->width,video->height,3,video->framebuf+8);
    lua_pushinteger(L,video->fps);
    lua_setfield(L,-2,"framerate");

    lua_setfield(L,-2,"video");

    return 1;
}
