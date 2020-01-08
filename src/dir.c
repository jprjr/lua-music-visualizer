/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */
#include "norm.h"

#ifdef JPR_WINDOWS
#include "utf.h"
#endif

#include "dir.h"
#include "str.h"

#include <stdlib.h>

#ifdef JPR_WINDOWS
struct jpr_dir_s {
    WIN32_FIND_DATAW find_data;
    HANDLE d;
    char *dir;
};
#else
#include <dirent.h>
#include <sys/stat.h>
struct jpr_dir_s {
    DIR *d;
    char *dir;
};
#endif

jpr_dire *dir_read(jpr_dir *dir) {
    jpr_dire *entry;
#ifdef JPR_WINDOWS
    size_t filename_width;
    jpr_uint8 *filename;
    ULARGE_INTEGER tmp;
    LARGE_INTEGER tmp2;

    entry = NULL;
    if(dir->d == INVALID_HANDLE_VALUE) return entry;
    entry = (jpr_dire *)malloc(sizeof(jpr_dire));
    if(entry == NULL) return entry;
    mem_set(entry,0,sizeof(jpr_dire));

    filename_width = utf_conv_utf16w_utf8(NULL,dir->find_data.cFileName,0);
    if(filename_width == 0) {
        dire_free(entry);
        return NULL;
    }
    filename = malloc(filename_width + 1);
    if(filename == NULL) {
        dire_free(entry);
        return NULL;
    }
    if(filename_width != utf_conv_utf16w_utf8(filename,dir->find_data.cFileName,0)) {
        dire_free(entry);
        return NULL;
    }
    filename[filename_width] = '\0';
    entry->filename = (char *)filename;

    if((dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
        entry->is_dir = 1;
    }
    if((dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) == FILE_ATTRIBUTE_NORMAL) {
      entry->is_file = 1;
    } else {
      if(
          ((dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) != FILE_ATTRIBUTE_DEVICE) &&
          ((dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) &&
          ((dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) != FILE_ATTRIBUTE_ENCRYPTED) &&
#ifdef FILE_ATTRIBUTE_INTEGRITY_STREAM
          ((dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM) != FILE_ATTRIBUTE_INTEGRITY_STREAM) &&
#endif
#ifdef FILE_ATTRIBUTE_NO_SCRUB_DATA
          ((dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA) != FILE_ATTRIBUTE_NO_SCRUB_DATA) &&
#endif
          ((dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) != FILE_ATTRIBUTE_OFFLINE) &&
          ((dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) != FILE_ATTRIBUTE_TEMPORARY)) {
          entry->is_file = 1;
      }

    }
    entry->path = malloc(str_len(dir->dir) + str_len(entry->filename) + 2);
    if(entry->path == NULL) {
        dire_free(entry);
        return NULL;
    }
    str_cpy(entry->path,dir->dir);
    str_cat(entry->path,"\\");
    str_cat(entry->path,entry->filename);

    tmp.LowPart  = dir->find_data.ftLastWriteTime.dwLowDateTime;
    tmp.HighPart = dir->find_data.ftLastWriteTime.dwHighDateTime;

    tmp2.QuadPart = tmp.QuadPart / 10000000;
    /* c89 doesn't allow integer long long */
    /* divide up these subtractions to effectively do -= 11644473600LL */

    tmp2.QuadPart -= 2147483647;
    tmp2.QuadPart -= 2147483647;
    tmp2.QuadPart -= 2147483647;
    tmp2.QuadPart -= 2147483647;
    tmp2.QuadPart -= 2147483647;
    tmp2.QuadPart -= 907055365;
    entry->mtime = tmp2.QuadPart;

    tmp.LowPart = dir->find_data.nFileSizeLow;
    tmp.HighPart = dir->find_data.nFileSizeHigh;
    entry->size = tmp.QuadPart;

    if(!FindNextFileW(dir->d,&dir->find_data)) {
        FindClose(dir->d);
        dir->d = INVALID_HANDLE_VALUE;
    }
#else
    struct dirent *de;
    struct stat st;
    int r;
    entry = NULL;

    de = readdir(dir->d);
    if(de == NULL) return NULL;

    entry = (jpr_dire *)malloc(sizeof(jpr_dire));
    if(entry == NULL) return entry;
    mem_set(entry,0,sizeof(jpr_dire));

    entry->filename = malloc(str_len(de->d_name)+1);

    /* LCOV_EXCL_START */
    if(entry->filename == NULL) {
        dire_free(entry);
        return NULL;
    }
    /* LCOV_EXCL_STOP */

    str_cpy(entry->filename,de->d_name);

    entry->path = malloc(str_len(dir->dir) + str_len(entry->filename) + 2);

    /* LCOV_EXCL_START */
    if(entry->path == NULL) {
        dire_free(entry);
        return NULL;
    }
    /* LCOV_EXCL_STOP */

    str_cpy(entry->path,dir->dir);
    str_cat(entry->path,"/");
    str_cat(entry->path,entry->filename);

    r = stat(entry->path,&st);

    /* LCOV_EXCL_START */
    if(r == -1) {
        dire_free(entry);
        return NULL;
    }
    /* LCOV_EXCL_STOP */

    if(S_ISDIR(st.st_mode)) {
        entry->is_dir = 1;
    }
    if(S_ISREG(st.st_mode)) {
        entry->is_file = 1;
    }
    entry->mtime = (jpr_int64)st.st_mtime;
    entry->size  = (jpr_int64)st.st_size;
#endif

    return entry;
}

jpr_dir *dir_open(const char *filename) {
    jpr_dir *dir;
#ifdef JPR_WINDOWS
    wchar_t *wide_filename;
    size_t wide_filename_len;
#endif
    dir = NULL;

    dir = (jpr_dir*)malloc(sizeof(jpr_dir));
    if(dir == NULL) return dir;
	dir->dir = NULL;

#ifdef JPR_WINDOWS
    wide_filename = NULL;
    wide_filename_len = 0;
    dir->d = INVALID_HANDLE_VALUE;

    wide_filename_len = utf_conv_utf8_utf16w(NULL,(const jpr_uint8 *)filename,0);
    if(wide_filename_len == 0) goto dir_open_error;
    wide_filename = (wchar_t *)malloc(sizeof(wchar_t) * (wide_filename_len + 4));
    if(wide_filename == NULL) goto dir_open_error;
    if(wide_filename_len != utf_conv_utf8_utf16w(wide_filename,(const jpr_uint8 *)filename, 0)) goto dir_open_error;
    if(wide_filename_len > 1) {
      switch(wide_filename[wide_filename_len-1]) {
          case L'/': /* fall-through */
          case L'\\': wide_filename_len--; break;
      }
    }
    wide_filename[wide_filename_len] = L'\\';
    wide_filename[wide_filename_len+1] = L'*';
    wide_filename[wide_filename_len+2] = L'\0';
    mem_set(&dir->find_data,0,sizeof(WIN32_FIND_DATAW));
    dir->d = FindFirstFile(wide_filename,&dir->find_data);
    if(dir->d == INVALID_HANDLE_VALUE) goto dir_open_error;

    goto dir_open_cleanup;

    dir_open_error:
    if(dir != NULL) {
        dir_close(dir);
        dir = NULL;
    }

    dir_open_cleanup:
    if(wide_filename != NULL) free(wide_filename);
#else
    dir->d = opendir(filename);
    if(dir->d == NULL) {
        free(dir);
        dir = NULL;
    }

#endif
    if(dir != NULL) {
      dir->dir = malloc(str_len(filename) + 1);
      /* LCOV_EXCL_START */
      if(dir->dir == NULL) {
          dir_close(dir);
          return NULL;
      }
      /* LCOV_EXCL_STOP */
      str_cpy(dir->dir,filename);
    }

    return dir;
}

void dire_free(jpr_dire *entry) {
    if(entry->filename != NULL) free(entry->filename);
    if(entry->path != NULL) free(entry->path);
    free(entry);
}

int dir_close(jpr_dir *dir) {
    int r = 0;
#ifdef JPR_WINDOWS
    if(dir->d != INVALID_HANDLE_VALUE) {
        r = FindClose(dir->d) != 0;
    }
#else
    r = closedir(dir->d);
#endif
    if(r == 0) {
        if(dir->dir != NULL) free(dir->dir);
        free(dir);
    }
    return r;
}
