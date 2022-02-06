#ifndef LUA_AUDIO_H
#define LUA_AUDIO_H

#include <lua.h>

#ifdef __cplusplus
extern "C" {
#endif

void luaaudio_init(lua_State *L, audio_processor *a);

int luaopen_audio(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif

