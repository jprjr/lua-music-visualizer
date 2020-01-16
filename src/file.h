#ifndef JPR_FILE_H
#define JPR_FILE_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include "norm.h"
#include "attr.h"
#include "int.h"

typedef struct jpr_file_s jpr_file;

enum JPR_FILE_POS {
    JPR_FILE_SET = 0,
    JPR_FILE_CUR = 1,
    JPR_FILE_END = 2
};

#ifdef JPR_WINDOWS

struct jpr_file_s {
    HANDLE fd;
    int eof;
    int closed;
};

#else

struct jpr_file_s {
    int fd;
    int eof;
    int closed;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

/* allocates a new file pointer, need to call file_free */
/* file_free automatically calls file_close as well */
attr_nonnull12
jpr_file *file_open(const char * RESTRICT filename, const char *mode);
attr_nonnull1
int file_close(jpr_file *f);

attr_nonnull1
jpr_int64 file_tell(jpr_file *f);

attr_nonnull1
jpr_int64 file_seek(jpr_file *f, jpr_int64 offset, enum JPR_FILE_POS whence);

attr_nonnull12
jpr_uint64 file_write(jpr_file *f, const void *buf, jpr_uint64 n);

attr_nonnull12
jpr_uint64 file_read(jpr_file *f, void *buf, jpr_uint64 n);

attr_nonnull1
int file_eof(jpr_file *f);

attr_nonnull1
int file_coe(jpr_file *f);   /* set close-on-exec flag */

attr_nonnull1
int file_uncoe(jpr_file *f); /* unset close-on-exec flag */

attr_nonnull12
int file_dupe(jpr_file *f,jpr_file *f2); /* duplicates f into f2 */

/* "slurps" an entire file in one go */
attr_nonnull1
jpr_uint8 *file_slurp(const char * RESTRICT filename, jpr_uint64 *size);

#ifdef __cplusplus
}
#endif

#endif
