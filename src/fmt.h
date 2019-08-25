#ifndef FMT_H
#define FMT_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifdef __cplusplus
extern "C" {
#endif

unsigned int fmt_uint(char *d, unsigned int num);
unsigned int fmt_int(char *d, int num);

#ifdef __cplusplus
}
#endif

#endif
