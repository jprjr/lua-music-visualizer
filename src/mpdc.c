#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include "mpdc.h"

#define MIN(a,b) (((a)<(b)) ? (a) : (b) )
#define MAX(a,b) (((a)>(b)) ? (a) : (b) )

#ifndef MPDC_NO_STDLIB
#define MPDC_NO_STDLIB 0
#endif

#define handshook(conn) ( ! (conn->major == 0 && conn->minor == 0 && conn->patch == 0) )

static const char *mpdc__host_default;
static const uint16_t mpdc__port_default;
static const char* const mpdc__command[101];
static const char mpdc__numtab[10];

#if MPDC_NO_STDLIB
static char mpdc__to_lower(char c);
static char mpdc__is_lower(char c);
static int mpdc__case_starts(const char *s, const char *b);
static unsigned int mpdc__str_len(const char *str, unsigned int max);
#else
#include <ctype.h>
#include <string.h>
#include <strings.h>
#define mpdc__to_lower(c) tolower(c)
#define mpdc__is_lower(c) islower(c)
#define mpdc__case_starts(s,b) (strncasecmp(s,b,strlen(b)) == 0)
#define mpdc__str_len(s,m) strnlen(s,m)
#endif

/* may depend on strchr if stdlib is allowed */
static inline unsigned int mpdc__str_chr(const char *str, char ch, unsigned int max);

static inline int mpdc__ringbuf_putchr(mpdc_ringbuf *rb, uint8_t byte);
static inline int mpdc__ringbuf_putstr(mpdc_ringbuf *rb, const char *src);
static inline uint8_t mpdc__ringbuf_getchr(mpdc_ringbuf *rb);
static inline int mpdc__ringbuf_findchr(mpdc_ringbuf *rb, char ch);
static inline int mpdc__ringbuf_read(mpdc_ringbuf *rb,unsigned int count);
static inline int mpdc__ringbuf_write(mpdc_ringbuf *rb, unsigned int count);
static inline int mpdc__ringbuf_flushline(mpdc_ringbuf *rb, int *exp);
static inline int mpdc__ringbuf_readline(mpdc_ringbuf *rb);
static inline int mpdc__ringbuf_getline(mpdc_ringbuf *rb, char *dest);
static inline int mpdc__ringbuf_getbytes(mpdc_ringbuf *rb, uint8_t *dest, unsigned int count);
static inline unsigned int mpdc__scan_uint(const char *str, unsigned int *dest);
static inline unsigned int mpdc__mpdc__scan_uint16(const char *str, uint16_t *dest);
static inline unsigned int mpdc__fmt_uint(char *dest, unsigned int n);
static inline unsigned int mpdc__fmt_int(char *dest, int n);

static int mpdc__handshake(mpdc_connection *conn, const char *buf, unsigned int r);
static int mpdc__op_queue(mpdc_connection *conn, char op);
static uint8_t mpdc__op_last(mpdc_connection *conn);
static int mpdc__line(mpdc_connection *conn, char *line, unsigned int r);
static int mpdc__process(mpdc_connection *conn, int *len);

static int mpdc__read_notify_default(mpdc_connection *conn);
static int mpdc__write_notify_default(mpdc_connection *conn);
static int mpdc__resolve_default(mpdc_connection *, char *hostname);
static int mpdc__connection_default(mpdc_connection *, char *hostname, uint16_t port);
static void mpdc__response_begin_default(mpdc_connection *, const char *cmd);
static void mpdc__response_end_default(mpdc_connection *, const char *cmd, int ok, const char *err);
static void mpdc__response_default(mpdc_connection *, const char *cmd, const char *key, const uint8_t *value, unsigned int length);
static int mpdc__receive_block(mpdc_connection *conn);
static int mpdc__receive_nonblock(mpdc_connection *conn);

static const char *mpdc__host_default = "127.0.0.1";
static const uint16_t mpdc__port_default = 6600;
static const char mpdc__numtab[10] = "0123456789";
static const char* const mpdc__command[101] = {
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
    "sendmessage"
};


#if MPDC_NO_STDLIB
static inline char mpdc__to_lower(char c) {
    if(c >= 65 && c <= 90) return c + 32;
    return c;
}

static char mpdc__is_lower(char c) {
    if(c >= 97 && c <= 122) return 1;
    return 0;
}

static int mpdc__case_starts(const char *s, const char *b) {
    while(*s && *b) {
        if(mpdc__to_lower(*s++) != mpdc__to_lower(*b++)) return 0;
    }
    /* if we got to the end of b (the shorter string) then it's a match */
    return !*b;
}
static unsigned int mpdc__str_len(const char *str, unsigned int max) {
    const char *s = str;
    unsigned int m = 0;
    while(*s != '\0' && m++ < max) {
        s++;
    }
    return s - str;
}
#endif

static unsigned int mpdc__str_chr(const char *str, char ch, unsigned int max) {
#if MPDC_NO_STDLIB
    const char *s = str;
    unsigned int m = 0;
    while(*s != '\0' && *s != ch && m++ < max) {
        s++;
    }
    return s - str;
#else
    (void)max;
    char *s = strchr(str,ch);
    if(s == NULL) return strlen(str);
    return s - str;
#endif
}

#define mpdc__ringbuf_reset(rb) \
    ((rb)->head = (rb)->tail = (rb)->buf)

#define mpdc__ringbuf_init(rb, BUF, SIZE) \
    (rb)->buf = BUF; \
    (rb)->size = SIZE;

#define mpdc__ringbuf_capacity(rb) \
  ((unsigned int)(((rb)->size) - 1))

#define mpdc__ringbuf_bytes_free(rb) \
    ((rb)->head >= (rb)->tail ? (unsigned int)(mpdc__ringbuf_capacity(rb) - ((rb)->head - (rb)->tail)) : (unsigned int)((rb)->tail - (rb)->head - 1))

#define mpdc__ringbuf_bytes_used(rb) \
    ((unsigned int)(mpdc__ringbuf_capacity(rb) - mpdc__ringbuf_bytes_free(rb)))

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


static inline int mpdc__ringbuf_putchr(mpdc_ringbuf *rb, uint8_t byte) {
    if(mpdc_ringbuf_is_full(rb)) return 0;
    *(rb->head) = byte;
    rb->head++;
    if(rb->head == mpdc__ringbuf_end(rb)) rb->head = rb->buf;
    return 1;
}

#define mpdc__ringbuf_endstr(rb) mpdc__ringbuf_putchr(rb,'\n')

/* does NOT terminate the string */
static int mpdc__ringbuf_putstr(mpdc_ringbuf *rb, const char *src) {
    if(mpdc__str_len(src,mpdc__ringbuf_capacity(rb)) > mpdc__ringbuf_bytes_free(rb)) {
        return 0;
    }
    const char *e = src;
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

static uint8_t mpdc__ringbuf_getchr(mpdc_ringbuf *rb) {
    uint8_t r = *(rb->tail);
    rb->tail++;
    if(rb->tail == mpdc__ringbuf_end(rb)) rb->tail = rb->buf;
    return r;
}

static uint8_t mpdc__op_last(mpdc_connection *conn) {
    if (conn->op.head == conn->op.buf) {
        return *(conn->op.buf + conn->op.size - 1);
    }
    return *(conn->op.head - 1);
}


static int mpdc__ringbuf_findchr(mpdc_ringbuf *rb, char ch) {
    unsigned int n = mpdc__ringbuf_bytes_used(rb);
    int r = 0;
    if( n == 0 ) return -1;
    uint8_t *t = rb->tail;
    while(n-- > 0) {
        if((char)*t == ch) return r;
        t++;
        r++;
        if(t == mpdc__ringbuf_end(rb)) t = rb->buf;
    }
    return -1;
}

static int mpdc__ringbuf_read(mpdc_ringbuf *rb,unsigned int count) {
    const uint8_t *bufend = mpdc__ringbuf_end(rb);
    unsigned int nfree = mpdc__ringbuf_bytes_free(rb);
    int n = 0;
    int r = 0;
    count = MIN(count,nfree);

    while(count > 0) {
        nfree = MIN((unsigned int)(bufend - rb->head), count);

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

static int mpdc__ringbuf_write(mpdc_ringbuf *rb, unsigned int count) {
    unsigned int bytes_used = mpdc__ringbuf_bytes_used(rb);
    if(count > bytes_used) return 0;

    const uint8_t *bufend = mpdc__ringbuf_end(rb);
    int m = 0;
    int n = 0;

    do {
        unsigned int d = MIN((unsigned int)(bufend - rb->tail),count);
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
    *exp = -1;
    int n = mpdc__ringbuf_findchr(rb,'\n');
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
        r = rb->read(rb->read_ctx,(uint8_t *)&c,1);
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

static int mpdc__ringbuf_getbytes(mpdc_ringbuf *rb, uint8_t *dest, unsigned int count) {
    unsigned int n = MIN(count,mpdc__ringbuf_bytes_used(rb));
    count = n;

    while(count--) {
        *dest++ = *rb->tail++;
        if(rb->tail == mpdc__ringbuf_end(rb)) rb->tail = rb->buf;
    }
    return n;
}

static unsigned int mpdc__scan_uint(const char *str, unsigned int *dest) {
    *dest = 0;
    unsigned int n = 0;
    int t = 0;
    while(1) {
        t = str[n] - 48;
        if(t < 0 || t > 9) break;
        *dest *= 10;
        *dest += t;
        n++;
    }
    return n;
}

static unsigned int mpdc__mpdc__scan_uint16(const char *str, uint16_t *dest) {
    unsigned int d = 0;
    unsigned int r = mpdc__scan_uint(str,&d);
    *dest = 0;
    if(r == 0) return 0;
    if(d > 65535) return 0;
    *dest = (uint16_t)d;
    return r;
}

static unsigned int mpdc__fmt_uint(char *dest, unsigned int n) {
    unsigned int d = n;
    unsigned int len = 1;
    unsigned int r = 0;
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

static unsigned int mpdc__fmt_int(char *dest, int n) {
    unsigned int len = 0;
    unsigned int p;
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

static int mpdc__handshake(mpdc_connection *conn, const char *buf, unsigned int r) {
    const char *b = buf;
    unsigned int i = 0;

    if(!mpdc__case_starts(buf,"ok mpd")) return -1;

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


static int mpdc__line(mpdc_connection *conn, char *line, unsigned int r) {
    if(mpdc__case_starts(line,"ack")) return -1;
    if(mpdc__case_starts(line,"ok")) {
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

static int mpdc__process(mpdc_connection *conn, int *len) {
    int ok = 0;
    *len = 0;
    int t = 0;
    char *s = NULL;
    do {
        if(conn->_mode == 0 || (conn->_mode ==1 && conn->_bytes == 0)) {
            *len = mpdc__ringbuf_getline(&conn->in,(char *)conn->scratch);
            if(*len <= 0) {
                if(conn->_mode == 1 && conn->_bytes == 0 && *len == 0) {
                    /* end of binary response */
                    conn->_mode = 0;
                    continue;
                }
                break;
            }
        } else {
            *len = mpdc__ringbuf_getbytes(&conn->in,conn->scratch,conn->_bytes);
            if(*len <= 0) break;
            conn->_bytes -= *len;
        }
        if(conn->_mode == 0) {
            ok = mpdc__line(conn,(char *)conn->scratch,(unsigned int)*len);
            if(ok == 0) {
                t = mpdc__str_chr((const char *)conn->scratch,':',*len);
                conn->scratch[t] = 0;
                s = (char *)conn->scratch;
                while(*s) {
                    *s = mpdc__to_lower(*s);
                    s++;
                }
                if(mpdc__case_starts((const char *)conn->scratch,"binary")) {
                    conn->_mode = 1;
                    mpdc__scan_uint((const char *)conn->scratch + t + 2,&conn->_bytes);
                }
                else {
                    conn->response(conn,mpdc__command[conn->state],(const char *)conn->scratch,(const uint8_t *)conn->scratch + t + 2,((unsigned int)*len) - t - 2);
                }
            }
            else {
                if(conn->state >= 0) {
                    conn->response_end(conn,mpdc__command[conn->state],ok,(const char *)conn->scratch);
                }
                conn->state = -1;
                break;
            }
        } else {
            conn->response(conn,mpdc__command[conn->state],"binary",conn->scratch,*len);
        }

    } while(*len > -1);

    if(ok == 1 && !mpdc_ringbuf_is_empty(&conn->op)) conn->write_notify(conn);
    return ok;
}

static int mpdc__read_notify_default(mpdc_connection *conn) {
    (void)conn;
    return 1;
}

static int mpdc__receive_block(mpdc_connection *conn) {
    int ok = 0;
    int len = 0;
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
    int len = 0;
    r = mpdc_ringbuf_fill(&conn->in);
    if(r == -1) return -1;
    while(!mpdc_ringbuf_is_empty(&conn->in)) {
        ok = mpdc__process(conn,&len);
        if(ok == -1) return -1;
        if(len == 0) break;
    }
    return ok;
}

static int mpdc__resolve_default(mpdc_connection *conn, char *hostname) {
    (void)conn;
    (void)hostname;
    return 1;
}

static int mpdc__connection_default(mpdc_connection *conn, char *hostname, uint16_t port) {
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

static void mpdc__response_default(mpdc_connection *conn, const char *cmd, const char *key, const uint8_t *value, unsigned int length) {
    (void)conn;
    (void)cmd;
    (void)key;
    (void)value;
    (void)length;
    return;
}

static int mpdc__op_queue(mpdc_connection *conn, char op) {
    if(!mpdc__ringbuf_putchr(&conn->op, op)) return -1;
    if(!handshook(conn)) return conn->read_notify(conn);
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
    conn->state = -1;
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
int mpdc_setup(mpdc_connection *conn, char *mpdc_host, char *mpdc_port_str, uint16_t mpdc_port) {
    if(mpdc_host == NULL) {
        conn->host = (char *)mpdc__host_default;
    }
    else {
        conn->host = mpdc_host;
        unsigned int len = mpdc__str_len(mpdc_host,255);
        unsigned int at  = mpdc__str_chr(mpdc_host,'@',len);
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
    if(mpdc_ringbuf_is_empty(&conn->out)) return 0;
    if(!handshook(conn)) return conn->read_notify(conn) > 0;

    int exp;
    int r = mpdc__ringbuf_flushline(&conn->out,&exp);
    if(r == -1) return -1;
    if(r == exp) {
        if(conn->state == MPDC_COMMAND_IDLE) {
            /* we just sent a "noidle" message, just do a read */
            return conn->read_notify(conn) > 0;
        }
        conn->state = mpdc__ringbuf_getchr(&conn->op);
        if(conn->state == MPDC_COMMAND_IDLE && !mpdc_ringbuf_is_empty(&conn->op)) {
            /* we have a "noidle" queued to cancel this idle, so go into write mode */
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

STATIC
int mpdc_idle(mpdc_connection *conn, uint_least16_t events) {
    int d = 0;
    uint8_t cur_op;

    if(!(mpdc_ringbuf_is_empty(&conn->op))) {
        d = 1;
        cur_op = mpdc__op_last(conn);
    }

    if(d == 1 && cur_op == MPDC_COMMAND_IDLE) {
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

#define ZEROARG_FUNC_IMPL(command,COMMAND) \
STATIC int mpdc_ ## command(mpdc_connection *conn) { \
    if(!mpdc__ringbuf_putstr(&conn->out,mpdc__command[MPDC_COMMAND_ ## COMMAND ])) return -1; \
    if(!mpdc__ringbuf_endstr(&conn->out)) return -1; \
    return mpdc__op_queue(conn,MPDC_COMMAND_ ## COMMAND ); \
}

/* fmt is a character string describing arguments */
/* if a character is capitalized or non-alpha, NO
 * space is placed into the buffer.
 * If a lower alpha, a space is placed.
 *
 * example: fmt = "sss" => "string string string"
 *          fmt = "u:U" => "integer:integer" (no space)"
 */

STATIC
int mpdc__put(mpdc_connection *conn, unsigned int cmd, const char *fmt, ...) {
    const char *f = fmt;
    va_list va;
    va_start(va,fmt);
    char *s;
    unsigned int u;
    int d = 0;
    uint8_t cur_op;

    if(!(mpdc_ringbuf_is_empty(&conn->op))) {
        d = 1;
        cur_op = mpdc__op_last(conn);
    }

    if(d == 1 && cur_op == MPDC_COMMAND_IDLE) {
        if(!mpdc__ringbuf_putstr(&conn->out,"noidle")) return -1;
        if(!mpdc__ringbuf_endstr(&conn->out)) return -1;
    }

    if(!mpdc__ringbuf_putstr(&conn->out,mpdc__command[cmd])) return -1;
    while(*f) {
        char t = *f++;
        if(mpdc__is_lower(t)) {
            if(!mpdc__ringbuf_putchr(&conn->out,' ')) return -1;
        }
        else {
            t = mpdc__to_lower(t);
        }
        switch(t) {
            case ':': {
                if(!mpdc__ringbuf_putchr(&conn->out,':')) return -1;
                break;
            }
            case 'r': {
                s = va_arg(va,char *);
                if(!mpdc__ringbuf_putstr(&conn->out,s)) return -1;
                break;
            }
            case 's': {
                s = va_arg(va,char *);
                if(!mpdc__ringbuf_putchr(&conn->out,'"')) return -1;
                if(!mpdc__ringbuf_putstr(&conn->out,s)) return -1;
                if(!mpdc__ringbuf_putchr(&conn->out,'"')) return -1;
                break;
            }
            case 'u': {
                u = va_arg(va, unsigned int);
                conn->scratch[mpdc__fmt_uint((char *)conn->scratch,u)] = 0;
                if(!mpdc__ringbuf_putstr(&conn->out,(const char *)conn->scratch)) return -1;
                break;
            }
            case 'd': {
                d = va_arg(va, int);
                conn->scratch[mpdc__fmt_int((char *)conn->scratch,d)] = 0;
                if(!mpdc__ringbuf_putstr(&conn->out,(const char *)conn->scratch)) return -1;
                break;
            }

        }
    }
    if(!mpdc__ringbuf_endstr(&conn->out)) return -1;
    return mpdc__op_queue(conn,cmd);
}

