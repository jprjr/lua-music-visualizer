#include <lua.h>
#include <lauxlib.h>
#include <math.h>
#include "color.lua.lh"

#include "hedley.h"

#if 0
/* the pure-lua version of color.lua does this,
 * but I can't seem to get the C implementation to
 * return the same values. */
#define POW52 4503599627370496.0
#define POW51 2251799813685248.0
#define ROUND(x) floor( ((x) + (POW52 + POW51) - (POW52 + POW51)))
#endif

#define ROUND(x) lrintf(x)

typedef struct tuple_s {
    union { float r; float h; };
    union { float g; float s; };
    union { float b; float l; float v; };
} tuple_s;

/* use the same logic as lua for fmod */
HEDLEY_CONST
HEDLEY_INLINE
HEDLEY_PRIVATE
int color_fmod(float a, float b) {
    float m = fmodf(a,b);

    /* original logic in lua
    if(((m) > 0) ? (b) < 0 : ((m) < 0 && (b) > 0)) m += b;
    we know b is always positive, simplify
    */
    if(m<0) m += b;
    return m;
}

HEDLEY_PURE
HEDLEY_NON_NULL(2)
HEDLEY_INLINE
HEDLEY_PRIVATE
float findmax(tuple_s rgb, int* HEDLEY_RESTRICT which) {
    float m = -1.0;
    int w = 0;

    if(rgb.r > m) {
        m = rgb.r;
        w = 1;
    }

    if(rgb.g > m) {
        m = rgb.g;
        w = 2;
    }

    if(rgb.b > m) {
        m = rgb.b;
        w = 3;
    }

    *which = w;
    return m;
}

HEDLEY_CONST
HEDLEY_INLINE
HEDLEY_PRIVATE
float findmin(tuple_s rgb) {
    float m = 2.0;

    if(rgb.r < m) {
        m = rgb.r;
    }

    if(rgb.g < m) {
        m = rgb.g;
    }

    if(rgb.b < m) {
        m = rgb.b;
    }

    return m;
}

HEDLEY_CONST
HEDLEY_INLINE
HEDLEY_PRIVATE
tuple_s rgb_to_hsl(tuple_s rgb) {
    tuple_s hsl;

    int which;
    float cmax, cmin, delta;

    rgb.r /= 255.0;
    rgb.g /= 255.0;
    rgb.b /= 255.0;

    which = 0;
    cmax = findmax(rgb,&which);
    cmin = findmin(rgb);
    delta = cmax - cmin;

    hsl.h = 0.0;
    hsl.s = 0.0;
    hsl.l = (cmax + cmin) / 2.0;

    if(delta > 0.0) {
        hsl.s = delta / ( 1.0 - fabs((2.0 * hsl.l) - 1.0));
        switch(which) {
            case 1: hsl.h = 60.0 * (color_fmod((rgb.g-rgb.b)/delta,6.0)); break;
            case 2: hsl.h = 60.0 * (((rgb.b-rgb.r)/delta) + 2.0); break;
            case 3: hsl.h = 60.0 * (((rgb.r-rgb.g)/delta) + 4.0); break;
            default: HEDLEY_UNREACHABLE(); break;
        }
    }

    hsl.s *= 100.0;
    hsl.l *= 100.0;

    return hsl;
}

HEDLEY_CONST
HEDLEY_INLINE
HEDLEY_PRIVATE
tuple_s rgb_to_hsv(tuple_s rgb) {
    tuple_s hsv;

    int which;
    float cmax, cmin, delta;

    rgb.r /= 255.0;
    rgb.g /= 255.0;
    rgb.b /= 255.0;

    which = 0;
    cmax = findmax(rgb,&which);
    cmin = findmin(rgb);
    delta = cmax - cmin;

    hsv.h = 0.0;
    hsv.s = 0.0;
    hsv.v = cmax;

    if(delta > 0.0) {
        switch(which) {
            case 1: hsv.h = 60.0 * (color_fmod((rgb.g-rgb.b)/delta,6.0)); break;
            case 2: hsv.h = 60.0 * (((rgb.b-rgb.r)/delta) + 2.0); break;
            case 3: hsv.h = 60.0 * (((rgb.r-rgb.g)/delta) + 4.0); break;
            default: HEDLEY_UNREACHABLE(); break;
        }
    }

    if(cmax != 0.0) {
        hsv.s = delta / cmax;
    }

    hsv.s *= 100.0;
    hsv.v *= 100.0;

    return hsv;
}

HEDLEY_CONST
HEDLEY_INLINE
HEDLEY_PRIVATE
tuple_s hsl_to_rgb(tuple_s hsl) {
    tuple_s rgb;
    float c, hp, x, m;
    int hpi;

    hsl.s /= 100.0;
    hsl.l /= 100.0;

    c = ( 1.0 - fabs((2.0 * hsl.l) - 1.0)) * hsl.s;
    hp = hsl.h / 60.0;
    x = c * (1.0 - fabs(color_fmod(hp,2.0) - 1.0));
    m = hsl.l - (c / 2.0);

    rgb.r = 0.0;
    rgb.g = 0.0;
    rgb.b = 0.0;

    hpi = (int)hp;
    switch(hpi) {
        case 0: rgb.r=c; rgb.g=x; break;
        case 1: rgb.r=x; rgb.g=c; break;
        case 2: rgb.g=c; rgb.b=x; break;
        case 3: rgb.g=x; rgb.b=c; break;
        case 4: rgb.r=x; rgb.b=c; break;
        case 5: rgb.r=c; rgb.b=x; break;
        default: HEDLEY_UNREACHABLE(); break;
    }

    rgb.r = (rgb.r + m) * 255.0;
    rgb.g = (rgb.g + m) * 255.0;
    rgb.b = (rgb.b + m) * 255.0;

    return rgb;
}

HEDLEY_CONST
HEDLEY_INLINE
HEDLEY_PRIVATE
tuple_s hsv_to_rgb(tuple_s hsv) {
    tuple_s rgb;
    float c, hp, x, m;
    int hpi;

    hsv.s /= 100.0;
    hsv.l /= 100.0;

    c = hsv.v * hsv.s;
    hp = hsv.h / 60.0;
    hpi = (int)hp;
    x = c * (1.0 - fabs(color_fmod(hp,2.0) - 1.0));
    m = hsv.v - c;

    rgb.r = 0.0;
    rgb.g = 0.0;
    rgb.b = 0.0;

    hpi = (int)hp;
    switch(hpi) {
        case 0: rgb.r=c; rgb.g=x; break;
        case 1: rgb.r=x; rgb.g=c; break;
        case 2: rgb.g=c; rgb.b=x; break;
        case 3: rgb.g=x; rgb.b=c; break;
        case 4: rgb.r=x; rgb.b=c; break;
        case 5: rgb.r=c; rgb.b=x; break;
        default: HEDLEY_UNREACHABLE(); break;
    }

    rgb.r = (rgb.r + m) * 255.0;
    rgb.g = (rgb.g + m) * 255.0;
    rgb.b = (rgb.b + m) * 255.0;

    return rgb;
}

HEDLEY_PRIVATE
int
luargb_to_hsl(lua_State *L) {
    tuple_s rgb;
    tuple_s hsl;

    rgb.r = (float)lua_tonumber(L,1);
    rgb.g = (float)lua_tonumber(L,2);
    rgb.b = (float)lua_tonumber(L,3);

    if( rgb.r <= 0.0) rgb.r = 0.0;
    if( rgb.r >= 255.0) rgb.r = 255.0;

    if( rgb.g <= 0.0) rgb.g = 0.0;
    if( rgb.g >= 255.0) rgb.g = 255.0;

    if( rgb.b <= 0.0) rgb.b = 0.0;
    if( rgb.b >= 255.0) rgb.b = 255.0;

    hsl = rgb_to_hsl(rgb);

    lua_pushinteger(L,(lua_Integer)ROUND(hsl.h));
    lua_pushinteger(L,(lua_Integer)ROUND(hsl.s));
    lua_pushinteger(L,(lua_Integer)ROUND(hsl.l));

    return 3;
}

HEDLEY_PRIVATE
int
luargb_to_hsv(lua_State *L) {
    tuple_s rgb;
    tuple_s hsv;

    rgb.r = (float)lua_tonumber(L,1);
    rgb.g = (float)lua_tonumber(L,2);
    rgb.b = (float)lua_tonumber(L,3);

    if( rgb.r <= 0.0) rgb.r = 0.0;
    if( rgb.r >= 255.0) rgb.r = 255.0;

    if( rgb.g <= 0.0) rgb.g = 0.0;
    if( rgb.g >= 255.0) rgb.g = 255.0;

    if( rgb.b <= 0.0) rgb.b = 0.0;
    if( rgb.b >= 255.0) rgb.b = 255.0;

    hsv = rgb_to_hsv(rgb);

    lua_pushinteger(L,(lua_Integer)ROUND(hsv.h));
    lua_pushinteger(L,(lua_Integer)ROUND(hsv.s));
    lua_pushinteger(L,(lua_Integer)ROUND(hsv.v));

    return 3;
}

HEDLEY_PRIVATE
int
luahsl_to_rgb(lua_State *L) {
    tuple_s hsl;
    tuple_s rgb;

    hsl.h = (float)lua_tonumber(L,1);
    hsl.s = (float)lua_tonumber(L,2);
    hsl.l = (float)lua_tonumber(L,3);

    if(hsl.h <= 0.0) hsl.h = 0.0;
    while(hsl.h >= 360.0) hsl.h -= 360.0;

    if( hsl.s <= 0.0) hsl.s = 0.0;
    if( hsl.s >= 100.0) hsl.s = 100.0;

    if( hsl.l <= 0.0) hsl.l = 0.0;
    if( hsl.l >= 100.0) hsl.l = 100.0;

    rgb = hsl_to_rgb(hsl);

    lua_pushnumber(L,(lua_Integer)ROUND( rgb.r ));
    lua_pushnumber(L,(lua_Integer)ROUND( rgb.g ));
    lua_pushnumber(L,(lua_Integer)ROUND( rgb.b ));

    return 3;
}

HEDLEY_PRIVATE
int
luahsv_to_rgb(lua_State *L) {
    tuple_s hsv;
    tuple_s rgb;

    hsv.h = (float)lua_tonumber(L,1);
    hsv.s = (float)lua_tonumber(L,2);
    hsv.v = (float)lua_tonumber(L,3);

    if(hsv.h <= 0.0) hsv.h = 0.0;
    while(hsv.h >= 360.0) hsv.h -= 360.0;

    if( hsv.s <= 0.0) hsv.s = 0.0;
    if( hsv.s >= 100.0) hsv.s = 100.0;

    if( hsv.v <= 0.0) hsv.v = 0.0;
    if( hsv.v >= 100.0) hsv.v = 100.0;

    rgb = hsv_to_rgb(hsv);

    lua_pushnumber(L,(lua_Integer)ROUND( rgb.r ));
    lua_pushnumber(L,(lua_Integer)ROUND( rgb.g ));
    lua_pushnumber(L,(lua_Integer)ROUND( rgb.b ));
    return 3;
}

int luaopen_color(lua_State *L) {
    lua_newtable(L);

    lua_pushcclosure(L,luargb_to_hsl,0);
    lua_setfield(L,-2,"rgb_to_hsl");

    lua_pushcclosure(L,luargb_to_hsv,0);
    lua_setfield(L,-2,"rgb_to_hsv");

    lua_pushcclosure(L,luahsl_to_rgb,0);
    lua_setfield(L,-2,"hsl_to_rgb");

    lua_pushcclosure(L,luahsv_to_rgb,0);
    lua_setfield(L,-2,"hsv_to_rgb");

    if(luaL_loadbuffer(L,color_lua,color_lua_length - 1, "color.lua")) {
        return lua_error(L);
    }

    lua_pushvalue(L,-2);

    if(lua_pcall(L,1,0,0)) {
        return lua_error(L);
    }

    return 1;
}
