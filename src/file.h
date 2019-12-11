#ifndef JPR_FILE_H
#define JPR_FILE_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include <stdint.h>

typedef struct jpr_file_s jpr_file;

enum JPR_FILE_POS {
    JPR_FILE_SET = 0,
    JPR_FILE_CUR = 1,
    JPR_FILE_END = 2,
};

#ifdef __cplusplus
extern "C" {
#endif

jpr_file *file_open(const char *filename, const char *mode);
int file_close(jpr_file *f);
int64_t file_tell(jpr_file *f);
int64_t file_seek(jpr_file *f, int64_t offset, enum JPR_FILE_POS whence);
uint64_t file_write(jpr_file *f, const void *buf, uint64_t n);
uint64_t file_read(jpr_file *f, void *buf, uint64_t n);
int file_eof(jpr_file *f);

#ifdef __cplusplus
}
#endif

#endif
