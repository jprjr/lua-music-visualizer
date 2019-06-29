#ifndef LUA_IMAGE_H
#define LUA_IMAGE_H

#include <lua.h>
#include "thread.h"

enum IMAGE_STATE {
    IMAGE_ERR,
    IMAGE_UNLOADED,
    IMAGE_LOADING,
    IMAGE_LOADED,
    IMAGE_FIXED,
};

typedef struct image_q {
    int table_ref;
    char *filename;
    unsigned int width;
    unsigned int height;
    unsigned int channels;
    unsigned int frames;
    uint8_t *image;
} image_q;


#ifdef __cplusplus
extern "C" {
#endif

int
luaopen_image(lua_State *L);

int
luaclose_image();

void
lua_load_image_cb(void *Lua, intptr_t table_ref, unsigned int frames, uint8_t *image);

void
queue_image_load(intptr_t table_ref,const char* filename, unsigned int width, unsigned int height, unsigned int channels);

void wake_queue(void);

int
luaimage_setup_threads(thread_queue_t *ret);

int
luaimage_stop_threads(void);

#ifdef __cplusplus
}
#endif

#endif

