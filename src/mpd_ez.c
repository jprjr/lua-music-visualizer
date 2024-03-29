#include "mpd_ez.h"
#include "scan.h"
#include "str.h"
#include "util.h"
#include "int.h"
#include "norm.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

#define MPD_FILE   0x01
#define MPD_TITLE  0x02
#define MPD_ALBUM  0x04
#define MPD_ARTIST 0x08
#define MPD_NAME   0x10

static void ez_mpdc_response_begin(mpdc_connection *conn,const char *cmd) {
    conn_info *info = (conn_info *)conn->ctx;
    video_generator *v = info->v;
    if(str_equals(cmd,"currentsong")) {
        v->mpd_tags = 0;
    }
}


static void ez_mpdc_response(mpdc_connection *conn, const char *cmd, const char *key, const jpr_uint8 *value, size_t length) {
#ifndef NDEBUG
    int lua_state;
#endif
    conn_info *info;
    video_generator *v;
    const char *t;
    char *c;
    unsigned int tmp_uint;
    unsigned int tmp_fac;

    (void)length;
    info = (conn_info *)conn->ctx;
    v = info->v;
#ifndef NDEBUG
    lua_state = lua_gettop(v->L);
#endif

    tmp_uint = 0;
    tmp_fac = 0;

    lua_getglobal(v->L,"song"); /* push */

    if(str_equals(cmd,"status")) {
        if(str_equals(key,"songid")) {
            scan_uint((char *)value,&tmp_uint);
            lua_pushinteger(v->L,tmp_uint);
            lua_setfield(v->L,-2,"songid");
        } else if(str_equals(key,"elapsed")) {
            t = (const char *)value;
            t += scan_uint(t,&tmp_uint);
            tmp_uint *= 1000;
            t++;
            scan_uint(t,&tmp_fac);
            tmp_uint += tmp_fac;
            v->elapsed = (double)tmp_uint;
            lua_pushnumber(v->L,v->elapsed / 1000.0f);
            lua_setfield(v->L,-2,"elapsed");
        } else if(str_equals(key,"duration")) {
            t = (const char *)value;
            t += scan_uint(t,&tmp_uint);
            tmp_uint *= 1000;
            t++;
            scan_uint(t,&tmp_fac);
            tmp_uint += tmp_fac;
            v->duration = tmp_uint / 1000.0f;
            lua_pushnumber(v->L,v->duration);
            lua_setfield(v->L,-2,"total");
        }
        else if(str_equals(key,"time")) {
            if(v->duration == 0.0f) {
                c = str_chr((const char *)value,':');
                if(c != NULL) {
                  t = &c[1];
                  scan_uint(t,&tmp_uint);
                  lua_pushnumber(v->L,tmp_uint);
                  lua_setfield(v->L,-2,"total");
                }
            }
        }
    } else if(str_equals(cmd,"currentsong")) {
        if(str_equals(key,"file")) {
            lua_pushstring(v->L,(const char *)value);
            lua_setfield(v->L,-2,"file");
            v->mpd_tags |= MPD_FILE;
        } else if(str_equals(key,"title")) {
            lua_pushstring(v->L,(const char *)value);
            lua_setfield(v->L,-2,"title");
            v->mpd_tags |= MPD_TITLE;
        } else if(str_equals(key,"album")) {
            lua_pushstring(v->L,(const char *)value);
            lua_setfield(v->L,-2,"album");
            v->mpd_tags |= MPD_ALBUM;
        } else if(str_equals(key,"artist")) {
            lua_pushstring(v->L,(const char *)value);
            lua_setfield(v->L,-2,"artist");
            v->mpd_tags |= MPD_ARTIST;
        } else if(str_equals(key,"name")) {
            lua_pushstring(v->L,(const char *)value);
            lua_setfield(v->L,-2,"name");
            v->mpd_tags |= MPD_NAME;
        }
    }
    else if(str_equals(cmd,"readmessages")) {
        if(str_equals(key,"message")) {
            lua_pushstring(v->L,(const char *)value); /* push */
            lua_setfield(v->L,-2,"message"); /* pop */
        }
        v->decoder->onchange(v->decoder->meta_ctx,"message");
    }

    lua_pop(v->L,1);

    if(str_equals(cmd,"idle")) {
        if(str_equals(key,"changed")) {
            if(str_equals((const char *)value,"player")) {
                mpdc_status(conn);
                mpdc_currentsong(conn);
            }
            else if(str_equals((const char *)value,"message")) {
                mpdc_readmessages(conn);
            }
        }
    }
#ifndef NDEBUG
    assert(lua_state == lua_gettop(v->L));
#endif
}

static void ez_mpdc_response_end(mpdc_connection *conn, const char *cmd, int ok, const char *err) {
    conn_info *info = (conn_info *)conn->ctx;
    video_generator *v = info->v;
#ifndef NDEBUG
    int lua_state = lua_gettop(v->L);
#endif

    if(ok == -1) {
        WRITE_STDERR("mpd_ok (");
        WRITE_STDERR(cmd);
        WRITE_STDERR(") returned -1: ");
        LOG_ERROR(err);
        JPR_EXIT(1);
    }
    if(str_equals(cmd,"idle")) {
        mpdc_idle(conn,MPDC_EVENT_PLAYER | MPDC_EVENT_MESSAGE);
        return;
    }

    if(str_equals(cmd,"currentsong")) {
        lua_getglobal(v->L,"song"); /* push */
        if((v->mpd_tags & MPD_FILE) == 0) {
            lua_pushnil(v->L);
            lua_setfield(v->L,-2,"file");
        }
        if((v->mpd_tags & MPD_TITLE) == 0) {
            lua_pushnil(v->L);
            lua_setfield(v->L,-2,"title");
        }
        if((v->mpd_tags & MPD_ALBUM) == 0) {
            lua_pushnil(v->L);
            lua_setfield(v->L,-2,"album");
        }
        if((v->mpd_tags & MPD_ARTIST) == 0) {
            lua_pushnil(v->L);
            lua_setfield(v->L,-2,"artist");
        }
        if((v->mpd_tags & MPD_NAME) == 0) {
            lua_pushnil(v->L);
            lua_setfield(v->L,-2,"name");
        }
        lua_pop(v->L,1);
    }

    if(str_equals(cmd,"status") || str_equals(cmd,"currentsong")) {
      v->decoder->onchange(v->decoder->meta_ctx,"player");
    }
#ifndef NDEBUG
    assert(lua_state == lua_gettop(v->L));
#endif
}

static int ez_ndelay_on(SOCKET fd)
{
#ifdef _WIN32
    unsigned long mode = 1;
    int res = ioctlsocket(fd, FIONBIO, &mode);
    return (res == NO_ERROR) ? 0 : -1;
#else
    int got = fcntl(fd, F_GETFL) ;
    return (got == -1) ? -1 : fcntl(fd, F_SETFL, got | O_NONBLOCK) ;
#endif
}

static int ez_read_func(void *ctx, jpr_uint8 *buf, size_t count) {
    conn_info *conn = (conn_info *)ctx;
    int r;
    do {
        r = recv(conn->fd,(char *)buf,count,0);
    } while( (r == -1) && (errno == EINTR) );
    return r;
}

static int ez_write_func(void *ctx, const jpr_uint8 *buf, size_t count) {
    conn_info *conn = (conn_info *)ctx;
    int r;
    do {
        r = send(conn->fd,(char *)buf,count,0);
    } while( (r == -1) && (errno == EINTR));
    return r;
}

static int ez_resolve(mpdc_connection *c, const char *hostname) {
#ifdef JPR_WINDOWS
	DWORD dwError;
#endif
    conn_info *conn = (conn_info *)c->ctx;
    if(hostname[0] == '/') return 1;

    if( (conn->he = gethostbyname(hostname)) == NULL) {
        LOG_ERROR("hostname resolution failed");
#ifdef JPR_WINDOWS
        dwError = WSAGetLastError();
        if(dwError != 0) {
            if(dwError == WSAHOST_NOT_FOUND) {
                LOG_ERROR("host not found");
            } else if(dwError == WSANO_DATA) {
                LOG_ERROR("no data record found");
            } else {
                LOG_ERROR("unknown gethostbyname error");
            }
        }
#endif
        return -1;
    }
    conn->fd = INVALID_SOCKET;
    return 1;
}


static int ez_connect(mpdc_connection *c, const char *host, jpr_uint16 port) {

#ifndef JPR_WINDOWS
    struct sockaddr_un u_addr;
#endif
    conn_info *conn = (conn_info *)c->ctx;
    if(conn->fd != INVALID_SOCKET) {
        closesocket(conn->fd);
        conn->fd = INVALID_SOCKET;
    }

#ifndef JPR_WINDOWS
    if(host[0] == '/') {
        conn->fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if(conn->fd == INVALID_SOCKET) {
            LOG_ERROR("failed to open unix socket");
            return -1;
        }
        mem_set(&u_addr,0,sizeof(u_addr));
        u_addr.sun_family = AF_UNIX;
        str_ncpy(u_addr.sun_path,host,sizeof(u_addr.sun_path)-1);
        if(connect(conn->fd,(struct sockaddr *)&u_addr,sizeof(u_addr)) < 0) {
            LOG_ERROR("failed to connect to unix socket");
            close(conn->fd);
            conn->fd = INVALID_SOCKET;
            return -1;
        }

    }
    else {
#endif
        conn->fd = socket(conn->he->h_addrtype, SOCK_STREAM, 0);
        if(conn->fd == INVALID_SOCKET) return -1;

        mem_set(&(conn->addr),0,sizeof(struct sockaddr_in));
        conn->addr.sin_port = htons(port);
        conn->addr.sin_family = AF_INET;
        mem_cpy(&(conn->addr.sin_addr),conn->he->h_addr_list[0],conn->he->h_length);

        if(connect(conn->fd,(struct sockaddr *)&(conn->addr),sizeof(struct sockaddr)) < 0) {
            closesocket(conn->fd);
            conn->fd = INVALID_SOCKET;
            return -1;
        }
#ifndef JPR_WINDOWS
    }
#endif

    ez_ndelay_on(conn->fd);
#ifdef JPR_WINDOWS
	(void)host;
    FD_ZERO(&conn->readfds);
    FD_ZERO(&conn->writefds);
#else
    conn->pfd.fd = conn->fd;
    conn->pfd.events = 0;
    conn->pfd.revents = 0;
#endif

    return 1;
}

static int ez_read_notify(mpdc_connection *c) {
    conn_info *conn = (conn_info *)c->ctx;
#ifdef _WIN32
    conn->mode = 0;
#else
    conn->pfd.events = POLLIN;
#endif
    return 1;
}

static int ez_write_notify(mpdc_connection *c) {
    conn_info *conn = (conn_info *)c->ctx;
#ifdef _WIN32
    conn->mode = 1;
#else
    conn->pfd.events = POLLOUT;
#endif
    return 1;
}

static void ez_disconnect(mpdc_connection *c) {
    conn_info *conn = (conn_info *)c->ctx;
    closesocket(conn->fd);
    conn->fd = INVALID_SOCKET;
    LOG_DEBUG("disconnect from mpd");
}

int mpd_ez_setup(video_generator *v) {
    int r = 0;
    char *mpdc_host;
    char *mpdc_port;
    conn_info *info;

    mpdc_host = getenv("MPD_HOST");
    mpdc_port = getenv("MPD_PORT");
    if(mpdc_host == NULL) return 0;

    v->mpd = (mpdc_connection *)malloc(sizeof(mpdc_connection));
    if(UNLIKELY(v->mpd == NULL)) return 1;

    info = (conn_info *)malloc(sizeof(conn_info));
    if(UNLIKELY(info == NULL)) return 1;

#ifdef _WIN32
    r = WSAStartup(MAKEWORD(2,2), &(info->wsaData));
    if(r != 0) return 1;
#endif

    info->s = NULL;
    info->a = 0;
    info->v = v;

    v->mpd->ctx = info;
    v->mpd->resolve = ez_resolve;
    v->mpd->connect = ez_connect;
    v->mpd->disconnect = ez_disconnect;
    v->mpd->read = ez_read_func;
    v->mpd->write = ez_write_func;
    v->mpd->response_begin = ez_mpdc_response_begin;
    v->mpd->response_end = ez_mpdc_response_end;
    v->mpd->response = ez_mpdc_response;
    v->mpd->read_notify = ez_read_notify;
    v->mpd->write_notify = ez_write_notify;

    r = mpdc_init(v->mpd);
    if(r < 0) return 1;

    r = mpdc_setup(v->mpd,mpdc_host,mpdc_port,0);
    if(r < 0) return 1;

    r = mpdc_connect(v->mpd);
    if(r<0) return 1;

    return 0;
}

void mpd_ez_start(video_generator *v) {
    mpdc_subscribe(v->mpd,"visualizer");
    mpdc_status(v->mpd);
    mpdc_currentsong(v->mpd);
    mpdc_idle(v->mpd, MPDC_EVENT_PLAYER | MPDC_EVENT_MESSAGE);
}

int mpd_ez_loop(video_generator *v) {
    int r = 0;
    conn_info *info = (conn_info *)v->mpd->ctx;
	int events;

#ifdef JPR_WINDOWS

    struct timeval tv;

    FD_ZERO(&info->readfds);
    FD_ZERO(&info->writefds);

    if(info->mode) {
        FD_SET(info->fd,&info->writefds);
    }
    else {
        FD_SET(info->fd,&info->readfds);
    }

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    do {
        events = select(1,&info->readfds,&info->writefds,NULL,&tv);
    } while ( (events == -1) && (errno == EINTR));
    if(events == 0) return 0;
    if(FD_ISSET(info->fd,&info->readfds)) {
        r = mpdc_receive(v->mpd);
    }
    else if(FD_ISSET(info->fd,&info->writefds)) {
        r = mpdc_send(v->mpd);
    }

#else

    do {
        events = poll(&(info->pfd),1,0);
    } while( (events == -1) && (errno == EINTR));

    if(events ==0) return 0;

    if(info->pfd.revents & POLLIN) {
        r = mpdc_receive(v->mpd);
    }
    if(info->pfd.revents & POLLOUT) {
      r = mpdc_send(v->mpd);
    }

    if(info->pfd.revents & (POLLHUP | POLLERR)) {
        WRITE_STDERR("disconnecting from mpd: ");
        LOG_ERROR(strerror(errno));
        mpdc_disconnect(v->mpd);
        return -1;
    }
#endif
    return r;
}


