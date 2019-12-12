#ifndef JPR_PROC_H
#define JPR_PROC_H

/*
  A simple public-domain, single-file library for starting processes.
  Features pipe data into, out of, and between processes, as well
  as redirecting input/output to/from files.

  To use: define JPR_PROC_IMPLEMENTATION into one C/C++ file within
  your project, then include this file.

  Example:
  #include ...
  #include ...
  #define JPR_PROC_IMPLEMENTATION
  #include "jpr_proc.h"

  By default, this will use standard library/built-ins like strlen
  and strcat.
  You can add
  #define JPRP_STRLEN(x) my_strlen(x)
  #define JPRP_STRCAT(dest,src) my_strcat(dest,src)
  #define JPRP_STRCPY(dest,src) my_strcpy(dest,src)
  #define JPRP_MEMSET(dest,val,len) my_memset(dest,val,len)
  #define JPRP_MALLOC(size) my_malloc(size)
  #define JPRP_FREE(ptr,userdata) my_free(ptr)
  if you want to use some other implementation. On Windows, you can
  build binaries that only link against kernel32 if you do this.

  There's two basic types you use: jpr_proc_info and jpr_proc_pipe.

  You allocate a proc_info struct and call jpr_proc_info_init on it.
  Also create whichever jpr_proc_pipe objects you need, call init,
  and either create pipes or open files.

  Then call jpr_proc_spawn, read/write from/to pipes, and finally
  call jpr_proc_wait to clean up.

  Tested on Windows, osx, and Linux.

  License is available at the end of the file.
*/

#include <stddef.h>
#include <limits.h>

#if !defined(JPRP_MALLOC) || !defined(JPRP_FREE)
#include <stdlib.h>
#ifndef JPRP_MALLOC
#define JPRP_MALLOC(size) malloc(size)
#endif
#ifndef JPRP_FREE
#define JPRP_FREE(ptr) free(ptr)
#endif
#endif

#if !defined(JPRP_STRLEN) || !defined(JPRP_STRCAT) || !defined(JPRP_STRCPY) || !defined(JPRP_MEMSET)
#include <string.h>
#ifndef JPRP_STRLEN
#define JPRP_STRLEN(x) strlen(x)
#endif
#ifndef JPRP_STRCAT
#define JPRP_STRCAT(d,s) strcat(d,s)
#endif
#ifndef JPRP_STRCPY
#define JPRP_STRCPY(d,s) strcpy(d,s)
#endif
#ifndef JPRP_MEMSET
#define JPRP_MEMSET(dest,val,len) memset(dest,val,len)
#endif
#endif


typedef struct jpr_proc_info_s jpr_proc_info;
typedef struct jpr_proc_pipe_s jpr_proc_pipe;

#ifdef __cplusplus
extern "C" {
#endif

int jpr_proc_info_init(jpr_proc_info *);
int jpr_proc_spawn(jpr_proc_info *, const char * const *argv, jpr_proc_pipe *in, jpr_proc_pipe *out, jpr_proc_pipe *err);
int jpr_proc_info_wait(jpr_proc_info *, int *);

int jpr_proc_pipe_init(jpr_proc_pipe *);
int jpr_proc_pipe_write(jpr_proc_pipe *, const char *, unsigned int len, unsigned int *written);
int jpr_proc_pipe_read(jpr_proc_pipe *, char *, unsigned int len, unsigned int *read);
int jpr_proc_pipe_close(jpr_proc_pipe *);

int jpr_proc_pipe_open_file(jpr_proc_pipe *, const char *filename, const char *mode);

#ifdef __cplusplus
}
#endif

#ifdef _WIN32
#include <windows.h>

struct jpr_proc_info_s {
    HANDLE handle;
    int pid;
};

struct jpr_proc_pipe_s {
    HANDLE pipe;
};


#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

struct jpr_proc_info_s {
    int pid;
};

struct jpr_proc_pipe_s {
    int pipe;
};

#endif
#endif

#ifdef JPR_PROC_IMPLEMENTATION

#ifndef _WIN32
extern char** environ;
static inline int jpr_coe(int fd) {
    int flags = fcntl(fd, F_GETFD, 0) ;
    if (flags < 0) return -1 ;
    return fcntl(fd, F_SETFD, flags | FD_CLOEXEC) ;
}
static inline ssize_t jpr_write(int fd, const void *buf, size_t nbyte) {
    int r;
    do {
        r = write(fd,buf,nbyte);
    } while( (r == -1) && (errno == EINTR) );
    return r;
}
static inline ssize_t jpr_read(int fd, void *buf, size_t nbyte) {
    int r;
    do {
        r = read(fd,buf,nbyte);
    } while( (r == -1) && (errno == EINTR) );
    return r;
}
static inline int jpr_dup2(int fd, int fd2) {
    int r;
    do {
        r = dup2(fd,fd2);
    } while( (r == -1) && (errno == EINTR));
    return r;
}
static inline int jpr_open(const char *path, int flags, int mode) {
    int r;
    do {
        r = open(path,flags,mode);
    } while ( (r == -1) && (errno == EINTR));
    return r;
}
static inline int jpr_close(int fd) {
    int r;
    do {
        r = close(fd);
    } while ( (r == -1) && (errno == EINTR));
    return r;
}
static inline pid_t jpr_waitpid(pid_t pid, int *stat_loc, int options) {
    pid_t r;
    do {
        r = waitpid(pid,stat_loc,options);
    } while( (r == -1) && (errno == EINTR));
    return r;
}
#endif

#ifdef _WIN32

static unsigned int jpr_strcat_escape(char *d, const char *s) {
    int n = 0;
    char echar = '\0';
    int ecount = 0;

    if(d != NULL) d += JPRP_STRLEN(d);
    while(*s) {
        ecount = 0;
        switch(*s) {
            case '"':  ecount=1; echar='\\'; break;
            case '\\': {
                if(JPRP_STRLEN(s) == 1) {
                    ecount=1;echar='\\';
                }
                else {
                    if(*(s+1) == '"') {
                        ecount=1;echar='\\';
                    }
                }
            }
            break;
            default: break;
        }
        n += ecount + 1;
        if(d != NULL) {
            while(ecount) {
                *d++ = echar;
                ecount--;
            }
            *d++ = *s;
        }
        s++;
    }

    if(d != NULL) *d = '\0';
    return n;
}

#endif

int jpr_proc_info_init(jpr_proc_info *info) {
#ifdef _WIN32
    info->handle = INVALID_HANDLE_VALUE;
    info->pid = -1;
#else
    info->pid = -1;
#endif
    return 0;
}


int jpr_proc_info_wait(jpr_proc_info *info, int *e) {
#ifdef _WIN32
    DWORD exitCode;
    if(info->handle == INVALID_HANDLE_VALUE) return 1;
    if(WaitForSingleObject(info->handle,INFINITE) != 0) {
        return 1;
    }
    if(!GetExitCodeProcess(info->handle,&exitCode)) {
        return 1;
    }
    jpr_proc_info_init(info);
    *e = (int)exitCode;
    return 0;
#else
    int st;
    if(info->pid == -1) return 1;
    jpr_waitpid(info->pid,&st,0);
    jpr_proc_info_init(info);
    if(WIFEXITED(st)) {
        *e = (int)WEXITSTATUS(st);
        return 0;
    }
    return 1;
#endif
}


int jpr_proc_pipe_init(jpr_proc_pipe *pipe) {
#ifdef _WIN32
    pipe->pipe = INVALID_HANDLE_VALUE;
#else
    pipe->pipe = -1;
#endif
    return 0;
}

int jpr_proc_pipe_write(jpr_proc_pipe *pipe, const char *buf, unsigned int len, unsigned int *bytesWritten) {
#ifdef _WIN32
    DWORD numBytes;
    if(WriteFile(
        pipe->pipe,
        buf,
        len,
        &numBytes,
        NULL) == 0) return 1;
    *bytesWritten = (unsigned int)numBytes;
    return 0;
#else
    int r;
    *bytesWritten = 0;
    r = jpr_write(pipe->pipe,buf,len);
    if(r < 0) return 1;
    *bytesWritten = (unsigned int)r;
    return 0;
#endif
}

int jpr_proc_pipe_read(jpr_proc_pipe *pipe, char *buf, unsigned int len,unsigned int *bytesRead) {
#ifdef _WIN32
    DWORD numBytes;
    if(ReadFile(
        pipe->pipe,
        buf,
        len,
        &numBytes,
        NULL) == 0) return 1;
    *bytesRead = (unsigned int)numBytes;
    return 0;
#else
    int r;
    *bytesRead = 0;
    r = jpr_read(pipe->pipe,buf,len);
    if(r < 0) return 1;
    *bytesRead = (unsigned int)r;
    return 0;
#endif
}

int jpr_proc_pipe_close(jpr_proc_pipe *pipe) {
#ifdef _WIN32
    BOOL r;
    if(pipe->pipe == INVALID_HANDLE_VALUE) return 0;
    r = CloseHandle(pipe->pipe);
    if(r) pipe->pipe = INVALID_HANDLE_VALUE;
    return !r;
#else
    int r;
    do {
    r = close(pipe->pipe);
    } while( (r == -1) && (errno == EINTR));
    if(r == 0) pipe->pipe = -1;
    return r;
#endif
}



int jpr_proc_spawn(jpr_proc_info *info, const char * const *argv, jpr_proc_pipe *in, jpr_proc_pipe *out, jpr_proc_pipe *err) {
    int r = 1;
#ifdef _WIN32
    char *cmdLine = NULL;
    wchar_t *wCmdLine = NULL;
    unsigned int args_len = 0;

    SECURITY_ATTRIBUTES *sa = NULL;
    PROCESS_INFORMATION *pi = NULL;
    STARTUPINFOW *si = NULL;

    HANDLE childStdInRd = INVALID_HANDLE_VALUE;
    HANDLE childStdInWr = INVALID_HANDLE_VALUE;
    HANDLE childStdOutRd = INVALID_HANDLE_VALUE;
    HANDLE childStdOutWr = INVALID_HANDLE_VALUE;
    HANDLE childStdErrRd = INVALID_HANDLE_VALUE;
    HANDLE childStdErrWr = INVALID_HANDLE_VALUE;

    const char * const *p = argv;

    if(info->pid != -1) return r;

    sa = (SECURITY_ATTRIBUTES *)JPRP_MALLOC(sizeof(SECURITY_ATTRIBUTES));
    if(sa == NULL) {
        goto error;
    }

    pi = (PROCESS_INFORMATION *)JPRP_MALLOC(sizeof(PROCESS_INFORMATION));
    if(pi == NULL) {
        goto error;
    }

    si = (STARTUPINFOW *)JPRP_MALLOC(sizeof(STARTUPINFOW));
    if(si == NULL) {
        goto error;
    }
    JPRP_MEMSET(sa,0,sizeof(SECURITY_ATTRIBUTES));
    JPRP_MEMSET(pi,0,sizeof(PROCESS_INFORMATION));
    JPRP_MEMSET(si,0,sizeof(STARTUPINFOW));

    while(*p != NULL) {
        args_len += jpr_strcat_escape(NULL,*p) + 3; /* +1 space, +2 quote */
        p++;
    }
    args_len += 25; /* null terminator, plus the api guide suggests having extra memory or something */

    cmdLine = (char *)JPRP_MALLOC(args_len);
    if(cmdLine == NULL) {
        goto error;
    }
    cmdLine[0] = 0;

    p = argv;
    JPRP_STRCAT(cmdLine,"\"");
    jpr_strcat_escape(cmdLine,*p);
    JPRP_STRCAT(cmdLine,"\"");
    p++;

    while(*p != NULL) {
        JPRP_STRCAT(cmdLine," \"");
        jpr_strcat_escape(cmdLine,*p);
        JPRP_STRCAT(cmdLine,"\"");
        p++;
    }

    args_len = MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,cmdLine,-1,NULL,0);
    wCmdLine = (wchar_t *)JPRP_MALLOC(args_len * sizeof(wchar_t));
    if(wCmdLine == NULL) {
        goto error;
    }
    JPRP_MEMSET(wCmdLine,0,args_len * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,cmdLine,args_len,wCmdLine,args_len);
    JPRP_FREE(cmdLine);

    sa->nLength = sizeof(SECURITY_ATTRIBUTES);
    sa->lpSecurityDescriptor = NULL;
    sa->bInheritHandle = TRUE;

    if(in != NULL) {
        if(in->pipe == INVALID_HANDLE_VALUE) {
            if(!CreatePipe(&childStdInRd, &childStdInWr, sa, 0)) {
                goto error;
            }

            if(!SetHandleInformation(childStdInWr, HANDLE_FLAG_INHERIT, 0)) {
                goto error;
            }
            in->pipe = childStdInWr;
        }
        else {
            if(!DuplicateHandle(GetCurrentProcess(),in->pipe,GetCurrentProcess(),&childStdInRd,0,TRUE,DUPLICATE_SAME_ACCESS)) {
                goto error;
            }
        }
    }

    if(out != NULL) {
        if(out->pipe == INVALID_HANDLE_VALUE) {
            if(!CreatePipe(&childStdOutRd, &childStdOutWr, sa, 0)) {
                goto error;
            }

            if(!SetHandleInformation(childStdOutRd, HANDLE_FLAG_INHERIT, 0)) {
                goto error;
            }
            out->pipe = childStdOutRd;
        }
        else {
            if(!DuplicateHandle(GetCurrentProcess(),out->pipe,GetCurrentProcess(),&childStdOutWr,0,TRUE,DUPLICATE_SAME_ACCESS)) {
                goto error;
            }
        }
    }

    if(err != NULL) {
        if(err->pipe == INVALID_HANDLE_VALUE) {
            if(!CreatePipe(&childStdErrRd, &childStdErrWr, sa, 0)) {
                goto error;
            }

            if(!SetHandleInformation(childStdErrRd, HANDLE_FLAG_INHERIT, 0)) {
                goto error;
            }
            err->pipe = childStdErrRd;
        }
        else {
            if(!DuplicateHandle(GetCurrentProcess(),err->pipe,GetCurrentProcess(),&childStdErrWr,0,TRUE,DUPLICATE_SAME_ACCESS)) {
                goto error;
            }
        }
    }

    si->cb = sizeof(STARTUPINFOW);
    si->dwFlags |= STARTF_USESTDHANDLES;

    if(childStdInRd != INVALID_HANDLE_VALUE) {
        si->hStdInput = childStdInRd;
    } else {
        si->hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }

    if(childStdOutWr != INVALID_HANDLE_VALUE) {
        si->hStdOutput = childStdOutWr;
    } else {
        si->hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    if(childStdErrWr != INVALID_HANDLE_VALUE) {
        si->hStdError = childStdErrWr;
    } else {
        si->hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }

    if(!CreateProcessW(NULL,
        wCmdLine,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        si,
        pi)) goto error;

    CloseHandle(pi->hThread);

    info->handle = pi->hProcess;
    info->pid = pi->dwProcessId;

    if(childStdInRd != INVALID_HANDLE_VALUE) {
        if(childStdInRd != in->pipe) {
            CloseHandle(childStdInRd);
        }
    }

    if(childStdOutWr != INVALID_HANDLE_VALUE) {
        if(childStdOutWr != out->pipe) {
            CloseHandle(childStdOutWr);
        }
    }

    if(childStdErrWr != INVALID_HANDLE_VALUE) {
        if(childStdErrWr != err->pipe) {
            CloseHandle(childStdErrWr);
        }
    }

    r = 0;
    goto success;

#else
    char *path;
    char argv0[4096];
    int argv0len;
    pid_t pid;
    char *t;
    int in_fds[2] = { -1, -1 };
    int out_fds[2] = { -1, -1 };
    int err_fds[2] = { -1, -1 };

    if(info->pid != -1) return r;
    path = NULL;
    argv0[0] = '\0';
    argv0len = 0;
    pid = -1;
    t = NULL;

    path = getenv("PATH");
    if(path == NULL) path = "/usr/bin:/usr/sbin:/bin:/sbin";

    if(in != NULL) {
        if(in->pipe == -1) {
            if(pipe(in_fds) != 0) goto error;
            if(jpr_coe(in_fds[1]) == -1) goto error;
            in->pipe = in_fds[1];
        } else {
            in_fds[0] = in->pipe;
        }
    }

    if(out != NULL) {
        if(out->pipe == -1) {
            if(pipe(out_fds) != 0) goto error;
            if(jpr_coe(out_fds[0]) == -1) goto error;
            out->pipe = out_fds[0];
        } else {
            out_fds[1] = out->pipe;
        }
    }

    if(err != NULL) {
        if(err->pipe == -1) {
            if(pipe(err_fds) != 0) goto error;
            if(jpr_coe(err_fds[0]) == -1) goto error;
            err->pipe = err_fds[0];
        } else {
            err_fds[1] = err->pipe;
        }
    }

    pid = fork();
    if(pid == -1) {
        goto error;
    }
    if(pid == 0) {
        if(in_fds[0] != -1) {
            jpr_dup2(in_fds[0],0);
            close(in_fds[0]);
        }

        if(out_fds[1] != -1) {
            jpr_dup2(out_fds[1],1);
            close(out_fds[1]);
        }

        if(err_fds[1] != -1) {
            jpr_dup2(err_fds[1],2);
            close(err_fds[1]);
        }

        if(strchr(argv[0],'/') == NULL) {
            while(path) {
                JPRP_STRCPY(argv0,path);
                t = strchr(argv0,':');
                if(t != NULL) {
                    *t = '\0';
                }

                if(JPRP_STRLEN(argv0)) {
                    JPRP_STRCAT(argv0,"/");
                }
                if(JPRP_STRLEN(argv0) + argv0len < 4095) {
                    JPRP_STRCAT(argv0,argv[0]);
                }
                execve(argv0,(char * const*)argv,environ);

                if(errno != ENOENT) exit(1);
                t = strchr(path,':');
                if(t) {
                    path = t + 1;
                }
                else {
                    path = 0;
                }
            }
            exit(1);
        }
        else {
            execve(argv[0],(char * const*)argv,environ);
            exit(1);
        }
    }

    if(in_fds[0] != -1) {
        close(in_fds[0]);
    }
    if(out_fds[1] != -1) {
        close(out_fds[1]);
    }
    if(err_fds[1] != -1) {
        close(err_fds[1]);
    }

    info->pid = pid;
    r = 0;
    goto success;
#endif

error:
#ifdef _WIN32
    if(childStdInRd != INVALID_HANDLE_VALUE) {
        CloseHandle(childStdInRd);
    }

    if(childStdInWr != INVALID_HANDLE_VALUE) {
        CloseHandle(childStdInWr);
    }

    if(childStdOutRd != INVALID_HANDLE_VALUE) {
        CloseHandle(childStdOutRd);
    }

    if(childStdOutWr != INVALID_HANDLE_VALUE) {
        CloseHandle(childStdOutWr);
    }

    if(childStdErrRd != INVALID_HANDLE_VALUE) {
        CloseHandle(childStdErrRd);
    }

    if(childStdErrWr != INVALID_HANDLE_VALUE) {
        CloseHandle(childStdErrWr);
    }
#else
    if(in_fds[0] > -1) close(in_fds[0]);
    if(in_fds[1] > -1) close(in_fds[1]);
    if(out_fds[0] > -1) close(out_fds[0]);
    if(out_fds[1] > -1) close(out_fds[1]);
    if(err_fds[0] > -1) close(err_fds[0]);
    if(err_fds[1] > -1) close(err_fds[1]);
#endif
    info = NULL;

success:
#ifdef _WIN32
    if(cmdLine != NULL)  JPRP_FREE(cmdLine);
    if(wCmdLine != NULL) JPRP_FREE(wCmdLine);
    if(sa != NULL)       JPRP_FREE(sa);
    if(pi != NULL)       JPRP_FREE(pi);
    if(si != NULL)       JPRP_FREE(si);
#endif

    return r;
}

int jpr_proc_pipe_open_file(jpr_proc_pipe *pipe, const char *filename, const char *mode) {
#ifdef _WIN32
    DWORD disp = 0;
    DWORD access = 0;
    wchar_t *wFilename = NULL;
    unsigned int wLen = 0;
    switch(mode[0]) {
        case 'r': {
            access = GENERIC_READ;
            disp = OPEN_EXISTING;
            break;
        }
        case 'w': {
            access = GENERIC_WRITE;
            disp = CREATE_ALWAYS;
            break;
        }
        case 'a': {
            access = GENERIC_WRITE;
            disp = OPEN_ALWAYS;
            break;
        }
        default: return -1;
    }
    mode++;
    switch(mode[0]) {
        case '+': access = GENERIC_READ | GENERIC_WRITE;
    }

    wLen = MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,filename,-1,NULL,0);
    wFilename = (wchar_t *)JPRP_MALLOC(sizeof(wchar_t)*wLen);
    if(wFilename == NULL) return -1;
    JPRP_MEMSET(wFilename,0,sizeof(wchar_t)*wLen);
    MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,filename,wLen,wFilename,wLen);

    pipe->pipe = CreateFileW(wFilename,access,0,NULL,disp,0,0);

    JPRP_FREE(wFilename);

    if(pipe->pipe == INVALID_HANDLE_VALUE) return 1;
    if(disp == OPEN_ALWAYS) {
        SetFilePointer(pipe->pipe,0,0,FILE_END);
    }
    return 0;
#else
    int flags = 0;
    int wr = 0;
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
        default: return 1;
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
    pipe->pipe = jpr_open(filename,flags,0666);
    if(pipe->pipe == -1) return 1;
    return 0;
#endif

}
#endif


/*

CC0 1.0 Universal

Statement of Purpose

The laws of most jurisdictions throughout the world automatically confer
exclusive Copyright and Related Rights (defined below) upon the creator and
subsequent owner(s) (each and all, an "owner") of an original work of
authorship and/or a database (each, a "Work").

Certain owners wish to permanently relinquish those rights to a Work for the
purpose of contributing to a commons of creative, cultural and scientific
works ("Commons") that the public can reliably and without fear of later
claims of infringement build upon, modify, incorporate in other works, reuse
and redistribute as freely as possible in any form whatsoever and for any
purposes, including without limitation commercial purposes. These owners may
contribute to the Commons to promote the ideal of a free culture and the
further production of creative, cultural and scientific works, or to gain
reputation or greater distribution for their Work in part through the use and
efforts of others.

For these and/or other purposes and motivations, and without any expectation
of additional consideration or compensation, the person associating CC0 with a
Work (the "Affirmer"), to the extent that he or she is an owner of Copyright
and Related Rights in the Work, voluntarily elects to apply CC0 to the Work
and publicly distribute the Work under its terms, with knowledge of his or her
Copyright and Related Rights in the Work and the meaning and intended legal
effect of CC0 on those rights.

1. Copyright and Related Rights. A Work made available under CC0 may be
protected by copyright and related or neighboring rights ("Copyright and
Related Rights"). Copyright and Related Rights include, but are not limited
to, the following:

  i. the right to reproduce, adapt, distribute, perform, display, communicate,
  and translate a Work;

  ii. moral rights retained by the original author(s) and/or performer(s);

  iii. publicity and privacy rights pertaining to a person's image or likeness
  depicted in a Work;

  iv. rights protecting against unfair competition in regards to a Work,
  subject to the limitations in paragraph 4(a), below;

  v. rights protecting the extraction, dissemination, use and reuse of data in
  a Work;

  vi. database rights (such as those arising under Directive 96/9/EC of the
  European Parliament and of the Council of 11 March 1996 on the legal
  protection of databases, and under any national implementation thereof,
  including any amended or successor version of such directive); and

  vii. other similar, equivalent or corresponding rights throughout the world
  based on applicable law or treaty, and any national implementations thereof.

2. Waiver. To the greatest extent permitted by, but not in contravention of,
applicable law, Affirmer hereby overtly, fully, permanently, irrevocably and
unconditionally waives, abandons, and surrenders all of Affirmer's Copyright
and Related Rights and associated claims and causes of action, whether now
known or unknown (including existing as well as future claims and causes of
action), in the Work (i) in all territories worldwide, (ii) for the maximum
duration provided by applicable law or treaty (including future time
extensions), (iii) in any current or future medium and for any number of
copies, and (iv) for any purpose whatsoever, including without limitation
commercial, advertising or promotional purposes (the "Waiver"). Affirmer makes
the Waiver for the benefit of each member of the public at large and to the
detriment of Affirmer's heirs and successors, fully intending that such Waiver
shall not be subject to revocation, rescission, cancellation, termination, or
any other legal or equitable action to disrupt the quiet enjoyment of the Work
by the public as contemplated by Affirmer's express Statement of Purpose.

3. Public License Fallback. Should any part of the Waiver for any reason be
judged legally invalid or ineffective under applicable law, then the Waiver
shall be preserved to the maximum extent permitted taking into account
Affirmer's express Statement of Purpose. In addition, to the extent the Waiver
is so judged Affirmer hereby grants to each affected person a royalty-free,
non transferable, non sublicensable, non exclusive, irrevocable and
unconditional license to exercise Affirmer's Copyright and Related Rights in
the Work (i) in all territories worldwide, (ii) for the maximum duration
provided by applicable law or treaty (including future time extensions), (iii)
in any current or future medium and for any number of copies, and (iv) for any
purpose whatsoever, including without limitation commercial, advertising or
promotional purposes (the "License"). The License shall be deemed effective as
of the date CC0 was applied by Affirmer to the Work. Should any part of the
License for any reason be judged legally invalid or ineffective under
applicable law, such partial invalidity or ineffectiveness shall not
invalidate the remainder of the License, and in such case Affirmer hereby
affirms that he or she will not (i) exercise any of his or her remaining
Copyright and Related Rights in the Work or (ii) assert any associated claims
and causes of action with respect to the Work, in either case contrary to
Affirmer's express Statement of Purpose.

4. Limitations and Disclaimers.

  a. No trademark or patent rights held by Affirmer are waived, abandoned,
  surrendered, licensed or otherwise affected by this document.

  b. Affirmer offers the Work as-is and makes no representations or warranties
  of any kind concerning the Work, express, implied, statutory or otherwise,
  including without limitation warranties of title, merchantability, fitness
  for a particular purpose, non infringement, or the absence of latent or
  other defects, accuracy, or the present or absence of errors, whether or not
  discoverable, all to the greatest extent permissible under applicable law.

  c. Affirmer disclaims responsibility for clearing rights of other persons
  that may apply to the Work or any use thereof, including without limitation
  any person's Copyright and Related Rights in the Work. Further, Affirmer
  disclaims responsibility for obtaining any necessary consents, permissions
  or other rights required for any use of the Work.

  d. Affirmer understands and acknowledges that Creative Commons is not a
  party to this document and has no duty or obligation with respect to this
  CC0 or use of the Work.

For more information, please see
<http://creativecommons.org/publicdomain/zero/1.0/>
*/
