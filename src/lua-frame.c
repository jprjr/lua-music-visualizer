#include "lua-frame.h"
#include "frame.lua.lh"
#include "str.h"
#include "util.h"
#include <lauxlib.h>

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath.h"

#define MY_PI 3.14159265358979323846
#define rad(degrees) ( ((double)degrees) * MY_PI / 180.0)

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

/* Handmade math uses floats everywhere, define a custom integer
 * vector */
typedef union jpr_vec2i {
    struct {
        int X, Y;
    };
    struct {
        int U, V;
    };
    struct {
        int Left, Right;
    };
    struct {
        int Width, Height;
    };
    int Elements[2];
} jpr_vec2i;

typedef union jpr_vec2ui {
    struct {
        unsigned int X, Y;
    };
    struct {
        unsigned int U, V;
    };
    struct {
        unsigned int Left, Right;
    };
    struct {
        unsigned int Width, Height;
    };
    unsigned int Elements[2];
} jpr_vec2ui;

HMM_INLINE jpr_vec2i JPR_Vec2i(int X, int Y) {
    jpr_vec2i r;
    r.X = X;
    r.Y = Y;

    return (r);
}

HMM_INLINE jpr_vec2ui JPR_Vec2ui(unsigned int X, unsigned int Y) {
    jpr_vec2ui r;
    r.X = X;
    r.Y = Y;

    return (r);
}

hmm_vec3 barycentric(jpr_vec2i *pts, jpr_vec2i P) {
    hmm_vec3 u = HMM_Cross(HMM_Vec3( pts[2].X - pts[0].X, pts[1].X - pts[0].X, pts[0].X - P.X  ), HMM_Vec3(pts[2].Y - pts[0].Y, pts[1].Y - pts[0].Y, pts[0].Y - P.Y));
    if(fabsf(u.Z) < 1) return HMM_Vec3(-1, 1, 1);
    return HMM_Vec3(1.f - (u.X + u.Y)/u.Z, u.Y/u.Z, u.X/u.Z);
}

static void
luaframe_draw_line_vertical(jpr_uint8 *frame, unsigned int width, unsigned int height, unsigned int channels, int x, int y1, int y2, jpr_uint8 r, jpr_uint8 g, jpr_uint8 b, jpr_uint8 a) {
    int s_x;
    int s_y1;
    int s_y2;
    int sy;
    int max_byte;
    unsigned int alpha;
    unsigned int alpha_inv;

    if(y1 <= y2) {
        sy = 1;
    }
    else {
        sy = -1;
    }

    max_byte = width * height * channels;

    s_x = x * channels;
    s_y1  = y1 * channels * width;
    s_y2  = y2 * channels * width;

    alpha = 1 + (unsigned int)a;
    alpha_inv = 256 - (unsigned int)a;
    sy *= width * channels;

    while(1) {
        if(s_x > 0 && s_y1 > 0 && (s_y1 + s_x < max_byte)) {
            frame[s_y1+s_x+0] = ((frame[s_y1+s_x+0] * alpha_inv) + (b * alpha)) >> 8;
            frame[s_y1+s_x+1] = ((frame[s_y1+s_x+1] * alpha_inv) + (g * alpha)) >> 8;
            frame[s_y1+s_x+2] = ((frame[s_y1+s_x+2] * alpha_inv) + (r * alpha)) >> 8;
        }
        if(s_y1 == s_y2) break;
        s_y1 += sy;
    }

}

static void
luaframe_draw_line_horizontal(jpr_uint8 *frame, unsigned int width, unsigned int height, unsigned int channels, int x1, int x2, int y, jpr_uint8 r, jpr_uint8 g, jpr_uint8 b, jpr_uint8 a) {
    int s_x1;
    int s_x2;
    int sx;
    int s_y;
    int max_byte;
    unsigned int alpha;
    unsigned int alpha_inv;

    if(x1 <= x2) {
        sx = 1;
    }
    else {
        sx = -1;
    }

    max_byte = width * height * channels;

    s_x1 = x1 * channels;
    s_x2 = x2 * channels;
    s_y  = y * channels * width;

    alpha = 1 + (unsigned int)a;
    alpha_inv = 256 - (unsigned int)a;
    sx *= channels;

    while(1) {
        if(s_y > 0 && s_x1 > 0 && (s_y + s_x1 < max_byte)) {
            frame[s_y+s_x1+0] = ((frame[s_y+s_x1+0] * alpha_inv) + (b * alpha)) >> 8;
            frame[s_y+s_x1+1] = ((frame[s_y+s_x1+1] * alpha_inv) + (g * alpha)) >> 8;
            frame[s_y+s_x1+2] = ((frame[s_y+s_x1+2] * alpha_inv) + (r * alpha)) >> 8;
        }
        if(s_x1 == s_x2) break;
        s_x1 += sx;
    }

}

static void
luaframe_draw_line_internal(jpr_uint8 *frame, unsigned int width, unsigned int height, unsigned int channels, int x1, int y1, int x2, int y2, jpr_uint8 r, jpr_uint8 g, jpr_uint8 b, jpr_uint8 a) {
    int s_x1;
    int s_x2;
    int s_y1;
    int s_y2;
    int sx;
    int sy;
    int dx;
    int dy;
    int err;
    int e2;
    int max_byte;

    unsigned int alpha;
    unsigned int alpha_inv;

    if(x1 <= x2) {
        sx = 1;
    }
    else {
        sx = -1;
    }

    if(y1 <= y2) {
        sy = 1;
    }
    else {
        sy = -1;
    }

    max_byte = width * height * channels;

    dx = labs(x2 - x1);
    dy = -1 * labs(y2 - y1);
    err = dx + dy;

    s_x1 = x1 * channels;
    s_x2 = x2 * channels;
    s_y1 = y1 * channels * width;
    s_y2 = y2 * channels * width;

    alpha = 1 + (unsigned int)a;
    alpha_inv = 256 - (unsigned int)a;
    sx *= channels;
    sy *= channels * width;

    while(1) {
        if(s_y1 > 0 && s_x1 > 0 && (s_y1 + s_x1 < max_byte)) {
            frame[s_y1+s_x1+0] = ((frame[s_y1+s_x1+0] * alpha_inv) + (b * alpha)) >> 8;
            frame[s_y1+s_x1+1] = ((frame[s_y1+s_x1+1] * alpha_inv) + (g * alpha)) >> 8;
            frame[s_y1+s_x1+2] = ((frame[s_y1+s_x1+2] * alpha_inv) + (r * alpha)) >> 8;
        }
        if(s_x1 == s_x2 || s_y1 == s_y2) break;
        e2 = 2 * err;
        if(e2 >= dy) {
            err += dy;
            s_x1 += sx;
        }
        if(e2 <= dx) {
            err += dx;
            s_y1 += sy;
        }
    }
    if(s_x1 != s_x2) {
        luaframe_draw_line_horizontal(frame,width,height,channels,s_x1 / channels, s_x2 / channels, s_y1 / (width * channels), r, g, b, a);
    }
    else if(s_y1 != s_y2) {
        luaframe_draw_line_vertical(frame,width,height,channels,s_x1 / channels, s_y1 / (width *channels), s_y2 / (width * channels), r, g, b, a);
    }
}

static void
luaframe_draw_triangle_internal(jpr_uint8 *frame, unsigned int *dimensions, jpr_vec2i *pts, jpr_uint8 *color) {
    /* assumptions:
     *   pts Y-axis has already been inverted */
    unsigned int width;
    unsigned int height;
    unsigned int channels;
    unsigned int byte;
    unsigned int offset;
    unsigned int i;
    unsigned int alpha;
    unsigned int alpha_inv;

    jpr_vec2i bbox_min;
    jpr_vec2i bbox_max;
    jpr_vec2i clamp;
    jpr_vec2i P;
    hmm_vec3 bc_screen;

    width = dimensions[0];
    height = dimensions[1];
    channels = dimensions[2];

    bbox_min.X = width - 1;
    bbox_min.Y = height - 1;
    bbox_max.X = 0;
    bbox_max.Y = 0;
    clamp.X = width - 1;
    clamp.Y = height - 1;

    alpha = 1 + (unsigned int)color[3];
    alpha_inv = 256 - (unsigned int)color[3];

    for(i=0;i<3;i++) {
        bbox_min.X = HMM_MAX(0, HMM_MIN((int)bbox_min.X,pts[i].X));
        bbox_min.Y = HMM_MAX(0, HMM_MIN((int)bbox_min.Y,pts[i].Y));
        bbox_max.X = HMM_MIN(clamp.X, HMM_MAX((int)bbox_max.X,pts[i].X));
        bbox_max.Y = HMM_MIN(clamp.Y, HMM_MAX((int)bbox_max.Y,pts[i].Y));
    }

    for(P.Y = bbox_min.Y; P.Y <= bbox_max.Y; P.Y++) {
        offset = P.Y * width;
        for(P.X = bbox_min.X; P.X <= bbox_max.X; P.X++) {
            bc_screen = barycentric(pts,P);
            if(bc_screen.X < 0 || bc_screen.Y < 0 || bc_screen.Z < 0) continue;
            byte = (offset + P.X) * channels;
            frame[byte + 0] = ((frame[byte + 0] * alpha_inv) + (color[0] * alpha)) >> 8;
            frame[byte + 1] = ((frame[byte + 1] * alpha_inv) + (color[1] * alpha)) >> 8;
            frame[byte + 2] = ((frame[byte + 2] * alpha_inv) + (color[2] * alpha)) >> 8;
        }
    }
}


static int
luaframe_set(lua_State *L) {
    /* frame:set(src) */
    jpr_uint8 *frame_one = NULL;
    jpr_uint8 *frame_two = NULL;
    lua_Integer frame_one_len = 0;
    lua_Integer frame_two_len = 0;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    if(!lua_istable(L,2)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument src");
        return 2;
    }

    lua_getfield(L,1,"frame");
    frame_one = lua_touserdata(L,-1);
    lua_getfield(L,1,"frame_len");
    frame_one_len = lua_tointeger(L,-1);

    lua_getfield(L,2,"frame");
    frame_two = lua_touserdata(L,-1);
    lua_getfield(L,2,"frame_len");
    frame_two_len = lua_tointeger(L,-1);

    if(frame_one != NULL && frame_two != NULL && (frame_one_len == frame_two_len)) {
        mem_cpy(frame_one,frame_two,frame_one_len);
        lua_pushboolean(L,1);
        return 1;
    }
    return 0;
}

static int
luaframe_blend(lua_State *L) {
    /* frame:blend(src,alpha) */
    int i = 0;
    jpr_uint8 *frame_one = NULL;
    jpr_uint8 *frame_two = NULL;
    lua_Integer a;
    lua_Integer alpha;
    lua_Integer alpha_inv;
    lua_Integer frame_one_len = 0;
    lua_Integer frame_two_len = 0;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    if(!lua_istable(L,2)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument src");
        return 2;
    }

    a = luaL_checkinteger(L,3);
    if(a == 0) {
        return 0;
    }

    alpha = 1 + a;
    alpha_inv = 256 - a;

    lua_getfield(L,1,"frame");
    frame_one = lua_touserdata(L,-1);
    lua_getfield(L,1,"frame_len");
    frame_one_len = lua_tointeger(L,-1);

    lua_getfield(L,2,"frame");
    frame_two = lua_touserdata(L,-1);
    lua_getfield(L,2,"frame_len");
    frame_two_len = lua_tointeger(L,-1);

    lua_pop(L,4);

    if(frame_one && frame_two && (frame_one_len == frame_two_len)) {
        for(i=0;i<frame_one_len;i+=8) {
            frame_one[i] =   (jpr_uint8)(((frame_one[i+0] * alpha_inv) + (frame_two[i+0] * alpha)) >> 8);
            frame_one[i+1] = (jpr_uint8)(((frame_one[i+1] * alpha_inv) + (frame_two[i+1] * alpha)) >> 8);
            frame_one[i+2] = (jpr_uint8)(((frame_one[i+2] * alpha_inv) + (frame_two[i+2] * alpha)) >> 8);
            frame_one[i+3] = (jpr_uint8)(((frame_one[i+3] * alpha_inv) + (frame_two[i+3] * alpha)) >> 8);
            frame_one[i+4] = (jpr_uint8)(((frame_one[i+4] * alpha_inv) + (frame_two[i+4] * alpha)) >> 8);
            frame_one[i+5] = (jpr_uint8)(((frame_one[i+5] * alpha_inv) + (frame_two[i+5] * alpha)) >> 8);
            frame_one[i+6] = (jpr_uint8)(((frame_one[i+6] * alpha_inv) + (frame_two[i+6] * alpha)) >> 8);
            frame_one[i+7] = (jpr_uint8)(((frame_one[i+7] * alpha_inv) + (frame_two[i+7] * alpha)) >> 8);
        }
        lua_pushboolean(L,1);
        return 1;
    }
    return 0;
}

static int luaframe_set_pixel(lua_State *L) {
    jpr_uint8 *frame = NULL;

    unsigned int index = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    unsigned int alpha = 0;
    unsigned int alpha_inv = 0;

    lua_Integer x;
    lua_Integer y;
    lua_Integer r;
    lua_Integer g;
    lua_Integer b;
    lua_Integer a;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    x = luaL_checkinteger(L,2);
    y = luaL_checkinteger(L,3);
    r = luaL_checkinteger(L,4);
    g = luaL_checkinteger(L,5);
    b = luaL_checkinteger(L,6);
    a = luaL_optinteger(L,7,255);

    if(a == 0) {
        lua_pushboolean(L,1);
        return 1;
    }

    lua_getfield(L,1,"frame");
    frame = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = (unsigned int)luaL_checkinteger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)luaL_checkinteger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)luaL_checkinteger(L,-1);

    lua_pop(L,4);

    if(x < 1 || y < 1 || (unsigned int)x > width || (unsigned int)y > height) {
        lua_pushboolean(L,0);
        return 1;
    }

    if(r > 255 || b > 255 || g > 255 || a > 255 ||
       r < 0   || b < 0   || g < 0 || a < 0 ) {
        lua_pushboolean(L,0);
        return 1;
    }
    lua_pushboolean(L,1);

    x = x - 1;
    y = height - y;

    index = (unsigned int)((y * width * channels) + (x * channels));

    if(a == 255) {
        frame[index] = (jpr_uint8)b;
        frame[index+1] = (jpr_uint8)g;
        frame[index+2] = (jpr_uint8)r;
        if(channels == 4) frame[index+3] = 255;
        return 1;
    }

    alpha = 1 + (unsigned int)a;
    alpha_inv = 256 - (unsigned int)a;

    frame[index]   = (jpr_uint8)(((frame[index] * alpha_inv) + (b * alpha)) >> 8);
    frame[index+1] = (jpr_uint8)(((frame[index+1] * alpha_inv) + (g * alpha)) >> 8);
    frame[index+2] = (jpr_uint8)(((frame[index+2] * alpha_inv) + (r * alpha)) >> 8);
    if(channels == 4) frame[index+3] = 255;

    return 1;
}

static int
luaframe_get_pixel(lua_State *L) {
    jpr_uint8 *frame = NULL;
    unsigned int index = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    lua_Integer x;
    lua_Integer y;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    x = luaL_checkinteger(L,2);
    y = luaL_checkinteger(L,3);

    lua_getfield(L,1,"frame");
    frame = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = (unsigned int)luaL_checkinteger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)luaL_checkinteger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)luaL_checkinteger(L,-1);

    if(x < 1 || y < 1 || (unsigned int)x > width || (unsigned int)y > height) {
        lua_pushboolean(L,0);
        return 1;
    }

    x = x - 1;
    y = height - y;

    index = (unsigned int)((y * width * channels) + (x * channels));

    lua_pushinteger(L,frame[index + 2]);
    lua_pushinteger(L,frame[index + 1]);
    lua_pushinteger(L,frame[index]);

    if(channels == 4) {
        lua_pushinteger(L,frame[index + 3]);
    }
    else {
        lua_pushinteger(L,255);
    }

    return 4;
}

static int
luaframe_set_alpha(lua_State *L) {
    jpr_uint8 *frame = NULL;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;

    unsigned int xstart = 0;
    unsigned int xend = 0;
    unsigned int ystart = 0;
    unsigned int yend = 0;
    unsigned int ytmp = 0;

    unsigned int byte;
    unsigned int offset;
    unsigned int counter;

    lua_Integer x1;
    lua_Integer y1;
    lua_Integer x2;
    lua_Integer y2;
    lua_Integer a;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    x1 = luaL_checkinteger(L,2);
    y1 = luaL_checkinteger(L,3);
    x2 = luaL_checkinteger(L,4);
    y2 = luaL_checkinteger(L,5);
    a  = luaL_checkinteger(L,6);

    lua_getfield(L,1,"frame");
    frame = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_pop(L,4);

    if(channels != 4 || a > 255 ||  a < 0 ) {
        lua_pushboolean(L,0);
        return 1;
    }

    x1 = ( x1 < 0 ) ? 0 : x1;
    y1 = ( y1 < 0 ) ? 0 : y1;
    x2 = ( x2 < 0 ) ? 0 : x2;
    y2 = ( y2 < 0 ) ? 0 : y2;

    if(x1 <= x2) {
        xstart = (unsigned int)x1;
        xend = (unsigned int)x2;
    }
    else {
        xstart = (unsigned int)x2;
        xend = (unsigned int)x1;
    }

    if(y1 <= y2) {
        ystart = (unsigned int)y1;
        yend = (unsigned int)y2;
    }
    else {
        ystart = (unsigned int)y2;
        yend = (unsigned int)y1;
    }

    if(xend < 1) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"xend < 1");
        return 2;
    }

    if(yend < 1) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"yend < 1");
        return 2;
    }

    if(xstart > width) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"xstart > width");
        return 2;
    }

    if(ystart > height) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"ystart > height");
        return 2;
    }

    if(xstart < 1) {
        xstart = 1;
    }

    if(ystart < 1) {
        ystart = 1;
    }

    if(xend > width) {
        xend = width;
    }

    if(yend > height) {
        yend = height;
    }
    xstart--;
    ystart--;

    /* invert the ys */
    ytmp = ystart;
    ystart = yend;
    yend = ytmp;

    ystart = height - ystart;
    yend   = height - yend;

    xstart *= channels;
    xend *= channels;
    offset = xend - xstart;
    width *= channels;
    ystart *= width;
    yend *= width;

    while(ystart < yend) {
        byte = ystart + xstart;
        counter = 0;
        while(counter < offset) {
            frame[byte+counter+3] = a;
            counter += channels;
        }
        ystart += width;
    }

    lua_pushboolean(L,1);
    return 1;
}

static int
luaframe_draw_rectangle(lua_State *L) {
    jpr_uint8 *frame = NULL;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;

    unsigned int xstart = 0;
    unsigned int xend = 0;
    unsigned int ystart = 0;
    unsigned int yend = 0;
    unsigned int ytmp = 0;

    unsigned int byte;
    unsigned int offset;
    unsigned int counter;
    unsigned int alpha;
    unsigned int alpha_inv;

    lua_Integer x1;
    lua_Integer y1;
    lua_Integer x2;
    lua_Integer y2;
    lua_Integer r;
    lua_Integer g;
    lua_Integer b;
    lua_Integer a;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    x1 = luaL_checkinteger(L,2);
    y1 = luaL_checkinteger(L,3);
    x2 = luaL_checkinteger(L,4);
    y2 = luaL_checkinteger(L,5);
    r  = luaL_checkinteger(L,6);
    g  = luaL_checkinteger(L,7);
    b  = luaL_checkinteger(L,8);
    a  = luaL_optinteger(L,9,255);

    if(a == 0) {
        lua_pushboolean(L,1);
        return 1;
    }

    lua_getfield(L,1,"frame");
    frame = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_pop(L,4);

    if(r > 255 || b > 255 || g > 255 || a > 255 ||
       r < 0   || b < 0   || g < 0 || a < 0 ) {
        lua_pushboolean(L,0);
        return 1;
    }

    x1 = ( x1 < 0 ) ? 0 : x1;
    y1 = ( y1 < 0 ) ? 0 : y1;
    x2 = ( x2 < 0 ) ? 0 : x2;
    y2 = ( y2 < 0 ) ? 0 : y2;

    if(x1 <= x2) {
        xstart = (unsigned int)x1;
        xend = (unsigned int)x2;
    }
    else {
        xstart = (unsigned int)x2;
        xend = (unsigned int)x1;
    }

    if(y1 <= y2) {
        ystart = (unsigned int)y1;
        yend = (unsigned int)y2;
    }
    else {
        ystart = (unsigned int)y2;
        yend = (unsigned int)y1;
    }

    if(xend < 1) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"xend < 1");
        return 2;
    }

    if(yend < 1) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"yend < 1");
        return 2;
    }

    if(xstart > width) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"xstart > width");
        return 2;
    }

    if(ystart > height) {
        lua_pushboolean(L,0);
        lua_pushstring(L,"ystart > height");
        return 2;
    }

    if(xstart < 1) {
        xstart = 1;
    }

    if(ystart < 1) {
        ystart = 1;
    }

    if(xend > width) {
        xend = width;
    }

    if(yend > height) {
        yend = height;
    }
    xstart--;
    ystart--;

    /* invert the ys */
    ytmp = ystart;
    ystart = yend;
    yend = ytmp;

    ystart = height - ystart;
    yend   = height - yend;

    xstart *= channels;
    xend *= channels;
    offset = xend - xstart;
    width *= channels;
    ystart *= width;
    yend *= width;
    alpha = 1 + (unsigned int)a;
    alpha_inv = 256 - (unsigned int)a;

    while(ystart < yend) {
        byte = ystart + xstart;
        counter = 0;
        while(counter < offset) {
            frame[byte+counter+0] = ((frame[byte+counter+0] * alpha_inv) + (b * alpha)) >> 8;
            frame[byte+counter+1] = ((frame[byte+counter+1] * alpha_inv) + (g * alpha)) >> 8;
            frame[byte+counter+2] = ((frame[byte+counter+2] * alpha_inv) + (r * alpha)) >> 8;
            counter += channels;
        }
        ystart += width;
    }

    lua_pushboolean(L,1);
    return 1;
}

static int
luaframe_draw_line(lua_State *L) {
    jpr_uint8 *frame = NULL;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    lua_Integer x1;
    lua_Integer y1;
    lua_Integer x2;
    lua_Integer y2;
    lua_Integer r;
    lua_Integer g;
    lua_Integer b;
    lua_Integer a;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    x1 = luaL_checkinteger(L,2);
    y1 = luaL_checkinteger(L,3);
    x2 = luaL_checkinteger(L,4);
    y2 = luaL_checkinteger(L,5);
    r  = luaL_checkinteger(L,6);
    g  = luaL_checkinteger(L,7);
    b  = luaL_checkinteger(L,8);
    a  = luaL_optinteger(L,9,255);

    if(a == 0) {
        lua_pushboolean(L,1);
        return 1;
    }

    lua_getfield(L,1,"frame");
    frame = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_pop(L,4);

    if(r > 255 || b > 255 || g > 255 || a > 255 ||
       r < 0   || b < 0   || g < 0 || a < 0 ) {
        lua_pushboolean(L,0);
        return 1;
    }

    x1--;
    x2--;
    y1 = height - y1;
    y2 = height - y2;

    luaframe_draw_line_internal(frame,width,height,channels,x1,y1,x2,y2,(jpr_uint8)r,(jpr_uint8)g,(jpr_uint8)b,(jpr_uint8)a);

    lua_pushboolean(L,1);
    return 1;
}

static int
luaframe_stamp_frame(lua_State *L) {
    jpr_uint8 *frame_one = NULL;
    jpr_uint8 *frame_two = NULL;

    lua_Integer x;
    lua_Integer y;

    int hflip = 0;
    int vflip = 0;
    jpr_uint8 r;
    jpr_uint8 g;
    jpr_uint8 b;
    jpr_uint8 a;
    lua_Integer aa = -1;

    int mask_left = 0;
    int mask_right = 0;
    int mask_top = 0;
    int mask_bottom = 0;
    unsigned int src_width = 0;
    unsigned int src_height = 0;
    unsigned int src_channels = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    int xi = 1;
    int yi = 1;
    int xm;
    int ym;
    int yii;
    int xii;
    int xt;
    int yt;
    unsigned int dxt;
    unsigned int dyt;
    unsigned int y_transform;
    unsigned int x_transform;
    int y_mult;
    int x_mult;
    int byte;
    int alpha;
    int alpha_inv;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    if(!lua_istable(L,2)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument src");
        return 2;
    }

    x = luaL_optinteger(L,3,1);
    y = luaL_optinteger(L,4,1);

    if(lua_istable(L,5)) {
        lua_getfield(L,5,"vflip");
        vflip = lua_toboolean(L,-1);
        lua_getfield(L,5,"hflip");
        hflip = lua_toboolean(L,-1);
    }

    if(lua_istable(L,6)) {
        lua_getfield(L,6,"left");
        mask_left = (int)luaL_optinteger(L,-1,0);
        lua_getfield(L,6,"right");
        mask_right = (int)luaL_optinteger(L,-1,0);
        lua_getfield(L,6,"top");
        mask_top = (int)luaL_optinteger(L,-1,0);
        lua_getfield(L,6,"bottom");
        mask_bottom = (int)luaL_optinteger(L,-1,0);
    }

    if(lua_isnumber(L,7)) {
        aa = lua_tointeger(L,7);
    }

    lua_getfield(L,1,"width");
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"frame");
    frame_one = lua_touserdata(L,-1);

    lua_getfield(L,2,"width");
    src_width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,2,"height");
    src_height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,2,"channels");
    src_channels = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,2,"frame");
    frame_two = lua_touserdata(L,-1);

    lua_pop(L,8);

    if(x < 1) {
        xi = (int)(xi + (x * -1) + 1);
    }
    if(y < 1) {
        yi = (int)(yi + (y * -1) + 1);
    }

    xm = src_width;
    ym = src_height;

    xi += mask_left;
    yi += mask_top;
    xm -= mask_right;
    ym -= mask_bottom;

    while(y - 1 + ym > (int)height) {
        ym--;
    }

    while(x - 1 + xm > (int)width) {
        xm--;
    }

    if(vflip) {
        y_mult = -1;
        y_transform = src_height - 1;
    } else {
        y_mult = 1;
        y_transform = 0;
    }

    x_transform = 1;
    x_mult = 1;

    if(hflip) {
        x_transform = src_width;
        x_mult = -1;
    }

    for(yii=yi;yii <= ym; yii++) {
        dyt = (unsigned int)(y - 1 + yii);
        dyt = (unsigned int)(height - dyt);

        yt = (src_height - yii - y_transform) * y_mult;

        for(xii=xi;xii <= xm; xii++) {
            dxt = (unsigned int)(x - 1 + xii);
            dxt = dxt - 1;

            xt = (xii - x_transform) * x_mult;

            byte = (yt * src_width * src_channels) + (xt * src_channels);

            if(src_channels == 4) {
                a = frame_two[byte + 3];
            }
            else {
                a = 255;
            }

            if(a == 0) {
                continue;
            }

            b = frame_two[byte + 0];
            g = frame_two[byte + 1];
            r = frame_two[byte + 2];

            if(aa > -1) {
                a = (jpr_uint8)aa;
            }

            byte = (dyt * width * channels) + (dxt * channels);

            if(a == 255) {
                frame_one[byte+0] = b;
                frame_one[byte+1] = g;
                frame_one[byte+2] = r;
                continue;
            }
            alpha = 1 + a;
            alpha_inv = 256 - a;

            frame_one[byte+0] = ((frame_one[byte+0] * alpha_inv) + (b * alpha)) >> 8;
            frame_one[byte+1] = ((frame_one[byte+1] * alpha_inv) + (g * alpha)) >> 8;
            frame_one[byte+2] = ((frame_one[byte+2] * alpha_inv) + (r * alpha)) >> 8;
        }
    }

    return 0;

}

static int
luaframe_stamp_letter(lua_State *L) {
    /* frame:stamp_letter(font,codepoint,scale,x,y,r,g,b,hloffset,hroffset,ytoffset,yboffset,hflip,vflip */
    /* returns width of letter rendered, in pixels */
    jpr_uint8 *frame;
    unsigned int codepoint;
    unsigned int scale;
    unsigned int x;
    unsigned int y;
    jpr_uint8 r;
    jpr_uint8 g;
    jpr_uint8 b;
    unsigned int hloffset;
    unsigned int hroffset;
    unsigned int ytoffset;
    unsigned int yboffset;
    unsigned int pixel_hroffset;
    unsigned int pixel_yboffset;
    unsigned int width;
    unsigned int height;
    unsigned int xc;
    unsigned int yc;
    unsigned int xcc;
    unsigned int xccc;
    unsigned int ycc;
    unsigned int xi;
    unsigned int yi;
    unsigned int w;
    unsigned int cur;
    unsigned int index;
    unsigned int scaled_width;
    unsigned int scaled_height;

    unsigned int frame_width;
    unsigned int frame_height;
    unsigned int frame_channels;

    int dest_x;
    int dest_y;

    int vflip;
    int hflip;

    /* self = 1 */
    /* font = 2 */

    lua_getfield(L,1,"frame");
    frame = lua_touserdata(L,-1);

    lua_getfield(L,1,"width");
    frame_width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height");
    frame_height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels");
    frame_channels = (unsigned int)lua_tointeger(L,-1);

    lua_pop(L,4);

    codepoint = (unsigned int)luaL_checkinteger(L,3);
    scale     = (unsigned int)luaL_checkinteger(L,4);
    x         = (unsigned int)luaL_checkinteger(L,5);
    y         = (unsigned int)luaL_checkinteger(L,6);
    r         = (jpr_uint8)luaL_checkinteger(L,7);
    g         = (jpr_uint8)luaL_checkinteger(L,8);
    b         = (jpr_uint8)luaL_checkinteger(L,9);
    hloffset  = (unsigned int)luaL_optinteger(L,10,0);
    hroffset  = (unsigned int)luaL_optinteger(L,11,0);
    ytoffset  = (unsigned int)luaL_optinteger(L,12,0);
    yboffset  = (unsigned int)luaL_optinteger(L,13,0);
    hflip     = (lua_isboolean(L,14) ? lua_toboolean(L,14) : 0);
    vflip     = (lua_isboolean(L,15) ? lua_toboolean(L,15) : 0);

    lua_getfield(L,2,"widths");
    lua_rawgeti(L,-1,codepoint);
    if(lua_isnil(L,-1)) {
        lua_getfield(L,2,"width");
        width = (unsigned int)lua_tointeger(L,-1);
        lua_pop(L,3);
        lua_pushinteger(L,width * scale);
        return 1;
    } else {
        width = (unsigned int)lua_tointeger(L,-1);
        lua_pop(L,2);
    }

    lua_getfield(L,2,"height");
    height = (unsigned int)lua_tointeger(L,-1);
    lua_pop(L,1);

    scaled_width = width * scale;
    scaled_height = height * scale;

    if(hroffset > scaled_width ||
       yboffset > scaled_height ||
       hloffset > scaled_width ||
       ytoffset > scaled_height) {
        lua_pushinteger(L,width * scale);
        return 1;
    }

    w = 1 << ( (width + 7) & ~0x07);

    pixel_hroffset = scaled_width  - hroffset;
    pixel_yboffset = scaled_height - yboffset;

    lua_getfield(L,2,"bitmaps");
    lua_rawgeti(L,-1,codepoint);

    /*for(yc=height;yc>=1;--yc) {*/
    for(yc=1;yc<=height;++yc) {
        lua_rawgeti(L,-1,(vflip ? height - yc + 1 : yc));
        cur = (unsigned int)lua_tointeger(L,-1);
        lua_pop(L,1);
        if(cur == 0) continue;
        for(xc=1;xc<=width;++xc) {
            xccc = (hflip ? width - xc + 1 : xc);
            if( cur & (w >> xccc)) {
                for(yi=0;yi<scale;++yi) {
                    ycc = (yc - 1) * scale + yi;
                    if(ycc >= ytoffset && ycc < pixel_yboffset) {
                        for(xi=0;xi<scale;xi++) {
                            xcc = (xc - 1) * scale + xi;
                            if(xcc >= hloffset && xcc < pixel_hroffset) {
                                /* self:set_pixel(x+xcc,y+((yc-1) * scale) + yi, r, g, b) */
                                /* inline this shit */
                                dest_y = (y + ycc);
                                dest_x = (x + xcc);
                                if(dest_x < 1 || dest_y < 1) continue;
                                if((unsigned int)dest_x > frame_width || (unsigned int)dest_y > frame_height) continue;
                                dest_x--;
                                dest_y = frame_height - dest_y;
                                index = (dest_y * frame_width * frame_channels) + (dest_x * frame_channels);
                                frame[index] = b;
                                frame[index+1] = g;
                                frame[index+2] = r;

                            }
                        }

                    }
                }

            }
        }
    }

    lua_pushinteger(L,width * scale);
    return 1;
}

static int
luaframe_draw_triangle(lua_State *L) {
    jpr_uint8 *frame;
    jpr_uint8 color[4];
    unsigned int dimensions[3];
    jpr_vec2i pts[3]; /* 3 points of a triangle */
    unsigned int a;

#ifndef NDEBUG
    int lua_top = lua_gettop(L);
#endif

    a  = luaL_optinteger(L,9,255);
    if(a == 0) return 0;
    color[0] = lua_tointeger(L,5); /* b */
    color[1] = lua_tointeger(L,4); /* g */
    color[2] = lua_tointeger(L,3); /* r */
    color[3] = a;

    lua_getfield(L,1,"width"); /* push */
    dimensions[0] = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height"); /* push */
    dimensions[1] = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels"); /* push */
    dimensions[2] = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"frame"); /* push */
    frame = lua_touserdata(L,-1);

    lua_pop(L,4);

    lua_rawgeti(L,2,1);
    lua_rawgeti(L,-1,1);
    pts[0].X = lua_tointeger(L,-1) - 1;
    lua_rawgeti(L,-2,2);
    pts[0].Y = dimensions[1] - lua_tointeger(L,-1);

    lua_rawgeti(L,2,2);
    lua_rawgeti(L,-1,1);
    pts[1].X = lua_tointeger(L,-1) - 1;
    lua_rawgeti(L,-2,2);
    pts[1].Y = dimensions[1] - lua_tointeger(L,-1);

    lua_rawgeti(L,2,3);
    lua_rawgeti(L,-1,1);
    pts[2].X = lua_tointeger(L,-1) - 1;
    lua_rawgeti(L,-2,2);
    pts[2].Y = dimensions[1] - lua_tointeger(L,-1);

    lua_pop(L,9);

    luaframe_draw_triangle_internal(frame, dimensions, pts, color);

#ifndef NDEBUG
    assert(lua_top == lua_gettop(L));
#endif

    return 0;
}

static int
luaframe_tile(lua_State *L) {
    /* input: an frame frame
     * returns a new 2d array, "tiled"
     */
    jpr_uint8 *original;
    jpr_uint8 *chunk;
    unsigned int width;
    unsigned int height;
    unsigned int channels;
    unsigned int x;
    unsigned int y;

    unsigned int x_tiles;
    unsigned int y_tiles;
    unsigned int x_ind;
    unsigned int y_ind;
    unsigned int y_cpy_ind;
    unsigned int tile_x;
    unsigned int tile_y;
    unsigned int y_ind_inv;
    unsigned int y_cpy_ind_inv;

    lua_getfield(L,1,"width"); /* push */
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height"); /* push */
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels"); /* push */
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"frame"); /* push */
    original = lua_touserdata(L,-1);

    lua_pop(L,4);

    x = luaL_checkinteger(L,2);
    y = luaL_checkinteger(L,3);

    x_tiles = width / x;
    if(width % x != 0) {
        x_tiles++;
    }

    y_tiles = height / y;
    if(height % y != 0) {
        y_tiles++;
    }

    lua_newtable(L); /* "tiled" array */

    x_ind = 0;
    while(x_ind < x_tiles) {
        lua_newtable(L); /* tiled[x_ind] */
        tile_x = x;
        if(x_ind == x_tiles - 1 && width % x != 0) {
            tile_x = width % x;
        }

        y_ind = 0;
        while(y_ind < y_tiles) {
            lua_newtable(L); /* tiled[x_ind][y_ind] */
            tile_y = y;

            if(y_ind == y_tiles - 1 && height % y != 0) {
                tile_y = height % y;
            }

            lua_pushinteger(L,tile_x);
            lua_setfield(L,-2,"width");

            lua_pushinteger(L,tile_y);
            lua_setfield(L,-2,"height");

            lua_pushinteger(L,channels);
            lua_setfield(L,-2,"channels");

            lua_pushinteger(L,tile_x * tile_y * channels);
            lua_setfield(L,-2,"frame_len");

            chunk = lua_newuserdata(L,tile_x * tile_y * channels);
            lua_setfield(L,-2,"frame");

            luaL_getmetatable(L,"frame");
            lua_setmetatable(L,-2);

            y_cpy_ind = 0;
            while(y_cpy_ind < tile_y) {
                y_ind_inv = height - (
                  (y_ind * y) + y_cpy_ind ) - 1;
                y_cpy_ind_inv = tile_y - y_cpy_ind - 1;
                memcpy(
                  &chunk[
                    (y_cpy_ind_inv * tile_x) * channels
                  ],
                  &original[
                    ((y_ind_inv * width) +
                    (x_ind * x)) * channels
                  ],
                  tile_x * channels);
                y_cpy_ind++;
            }
            lua_rawseti(L,-2,++y_ind);
        }
        lua_rawseti(L,-2,++x_ind);
    }

    return 1;
}

static int
luaframe_rotate(lua_State *L) {
    jpr_uint8 *original;
    jpr_uint8 *rotated;
    int width;
    int height;
    int h_width;
    int h_height;
    int h_diag;
    int d_x;
    int d_y;
    int x;
    int y;
    int xs;
    int ys;
    int channels;
    int diag;
    unsigned int index;
    jpr_uint8 r;
    jpr_uint8 g;
    jpr_uint8 b;
    jpr_uint8 a;
    double rads;

    double cos_rads;
    double sin_rads;
    double cosmax;
    double sinmax;
    double cosmaxt_p_hwidth_p_sinmahheight;
    double sinmaxt_p_hheight_m_cosmahheight;
#ifndef NDEBUG
    int top;
#endif

    lua_Number rotation;

    if(!lua_istable(L,1)) {
        lua_pushnil(L);
        lua_pushliteral(L,"Missing argument self");
        return 2;
    }

    rotation = luaL_checknumber(L,2);

#ifndef NDEBUG
    top = lua_gettop(L);
#endif

    lua_getfield(L,1,"width"); /* push */
    width = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"height"); /* push */
    height = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"channels"); /* push */
    channels = (unsigned int)lua_tointeger(L,-1);

    lua_getfield(L,1,"frame"); /* push */
    original = lua_touserdata(L,-1);

    lua_pop(L,4);

#ifndef NDEBUG
    assert(top == lua_gettop(L));
#endif

    diag = (int)(ceil(sqrt((width*width) + (height*height))));

    /* if diag is set, the frame is already rotated + centered */
    lua_newtable(L); /* push */

    lua_pushinteger(L,4);
    lua_setfield(L,-2,"channels");

    lua_pushinteger(L,diag);
    lua_setfield(L,-2,"width");

    lua_pushinteger(L,diag);
    lua_setfield(L,-2,"height");

    lua_pushinteger(L, (diag >> 1) - (width >> 1));
    lua_setfield(L,-2,"width_offset");

    lua_pushinteger(L, (diag >> 1) - (height >> 1));
    lua_setfield(L,-2,"height_offset");

    rotated = lua_newuserdata(L,diag*diag*4);
    lua_setfield(L,-2,"frame");

    lua_pushinteger(L,diag * diag * 4);
    lua_setfield(L,-2,"frame_len");

    luaL_getmetatable(L,"frame");
    lua_setmetatable(L,-2);

    mem_set(rotated,0,diag*diag*4);

    h_width = width >> 1;
    h_height = height >> 1;
    h_diag = diag >> 1;
    d_x = h_diag - h_width;
    d_y = h_diag - h_height;

    /* we have pointers to the original frame and the rotated, time to rotate */

    while(rotation < 0.0) {
        rotation += 360.0;
    }
    while(rotation > 359.0) {
        rotation -= 360.0;
    }
    rads = rad(rotation);

    cos_rads = cos(rads);
    sin_rads = sin(rads);

    cosmax = -cos_rads * h_diag + h_diag + sin_rads * h_diag;
    sinmax = -sin_rads * h_diag + h_diag - cos_rads * h_diag;

    for(x=0;x<diag;++x) {
        cosmaxt_p_hwidth_p_sinmahheight = cosmax;
        sinmaxt_p_hheight_m_cosmahheight = sinmax;

        for(y=0;y<diag;++y) {
            xs = ((int)(round(cosmaxt_p_hwidth_p_sinmahheight))) - d_x;
            ys = ((int)(round(sinmaxt_p_hheight_m_cosmahheight))) - d_y;

            if(xs >= 0 && xs < width && ys >= 0 && ys < height) { /* {{{ */
                /* ysi = height - (ys + 1); */
                index = (ys * width * channels) + (xs * channels);
                b = original[index];
                g = original[index+1];
                r = original[index+2];

                if(channels == 4) {
                    a = original[index+3];
                } else {
                    a = 255;
                }

                /* ysi = diag - (y + 1); */
                index = (y * diag * 4) + (x * 4);

                rotated[index] = b;
                rotated[index+1] = g;
                rotated[index+2] = r;
                rotated[index+3] = a;
            } /* }}} */
            cosmaxt_p_hwidth_p_sinmahheight -= sin_rads;
            sinmaxt_p_hheight_m_cosmahheight += cos_rads;
        }
        cosmax += cos_rads;
        sinmax += sin_rads;
    }

    return 1;
}

/* called with width, height, channels (3 or 4)
 * returns a table with:
 *   frame - userdata
 *   frame_len - length of frame in bytes
 *   width
 *   height
 *   channels
 */

static int lframe_new(lua_State *L) {
    lua_Integer width;
    lua_Integer height;
    lua_Integer channels;
    const jpr_uint8 *data;
    size_t len;

    width    = luaL_checkinteger(L,1);
    height   = luaL_checkinteger(L,2);
    channels = luaL_checkinteger(L,3);
    data     = (const jpr_uint8 *)lua_tolstring(L,4,&len);

    return luaframe_new(L,width,height,channels,data);
}

int
luaframe_new(lua_State *L, lua_Integer width, lua_Integer height, lua_Integer channels, const jpr_uint8 *data) {
    void *ud = NULL;
    lua_Integer len = 0;

    if(width < 0 || height < 0 || channels < 0) {
        lua_pushnil(L);
        lua_pushstring(L,"invalid parameters");
        return 2;
    }

    len = width * height * channels;
    if(len < 0) {
        lua_pushnil(L);
        lua_pushstring(L,"integer overflow");
        return 2;
    }

    lua_newtable(L);
    ud = lua_newuserdata(L,len);
    if(ud == NULL) {
        return luaL_error(L,"out of memory");
    }
    lua_setfield(L,-2,"frame");
    lua_pushinteger(L,len);
    lua_setfield(L,-2,"frame_len");

    lua_pushinteger(L,width);
    lua_setfield(L,-2,"width");

    lua_pushinteger(L,height);
    lua_setfield(L,-2,"height");

    lua_pushinteger(L,channels);
    lua_setfield(L,-2,"channels");

    luaL_getmetatable(L,"frame");
    lua_setmetatable(L,-2);

    if(data != NULL) {
        mem_cpy(ud,data,len);
    }

    return 1;
}


static const struct luaL_Reg luaframe_methods[] = {
    { "set"      ,      luaframe_set            },
    { "blend"    ,      luaframe_blend          },
    { "set_pixel",      luaframe_set_pixel      },
    { "get_pixel",      luaframe_get_pixel      },
    { "set_alpha",      luaframe_set_alpha      },
    { "draw_rectangle", luaframe_draw_rectangle },
    { "draw_line",      luaframe_draw_line      },
    { "stamp_frame",    luaframe_stamp_frame    },
    { "stamp_letter",   luaframe_stamp_letter   },
    { "draw_triangle",  luaframe_draw_triangle  },
    { "tile"         ,  luaframe_tile           },
    { NULL, NULL },
};

static const struct luaL_Reg luaframe_functions[] = {
    { "new", lframe_new },
    { NULL, NULL },
};

int luaopen_frame(lua_State *L) {
    const char *s;
    luaL_newmetatable(L,"frame");
    lua_newtable(L);
    luaL_setfuncs(L,luaframe_methods,0);
    lua_setfield(L,-2,"__index");
    lua_pop(L,1);

    lua_newtable(L);
    luaL_setfuncs(L,luaframe_functions,0);
    lua_setglobal(L,"frame");

    if(luaL_loadbuffer(L,frame_lua,frame_lua_length-1,"frame.lua")) {
        s = lua_tostring(L,-1);
        WRITE_STDERR("error: ");
        LOG_ERROR(s);
        return 1;
    }

    if(lua_pcall(L,0,0,0)) {
        s = lua_tostring(L,-1);
        WRITE_STDERR("error: ");
        LOG_ERROR(s);
        return 1;
    }

#ifndef NDEBUG
    assert(lua_top == lua_gettop(L));
#endif

    return 0;
}
