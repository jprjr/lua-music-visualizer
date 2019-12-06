#include "mpd_ez.h"
#include "scan.h"
#include "str.h"
#include <assert.h>
#include <errno.h>

#define MPD_FILE   0x01
#define MPD_TITLE  0x02
#define MPD_ALBUM  0x04
#define MPD_ARTIST 0x08

static void ez_mpdc_response_begin(mpdc_connection *conn,const char *cmd) {
    conn_info *info = (conn_info *)conn->ctx;
    video_generator *v = info->v;
    if(str_equals(cmd,"currentsong")) {
        v->mpd_tags = 0;
    }
}


static void ez_mpdc_response(mpdc_connection *conn, const char *cmd, const char *key, const uint8_t *value, unsigned int length) {
    (void)length;

    conn_info *info = (conn_info *)conn->ctx;
    video_generator *v = info->v;
    const char *t;
#ifndef NDEBUG
    int lua_state = lua_gettop(v->L);
#endif

    unsigned int tmp_int = 0;
    unsigned int tmp_fac = 0;

    lua_getglobal(v->L,"song"); /* push */

    if(str_equals(cmd,"status")) {
        if(str_equals(key,"songid")) {
            scan_uint((char *)value,&tmp_int);
            lua_pushinteger(v->L,tmp_int);
            lua_setfield(v->L,-2,"songid");
        } else if(str_equals(key,"elapsed")) {
            t = (const char *)value;
            t += scan_uint(t,&tmp_int);
            tmp_int *= 1000;
            t++;
            scan_uint(t,&tmp_fac);
            tmp_int += tmp_fac;
            v->elapsed = (double)tmp_int;
            lua_pushnumber(v->L,v->elapsed / 1000.0f);
            lua_setfield(v->L,-2,"elapsed");
        } else if(str_equals(key,"duration")) {
            t = (const char *)value;
            t += scan_uint(t,&tmp_int);
            tmp_int *= 1000;
            t++;
            scan_uint(t,&tmp_fac);
            tmp_int += tmp_fac;
            v->duration = tmp_int / 1000.0f;
            lua_pushnumber(v->L,v->duration);
            lua_setfield(v->L,-2,"total");
        }
        else if(str_equals(key,"time")) {
            if(v->duration == 0.0f) {
                tmp_int = str_chr((const char *)value,':');
                t = (const char *)value + tmp_int + 1;
                scan_uint(t,&tmp_int);
                lua_pushnumber(v->L,tmp_int);
                lua_setfield(v->L,-2,"total");
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
        }
    }
    else if(str_equals(cmd,"readmessages")) {
        if(str_equals(key,"message")) {
            lua_pushstring(v->L,(const char *)value); /* push */
            lua_setfield(v->L,-2,"message"); /* pop */
        }
        lua_rawgeti(v->L,LUA_REGISTRYINDEX,v->lua_ref);
        lua_getfield(v->L,-1,"onchange");
        if(lua_isfunction(v->L,-1)) {
            lua_pushvalue(v->L,-2);
            lua_pushstring(v->L,"message");
            if(lua_pcall(v->L,2,0,0)) {
              fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
            }

        } else {
            lua_pop(v->L,1);
        }
        lua_pop(v->L,1);
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
        fprintf(stderr,"mpd_ok (%s) returned -1: %s\n",cmd, err);
        exit(1);
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
        lua_pop(v->L,1);
    }

    if(str_equals(cmd,"status") || str_equals(cmd,"currentsong")) {
      lua_rawgeti(v->L,LUA_REGISTRYINDEX,v->lua_ref);
      lua_getfield(v->L,-1,"onchange");
      if(lua_isfunction(v->L,-1)) {
          lua_pushvalue(v->L,-2);
          lua_pushstring(v->L,"player");
          if(lua_pcall(v->L,2,0,0)) {
              fprintf(stderr,"error: %s\n",lua_tostring(v->L,-1));
          }

      } else {
          lua_pop(v->L,1);
      }
      lua_pop(v->L,1);

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

static int ez_read_func(void *ctx, uint8_t *buf, unsigned int count) {
    conn_info *conn = (conn_info *)ctx;
    int r = recv(conn->fd,(char *)buf,count,0);
    return r;
}

static int ez_write_func(void *ctx, const uint8_t *buf, unsigned int count) {
    conn_info *conn = (conn_info *)ctx;
    int r = send(conn->fd,(char *)buf,count,0);
    return r;
}

static int ez_resolve(mpdc_connection *c, const char *hostname) {
    conn_info *conn = (conn_info *)c->ctx;
    if(hostname[0] == '/') return 1;

    if( (conn->he = gethostbyname(hostname)) == NULL) {
        fprintf(stderr,"hostname resolution failed for %s\n",hostname);
#ifdef _WIN32
        DWORD dwError = WSAGetLastError();
        if(dwError != 0) {
            if(dwError == WSAHOST_NOT_FOUND) {
                fprintf(stderr,"Host not found\n");
            } else if(dwError == WSANO_DATA) {
                fprintf(stderr,"No data record found\n");
            } else {
                fprintf(stderr,"gethostbyname failed with error: %ld\n",dwError);
            }
        }
#endif
        return -1;
    }
    conn->fd = INVALID_SOCKET;
    return 1;
}


static int ez_connect(mpdc_connection *c, const char *host, uint16_t port) {

#ifndef _WIN32
    struct sockaddr_un u_addr;
#else
    (void)host;
#endif

    conn_info *conn = (conn_info *)c->ctx;
    if(conn->fd != INVALID_SOCKET) {
        closesocket(conn->fd);
        conn->fd = INVALID_SOCKET;
    }

#ifndef _WIN32
    if(host[0] == '/') {
        conn->fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if(conn->fd == INVALID_SOCKET) {
            fprintf(stderr,"failed to open unix socket\n");
            return -1;
        }
        memset(&u_addr,0,sizeof(u_addr));
        u_addr.sun_family = AF_UNIX;
        str_ncpy(u_addr.sun_path,host,sizeof(u_addr.sun_path)-1);
        if(connect(conn->fd,(struct sockaddr *)&u_addr,sizeof(u_addr)) < 0) {
            fprintf(stderr,"failed to connect to unix socket: %s\n",strerror(errno));
            close(conn->fd);
            conn->fd = INVALID_SOCKET;
            return -1;
        }

    }
    else {
#endif
        conn->fd = socket(conn->he->h_addrtype, SOCK_STREAM, 0);
        if(conn->fd == INVALID_SOCKET) return -1;

        memset(&(conn->addr),0,sizeof(struct sockaddr_in));
        conn->addr.sin_port = htons(port);
        conn->addr.sin_family = AF_INET;
        memcpy(&(conn->addr.sin_addr),conn->he->h_addr,conn->he->h_length);

        if(connect(conn->fd,(struct sockaddr *)&(conn->addr),sizeof(struct sockaddr)) < 0) {
            closesocket(conn->fd);
            conn->fd = INVALID_SOCKET;
            return -1;
        }
#ifndef _WIN32
    }
#endif

    ez_ndelay_on(conn->fd);
#ifdef _WIN32
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
    fprintf(stderr,"disconnected from mpd\n");
    fflush(stderr);
}

int mpd_ez_setup(video_generator *v) {
    int r = 0;
    char *mpdc_host = getenv("MPD_HOST");
    char *mpdc_port = getenv("MPD_PORT");
    if(mpdc_host == NULL) return 0;

    v->mpd = malloc(sizeof(mpdc_connection));
    if(v->mpd == NULL) return 1;

    conn_info *info = NULL;
    info = malloc(sizeof(conn_info));
    if(info == NULL) return 1;

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

#ifndef _WIN32
    int events = poll(&(info->pfd),1,0);
    if(events ==0) return 0;

    if(info->pfd.revents & POLLIN) {
        r = mpdc_receive(v->mpd);
    }
    if(info->pfd.revents & POLLOUT) {
      r = mpdc_send(v->mpd);
    }
    if(info->pfd.revents & (POLLHUP | POLLERR)) {
        fprintf(stderr,"disconnecting from mpd: %s\n",strerror(errno));
        fflush(stderr);
        mpdc_disconnect(v->mpd);
        return -1;
    }
#else
    FD_ZERO(&info->readfds);
    FD_ZERO(&info->writefds);

    if(info->mode) {
        FD_SET(info->fd,&info->writefds);
    }
    else {
        FD_SET(info->fd,&info->readfds);
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    int events = select(1,&info->readfds,&info->writefds,NULL,&tv);
    if(events == 0) return 0;
    if(FD_ISSET(info->fd,&info->readfds)) {
        r = mpdc_receive(v->mpd);
    }
    else if(FD_ISSET(info->fd,&info->writefds)) {
        r = mpdc_send(v->mpd);
    }
#endif
    return r;
}


