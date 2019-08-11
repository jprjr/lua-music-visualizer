#ifndef MPD_EZ_H
#define MPD_EZ_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "mpdc.h"
#include "video-generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

struct conn_info_s {
#ifdef _WIN32
    WSADATA wsaData;
    fd_set readfds;
    fd_set writefds;
    int mode;
#else
    struct pollfd pfd;
#endif
    SOCKET fd;
    struct sockaddr_in addr;
    struct hostent *he;
    video_generator *v;
    char *s;
    unsigned int a;
};
typedef struct conn_info_s conn_info;


#ifdef __cplusplus
extern "C" {
#endif

int mpd_ez_setup(video_generator *v);
void mpd_ez_start(video_generator *v);
int mpd_ez_loop(video_generator *v);


#ifdef __cplusplus
}
#endif

#endif
