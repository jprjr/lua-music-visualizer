#include "lua-thread.h"
#include "attr.h"

#if (!defined LUA_VERSION_NUM) || LUA_VERSION_NUM == 501
#define lua_setuservalue(L,i) lua_setfenv((L),(i))
#define lua_getuservalue(L,i) lua_getfenv((L),(i))
#endif

#include <lauxlib.h>


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

static int lmv_thread__gc(lua_State *L) {
    void *q = NULL;
    lmv_thread_t *t = (lmv_thread_t *)lua_touserdata(L,1);

    /* send a quit item to the thread */
    thread_queue_produce(&t->incoming,NULL);
    thread_signal_raise(&t->signal);
    thread_join(t->thread);

    thread_destroy(t->thread);
    thread_signal_term(&t->signal);

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

    return 0;
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

    t->thread = NULL;
    t->in = NULL;
    t->out = NULL;
    t->free_in = free_in;
    t->free_out =free_out;

    luaL_getmetatable(L,"lmv.thread");
    lua_setmetatable(L,-2);

    t->process = process;

    /* create a uservalue table for storing our queues */
    lua_newtable(L);
    t->in  = (void **)lua_newuserdata(L,sizeof(void *) * qsize);
    lua_setfield(L,-2,"in");
    t->out = (void **)lua_newuserdata(L,sizeof(void *) * qsize);
    lua_setfield(L,-2,"out");
    lua_setuservalue(L,-2);

    thread_signal_init(&t->signal);
    thread_queue_init(&t->incoming,qsize,t->in,0);
    thread_queue_init(&t->outgoing,qsize,t->out,0);
    t->thread = thread_create(lmv_thread, t, NULL, THREAD_STACK_SIZE_DEFAULT);
    if(t->thread == NULL) {
        return luaL_error(L,"error spawning thread");
    }

    return 1;
}

void lmv_thread_init(lua_State *L) {
    if(luaL_newmetatable(L,"lmv.thread")) {
      lua_pushcclosure(L,lmv_thread__gc,0);
      lua_setfield(L,-2,"__gc");
      lua_pop(L,1);
    }
}

