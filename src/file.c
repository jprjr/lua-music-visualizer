/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */
#include "norm.h"

#ifdef JPR_WINDOWS
#include "utf.h"
#endif

#include "file.h"
#include "mem.h"
#include "str.h"
#include <limits.h>

#ifndef JPR_NO_STDLIB

#include <stdio.h>

struct jpr_file_s {
    FILE *fd;
};

#elif defined(JPR_WINDOWS)

struct jpr_file_s {
    HANDLE fd;
    int eof;
};

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

struct jpr_file_s {
    int fd;
    int eof;
};

static int jpr_open(const char *path, int flag, int mode) {
    int r;
    do {
        r = open(path,flag,mode);
    } while ( (r == -1) && (errno == EINTR));
    return r;
}

static int jpr_close(int fd) {
    int r;
    do {
        r = close(fd);
    } while ( (r == -1) && (errno == EINTR));
    return r;
}

static ssize_t jpr_write(int fd, const void *buf, size_t n) {
    ssize_t r;
    do {
        r = write(fd,buf,n);
    } while ( (r == -1) && (errno == EINTR));
    return r;
}

static ssize_t jpr_read(int fd, void *buf, size_t n) {
    ssize_t r;
    do {
        r = read(fd,buf,n);
    } while ( (r == -1) && (errno == EINTR));
    return r;
}

#endif

jpr_file *file_open(const char *filename, const char *mode) {
    jpr_file *f;
#ifdef JPR_WINDOWS
    wchar_t *wide_filename;
    unsigned int wide_filename_len;

#ifdef JPR_NO_STDLIB
    uint32_t disposition;
    uint32_t access;
#else
    int bin_flag;
    char *m;
    wchar_t *wide_mode;
    unsigned int wide_mode_len;
#endif

    f = NULL;
    if(filename == NULL) return f;
    if(mode == NULL) return f;

    wide_filename = NULL;
    wide_filename_len = 0;

#ifdef JPR_NO_STDLIB
    disposition = 0;
    access = 0;
#else
    bin_flag = 0;
    m = NULL;
    wide_mode = NULL;
    wide_mode_len = 0;
#endif

    f = (jpr_file *)mem_alloc(sizeof(jpr_file));
    if(f == NULL) goto file_open_cleanup;

#ifdef JPR_NO_STDLIB
    f->fd = INVALID_HANDLE_VALUE;
    f->eof = 0;
#else
    f->fd = NULL;
#endif

    wide_filename_len = utf_conv_utf8_utf16w(NULL,(const uint8_t *)filename,0);
    if(wide_filename_len == 0) goto file_open_cleanup;
    wide_filename = (wchar_t *)mem_alloc(sizeof(wchar_t) * (wide_filename_len + 1));
    if(wide_filename == NULL) goto file_open_cleanup;
    if(wide_filename_len != utf_conv_utf8_utf16w(wide_filename,(const uint8_t *)filename,0)) goto file_open_cleanup;
    wide_filename[wide_filename_len] = 0;


#ifdef JPR_NO_STDLIB
    switch(mode[0]) {
        case 'r': {
            access = GENERIC_READ;
            disposition = OPEN_EXISTING;
            break;
        }
        case 'w': {
            access = GENERIC_WRITE;
            disposition = CREATE_ALWAYS;
            break;
        }
        case 'a': {
            access = GENERIC_WRITE;
            disposition = OPEN_ALWAYS;
            break;
        }
        default: {
            mem_free(f);
            f = NULL;
            goto file_open_cleanup;
        }
    }
    if(mode[str_chr(mode,'+')] == '+') { /* '+' found in string */
        access = GENERIC_READ | GENERIC_WRITE;
    }

    f->fd = CreateFileW(wide_filename,access,0,NULL,disposition,0,0);
    if(f->fd == INVALID_HANDLE_VALUE) {
        mem_free(f);
        f = NULL;
    }
    else {
        if(mode[str_chr(mode,'a')] == 'a') { /* 'a' found in string */
            file_seek(f,0,2);
        }
    }
#else
    /* check for 'b' */
    if(mode[str_chr(mode,'b')] == 0) { /* 'b' not found in string */
        bin_flag = 1;
    }

    m = mem_alloc(str_len(mode) + bin_flag + 1);
    if(m == NULL) goto file_open_cleanup;
    str_cpy(m,mode);
    if(bin_flag) str_cat(m,"b");

    wide_mode_len = utf_conv_utf8_utf16w(NULL,(const uint8_t *)m,0);
    if(wide_mode_len == 0) goto file_open_cleanup;
    wide_mode = (wchar_t *)mem_alloc(sizeof(wchar_t) * (wide_mode_len + 1));
    if(wide_mode == NULL) goto file_open_cleanup;
    if(wide_mode_len != utf_conv_utf8_utf16w(wide_mode,(const uint8_t *)m,0)) goto file_open_cleanup;
    wide_mode[wide_mode_len] = 0;

    f->fd = _wfopen(wide_filename,wide_mode);
    if(f->fd == NULL) {
        mem_free(f);
        f = NULL;
    }
    else {
        if(mode[str_chr(mode,'a')] == 'a') { /* 'a' found in string */
            file_seek(f,0,2);
        }
    }
#endif

file_open_cleanup:
    if(wide_filename != NULL) mem_free(wide_filename);
#ifndef JPR_NO_STDLIB
    if(wide_mode != NULL) mem_free(wide_mode);
    if(m != NULL) mem_free(m);
#endif

    return f;
#else

#ifdef JPR_NO_STDLIB
    int flags;
    int wr;
    flags = 0;
    wr = 0;
#endif

    f = NULL;

    if(filename == NULL) return f;
    if(mode == NULL) return f;

    f = (jpr_file *)mem_alloc(sizeof(jpr_file));
    if(f == NULL) return f;

#ifdef JPR_NO_STDLIB
    f->eof = 0;
    f->fd = -1;
    switch(mode[0]) {
        case 'r': {
            flags |= O_RDONLY;
            break;
        }
        case 'w': {
            wr = 1;
            break;
        }
        case 'a': {
            wr = 2;
            break;
        }
        default: return NULL;
    }

    if(mode[str_chr(mode,'+')] == '+') { /* '+' found in string */
        flags = O_RDWR;
    } else {
        if(wr) flags |= O_WRONLY;
    }

    switch(wr) {
        case 1: {
            flags |= O_CREAT;
            flags |= O_TRUNC;
            break;
        }
        case 2: {
            flags |= O_CREAT;
            flags |= O_APPEND;
            break;
        }
    }
    f->fd = jpr_open(filename,flags,0666);
    if(f->fd == -1) {
        mem_free(f);
        f = NULL;
    }
    else {
        if(mode[str_chr(mode,'a')] == 'a') { /* 'a' found in string */
            file_seek(f,0,2);
        }
    }
#else
    f->fd = fopen(filename,mode);
    if(f->fd == NULL) {
        mem_free(f);
        f = NULL;
    }
    else {
        if(mode[str_chr(mode,'a')] == 'a') { /* 'a' found in string */
            file_seek(f,0,2);
        }
    }
#endif
    return f;
#endif
}

int file_close(jpr_file *f) {
    int r;
#ifdef JPR_NO_STDLIB

#ifdef JPR_WINDOWS
    r = CloseHandle(f->fd);
    if(r) mem_free(f);
    return !r;
#else
    r = jpr_close(f->fd);
    if(r == 0) mem_free(f);
    return r;
#endif

#else
    r = fclose(f->fd);
    if(r == 0) mem_free(f);
    return r;
#endif
}

int64_t file_tell(jpr_file *f) {
#ifdef JPR_NO_STDLIB
#ifdef JPR_WINDOWS
    LARGE_INTEGER zero;
    LARGE_INTEGER r;
    zero.QuadPart = 0;
    if(SetFilePointerEx(f->fd,zero,&r,FILE_CURRENT)) {
        return r.QuadPart;
    }
    return -1;
#else

#ifdef JPR_LINUX
    return lseek64(f->fd,0,SEEK_CUR);
#else
    return lseek(f->fd,0,SEEK_CUR);
#endif

#endif
#else
#ifdef JPR_WINDOWS
    return _ftelli64(f->fd);
#else
    return ftello(f->fd);
#endif
#endif
}


int64_t file_seek(jpr_file *f, int64_t offset, enum JPR_FILE_POS whence) {
#ifdef JPR_NO_STDLIB
#ifdef JPR_WINDOWS
    uint32_t w_whence;
    LARGE_INTEGER o;
    LARGE_INTEGER diff;
    switch(whence) {
        case JPR_FILE_SET: w_whence = FILE_BEGIN; break;
        case JPR_FILE_CUR: w_whence = FILE_CURRENT; break;
        case JPR_FILE_END: w_whence = FILE_END; break;
        default: return -1;
    }
    o.QuadPart = offset;
    if(SetFilePointerEx(f->fd,o,&diff,w_whence)) {
        f->eof = 0;
        return diff.QuadPart;
    }
    return -1;
#else
    int64_t r;
    int i_whence;
    switch(whence) {
        case JPR_FILE_SET: i_whence = SEEK_SET; break;
        case JPR_FILE_CUR: i_whence = SEEK_CUR; break;
        case JPR_FILE_END: i_whence = SEEK_END; break;
        default: return -1;
    }
#ifdef JPR_LINUX
    r = lseek64(f->fd,offset,i_whence);
#else
    r = lseek(f->fd,offset,whence);
#endif
    if(r >= 0) {
        f->eof = 0;
    }
    return r;

#endif
#else
    int i_whence;
    switch(whence) {
        case JPR_FILE_SET: i_whence = SEEK_SET; break;
        case JPR_FILE_CUR: i_whence = SEEK_CUR; break;
        case JPR_FILE_END: i_whence = SEEK_END; break;
        default: return -1;
    }
#ifdef JPR_WINDOWS
    if(_fseeki64(f->fd,offset,i_whence) == 0) {
        return file_tell(f);
    }
    return -1;
#else
    if(fseeko(f->fd,offset,i_whence) == 0) {
        return file_tell(f);
    }
    return -1;
#endif
#endif
}

uint64_t file_write(jpr_file *f, const void *buf, uint64_t n) {
    uint64_t r;
#ifndef JPR_NO_STDLIB
#define TEMP_TYPE size_t
#define MAX_TYPE size_t
#define MAX_VAL SIZE_MAX
#define WRITE_IMP \
    t = fwrite(buf,1,m,f->fd); \
    if(t == 0) return r;
#else
#ifdef JPR_WINDOWS
#define TEMP_TYPE DWORD
#define MAX_TYPE DWORD
#define MAX_VAL UINT32_MAX
#define WRITE_IMP \
    if(!WriteFile(f->fd,buf,m,&t,NULL)) return 0;
#else
#define TEMP_TYPE ssize_t
#define MAX_TYPE size_t
#define MAX_VAL SIZE_MAX
#define WRITE_IMP \
    t = jpr_write(f->fd,buf,m); \
    if(t <= 0) return r;
#endif
#endif
    TEMP_TYPE t;
    MAX_TYPE m;
    r = 0;
    t = 0;
    m = 0;
    while(n > 0) {
        m = ( n > MAX_VAL ? MAX_VAL : n );
        WRITE_IMP
        r += t;
        n -= t;
        if((MAX_TYPE)t != m) return r;
    }
    return r;
#undef TEMP_TYPE
#undef MAX_TYPE
#undef MAX_VAL
#undef WRITE_IMP
}

uint64_t file_read(jpr_file *f, void *buf, uint64_t n) {
    uint64_t r;
#ifndef JPR_NO_STDLIB
#define TEMP_TYPE size_t
#define MAX_TYPE size_t
#define MAX_VAL SIZE_MAX
#define READ_IMP \
    t = fread(buf,1,m,f->fd); \
    if(t == 0) return r;
#define FILE_EOF
#else
#ifdef JPR_WINDOWS
#define TEMP_TYPE DWORD
#define MAX_TYPE DWORD
#define MAX_VAL UINT32_MAX
#define READ_IMP \
    if(!ReadFile(f->fd,buf,m,&t,NULL)) return 0;
#define FILE_EOF f->eof = 1;
#else
#define TEMP_TYPE ssize_t
#define MAX_TYPE size_t
#define MAX_VAL SIZE_MAX
#define READ_IMP \
    t = jpr_read(f->fd,buf,m); \
    if(t < 0) return r;
#define FILE_EOF f->eof = 1;
#endif
#endif
    TEMP_TYPE t;
    MAX_TYPE m;
    r = 0;
    t = 0;
    m = 0;
    while(n > 0) {
        m = ( n > MAX_VAL ? MAX_VAL : n );
        READ_IMP
        r += t;
        n -= t;
        if((MAX_TYPE)t != m) {
            FILE_EOF
            return r;
        }
    }
    return r;
#undef TEMP_TYPE
#undef MAX_TYPE
#undef MAX_VAL
#undef READ_IMP
#undef FILE_EOF
}

int file_eof(jpr_file *f) {
#ifdef JPR_NO_STDLIB
    return f->eof;
#else
    return feof(f->fd);
#endif
}
