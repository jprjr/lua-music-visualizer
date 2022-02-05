#ifndef LMV_THREAD_H
#define LMV_THREAD_H

/* creates an object for Lua to background tasks into C,
 * that makes sure threads are cleaned up on exit. It's
 * a bit awkward - you can't really use it from within
 * Lua, just in C modules.
 *
 * call lvm_thread_new, this places a new userdata on top of the stack (or
 * returns NULL if there's an error), returns 1 on success, throws an error otherwise.
 * lvm_thread_new requires 3 callbacks and 1 integer.
 *   1. a callback to process items placed into the queue, this runs in its own thread
 *   2. a callback for freeing any items in the "incoming" queue, when garbage collection occurs
 *   3. a callback for freeing any items in the "outgoing" queue, when garbage collection occurs
 *   4. an integer specifying the queue size, the size is the same for incoming and outgoing.
 *
 * The "process" callback can NOT interact with Lua - it's running in a different thread.
 * The "process" callback will receive 3 parameters:
 *   - an item from the incoming queue
 *   - a callback to submit the item to the outgoing queue
 *   - the address of the outgoing queue
 * If the "process" callback returns a non-zero value, the running thread exits.
 *
 * The "free" callbacks will run in the main thread, where it's safe to use Lua. They should
 * not place any values on the Lua stack, it should be reset if that happens.
 *
 * Once the thread object is created, you use lmv_thread_inject to place a new item into the
 * incoming queue. You can NOT place a NULL pointer into the queue, this is used to end the
 * thread.
 *
 * The running thread will remove the item from the queue and call your "process" callback.
 *
 * You can then use lmv_thread_result to get a value from the queue, if available - it will
 * return NULL if it is empty. It also writes 1 to the out-variable if there was a result,
 * useful in case your process callback submits NULLs for any reason.
 *
 */

#include <lua.h>
#include "int.h"
#define THREAD_U64 jpr_uint64
#include "thread.h"

typedef void (*lmv_produce)(void *queue, void *value);
typedef int  (*lmv_process)(void *userdata, lmv_produce, void *queue);

/* used to free any incoming/outgoing queue items in garbage collection */
typedef void (*lmv_free)(lua_State *L, void *value);

struct lmv_thread_s {
    thread_ptr_t      thread;
    thread_signal_t   signal;
    thread_queue_t  incoming;
    thread_queue_t  outgoing;
    lmv_process      process;
    lmv_free         free_in;
    lmv_free        free_out;
    void                **in;
    void               **out;
};

typedef struct lmv_thread_s lmv_thread_t;

#ifdef __cplusplus
extern "C" {
#endif

/* this registers the thread metatable, can be called multiple times */
void lmv_thread_init(lua_State *L);

int lmv_thread_new(lua_State *L, lmv_process process, lmv_free free_in, lmv_free free_out, lua_Integer qsize);

void *lmv_thread_result(lmv_thread_t *, int *res);
void lmv_thread_inject(lmv_thread_t *, void *);

#ifdef __cplusplus
}
#endif

#endif
