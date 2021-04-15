#include <stddef.h>
#include "fmt.h"
#include "int.h"
#include "mpdc.h"
#include "char.h"
#include "str.h"
#include "scan.h"

#define MIN(a,b) (((a)<(b)) ? (a) : (b) )
#define MAX(a,b) (((a)>(b)) ? (a) : (b) )

#define handshook(conn) ( ! (conn->major == 0 && conn->minor == 0 && conn->patch == 0) )

static const char *mpdc__host_default;
static const jpr_uint16 mpdc__port_default;
static const char* const mpdc__command[105];
static const char mpdc__numtab[10];

static int mpdc__ringbuf_putchr(mpdc_ringbuf *rb, jpr_uint8 byte);
static size_t mpdc__ringbuf_putstr(mpdc_ringbuf *rb, const char *src);
static jpr_uint8 mpdc__ringbuf_getchr(mpdc_ringbuf *rb);
static int mpdc__ringbuf_findchr(mpdc_ringbuf *rb, char ch);
static int mpdc__ringbuf_read(mpdc_ringbuf *rb,size_t count);
static int mpdc__ringbuf_write(mpdc_ringbuf *rb, size_t count);
static int mpdc__ringbuf_flushline(mpdc_ringbuf *rb, int *exp);
static int mpdc__ringbuf_readline(mpdc_ringbuf *rb);
static int mpdc__ringbuf_getline(mpdc_ringbuf *rb, char *dest);
static size_t mpdc__ringbuf_getbytes(mpdc_ringbuf *rb, jpr_uint8 *dest, size_t count);
static size_t mpdc__mpdc__scan_uint16(const char *str, jpr_uint16 *dest);
static size_t mpdc__fmt_uint(char *dest, size_t n);
static size_t mpdc__fmt_int(char *dest, int n);

static int mpdc__handshake(mpdc_connection *conn, const char *buf, size_t r);
static int mpdc__op_queue(mpdc_connection *conn, char op);
static jpr_uint8 mpdc__op_last(mpdc_connection *conn);
static int mpdc__line(mpdc_connection *conn, char *line, size_t r);
static int mpdc__process(mpdc_connection *conn, size_t *len);

static int mpdc__read_notify_default(mpdc_connection *conn);
static int mpdc__write_notify_default(mpdc_connection *conn);
static int mpdc__resolve_default(mpdc_connection *, const char *hostname);
static int mpdc__connection_default(mpdc_connection *, const char *hostname, jpr_uint16 port);
static void mpdc__response_begin_default(mpdc_connection *, const char *cmd);
static void mpdc__response_end_default(mpdc_connection *, const char *cmd, int ok, const char *err);
static void mpdc__response_default(mpdc_connection *, const char *cmd, const char *key, const jpr_uint8 *value, size_t length);
static int mpdc__receive_block(mpdc_connection *conn);
static int mpdc__receive_nonblock(mpdc_connection *conn);

static const char *mpdc__host_default = "127.0.0.1";
static const jpr_uint16 mpdc__port_default = 6600;
static const char mpdc__numtab[10] = "0123456789";
static const char* const mpdc__command[105] = {
    "clearerror", "currentsong", "idle", "status", "stats", /* 5 */
    "consume", "crossfade", "mixrampdb", "mixrampdelay", "random", /* 5 */
    "repeat", "setvol", "single", "replay_gain_mode", "replay_gain_staus", /* 5 */
    "volume", "next", "pause", "play", "playid", "previous", /* 6 */
    "seek", "seekid", "seekcur", "stop", "add", "addid", "clear", /* 7 */
    "delete", "deleteid", "move", "moveid", "playlist", "playlistfind", /* 6 */
    "playlistid", "playlistinfo", "playlistsearch", "plchanges", "plchangesposid", /* 5 */
    "prio", "prioid", "rangeid", "shuffle", "swap", "swapid", "addtagid", "cleartagid", /* 8 */
    "listplaylist", "listplaylistinfo", "listplaylists", "load", "playlistadd", /* 5 */
    "playlistclear", "playlistdelete", "playlistmove", "rename", "rm", "save", /* 6 */
    "albumart", "count", "getfingerprint", "find", "findadd", "list", "listall", /* 7 */
    "listallinfo", "listfiles", "lsinfo", "readcomments", "search", "searchadd", /* 6 */
    "searchaddpl", "update", "rescan", "mount", "unmount", "listmounts", "listneighbors", /* 7 */
    "sticker", "close", "kill", "password", "ping", "tagtypes", "partition", "listpartitions", /* 8 */
    "newpartition", "disableoutput", "enableoutput", "outputs", "outputset", "config", "commands", /* 7 */
    "notcommands", "urlhandlers", "decoders", "subscribe", "unsubscribe", "channels", "readmessages", /* 7 */
    "sendmessage", "readpicture", "moveoutput", "delpartition", "binarylimit",
};

#define mpdc__ringbuf_reset(rb) \
    ((rb)->head = (rb)->tail = (rb)->buf)

#define mpdc__ringbuf_init(rb, BUF, SIZE) \
    (rb)->buf = BUF; \
    (rb)->size = SIZE;

#define mpdc__ringbuf_capacity(rb) \
  ((size_t)(((rb)->size) - 1))

#define mpdc__ringbuf_bytes_free(rb) \
    ((rb)->head >= (rb)->tail ? (size_t)(mpdc__ringbuf_capacity(rb) - ((rb)->head - (rb)->tail)) : (size_t)((rb)->tail - (rb)->head - 1))

#define mpdc__ringbuf_bytes_used(rb) \
    ((size_t)(mpdc__ringbuf_capacity(rb) - mpdc__ringbuf_bytes_free(rb)))

#define mpdc_ringbuf_is_full(rb) \
    (mpdc__ringbuf_bytes_free(rb) == 0)

#define mpdc_ringbuf_is_empty(rb) \
    (mpdc__ringbuf_bytes_free(rb) == mpdc__ringbuf_capacity(rb))

#define mpdc__ringbuf_buffer_size(rb) \
  ((rb)->size)

#define mpdc__ringbuf_tail(rb) (rb->tail)
#define mpdc__ringbuf_head(rb) (rb->head)
#define mpdc__ringbuf_end(rb) \
    (rb->buf + mpdc__ringbuf_buffer_size(rb))


static int mpdc__ringbuf_putchr(mpdc_ringbuf *rb, jpr_uint8 byte) {
    if(mpdc_ringbuf_is_full(rb)) return 0;
    *(rb->head) = byte;
    rb->head++;
    if(rb->head == mpdc__ringbuf_end(rb)) rb->head = rb->buf;
    return 1;
}

#define mpdc__ringbuf_endstr(rb) mpdc__ringbuf_putchr(rb,'\n')

static size_t mpdc__str_chr(const char *str, char ch, size_t max) {
    char *s = str_nchr(str,ch,max);
    if(s == NULL) return str_len(str);
    return s - str;
}

static size_t str_nlen_e(const char *src, size_t max) {
    size_t len = 0;
    const char *e = src;
    while(*e) {
        if(*e == '"' || *e == '\\') {
            len++;
        }
        len++;
        e++;
        if(len >= max) break;
    }

    return len;
}

/* does NOT terminate the string */
static size_t mpdc__ringbuf_putstr(mpdc_ringbuf *rb, const char *src) {
    const char *e;
    if(str_nlen_e(src,mpdc__ringbuf_capacity(rb)) > mpdc__ringbuf_bytes_free(rb)) {
        return 0;
    }
    e = src;
    while(*e) {
        if(*e == '"' || *e == '\\') {
            *rb->head++ = '\\';
            if(rb->head == mpdc__ringbuf_end(rb)) rb->head = rb->buf;
        }
        *rb->head++ = *e++;
        if(rb->head == mpdc__ringbuf_end(rb)) rb->head = rb->buf;
    }

    return e - src;
}

static jpr_uint8 mpdc__ringbuf_getchr(mpdc_ringbuf *rb) {
    jpr_uint8 r = *(rb->tail);
    rb->tail++;
    if(rb->tail == mpdc__ringbuf_end(rb)) rb->tail = rb->buf;
    return r;
}

static jpr_uint8 mpdc__op_last(mpdc_connection *conn) {
    jpr_uint8 op =  255;
    if(!(mpdc_ringbuf_is_empty(&conn->op))) {
      if (conn->op.head == conn->op.buf) {
          op = *(conn->op.buf + conn->op.size - 1);
      }
      else{
          op =  *(conn->op.head - 1);
      }
    }
    return op;
}


static int mpdc__ringbuf_findchr(mpdc_ringbuf *rb, char ch) {
    jpr_uint8 *t;
    size_t n;
    int r;

    n = mpdc__ringbuf_bytes_used(rb);
    if( n == 0 ) return -1;

    r = 0;
    t = rb->tail;
    while(n-- > 0) {
        if((char)*t == ch) return r;
        t++;
        r++;
        if(t == mpdc__ringbuf_end(rb)) t = rb->buf;
    }
    return -1;
}

static int mpdc__ringbuf_read(mpdc_ringbuf *rb,size_t count) {
    const jpr_uint8 *bufend = mpdc__ringbuf_end(rb);
    size_t nfree = mpdc__ringbuf_bytes_free(rb);
    int n = 0;
    int r = 0;
    count = MIN(count,nfree);

    while(count > 0) {
        nfree = MIN((size_t)(bufend - rb->head), count);

        n = rb->read(rb->read_ctx,rb->head,nfree);
        r += n;
        if(n <= 0) break;

        rb->head += n;
        if(rb->head == bufend) rb->head = rb->buf;
        count -= n;
    }

    return r;
}

#define mpdc_ringbuf_fill(rb) mpdc__ringbuf_read(rb,mpdc__ringbuf_bytes_free(rb))

static int mpdc__ringbuf_write(mpdc_ringbuf *rb, size_t count) {
    const jpr_uint8 *bufend;
    int m;
    int n;
    size_t bytes_used;
    size_t d;

    bytes_used = mpdc__ringbuf_bytes_used(rb);
    if(count > bytes_used) return 0;

    bufend = mpdc__ringbuf_end(rb);
    m = 0;
    n = 0;

    do {
        d = MIN((size_t)(bufend - rb->tail),count);
        n = rb->write(rb->write_ctx,rb->tail,d);
        if(n > 0) {
            rb->tail += n;
            if(rb->tail == bufend) rb->tail = rb->buf;
            count -= n;
            m += n;
        }
    } while(n > 0 && count > 0);

    return m;
}

static int mpdc__ringbuf_flushline(mpdc_ringbuf *rb, int *exp) {
    int n;

    *exp = -1;
    n = mpdc__ringbuf_findchr(rb,'\n');
    if(n != -1) {
        *exp = n+1;
        n = mpdc__ringbuf_write(rb,n+1);
    }
    return n;
}

static int mpdc__ringbuf_readline(mpdc_ringbuf *rb) {
    char c;
    int b = 0;
    int r = 0;
    do {
        r = rb->read(rb->read_ctx,(jpr_uint8 *)&c,1);
        if(r <= 0) return r;
        *rb->head++ = c;
        b++;
        if(rb->head == mpdc__ringbuf_end(rb)) rb->head = rb->buf;
    } while(c != '\n');
    return b;
}

static int mpdc__ringbuf_getline(mpdc_ringbuf *rb, char *dest) {
    int r = 0;
    int n = 0;
    char *e = dest;

    r = mpdc__ringbuf_findchr(rb,'\n');
    if(r == -1) { return r; }

    while(n < r) {
        *e++ = *rb->tail++;
        if(rb->tail == mpdc__ringbuf_end(rb)) rb->tail = rb->buf;
        n++;
    }
    *e = 0;
    rb->tail++;
    if(rb->tail == mpdc__ringbuf_end(rb)) rb->tail = rb->buf;
    return n;
}

static size_t mpdc__ringbuf_getbytes(mpdc_ringbuf *rb, jpr_uint8 *dest, size_t count) {
    size_t n = MIN(count,mpdc__ringbuf_bytes_used(rb));
    count = n;

    while(count--) {
        *dest++ = *rb->tail++;
        if(rb->tail == mpdc__ringbuf_end(rb)) rb->tail = rb->buf;
    }
    return n;
}

static size_t mpdc__mpdc__scan_uint16(const char *str, jpr_uint16 *dest) {
	return scan_uint16(str,dest);
}

static size_t mpdc__fmt_uint(char *dest, size_t n) {
    size_t d = n;
    size_t len = 1;
    size_t r = 0;
    while( (d /= 10) > 0) {
        len++;
    }
    r = len;
    while(len--) {
        dest[len] = mpdc__numtab[ n % 10 ];
        n /= 10;
    }
    return r;
}

static size_t mpdc__fmt_int(char *dest, int n) {
    size_t len = 0;
    size_t p;
    if(n < 0) {
        len++;
        *dest++ = '-';
        p = n * -1;
    } else {
        p = n;
    }
    len += mpdc__fmt_uint(dest,p);

    return len;
}

static int mpdc__handshake(mpdc_connection *conn, const char *buf, size_t r) {
    const char *b = buf;
    size_t i = 0;

    if(!str_istarts(buf,"ok mpd")) return -1;

    b+= 7;
    mpdc__mpdc__scan_uint16(b,&conn->major);
    i = mpdc__str_chr(b,'.',r);

    if(b[i]) {
        mpdc__mpdc__scan_uint16(b+i+1,&conn->minor);
        i += 1 + mpdc__str_chr(b+i+1,'.',r-i-1);
        if(b[i]) {
            mpdc__mpdc__scan_uint16(b+i+1,&conn->patch);
        }
    }
    if(!handshook(conn)) return -1;
    return 1;
}


static int mpdc__line(mpdc_connection *conn, char *line, size_t r) {
    if(str_istarts(line,"ack")) return -1;
    if(str_istarts(line,"ok")) {
        if(!handshook(conn)) return mpdc__handshake(conn,line,r);
        return 1;
    }
    if(!handshook(conn)) return -1;
    return 0;
}

static int mpdc__write_notify_default(mpdc_connection *conn) {
    (void)conn;
    return 1;
}

static int mpdc__process(mpdc_connection *conn, size_t *len) {
    int ok = 0;
    size_t t = 0;
    char *s = NULL;
    int ilen = 0;

    *len = 0;
    do {
        if(conn->_mode == 0 || (conn->_mode == 1 && conn->_bytes == 0)) {
            ilen = mpdc__ringbuf_getline(&conn->in,(char *)conn->scratch);
            if(ilen >= 0) *len = (unsigned int)ilen;
            if(ilen <= 0) {
                if(conn->_mode == 1 && conn->_bytes == 0 && *len == 0) {
                    /* end of binary response */
                    conn->_mode = 0;
                    continue;
                }
                break;
            }
        } else {
            ilen = mpdc__ringbuf_getbytes(&conn->in,conn->scratch,conn->_bytes);
            if(ilen >= 0) *len = (unsigned int)ilen;
            if(ilen <= 0) break;
            conn->_bytes -= *len;
        }

        if(conn->_mode == 0) {
            ok = mpdc__line(conn,(char *)conn->scratch,(size_t)*len);
            if(ok == 0) {
                t = mpdc__str_chr((const char *)conn->scratch,':',*len);
                conn->scratch[t] = 0;
                s = (char *)conn->scratch;
                while(*s) {
                    *s = char_lower(*s);
                    s++;
                }
                if(str_istarts((const char *)conn->scratch,"binary")) {
                    conn->_mode = 1;
                    scan_sizet((const char *)conn->scratch + t + 2, &conn->_bytes);
                }
                else {
                    conn->cb_level++;
                    conn->response(conn,mpdc__command[conn->state],(const char *)conn->scratch,(const jpr_uint8 *)conn->scratch + t + 2,((size_t)*len) - t - 2);
                    conn->cb_level--;
                }
            }
            else {
                if(conn->state != 255) {
                    conn->cb_level++;
                    conn->response_end(conn,mpdc__command[conn->state],ok,(const char *)conn->scratch);
                    conn->cb_level--;
                }
                conn->state = 255;
                break;
            }
        } else {
            conn->cb_level++;
            conn->response(conn,mpdc__command[conn->state],"binary",conn->scratch,*len);
            conn->cb_level--;
        }

    } while(ilen > -1);

    if(ilen == -1) {
        /* we couldn't find a newline, so just reset the input buffer */
        mpdc__ringbuf_reset(&conn->in);
    }

    if(ok == 1 && !mpdc_ringbuf_is_empty(&conn->op)) {
        conn->write_notify(conn);
    }
    return ok;
}

static int mpdc__read_notify_default(mpdc_connection *conn) {
    (void)conn;
    return 1;
}

static int mpdc__receive_block(mpdc_connection *conn) {
    int ok = 0;
    size_t len = 0;
    int r = 0;
    while( ok == 0 ) {
        if(conn->_mode == 0 || (conn->_mode == 1 && conn->_bytes == 0)) {
            r = mpdc__ringbuf_readline(&conn->in);
        }
        else {
            r = mpdc__ringbuf_read(&conn->in,conn->_bytes);
        }
        if(r <= 0) { ok = -1; break; }
        ok = mpdc__process(conn,&len);
    }
    return ok;
}

static int mpdc__receive_nonblock(mpdc_connection *conn) {
    int ok = 0;
    int r = 0;
    size_t len = 0;
    r = mpdc_ringbuf_fill(&conn->in);
    if(r == -1) return -1;
    while(!mpdc_ringbuf_is_empty(&conn->in)) {
        ok = mpdc__process(conn,&len);
        if(ok == -1) return -1;
        if(len == 0) break;
    }
    return ok;
}

static int mpdc__resolve_default(mpdc_connection *conn, const char *hostname) {
    (void)conn;
    (void)hostname;
    return 1;
}

static int mpdc__connection_default(mpdc_connection *conn, const char *hostname, jpr_uint16 port) {
    (void)conn;
    (void)hostname;
    (void)port;
    return -1;
}

static void mpdc__response_begin_default(mpdc_connection *conn, const char *cmd) {
    (void)conn;
    (void)cmd;
    return;
}

static void mpdc__response_end_default(mpdc_connection *conn, const char *cmd, int ok, const char *err) {
    (void)conn;
    (void)cmd;
    (void)ok;
    (void)err;
    return;
}

static void mpdc__response_default(mpdc_connection *conn, const char *cmd, const char *key, const jpr_uint8 *value, size_t length) {
    (void)conn;
    (void)cmd;
    (void)key;
    (void)value;
    (void)length;
    return;
}

static int mpdc__op_queue(mpdc_connection *conn, char op) {
    if(!mpdc__ringbuf_putchr(&conn->op, op)) return -1;
    if(!handshook(conn)) {
        return conn->read_notify(conn);
    }
    return conn->write_notify(conn);
}

STATIC
void mpdc_reset(mpdc_connection *conn) {
    mpdc__ringbuf_reset(&conn->op);
    mpdc__ringbuf_reset(&conn->out);
    mpdc__ringbuf_reset(&conn->in);
    conn->major = 0;
    conn->minor = 0;
    conn->patch = 0;
    conn->cb_level = 0;
    conn->state = 255;
    conn->_mode = 0;
    conn->_bytes = 0;
}

STATIC
int mpdc_init(mpdc_connection *conn) {
    mpdc__ringbuf_init((&conn->op),conn->op_buf,MPDC_OP_QUEUE_SIZE);
    mpdc__ringbuf_init((&conn->out),conn->out_buf,MPDC_BUFFER_SIZE);
    mpdc__ringbuf_init((&conn->in),conn->in_buf,MPDC_BUFFER_SIZE);

    conn->host = NULL;
    conn->password = NULL;
    conn->port = 0;

    mpdc_reset(conn);

    if(conn->resolve == NULL) conn->resolve = mpdc__resolve_default;
    if(conn->connect == NULL) conn->connect = mpdc__connection_default;
    if(conn->response_begin == NULL) conn->response_begin = mpdc__response_begin_default;
    if(conn->response_end == NULL) conn->response_end = mpdc__response_end_default;
    if(conn->response == NULL) conn->response = mpdc__response_default;
    if(conn->write_notify == NULL) conn->write_notify = mpdc__write_notify_default;
    if(conn->read_notify == NULL)  conn->read_notify = mpdc__read_notify_default;
    if(conn->read_notify == mpdc__read_notify_default) {
        conn->_receive = mpdc__receive_block;
    } else {
        conn->_receive = mpdc__receive_nonblock;
    }

    conn->in.read_ctx = conn->ctx;
    conn->out.write_ctx = conn->ctx;
    conn->in.read = conn->read;
    conn->out.write = conn->write;

    return 1;
}

STATIC
int mpdc_setup(mpdc_connection *conn, char *mpdc_host, char *mpdc_port_str, jpr_uint16 mpdc_port) {
    size_t len;
    size_t at;
    if(mpdc_host == NULL) {
        conn->host = (char *)mpdc__host_default;
    }
    else {
        conn->host = mpdc_host;
        len = str_nlen(mpdc_host,255);
        at  = mpdc__str_chr(mpdc_host,'@',len);
        if(at < len) {
            conn->password = mpdc_host;
            conn->password[at] = 0;
            conn->host = mpdc_host + at + 1;
        }
    }

    if(mpdc_port == 0) {
        if(mpdc_port_str == NULL) {
            mpdc_port = mpdc__port_default;
        }
        else {
            if(mpdc__mpdc__scan_uint16(mpdc_port_str,&mpdc_port) == 0) {
                mpdc_port = mpdc__port_default;
            }
            else if(mpdc_port == 0) mpdc_port = mpdc__port_default;
        }
    }

    conn->port = mpdc_port;

    return conn->resolve(conn,conn->host);
}

STATIC
int mpdc_connect(mpdc_connection *conn) {
    return conn->connect(conn,conn->host,conn->port);
}

STATIC
int mpdc_send(mpdc_connection *conn) {
    int exp;
    int r;

    if(mpdc_ringbuf_is_empty(&conn->out)) return 0;
    if(!handshook(conn)) {
        return conn->read_notify(conn) > 0;
    }

    r = mpdc__ringbuf_flushline(&conn->out,&exp);
    if(r == -1) return -1;
    if(r == exp) {
        if(conn->state == MPDC_COMMAND_IDLE) {
            /* we just sent a "noidle" message, just do a read */
            return conn->read_notify(conn) > 0;
        }
        conn->state = mpdc__ringbuf_getchr(&conn->op);
        conn->response_begin(conn,mpdc__command[conn->state]);
        if(conn->state == MPDC_COMMAND_IDLE && !mpdc_ringbuf_is_empty(&conn->op)) {
            /* we've queued messages during a callback while in an idle state, we
             * have a "noidle" message to send, so send it */
            return conn->write_notify(conn) > 0;
        }
        return conn->read_notify(conn) > 0;
    }

    return 0;
}

STATIC
int mpdc_receive(mpdc_connection *conn) {
    return conn->_receive(conn);
}

STATIC
int mpdc_password(mpdc_connection *conn, const char *password) {
    if(password == NULL) password = conn->password;
    if(password == NULL) return -1;
    return mpdc__put(conn,MPDC_COMMAND_PASSWORD,"r",password);
}

static
size_t mpdc__idlesize(mpdc_connection *conn, jpr_uint16 events) {
    size_t len = 0;
    jpr_uint8 cur_op = 255;

    cur_op = mpdc__op_last(conn);

    if( cur_op == MPDC_COMMAND_IDLE || (cur_op == 255 && conn->state == MPDC_COMMAND_IDLE && conn->cb_level == 0)) {
        len += 7;
    }

    len += 4;

    if(events & MPDC_EVENT_DATABASE) {
        len += 9;
    }
    if(events & MPDC_EVENT_UPDATE) {
        len += 7;
    }
    if(events & MPDC_EVENT_STORED_PLAYLIST) {
        len += 16;
    }
    if(events & MPDC_EVENT_PLAYLIST) {
        len += 9;
    }
    if(events & MPDC_EVENT_PLAYER) {
        len += 7;
    }
    if(events & MPDC_EVENT_MIXER) {
        len += 6;
    }
    if(events & MPDC_EVENT_OPTIONS) {
        len += 8;
    }
    if(events & MPDC_EVENT_PARTITION) {
        len += 10;
    }
    if(events & MPDC_EVENT_STICKER) {
        len += 8;
    }
    if(events & MPDC_EVENT_SUBSCRIPTION) {
        len += 13;
    }
    if(events & MPDC_EVENT_MESSAGE) {
        len += 8;
    }
    len++;

    return len;
}

STATIC
int mpdc_idle(mpdc_connection *conn, jpr_uint16 events) {
    size_t idlesize;
    jpr_uint8 cur_op = 255;

    idlesize = mpdc__idlesize(conn,events);
    if(idlesize > mpdc__ringbuf_bytes_free(&conn->out)) {
        return -1;
    }

    cur_op = mpdc__op_last(conn);

    if( cur_op == MPDC_COMMAND_IDLE || (cur_op == 255 && conn->state == MPDC_COMMAND_IDLE && conn->cb_level == 0)) {
        if(!mpdc__ringbuf_putstr(&conn->out,"noidle")) return -1;
        if(!mpdc__ringbuf_endstr(&conn->out)) return -1;
    }

    if(!mpdc__ringbuf_putstr(&conn->out,"idle")) return -1;

    if(events & MPDC_EVENT_DATABASE) {
        if(!mpdc__ringbuf_putstr(&conn->out," database")) return -1;
    }
    if(events & MPDC_EVENT_UPDATE) {
        if(!mpdc__ringbuf_putstr(&conn->out," update")) return -1;
    }
    if(events & MPDC_EVENT_STORED_PLAYLIST) {
        if(!mpdc__ringbuf_putstr(&conn->out," stored_playlist")) return -1;
    }
    if(events & MPDC_EVENT_PLAYLIST) {
        if(!mpdc__ringbuf_putstr(&conn->out," playlist")) return -1;
    }
    if(events & MPDC_EVENT_PLAYER) {
        if(!mpdc__ringbuf_putstr(&conn->out," player")) return -1;
    }
    if(events & MPDC_EVENT_MIXER) {
        if(!mpdc__ringbuf_putstr(&conn->out," mixer")) return -1;
    }
    if(events & MPDC_EVENT_OPTIONS) {
        if(!mpdc__ringbuf_putstr(&conn->out," options")) return -1;
    }
    if(events & MPDC_EVENT_PARTITION) {
        if(!mpdc__ringbuf_putstr(&conn->out," partition")) return -1;
    }
    if(events & MPDC_EVENT_STICKER) {
        if(!mpdc__ringbuf_putstr(&conn->out," sticker")) return -1;
    }
    if(events & MPDC_EVENT_SUBSCRIPTION) {
        if(!mpdc__ringbuf_putstr(&conn->out," subscription")) return -1;
    }
    if(events & MPDC_EVENT_MESSAGE) {
        if(!mpdc__ringbuf_putstr(&conn->out," message")) return -1;
    }
    if(!mpdc__ringbuf_endstr(&conn->out)) return -1;
    return mpdc__op_queue(conn,MPDC_COMMAND_IDLE);
}

STATIC
void mpdc_disconnect(mpdc_connection *conn) {
    conn->disconnect(conn);
    mpdc_reset(conn);
}

STATIC
void mpdc_block(mpdc_connection *conn, int status) {
    if(status == 0) {
        conn->_receive = mpdc__receive_nonblock;
    }
    else {
        conn->_receive = mpdc__receive_block;
    }
}

/* fmt is a character string describing arguments */
/* if a character is capitalized or non-alpha, NO
 * space is placed into the buffer.
 * If a lower alpha, a space is placed.
 *
 * example: fmt = "sss" => "string string string"
 *          fmt = "u:U" => "integer:integer" (no space)"
 */

#if __STDC_VERSION__ >= 199901L
STATIC
size_t mpdc__putsize(mpdc_connection *conn, jpr_uint8 cmd, const char *fmt, va_list va_og) {
    size_t len = 0;
    const char *f = fmt;
    va_list va;
    va_copy(va,va_og);
    char *s;
    size_t u;
    int d;
    jpr_uint8 cur_op = 255;

    cur_op = mpdc__op_last(conn);

    if( cur_op == MPDC_COMMAND_IDLE || (cur_op == 255 && conn->state == MPDC_COMMAND_IDLE && conn->cb_level == 0)) {
        len += 7;
    }

    len += str_nlen(mpdc__command[cmd],MPDC_BUFFER_SIZE);
    while(*f) {
        char t = *f++;
        if(char_islower(t)) {
            len += 1;
        }
        else {
            t = char_lower(t);
        }
        switch(t) {
            case ':': {
                len += 1;
                break;
            }
            case 'r': {
                s = va_arg(va,char *);
                len += str_nlen(s,MPDC_BUFFER_SIZE);
                break;
            }
            case 's': {
                s = va_arg(va,char *);
                len += str_nlen_e(s,MPDC_BUFFER_SIZE);
                len += 2;
                break;
            }
            case 'u': {
                u = va_arg(va, size_t);
                conn->scratch[mpdc__fmt_uint((char *)conn->scratch,u)] = 0;
                len += str_nlen((const char *)conn->scratch,MPDC_BUFFER_SIZE);
                conn->scratch[0] = 0;
                break;
            }
            case 'd': {
                d = va_arg(va, int);
                conn->scratch[mpdc__fmt_int((char *)conn->scratch,d)] = 0;
                len += str_nlen((const char *)conn->scratch,MPDC_BUFFER_SIZE);
                conn->scratch[0] = 0;
                break;
            }

        }
    }
    len++;
    return len;
}
#endif

STATIC
int mpdc__put(mpdc_connection *conn, jpr_uint8 cmd, const char *fmt, ...) {
    char *s;
    size_t u;
    int d;
    const char *f;
    va_list va;
    jpr_uint8 cur_op;

    f = fmt;
    va_start(va,fmt);
    cur_op = 255;

#if __STDC_VERSION__ > 199901L
    size_t putsize = mpdc__putsize(conn,cmd,fmt,va);

    if(putsize > mpdc__ringbuf_bytes_free(&conn->out)) {
        va_end(va);
        return -1;
    }
#endif

    cur_op = mpdc__op_last(conn);

    if( cur_op == MPDC_COMMAND_IDLE || (cur_op == 255 && conn->state == MPDC_COMMAND_IDLE && conn->cb_level == 0)) {
        if(!mpdc__ringbuf_putstr(&conn->out,"noidle")) {
            va_end(va);
            return -1;
        }
        if(!mpdc__ringbuf_endstr(&conn->out)) {
            va_end(va);
            return -1;
        }
    }


    if(!mpdc__ringbuf_putstr(&conn->out,mpdc__command[cmd])) { va_end(va); return -1; }
    while(*f) {
        char t = *f++;
        if(char_islower(t)) {
            if(!mpdc__ringbuf_putchr(&conn->out,' ')) { va_end(va); return -1; }
        }
        else {
            t = char_lower(t);
        }
        switch(t) {
            case ':': {
                if(!mpdc__ringbuf_putchr(&conn->out,':')) { va_end(va); return -1; }
                break;
            }
            case 'r': {
                s = va_arg(va,char *);
                if(!mpdc__ringbuf_putstr(&conn->out,s)) { va_end(va); return -1; }
                break;
            }
            case 's': {
                s = va_arg(va,char *);
                if(!mpdc__ringbuf_putchr(&conn->out,'"')) { va_end(va); return -1; }
                if(!mpdc__ringbuf_putstr(&conn->out,s)) { va_end(va); return -1; }
                if(!mpdc__ringbuf_putchr(&conn->out,'"')) { va_end(va); return -1; }
                break;
            }
            case 'u': {
                u = va_arg(va, size_t);
                conn->scratch[mpdc__fmt_uint((char *)conn->scratch,u)] = 0;
                if(!mpdc__ringbuf_putstr(&conn->out,(const char *)conn->scratch)) { va_end(va); return -1; }
                break;
            }
            case 'd': {
                d = va_arg(va, int);
                conn->scratch[mpdc__fmt_int((char *)conn->scratch,d)] = 0;
                if(!mpdc__ringbuf_putstr(&conn->out,(const char *)conn->scratch)) { va_end(va); return -1; }
                break;
            }

        }
    }
    if(!mpdc__ringbuf_endstr(&conn->out)) { va_end(va); return -1; }
    va_end(va);
    return mpdc__op_queue(conn,cmd);
}

