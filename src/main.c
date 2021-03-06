#include "norm.h"

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
#endif

#include <stdlib.h>

#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#include <stdio.h>
#endif

#include "int.h"
#include "gui.h"
#include "cli.h"
#include "utf.h"
#include "str.h"

static char *w_to_mb(const wchar_t *src) {
    char *str;
    size_t width;
    str = NULL;

    width = utf_conv_utf16w_utf8(NULL,src,0);
    str = malloc(sizeof(char) * (width + 1));
    if(UNLIKELY(str == NULL)) {
        return NULL;
    }

    width = utf_conv_utf16w_utf8((jpr_uint8 *)str,src,0);
    str[width] = 0;

    return str;
}

int main(int argc, char *argv[], char *envp[]) {
    int ret;
    (void)envp;

#ifdef JPR_WINDOWS
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    ret = 0;

    if(argc < 2) {
        ret = gui_start(argc,argv);
        if(ret != -1) {
#ifdef CHECK_LEAKS
            printf("stb_leakcheck_dumpmem follows (nothing == no leaks)\n");
            stb_leakcheck_dumpmem();
#endif
            return ret;
        }
    }
    ret = cli_start(argc,argv);
#ifdef CHECK_LEAKS
    printf("stb_leakcheck_dumpmem follows (nothing == no leaks)\n");
    stb_leakcheck_dumpmem();
#endif
    return ret;
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[]) {
	int r;
	char **newargv;
	int i;

	(void)envp;
	newargv = (char **)malloc(sizeof(char *) * (argc + 1));
    if(UNLIKELY(newargv == NULL)) return 1;

    for(i = 0; i < argc; i++) {
        newargv[i] = w_to_mb(argv[i]);
        if(newargv[i] == NULL) return 1;
    }
    newargv[argc] = NULL;

	r = main(argc, newargv, NULL);

	for(i = 0; i < argc; i++) {
        free(newargv[i]);
    }
    free(newargv);

	return r;
}


