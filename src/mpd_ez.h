#ifndef MPD_EZ_H
#define MPD_EZ_H

#include "mpdc.h"
#include "video-generator.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

struct conn_info_s {
    int fd;
    struct sockaddr_in addr;
    struct hostent *he;
    struct pollfd pfd;
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
