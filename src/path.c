#include "norm.h"
#include "path.h"
#include "utf.h"

#ifdef JPR_WINDOWS
#include "mem.h"
#else
#include <unistd.h>
#endif

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

