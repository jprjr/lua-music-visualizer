#ifndef LUA_AUDIO_H
#define LUA_AUDIO_H

#include <lua.h>

#ifdef __cplusplus
extern "C" {
#endif

int luaopen_audio(lua_State *L,audio_processor *a);

#ifdef __cplusplus
}
#endif

#endif

