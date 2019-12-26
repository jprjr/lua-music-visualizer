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

/* returns the current working directory */
/* remember to free the returned pointer */
char *path_getcwd(void);

int path_setcwd(const char *dir);

/* converts a path to absolute,
 * allocates a new string */
char *path_absolute(const char *filename);

#ifdef __cplusplus
}
#endif

#endif
