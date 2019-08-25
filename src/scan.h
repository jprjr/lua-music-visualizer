#ifndef SCAN_H
#define SCAN_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#ifdef __cplusplus
extern "C" {
#endif

unsigned int scan_uint(const char *s, unsigned int *num);
unsigned int scan_int(const char *s, int *num);


#ifdef __cplusplus
}
#endif

#endif
