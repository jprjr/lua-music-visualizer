#include "norm.h"
#include "path.h"
#include "utf.h"
#include "str.h"
#include "mem.h"

#ifdef JPR_WINDOWS
#include "mem.h"
#else
#include <unistd.h>
#include <limits.h>
#endif

static inline void trim_slash(char *filename, unsigned int len) {
    while(len--) {
        if(filename[len] == '/') {
            filename[len] = '\0';
        }
#ifdef JPR_WINDOWS
        else if(filename[len] == '\\') {
            filename[len] = '\0';
        }
#endif
        else {
            break;
        }
    }
}

static unsigned int last_slash(const char *filename, unsigned int *len) {
    unsigned int sep;
    unsigned int t;
#ifdef JPR_WINDOWS
    unsigned int t2;
#endif

    *len = str_len(filename);
    sep = *len;

    do {
        t = str_nrchr(filename,'/',sep);
#ifdef JPR_WINDOWS
        t2 = str_nrchr(filename,'\\',sep);
        if(t == sep) {
            t = t2;
        } else if(t2 != sep && t2 > t) {
            t = t2;
        }
#endif
        if(t == sep) break;
        sep = t;
    } while(filename[sep] == '\0' || filename[sep+1] == '\0' || filename[sep+1] == '/'
#ifdef JPR_WINDOWS
       || filename[sep+1] == '\\'
#endif
    );

    return sep;
}

int path_exists(const char *filename) {
    int r = 0;
#ifdef JPR_WINDOWS
    wchar_t *wide_filename;
    unsigned int wide_filename_len;

    wide_filename = NULL;
    wide_filename_len = 0;

    wide_filename_len = utf_conv_utf8_utf16w(NULL,(const uint8_t *)filename,0);
    if(wide_filename_len == 0) {
        r = -1;
        goto path_exists_cleanup;
    }
    wide_filename = (wchar_t *)mem_alloc(sizeof(wchar_t) * (wide_filename_len + 1));
    if(wide_filename == NULL) {
        r = -1;
        goto path_exists_cleanup;
    }
    if(wide_filename_len != utf_conv_utf8_utf16w(wide_filename,(const uint8_t *)filename,0)) {
        r = -1;
        goto path_exists_cleanup;
    }
    wide_filename[wide_filename_len] = 0;

    r = (GetFileAttributesW(wide_filename) != INVALID_FILE_ATTRIBUTES);

    path_exists_cleanup:
    if(wide_filename != NULL) mem_free(wide_filename);
#else
    if(access(filename,F_OK) != -1) {
        r = 1;
    }
#endif
    return r;
}

char *path_basename(const char *filename) {
    unsigned int len;
    unsigned int sep;
    char *ret;

    if(filename == NULL || str_len(filename) == 0) {
        ret = mem_alloc(2);
        if(ret == NULL) return ret;
        ret[0] = '.';
        ret[1] = '\0';
        return ret;
    }

    sep = last_slash(filename, &len);

    if(filename[sep] == '\0') { /* no slashes found */
        ret = mem_alloc(len + 1);
        if(ret == NULL) return ret;
        str_cpy(ret,filename);
    }
    else {
        len = len - sep - 1;
        ret = mem_alloc(len+1);
        str_cpy(ret,&filename[sep+1]);
    }

    trim_slash(ret,len);

    if(ret[0] == '\0') {
#ifdef JPR_WINDOWS
        ret[0] = '\\';
#else
        ret[0] = '/';
#endif
        ret[1] = '\0';
    }

    return ret;
}

char *path_dirname(const char *filename) {
    unsigned int len;
    unsigned int sep;
    char *ret;

    if(filename != NULL) {
        sep = last_slash(filename,&len);
    }

    if(filename == NULL || str_len(filename) == 0 || filename[sep] == '\0') {
        ret = mem_alloc(2);
        if(ret == NULL) return ret;
        ret[0] = '.';
        ret[1] = '\0';
        return ret;
    }

    ret = mem_alloc(sep);
    if(ret == NULL) return ret;
    str_ncpy(ret,filename,sep);
    ret[sep] = '\0';

    trim_slash(ret,sep);

    if(ret[0] == '\0') {
#ifdef JPR_WINDOWS
        ret[0] = '\\';
#else
        ret[0] = '/';
#endif
        ret[1] = '\0';
    }

    return ret;
}

char *path_getcwd(void) {
    char *dir;
#ifdef JPR_WINDOWS
    TCHAR *wdir;
    DWORD len;

    len = GetCurrentDirectory(0,NULL);

    wdir = (TCHAR *)mem_alloc(sizeof(TCHAR) * len);
    if(wdir == NULL) {
        return NULL;
    }
    GetCurrentDirectory(len,wdir);

    len = utf_conv_utf16w_utf8(NULL,wdir,0);
    if(len == 0) {
        mem_free(wdir);
        return NULL;
    }
    dir = mem_alloc(len + 1);
    if(dir == NULL) {
        mem_free(wdir);
        return NULL;
    }
    if(len != utf_conv_utf16w_utf8((uint8_t *)dir,wdir,0)) {
        mem_free(wdir);
        mem_free(dir);
        return NULL;
    }
    dir[len] = '\0';

    mem_free(wdir);
#else
    dir = mem_alloc(PATH_MAX);
    if(dir == NULL) return NULL;
    if(getcwd(dir,PATH_MAX) == NULL) {
        mem_free(dir);
        return NULL;
    }
#endif
    return dir;
}
