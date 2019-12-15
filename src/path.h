#ifndef JPR_PATH_H
#define JPR_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

int path_exists(const char *filename);

/* returns basename/dirname without modifying original */
/* remember to free the returned pointer */
char *path_basename(const char *filename);
char *path_dirname(const char *filename);

#ifdef __cplusplus
}
#endif

#endif
