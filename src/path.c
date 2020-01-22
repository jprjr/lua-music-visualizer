/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include "norm.h"
#include "path.h"
#include "utf.h"
#include "str.h"
#include "attr.h"

#ifndef JPR_WINDOWS
#include <unistd.h>
#include <limits.h>
#endif

#include <stdlib.h>
#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

#ifdef JPR_WINDOWS
static wchar_t *to_wchar(const char *str) {
    wchar_t *wide_str;
    size_t wide_str_len;

    wide_str = NULL;
    wide_str_len = 0;

    wide_str_len = utf_conv_utf8_utf16w(NULL,(const jpr_uint8 *)str,0);
    if(wide_str_len == 0) {
        return NULL;
    }
    wide_str = (wchar_t *)malloc(sizeof(wchar_t) * (wide_str_len + 1));
    /* LCOV_EXCL_START */
    if(UNLIKELY(wide_str == NULL)) {
        return NULL;
    }
    if(UNLIKELY(wide_str_len != utf_conv_utf8_utf16w(wide_str,(const jpr_uint8 *)str,0))) {
        free(wide_str);
        return NULL;
    }
    wide_str[wide_str_len] = 0;
    /* LCOV_EXCL_STOP */
    return wide_str;
}
#endif

static attr_inline void trim_slash(char *filename, size_t len) {
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

static size_t last_slash(const char *filename, size_t *len) {
    size_t sep;
    char *t;
#ifdef JPR_WINDOWS
    char *t2;
#endif

    *len = str_len(filename);
    sep = *len;

    do {
        t = str_nrchr(filename,'/',sep);
#ifdef JPR_WINDOWS
        t2 = str_nrchr(filename,'\\',sep);
        if(t == NULL || t == &filename[sep]) {
            t = t2;
        } else if(t2 != NULL && t2 != &filename[sep] && t2 > t) {
            t = t2;
        }
#endif
        if(t == NULL || t == &filename[sep]) break;
        sep = t - filename;
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
    wide_filename = to_wchar(filename);

    /* LCOV_EXCL_START */
    if(UNLIKELY(wide_filename == NULL)) {
        r = -1;
        goto path_exists_cleanup;
    }
    /* LCOV_EXCL_STOP */

    r = (GetFileAttributesW(wide_filename) != INVALID_FILE_ATTRIBUTES);

    path_exists_cleanup:
    if(wide_filename != NULL) free(wide_filename);
#else
    if(access(filename,F_OK) != -1) {
        r = 1;
    }
#endif
    return r;
}

char *path_basename(const char *filename) {
    size_t len;
    size_t sep;
    char *ret;

    if(filename == NULL || str_len(filename) == 0) {
        ret = (char *)malloc(2);
        /* LCOV_EXCL_START */
        if(UNLIKELY(ret == NULL)) return ret;
        /* LCOV_EXCL_STOP */
        ret[0] = '.';
        ret[1] = '\0';
        return ret;
    }

    sep = last_slash(filename, &len);

    if(filename[sep] == '\0') { /* no slashes found */
        ret = (char *)malloc(len + 1);
        /* LCOV_EXCL_START */
        if(UNLIKELY(ret == NULL)) return ret;
        /* LCOV_EXCL_STOP */
        str_cpy(ret,filename);
    }
    else {
        len = len - sep - 1;
        if(len == 0) len++;
        ret = (char *)malloc(len + 1);
        /* LCOV_EXCL_START */
        if(UNLIKELY(ret == NULL)) return ret;
        /* LCOV_EXCL_STOP */
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
    size_t len;
    size_t sep;
    char *ret;

    if(filename != NULL) {
        sep = last_slash(filename,&len);
    }

    if(filename == NULL || str_len(filename) == 0 || filename[sep] == '\0') {
        ret = (char *)malloc(2);
        /* LCOV_EXCL_START */
        if(UNLIKELY(ret == NULL)) return ret;
        /* LCOV_EXCL_STOP */
        ret[0] = '.';
        ret[1] = '\0';
        return ret;
    }

    ret = (char *)malloc(sep + 1 + (sep == 0));
    /* LCOV_EXCL_START */
    if(UNLIKELY(ret == NULL)) return ret;
    /* LCOV_EXCL_STOP */
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
	size_t slen;

    len = GetCurrentDirectory(0,NULL);

    wdir = (TCHAR *)malloc(sizeof(TCHAR) * len);
    /* LCOV_EXCL_START */
    if(UNLIKELY(wdir == NULL)) {
        return NULL;
    }
    /* LCOV_EXCL_STOP */
    GetCurrentDirectory(len,wdir);

    slen = utf_conv_utf16w_utf8(NULL,wdir,0);
    if(slen == 0) {
        free(wdir);
        return NULL;
    }
    dir = malloc(slen + 1 + (slen == 0));
    /* LCOV_EXCL_START */
    if(UNLIKELY(dir == NULL)) {
        free(wdir);
        return NULL;
    }
    /* LCOV_EXCL_STOP */
    if(slen != utf_conv_utf16w_utf8((jpr_uint8 *)dir,wdir,0)) {
        free(wdir);
        free(dir);
        return NULL;
    }
    dir[slen] = '\0';

    free(wdir);
#else
    dir = (char *)malloc(PATH_MAX);
    /* LCOV_EXCL_START */
    if(UNLIKELY(dir == NULL)) return NULL;
    if(getcwd(dir,PATH_MAX) == NULL) {
        free(dir);
        return NULL;
    }
    /* LCOV_EXCL_STOP */
#endif
    return dir;
}

char *path_absolute(const char *f) {
    size_t is_absolute;
    size_t f_len;
    size_t t_len;
    char *t;
    char *cwd;
    is_absolute = 0;
    f_len = str_len(f);

    switch(f[0]) {
#ifdef JPR_WINDOWS
        case '\\': /* fall-through */
#endif
        case '/': is_absolute = 1; break;
    }

#ifdef JPR_WINDOWS
    if(is_absolute == 0) {
        if(f_len > 2 && f[1] == ':' && f[2] == '\\') { /* C:\ */
            is_absolute = 1;
        }
    }
#endif
    if (is_absolute) {
        return str_dup(f);
    }

    cwd = path_getcwd();
    t_len = str_len(cwd) + f_len + 1;
    t = (char *)malloc(t_len+1 + (t_len == 0));
    /* LCOV_EXCL_START */
    if(UNLIKELY(t == NULL)) {
        free(cwd);
        return t;
    }
    /* LCOV_EXCL_STOP */
    str_cpy(t,cwd);
    free(cwd);
#ifdef JPR_WINDOWS
    str_cat(t,"\\");
#else
    str_cat(t,"/");
#endif
    str_cat(t,f);

#ifdef JPR_WINDOWS
    for(f_len=0;f_len<t_len;f_len++) {
        if(t[f_len] == '/') t[f_len] = '\\';
    }
#endif

    return t;
}

/* returns 0 on success */
int path_setcwd(const char *dir) {
    int r = 1;
#ifdef JPR_WINDOWS
    wchar_t *wide_filename;
    wide_filename = to_wchar(dir);

    /* LCOV_EXCL_START */
    if(wide_filename == NULL) {
        goto path_setcwd_cleanup;
    }
    /* LCOV_EXCL_STOP */

    r = SetCurrentDirectoryW(wide_filename) == 0;

    path_setcwd_cleanup:
    if(wide_filename != NULL) free(wide_filename);

#else
    r = chdir(dir);
#endif

    return r;
}
