#ifndef JPR_DIR_H
#define JPR_DIR_H

#include "int.h"

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

typedef struct jpr_dir_s  jpr_dir;
typedef struct jpr_dire_s jpr_dire;

struct jpr_dire_s {
    char *filename;
    char *path;
    int is_file;
    int is_dir;
    jpr_int64 mtime;
    jpr_uint64 size;
};

#ifdef __cplusplus
extern "C" {
#endif

jpr_dir *dir_open(const char *filename);
int dir_close(jpr_dir *dir);

jpr_dire *dir_read(jpr_dir *dir);
void dire_free(jpr_dire *entry);

#ifdef __cplusplus
}
#endif

#endif
