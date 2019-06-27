#include "mpd_ez.h"
#include "scan.h"
#include "str.h"
#include <assert.h>

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

    if(strcmp(cmd,"status") == 0) {
        if(strcmp(key,"songid") == 0) {
            scan_uint((char *)value,&tmp_int);
            lua_pushinteger(v->L,tmp_int);
            lua_setfield(v->L,-2,"songid");
        } else if(strcmp(key,"elapsed") == 0) {
            t = (const char *)value;
            t += scan_uint(t,&tmp_int);
            tmp_int *= 1000;
            t++;
            scan_uint(t,&tmp_fac);
            tmp_int += tmp_fac;
            v->elapsed = (double)tmp_int;
            lua_pushnumber(v->L,v->elapsed / 1000.0f);
            lua_setfield(v->L,-2,"elapsed");
        } else if(strcmp(key,"duration") == 0) {
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
        else if(strcmp(key,"time") == 0) {
            if(v->duration == 0.0f) {
                tmp_int = str_chr((const char *)value,':');
                t = (const char *)value + tmp_int + 1;
                scan_uint(t,&tmp_int);
                lua_pushnumber(v->L,tmp_int);
                lua_setfield(v->L,-2,"total");
            }
        }
    } else if(strcmp(cmd,"currentsong") ==0) {
        if(strcmp(key,"file") == 0) {
            lua_pushstring(v->L,(const char *)value);
            lua_setfield(v->L,-2,"file");
        } else if(strcmp(key,"title") == 0) {
            lua_pushstring(v->L,(const char *)value);
            lua_setfield(v->L,-2,"title");
        } else if(strcmp(key,"album") == 0) {
            lua_pushstring(v->L,(const char *)value);
            lua_setfield(v->L,-2,"album");
        } else if(strcmp(key,"artist") == 0) {
            lua_pushstring(v->L,(const char *)value);
            lua_setfield(v->L,-2,"artist");
        }
    }
    else if(strcmp(cmd,"readmessages") == 0) {
        if(strcmp(key,"message") == 0) {
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

    if(strcmp(cmd,"idle") == 0) {
        if(strcmp(key,"changed") == 0) {
            if(strcmp((const char *)value,"player") == 0) {
                mpdc_status(conn);
                mpdc_currentsong(conn);
            }
            else if(strcmp((const char *)value,"message") == 0) {
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
    if(strcmp(cmd,"idle") == 0) {
        mpdc_idle(conn,MPDC_EVENT_PLAYER | MPDC_EVENT_MESSAGE);
        return;
    }

    if(strcmp(cmd,"status") == 0 || strcmp(cmd,"currentsong") == 0) {
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

static int ez_ndelay_on(int fd)
{
    int got = fcntl(fd, F_GETFL) ;
    return (got == -1) ? -1 : fcntl(fd, F_SETFL, got | O_NONBLOCK) ;
}

static int ez_read_func(void *ctx, uint8_t *buf, unsigned int count) {
    conn_info *conn = (conn_info *)ctx;
    int r = read(conn->fd,buf,count);
    write(2,"<< ",3);
    write(2,buf,r);
    return r;
}

static int ez_write_func(void *ctx, const uint8_t *buf, unsigned int count) {
    conn_info *conn = (conn_info *)ctx;
    write(2,">> ",3);
    write(2,buf,count);
    int r = write(conn->fd,buf,count);
    return r;
}

static int ez_resolve(mpdc_connection *c, char *hostname) {
    conn_info *conn = (conn_info *)c->ctx;
    if( (conn->he = gethostbyname(hostname)) == NULL) {
        return -1;
    }
    conn->fd = -1;
    return 1;
}


static int ez_connect(mpdc_connection *c, char *host, uint16_t port) {
    (void)host;
    conn_info *conn = (conn_info *)c->ctx;
    if(conn->fd > -1) {
        close(conn->fd);
        conn->fd = -1;
    }

    conn->fd = socket(conn->he->h_addrtype, SOCK_STREAM, 0);
    if(conn->fd == -1) return -1;

    memset(&(conn->addr),0,sizeof(struct sockaddr_in));
    conn->addr.sin_port = htons(port);
    conn->addr.sin_family = AF_INET;
    memcpy(&(conn->addr.sin_addr),conn->he->h_addr,conn->he->h_length);

    if(connect(conn->fd,(struct sockaddr *)&(conn->addr),sizeof(struct sockaddr)) < 0) {
        close(conn->fd);
        conn->fd = -1;
        return -1;
    }

    ez_ndelay_on(conn->fd);
    conn->pfd.fd = conn->fd;
    conn->pfd.events = 0;
    conn->pfd.revents = 0;

    return 1;
}

static int ez_read_notify(mpdc_connection *c) {
    conn_info *conn = (conn_info *)c->ctx;
    conn->pfd.events = POLLIN;
    return 1;
}

static int ez_write_notify(mpdc_connection *c) {
    conn_info *conn = (conn_info *)c->ctx;
    conn->pfd.events = POLLOUT;
    return 1;
}

static void ez_disconnect(mpdc_connection *c) {
    conn_info *conn = (conn_info *)c->ctx;
    close(conn->fd);
    conn->fd = -1;
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

    info->s = NULL;
    info->a = 0;
    info->v = v;

    v->mpd->ctx = info;
    v->mpd->resolve = ez_resolve;
    v->mpd->connect = ez_connect;
    v->mpd->disconnect = ez_disconnect;
    v->mpd->read = ez_read_func;
    v->mpd->write = ez_write_func;
    v->mpd->response_begin = NULL;
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
    int events = poll(&(info->pfd),1,0);
    if(events ==0) return 0;

    if(info->pfd.revents & POLLIN) {
        r = mpdc_receive(v->mpd);
    }
    if(info->pfd.revents & POLLOUT) {
      r = mpdc_send(v->mpd);
    }
    if(info->pfd.revents & (POLLHUP | POLLERR)) {
        mpdc_disconnect(v->mpd);
        return -1;
    }
    return r;
}


