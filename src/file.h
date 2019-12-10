#ifndef JPR_FILE_H
#define JPR_FILE_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include <stdint.h>

typedef struct jpr_file_s jpr_file;

#ifdef __cplusplus
extern "C" {
#endif

jpr_file *file_open(const char *filename, const char *mode);
int file_close(jpr_file *f);
int64_t file_tell(jpr_file *f);
int file_seek(jpr_file *f, int64_t offset, int whence);

#ifdef __cplusplus
}
#endif

#endif
