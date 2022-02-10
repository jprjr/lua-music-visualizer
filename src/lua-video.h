#ifndef LUA_VIDEO_H
#define LUA_VIDEO_H

#include <lua.h>
#include "video-generator.h"

#ifdef __cplusplus
extern "C" {
#endif

void luavideo_init(lua_State *L, video_generator *v);
int luaopen_video(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif
