/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include "norm.h"
#include "file.h"
#include "mem.h"
#include "str.h"
#include "util.h"
#include <limits.h>

#ifdef JPR_WINDOWS
#include "utf.h"
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

static int jpr_coe(int fd) {
    int flags = fcntl(fd, F_GETFD, 0) ;
    if (flags < 0) return -1 ;
    return fcntl(fd, F_SETFD, flags | FD_CLOEXEC) ;
}

static int jpr_uncoe(int fd) {
    int flags = fcntl(fd, F_GETFD, 0) ;
    if (flags < 0) return -1 ;
    return fcntl(fd, F_SETFD, flags & ~FD_CLOEXEC);
}

static int jpr_dup2(int fd, int fd2) {
    int r;
    do {
        r = dup2(fd,fd2);
    } while( ( r == -1) && (errno == EINTR) );
    return r;
}

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
    size_t wide_filename_len;

    DWORD disposition;
    DWORD access;
    DWORD shared;

    f = NULL;
    if(filename == NULL) {
        return f;
    }
    if(mode == NULL) {
        return f;
    }

    wide_filename = NULL;
    wide_filename_len = 0;

    disposition = 0;
    access = 0;
    shared = 0;

    f = (jpr_file *)mem_alloc(sizeof(jpr_file));
    if(f == NULL) {
        goto file_open_cleanup;
    }

    f->fd = INVALID_HANDLE_VALUE;
    f->eof = 0;
    f->closed = 0;

    if(str_cmp(filename,"-") == 0) {
        if(str_chr(mode,'r') != NULL) {
            f->fd = GetStdHandle( STD_INPUT_HANDLE );
        } else {
            f->fd = GetStdHandle( STD_OUTPUT_HANDLE );
        }
        return f;
    }

    wide_filename_len = utf_conv_utf8_utf16w(NULL,(const jpr_uint8 *)filename,0);
    if(wide_filename_len == 0) {
        goto file_open_cleanup;
    }
    wide_filename = (wchar_t *)mem_alloc(sizeof(wchar_t) * (wide_filename_len + 1));
    if(wide_filename == NULL) {
        goto file_open_cleanup;
    }
    if(wide_filename_len != utf_conv_utf8_utf16w(wide_filename,(const jpr_uint8 *)filename,0)) {
        goto file_open_cleanup;
    }
    wide_filename[wide_filename_len] = 0;

    switch(mode[0]) {
        case 'r': {
            access = GENERIC_READ;
            disposition = OPEN_EXISTING;
            shared = FILE_SHARE_READ;
            break;
        }
        case 'w': {
            access = GENERIC_WRITE;
            disposition = CREATE_ALWAYS;
            shared = FILE_SHARE_WRITE;
            break;
        }
        case 'a': {
            access = GENERIC_WRITE;
            disposition = OPEN_ALWAYS;
            shared = FILE_SHARE_WRITE;
            break;
        }
        default: {
            mem_free(f);
            f = NULL;
            goto file_open_cleanup;
        }
    }
    if(str_chr(mode,'+') != NULL) { /* '+' found in string */
        access = GENERIC_READ | GENERIC_WRITE;
    }

    f->fd = CreateFileW(wide_filename,access,shared,NULL,disposition,0,0);
    if(f->fd == INVALID_HANDLE_VALUE) {
        mem_free(f);
        f = NULL;
    }
    else {
        if(str_chr(mode,'a') != NULL) { /* 'a' found in string */
            file_seek(f,0,2);
        }
    }

file_open_cleanup:
    if(wide_filename != NULL) mem_free(wide_filename);

#else

    int flags;
    int wr;
    flags = 0;
    wr = 0;

    f = NULL;

    if(filename == NULL) return f;
    if(mode == NULL) return f;

    f = (jpr_file *)mem_alloc(sizeof(jpr_file));
    if(f == NULL) return f;

    f->eof = 0;
    f->fd = -1;
    f->closed = 0;

    if(str_cmp(filename,"-") == 0) {
        if(str_chr(mode,'r') != NULL) {
            f->fd = 0;
        } else {
            f->fd = 1;
        }
        return f;
    }
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
        default: {
            mem_free(f);
            return NULL;
        }
    }

    if(str_chr(mode,'+') != NULL) { /* '+' found in string */
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
        if(str_chr(mode,'a') != NULL) { /* 'a' found in string */
            file_seek(f,0,2);
        }
    }
#endif
    return f;
}

void file_free(jpr_file *f) {
    if(!f->closed) {
        file_close(f);
    }
    mem_free(f);
    return;
}

int file_close(jpr_file *f) {
    int r;
    if(f == NULL || f->closed) {
        return 0;
    }

#ifdef JPR_WINDOWS
    r = CloseHandle(f->fd);
    if(r) {
        f->closed = 1;
    }
    return !r;
#else
    r = jpr_close(f->fd);
    if(r == 0) {
        f->closed = 1;
    }
    return r;
#endif

}

jpr_int64 file_tell(jpr_file *f) {
#if defined(JPR_WINDOWS)
    LARGE_INTEGER zero;
    LARGE_INTEGER r;
    zero.QuadPart = 0;
    if(SetFilePointerEx(f->fd,zero,&r,FILE_CURRENT)) {
        return r.QuadPart;
    }
    return -1;
#elif defined(JPR_LINUX)
    return lseek64(f->fd,0,SEEK_CUR);
#else
    return lseek(f->fd,0,SEEK_CUR);
#endif
}


jpr_int64 file_seek(jpr_file *f, jpr_int64 offset, enum JPR_FILE_POS whence) {
#ifdef JPR_WINDOWS
    DWORD w_whence;
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
    jpr_int64 r;
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
}

jpr_uint64 file_write(jpr_file *f, const void *buf, jpr_uint64 n) {
    jpr_uint64 r;
#ifdef JPR_WINDOWS
#define TEMP_TYPE DWORD
#define MAX_TYPE DWORD
#define MAX_VAL 0xFFFFFFFFU
#define WRITE_IMP \
    if(!WriteFile(f->fd,(const void *)&b[r],m,&t,NULL)) return 0;
#else
#define TEMP_TYPE ssize_t
#define MAX_TYPE size_t
#define MAX_VAL SIZE_MAX
#define WRITE_IMP \
    t = jpr_write(f->fd,(const void *)&b[r],m); \
    if(t <= 0) return r;
#endif
    TEMP_TYPE t;
    MAX_TYPE m;
    const jpr_uint8 *b;
    r = 0;
    t = 0;
    m = 0;
    b = buf;
    while(n > 0) {
        m = (MAX_TYPE)( n > MAX_VAL ? MAX_VAL : n );
        WRITE_IMP
        r += t;
        n -= t;
        if(t == 0) return r;
    }
    return r;
#undef TEMP_TYPE
#undef MAX_TYPE
#undef MAX_VAL
#undef WRITE_IMP
}

jpr_uint64 file_read(jpr_file *f, void *buf, jpr_uint64 n) {
    jpr_uint64 r;
#ifdef JPR_WINDOWS
#define TEMP_TYPE DWORD
#define MAX_TYPE DWORD
#define MAX_VAL 0xFFFFFFFFU
#define READ_IMP \
    if(!ReadFile(f->fd,(void *)&b[r],m,&t,NULL)) return 0;
#define FILE_EOF f->eof = 1;
#else
#define TEMP_TYPE ssize_t
#define MAX_TYPE size_t
#define MAX_VAL SIZE_MAX
#define READ_IMP \
    t = jpr_read(f->fd,(void *)&b[r],m); \
    if(t < 0) return r;
#define FILE_EOF f->eof = 1;
#endif
    TEMP_TYPE t;
    MAX_TYPE m;
    jpr_uint8 *b;
    r = 0;
    t = 0;
    m = 0;
    b = buf;
    while(n > 0) {
        m = (MAX_TYPE)( n > MAX_VAL ? MAX_VAL : n );
        READ_IMP
        r += t;
        n -= t;
        if(t == 0) {
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
    return f->eof;
}

int file_coe(jpr_file *f) {
#ifdef JPR_WINDOWS
    if(f->fd != INVALID_HANDLE_VALUE) {
      if(!SetHandleInformation(f->fd,HANDLE_FLAG_INHERIT,0)) {
        return -1;
      }
    }
    return 0;
#else
    if(f->fd != -1) {
      return jpr_coe(f->fd);
    }
    return 0;
#endif
}

int file_uncoe(jpr_file *f) {
#ifdef JPR_WINDOWS
    if(f->fd != INVALID_HANDLE_VALUE) {
      if(!SetHandleInformation(f->fd,HANDLE_FLAG_INHERIT,1)) {
        return -1;
      }
    }
    return 0;
#else
    if(f->fd != -1) {
      return jpr_uncoe(f->fd);
    }
    return 0;
#endif
}

int file_dupe(jpr_file *f, jpr_file *f2) {
#ifdef JPR_WINDOWS
    if(f->fd != INVALID_HANDLE_VALUE) {
      if(!DuplicateHandle(GetCurrentProcess(),f->fd,GetCurrentProcess(),&(f2->fd),0,TRUE,DUPLICATE_SAME_ACCESS)) {
        return -1;
      }
    }
    return 0;
#else
    if(f->fd != -1) {
      return jpr_dup2(f->fd,f2->fd);
    }
    return 0;
#endif
}

