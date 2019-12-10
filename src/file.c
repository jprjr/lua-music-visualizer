/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#if defined(_WIN32) || defined(_WIN64)
#ifndef _UNICODE
#define _UNICODE 1
#endif
#ifndef UNICODE
#define UNICODE 1
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#include "utf.h"
#elif defined(__linux__)
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 1
#endif
#endif

#if !( (defined(_WIN32) || defined(_WIN64)))
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "file.h"
#include "mem.h"

#if defined(_WIN32) || defined(_WIN64)
struct jpr_file_s {
    HANDLE fd;
};
#else
struct jpr_file_s {
    int fd;
};
#endif

jpr_file *file_open(const char *filename, const char *mode) {
#if defined(_WIN32) || defined(_WIN64)
    jpr_file *f;
    wchar_t *wide_filename;
    unsigned int wide_filename_len;
    uint32_t disposition;
    uint32_t access;

    f = NULL;
    wide_filename = NULL;
    wide_filename_len = 0;
    disposition = 0;
    access = 0;

    f = (jpr_file *)mem_alloc(sizeof(jpr_file));
    if(f == NULL) goto file_open_cleanup;

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
    mode++;
    switch(mode[0]) {
        case '+': access = GENERIC_READ | GENERIC_WRITE;
    }

    wide_filename_len = utf_conv_utf8_utf16w(NULL,(const uint8_t *)filename,0);
    if(wide_filename_len == 0) goto file_open_cleanup;
    wide_filename = (wchar_t *)mem_alloc(sizeof(wchar_t) * (wide_filename_len + 1));
    if(wide_filename == NULL) goto file_open_cleanup;
    if(wide_filename_len != utf_conv_utf8_utf16w(wide_filename,(const uint8_t *)filename,0)) goto file_open_cleanup;
    wide_filename[wide_filename_len] = 0;

    f->fd = CreateFileW(wide_filename,access,0,NULL,disposition,0,0);
    if(f->fd == INVALID_HANDLE_VALUE) {
        mem_free(f);
        f = NULL;
    }

file_open_cleanup:
    if(wide_filename != NULL) mem_free(wide_filename);

    return f;
#else
    jpr_file *f;
    int flags;
    int wr;
    f = NULL;
    flags = 0;
    wr = 0;
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
    mode++;
    switch(mode[0]) {
        case '+': flags = O_RDWR; break;
        default: if(wr) flags |= O_WRONLY;
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
    f = (jpr_file *)malloc(sizeof(jpr_file));
    if(f == NULL) return f;
    f->fd = open(filename,flags,0666);
    if(f->fd == -1) {
        mem_free(f);
        f = NULL;
    }
    return f;
#endif
}

int file_close(jpr_file *f) {
    int r;
#if defined(_WIN32) || defined(_WIN64)
    r = CloseHandle(f->fd);
    if(r) mem_free(f);
    return !r;
#else
    r = close(f->fd);
    if(r == 0) mem_free(f);
    return r;
#endif
}

int64_t file_tell(jpr_file *f) {
#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER zero;
    LARGE_INTEGER r;
    zero.QuadPart = 0;
    if(SetFilePointerEx(f->fd,zero,&r,FILE_CURRENT)) {
        return r.QuadPart;
    }
    return -1;
#else
    return lseek64(f->fd,0,SEEK_CUR);
#endif
}


int file_seek(jpr_file *f, int64_t offset, int whence) {
#if defined(_WIN32) || defined(_WIN64)
    uint32_t w_whence;
    LARGE_INTEGER o;
    LARGE_INTEGER diff;
    switch(whence) {
        case 0: w_whence = FILE_BEGIN; break;
        case 1: w_whence = FILE_CURRENT; break;
        case 2: w_whence = FILE_END; break;
        default: return -1;
    }
    o.QuadPart = offset;
    if(SetFilePointerEx(f->fd,o,&diff,w_whence)) {
        if(diff.QuadPart > INT_MAX) diff.QuadPart = INT_MAX;
        else if(diff.QuadPart < INT_MIN) diff.QuadPart = INT_MIN;
        return (int)diff.QuadPart;
    }
    return -1;
#else
    return lseek64(f->fd,offset,whence);
#endif
}

