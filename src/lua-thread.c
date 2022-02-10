#include "lua-thread.h"
#include "attr.h"

#if (!defined LUA_VERSION_NUM) || LUA_VERSION_NUM == 501
#define lua_setuservalue(L,i) lua_setfenv((L),(i))
#define lua_getuservalue(L,i) lua_getfenv((L),(i))
#endif

#include <lauxlib.h>
#include <stdlib.h>


static int
lmv_thread(void *userdata) {
    void *q = NULL;
    lmv_thread_t *t = (lmv_thread_t *)userdata;
    int r;
    while(1) {
        thread_signal_wait(&t->signal,THREAD_SIGNAL_WAIT_INFINITE);
        while(thread_queue_count(&t->incoming) > 0) {
            q = thread_queue_consume(&t->incoming);
            if(UNLIKELY(q == NULL)) {
                thread_exit(0);
                return 0;
            }
            /* if the process function returns non-zero, exit */
            r = t->process(q,(lmv_produce)thread_queue_produce,&t->outgoing);
            if(r) {
                thread_exit(r);
                return r;
            }
            q = NULL;
        }
    }
    thread_exit(1);
    return 1;
}

void lmv_thread_inject(lmv_thread_t *t, void *val) {
    thread_queue_produce(&t->incoming,val);
    thread_signal_raise(&t->signal);
}

void *lmv_thread_result(lmv_thread_t *t, int *res) {
    void *val = NULL;
    *res = 0;
    if(thread_queue_count(&t->outgoing) > 0) {
        val = thread_queue_consume(&t->outgoing);
        *res = 1;
    }
    return val;
}

lmv_thread_t * lmv_thread_ref(lua_State *L,int idx) {
    return (lmv_thread_t *)lua_touserdata(L, idx);
}

static void lmv_thread_clean(lua_State *L, lmv_thread_t *t) {
    void *q = NULL;

    /* send a quit item to the thread */
    thread_queue_produce(&t->incoming,NULL);
    thread_signal_raise(&t->signal);
    thread_join(t->thread);

    thread_destroy(t->thread);
    thread_signal_term(&t->signal);

#ifndef NDEBUG
    /* override the id_produce and id_consume fields */
    t->incoming.id_produce = thread_current_thread_id();
    t->incoming.id_consume = thread_current_thread_id();
#endif

    /* free any remaining queue items */
    if(t->free_in != NULL) {
        while(thread_queue_count(&t->incoming) > 0) {
            q = thread_queue_consume(&t->incoming);
            if(q != NULL) {
                t->free_in(L,q);
            }
        }
    }
    if(t->free_out != NULL) {
        while(thread_queue_count(&t->outgoing) > 0) {
            q = thread_queue_consume(&t->outgoing);
            if(q != NULL) {
                t->free_out(L,q);
            }
        }
    }
    thread_queue_term(&t->incoming);
    thread_queue_term(&t->outgoing);
    free(t->in);
    free(t->out);
}

void lmv_thread_free(lua_State *L, lmv_thread_t *t) {
    lmv_thread_clean(L,t);
    free(t);
}

static int lmv_thread__gc(lua_State *L) {
    lmv_thread_t *t = (lmv_thread_t *)lua_touserdata(L,1);
    lmv_thread_clean(L,t);

    return 0;
}

static int lmv_thread_new_internal(lua_State *L,lmv_thread_t *t, lmv_process process, lmv_free free_in, lmv_free free_out, lua_Integer qsize, const char *metaname) {

    t->thread = NULL;
    t->in = NULL;
    t->out = NULL;
    t->free_in = free_in;
    t->free_out =free_out;

    luaL_getmetatable(L,metaname);
    lua_setmetatable(L,-2);

    t->process = process;

    t->in  = (void **)malloc(sizeof(void *) * qsize);
    t->out = (void **)malloc(sizeof(void *) * qsize);

    thread_signal_init(&t->signal);
    thread_queue_init(&t->incoming,qsize,t->in,0);
    thread_queue_init(&t->outgoing,qsize,t->out,0);
    t->thread = thread_create(lmv_thread, t, NULL, THREAD_STACK_SIZE_DEFAULT);
    if(t->thread == NULL) {
        return luaL_error(L,"error spawning thread");
    }

    return 1;

}

int lmv_thread_newmalloc(lua_State *L,lmv_process process, lmv_free free_in, lmv_free free_out, lua_Integer qsize) {
    lmv_thread_t *t = NULL;

    if(process == NULL) {
        return luaL_error(L,"invalid process function");
    }

    if(qsize < 1) {
        return luaL_error(L,"invalid queue size");
    }

    t = (lmv_thread_t *)malloc(sizeof(lmv_thread_t));
    if(t == NULL) {
        return luaL_error(L,"out of memory");
    }
    lua_pushlightuserdata(L,t);

    return lmv_thread_new_internal(L, t, process, free_in, free_out, qsize, "lmv.threadmalloc");
}

int lmv_thread_new(lua_State *L,lmv_process process, lmv_free free_in, lmv_free free_out, lua_Integer qsize) {
    lmv_thread_t *t = NULL;

    if(process == NULL) {
        return luaL_error(L,"invalid process function");
    }

    if(qsize < 1) {
        return luaL_error(L,"invalid queue size");
    }

    t = (lmv_thread_t *)lua_newuserdata(L,sizeof(lmv_thread_t));
    if(t == NULL) {
        return luaL_error(L,"out of memory");
    }

    return lmv_thread_new_internal(L, t, process, free_in, free_out, qsize, "lmv.thread");
}

void lmv_thread_init(lua_State *L) {
    if(luaL_newmetatable(L,"lmv.thread")) {
        lua_pushcclosure(L,lmv_thread__gc,0);
        lua_setfield(L,-2,"__gc");
    }
    lua_pop(L,1);
    luaL_newmetatable(L,"lmv.threadmalloc");
    lua_pop(L,1);
}

