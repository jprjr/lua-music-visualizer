#ifndef LUA_FRAME_H
#define LUA_FRAME_H

/* defines a common "frame" format used by images
 * and videos */

#include <lua.h>
#include "int.h"

#ifdef __cplusplus
extern "C" {
#endif

void luaframe_init(lua_State *L);

int
luaopen_frame(lua_State *L);

int
luaframe_new(lua_State *L, lua_Integer width, lua_Integer height, lua_Integer channels, const jpr_uint8 *data, unsigned int linesize);

int
luaframe_from(lua_State *L, lua_Integer width, lua_Integer height, lua_Integer channels, jpr_uint8 *data);

#ifdef __cplusplus
}
#endif

#endif
