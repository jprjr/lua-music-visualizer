#include "id3.h"
#include "utf.h"
#include "str.h"
#include "unpack.h"
#include "file.h"
#include <stdlib.h>
#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

#define ID3_MIN(a,b) ( a < b ? a : b)

static const jpr_uint8 allzero[10] = "\0\0\0\0\0\0\0\0\0\0";

struct str_alloc_s {
    char *s;
    size_t a;
};

typedef struct str_alloc_s str_alloc;

#define STR_ALLOC_ZERO { NULL, 0 }

static int str_alloc_resize(str_alloc *s, size_t size) {
    char *t = NULL;
    if(s->a >= size) return 0;
    t = realloc(s->s,size);
    if(t == NULL) {
      if(s->s != NULL) {
        free(s->s);
        s->s = NULL;
      }
      return 1;
    }
    s->a = size;
    s->s = t;
    return 0;
}

static size_t utf8_len_or_copy(jpr_uint8 *dest, const jpr_uint8 *src, size_t max) {
    size_t len = ID3_MIN(str_len((const char *)src),max);
    if(dest != NULL) {
        str_ncpy((char *)dest,(const char *)src,len);
    }
    return len;
}

void process_id3(audio_decoder *a) {
    /* does not close out the file, that's the responsibility of the
     * calling function */
    char buffer[10];
    str_alloc buffer2 = STR_ALLOC_ZERO;
    str_alloc buffer3 = STR_ALLOC_ZERO;
    size_t (*text_func)(jpr_uint8 *, const jpr_uint8 *, size_t) = NULL;
    size_t id3_size = 0;
    size_t header_size = 0;
    size_t frame_size = 0;
    size_t dec_len = 0;
    jpr_uint32 pos = 0;
    jpr_uint8 id3_ver = 0;

    buffer[0] = 0;

    pos = a->tell(a);

    if(a->read(a,buffer,10) != 10) {
        a->seek(a,pos,0);
        return;
    }
    if(str_ncmp(buffer,"ID3",3) != 0) {
        a->seek(a,pos,0);
        return;
    }

    id3_ver = (jpr_uint8)buffer[3];
    id3_size =
      (((size_t)buffer[6]) << 21) +
      (((size_t)buffer[7]) << 14) +
      (((size_t)buffer[8]) << 7 ) +
      (((size_t)buffer[9]));

    switch(id3_ver) {
        case 2: header_size = 6; break;
        case 3: header_size = 10; break;
        case 4: header_size = 10; break;
        default: {
          a->seek(a,pos,0);
          return;
        }
    }

    if(buffer[5] & 0x20) {
        if(a->read(a,buffer,6) != 6) {
            a->seek(a,pos,0);
            return;
        }
        frame_size =
          (((size_t)buffer[0]) << 21) +
          (((size_t)buffer[1]) << 14) +
          (((size_t)buffer[2]) << 7 ) +
          (((size_t)buffer[3]));
        frame_size -= 6;
        while(frame_size > 0x7FFFFFFF) {
            a->seek(a,0x7FFFFFFF,JPR_FILE_CUR);
            frame_size -= 0x7FFFFFFF;
        }
        a->seek(a,frame_size,JPR_FILE_CUR);
    }

    while(id3_size > 0) {
        if(a->read(a,buffer,header_size) != header_size) goto id3_done;
        if(mem_cmp((const jpr_uint8 *)buffer,allzero,header_size) == 0) goto id3_done;

        id3_size -= header_size;
        if(header_size == 10) {
            frame_size = unpack_uint32be((jpr_uint8 *)buffer + 4);
            buffer[4] = 0;
        } else {
            frame_size = unpack_uint24be((jpr_uint8 *)buffer+3);
            buffer[3] = 0;
        }
        id3_size -= frame_size;

        if(str_alloc_resize(&buffer2,frame_size)) goto id3_done;

        if(a->read(a,buffer2.s,frame_size) != frame_size) goto id3_done;

        if(buffer[0] != 'T') {
            continue;
        }

        if(buffer2.s == NULL) continue;

        switch(buffer2.s[0]) {
            case 0: text_func = utf_conv_iso88591_utf8; break;
            case 1: text_func = utf_conv_utf16_utf8; break;
            case 2: text_func = utf_conv_utf16be_utf8; break;
            case 3: text_func = utf8_len_or_copy; break;
            default: text_func = NULL;
        }

        if(text_func == NULL) continue;

        dec_len = text_func(NULL,(jpr_uint8 *)buffer2.s+1,frame_size-1) + 1;

        if(str_alloc_resize(&buffer3,dec_len)) goto id3_done;
        buffer3.s[text_func((jpr_uint8 *)buffer3.s,(jpr_uint8 *)buffer2.s+1,frame_size-1)] = 0;

        if(str_icmp(buffer,"tpe1") == 0) {
            a->onmeta(a->meta_ctx,"artist",buffer3.s);
        }
        else if(str_icmp(buffer,"tit2") == 0) {
            a->onmeta(a->meta_ctx,"title",buffer3.s);
        }
        else if(str_icmp(buffer,"talb") == 0) {
            a->onmeta(a->meta_ctx,"album",buffer3.s);
        }
        else if(str_icmp(buffer,"tt2") == 0) {
            a->onmeta(a->meta_ctx,"title",buffer3.s);
        }
        else if(str_icmp(buffer,"tp1") == 0) {
            a->onmeta(a->meta_ctx,"artist",buffer3.s);
        }
        else if(str_icmp(buffer,"tal") == 0) {
            a->onmeta(a->meta_ctx,"album",buffer3.s);
        }

    }

    id3_done:

    if(buffer2.s != NULL) free(buffer2.s);
    if(buffer3.s != NULL) free(buffer3.s);

    a->seek(a,pos,0);
    return;
}

