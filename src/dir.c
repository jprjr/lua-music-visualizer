/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */
#include "norm.h"

#ifdef JPR_WINDOWS
#include "utf.h"
#endif

#include "dir.h"
#include "mem.h"
#include "str.h"

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
    unsigned int filename_width;
    uint8_t *filename;
    ULARGE_INTEGER tmp;
    LARGE_INTEGER tmp2;

    entry = NULL;
    if(dir->d == INVALID_HANDLE_VALUE) return entry;
    entry = (jpr_dire *)mem_alloc(sizeof(jpr_dire));
    if(entry == NULL) return entry;
    mem_set(entry,0,sizeof(jpr_dire));

    filename_width = utf_conv_utf16w_utf8(NULL,dir->find_data.cFileName,0);
    if(filename_width == 0) {
        dire_free(entry);
        return NULL;
    }
    filename = mem_alloc(filename_width + 1);
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
    entry->path = mem_alloc(str_len(dir->dir) + str_len(entry->filename) + 2);
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
    tmp2.QuadPart -= 11644473600LL;
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
    entry = NULL;

    de = readdir(dir->d);
    if(de == NULL) return NULL;

    entry = (jpr_dire *)mem_alloc(sizeof(jpr_dire));
    if(entry == NULL) return entry;
    mem_set(entry,0,sizeof(jpr_dire));

    entry->filename = mem_alloc(str_len(de->d_name)+1);
    if(entry->filename == NULL) {
        dire_free(entry);
        return NULL;
    }
    str_cpy(entry->filename,de->d_name);

    entry->path = mem_alloc(str_len(dir->dir) + str_len(entry->filename) + 2);
    if(entry->path == NULL) {
        dire_free(entry);
        return NULL;
    }
    str_cpy(entry->path,dir->dir);
    str_cat(entry->path,"/");
    str_cat(entry->path,entry->filename);

    if(stat(entry->path,&st) == -1) {
        dire_free(entry);
        return NULL;
    }
    if(S_ISDIR(st.st_mode)) {
        entry->is_dir = 1;
    }
    if(S_ISREG(st.st_mode)) {
        entry->is_file = 1;
    }
    entry->mtime = (int64_t)st.st_mtime;
    entry->size  = (int64_t)st.st_size;
#endif

    return entry;
}

jpr_dir *dir_open(const char *filename) {
    jpr_dir *dir;
#ifdef JPR_WINDOWS
    wchar_t *wide_filename;
    unsigned int wide_filename_len;
#endif
    dir = NULL;

    dir = (jpr_dir*)mem_alloc(sizeof(jpr_dir));
    if(dir == NULL) return dir;

#ifdef JPR_WINDOWS
    wide_filename = NULL;
    wide_filename_len = 0;
    dir->d = INVALID_HANDLE_VALUE;

    wide_filename_len = utf_conv_utf8_utf16w(NULL,(const uint8_t *)filename,0);
    if(wide_filename_len == 0) goto dir_open_error;
    wide_filename = (wchar_t *)mem_alloc(sizeof(wchar_t) * (wide_filename_len + 4));
    if(wide_filename == NULL) goto dir_open_error;
    if(wide_filename_len != utf_conv_utf8_utf16w(wide_filename,(const uint8_t *)filename, 0)) goto dir_open_error;
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
    if(wide_filename != NULL) mem_free(wide_filename);
#else
    dir->d = opendir(filename);
    if(dir->d == NULL) {
        mem_free(dir);
        dir = NULL;
    }

#endif
    if(dir != NULL) {
      dir->dir = mem_alloc(str_len(filename) + 1);
      if(dir->dir == NULL) {
          dir_close(dir);
          return NULL;
      }
      dir->dir[str_cpy(dir->dir,filename)] = 0;
    }

    return dir;
}

void dire_free(jpr_dire *entry) {
    if(entry->filename != NULL) mem_free(entry->filename);
    if(entry->path != NULL) mem_free(entry->path);
    mem_free(entry);
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
        if(dir->dir != NULL) mem_free(dir->dir);
        mem_free(dir);
    }
    return r;
}
