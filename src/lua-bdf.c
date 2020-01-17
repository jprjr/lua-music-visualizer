#include "bdf.h"
#include "utf.h"
#include "bdf.lua.lh"
#include "util.h"

#include <lua.h>
#include <lauxlib.h>

static int luabdf_pixel(lua_State *L) {
    lua_Integer index = lua_tointeger(L,2);
    lua_Integer x = lua_tointeger(L,3);
    lua_Integer y = lua_tointeger(L,4);
    unsigned int width;
    unsigned int bits;
    lua_getfield(L,1,"widths");
    lua_rawgeti(L,-1,index);
    if(lua_isnil(L,-1)) return 0;

    width = lua_tointeger(L,-1);
    lua_pop(L,2);

    width = (width + 7) & (~0x07);

    lua_getfield(L,1,"bitmaps");
    lua_rawgeti(L,-1,index);
    lua_rawgeti(L,-1,y);
    bits = lua_tointeger(L,-1);

    lua_pushboolean(L,bits & (( 1 << width) >> x));
    return 1;
}

static int luabdf_utf_to_table(lua_State *L) {
    const char *str;
    jpr_uint32 cp;
    unsigned int i = 1;

    str = luaL_checkstring(L,1);

    lua_newtable(L);

    while(*str) {
        str += utf_dec_utf8(&cp,(const jpr_uint8 *)str);
        lua_pushinteger(L,cp);
        lua_rawseti(L,-2,i++);
    }

    return 1;
}

static int luabdf_load(lua_State *L) {
    bdf *font;
    const char *filename;
    unsigned int i;
    unsigned int j;
    filename = luaL_checkstring(L,1);
    font = bdf_load(filename);
    if(font == NULL) return 0;

    lua_newtable(L);

    lua_pushinteger(L,font->width);
    lua_setfield(L,-2,"width");
    lua_pushinteger(L,font->height);
    lua_setfield(L,-2,"height");

    lua_createtable(L,font->max_glyph+1,0);
    lua_createtable(L,font->max_glyph+1,0);

    /* glyphs/widths is NOT zero-based, do not adjust i */
    for(i=0;i<=font->max_glyph;i++) {
        if(font->glyphs[i] == NULL) continue;
        lua_pushinteger(L,font->widths[i]);
        lua_rawseti(L,-3,i);

        lua_createtable(L,font->height,0);

        for(j=0;j<font->height;j++) {
            lua_pushinteger(L,font->glyphs[i][j]);
            lua_rawseti(L,-2,j+1);
        }
        lua_rawseti(L,-2,i);
    }

    lua_setfield(L,-3,"bitmaps");
    lua_setfield(L,-2,"widths");

    lua_pushvalue(L,lua_upvalueindex(1));
    lua_setmetatable(L,-2);

    bdf_free(font);
    return 1;
}

int luaopen_bdf(lua_State *L) {
    const char *err_str;
    if(luaL_loadbuffer(L,bdf_lua,bdf_lua_length-1,"bdf.lua")) {
        return 0;
    }

    lua_newtable(L);
    lua_pushvalue(L,-1);
    lua_setfield(L,-2,"__index");

    lua_pushcfunction(L,luabdf_pixel);
    lua_setfield(L,-2,"pixel");

    lua_newtable(L);
    lua_pushvalue(L,-2);
    lua_pushcclosure(L,luabdf_load,1);
    lua_setfield(L,-2,"load");

    lua_getfield(L,-1,"load");
    lua_setfield(L,-2,"new");

    lua_pushcfunction(L,luabdf_utf_to_table);
    lua_setfield(L,-2,"utf8_to_table");

    lua_pushstring(L,"1.0.1");
    lua_setfield(L,-2,"_VERSION");

    /* for possible LuaJIT extensions */
    lua_pushlightuserdata(L,bdf_load);
    lua_pushlightuserdata(L,bdf_free);
    lua_pushlightuserdata(L,utf_dec_utf8);

    if(lua_pcall(L,5,1,0)) {
        err_str = lua_tostring(L,-1);
        WRITE_STDERR("error loading bdf module: ");
        LOG_ERROR(err_str);
        return 0;
    }

    return 1;
}
