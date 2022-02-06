#ifndef LUA_STREAM_H
#define LUA_STREAM_H

#include <lua.h>
#include "int.h"
#include "video-generator.h"

#ifdef __cplusplus
extern "C" {
#endif

void luastream_init(lua_State *L, video_generator *v);

int
luaopen_stream(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif
