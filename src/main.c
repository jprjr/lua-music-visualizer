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
#define MAIN_SIG int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
#else
#define MAIN_SIG int main(int argc, char *argv[], char *envp[])
#endif

#include <stdlib.h>
#include <string.h>
#include "gui.h"
#include "cli.h"
#include "utf.h"

#if defined(_WIN32) || defined(_WIN64)
static char *w_to_mb(const wchar_t *src) {
    char *str;
    int width;
    str = NULL;

    width = utf_conv_utf16w_utf8(NULL,src,0);

    if(width <= 0) {
        return NULL;
    }

    str = malloc(sizeof(char) * (width + 1));
    if(str == NULL) {
        return NULL;
    }

    width = utf_conv_utf16w_utf8((uint8_t *)str,src,0);

    if(width <= 0) {
        free(str);
        return NULL;
    }

    str[width] = 0;
    return str;
}
#else
static char *w_to_mb(const char *s) {
    return strdup(s);
}
#endif

MAIN_SIG {
    int ret;
    (void)envp;
    char **newargv;
    int i;

#if defined(_WIN32) || defined(_WIN64)
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    newargv = (char **)malloc(sizeof(char *) * argc);
    if(newargv == NULL) return 1;

    for(i = 0; i < argc; i++) {
        newargv[i] = w_to_mb(argv[i]);
        if(newargv[i] == NULL) return 1;
    }
    ret = 0;

    if(argc < 2) {
        ret = gui_start(argc,newargv);
        if(ret != -1) {
            for(i = 0; i < argc; i++) {
                free(newargv[i]);
            }
            free(newargv);
            return ret;
        }
    }
    ret = cli_start(argc,newargv);
    for(i = 0; i < argc; i++) {
        free(newargv[i]);
    }
    free(newargv);
    return ret;
}
