#include "lua-datetime.h"

#include "datetime-struct.h"
#include "datetime.h"
#include "str.h"

#include <lua.h>
#include <lauxlib.h>

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
lua_get_datetime(lua_State *L) {
    datetime_t dt;
    int utc = lua_toboolean(L,1);
    get_datetime(&dt,utc);
    lua_newtable(L);

    lua_pushinteger(L,dt.year);
    lua_setfield(L,-2,"year");

    lua_pushinteger(L,dt.month);
    lua_setfield(L,-2,"month");

    lua_pushinteger(L,dt.day);
    lua_setfield(L,-2,"day");

    lua_pushinteger(L,dt.hour);
    lua_setfield(L,-2,"hour");

    lua_pushinteger(L,dt.min);
    lua_setfield(L,-2,"min");

    lua_pushinteger(L,dt.sec);
    lua_setfield(L,-2,"sec");

    lua_pushinteger(L,dt.wday);
    lua_setfield(L,-2,"wday");

    lua_pushinteger(L,dt.yday);
    lua_setfield(L,-2,"yday");

    lua_pushinteger(L,dt.isdst);
    lua_setfield(L,-2,"isdst");

    lua_pushinteger(L,dt.msec);
    lua_setfield(L,-2,"msec");

    return 1;
}

static const struct luaL_Reg lua_datetime_methods[] = {
    { "now", lua_get_datetime },
    { NULL, NULL },
};

int luaopen_datetime(lua_State *L) {
    lua_newtable(L);
    luaL_setfuncs(L,lua_datetime_methods,0);
    return 1;
}
