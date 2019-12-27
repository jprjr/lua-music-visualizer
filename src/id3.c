#include "id3.h"
#include "utf.h"
#include "str.h"
#include "unpack.h"
#include "file.h"
#include "mem.h"

#define ID3_MIN(a,b) ( a < b ? a : b)

static const uint8_t allzero[10] = "\0\0\0\0\0\0\0\0\0\0";

struct str_alloc_s {
    char *s;
    unsigned int a;
};

typedef struct str_alloc_s str_alloc;

#define STR_ALLOC_ZERO { .s = NULL, .a = 0 }

static int str_alloc_resize(str_alloc *s, unsigned int size) {
    char *t = NULL;
    if(s->a >= size) return 0;
    t = mem_realloc(s->s,size);
    if(t == NULL) {
      if(s->s != NULL) {
        mem_free(s->s);
        s->s = NULL;
      }
      return 1;
    }
    s->a = size;
    s->s = t;
    return 0;
}

static unsigned int utf8_len_or_copy(uint8_t *dest, const uint8_t *src, unsigned int max) {
    unsigned int len = ID3_MIN(str_len((const char *)src),max);
    if(dest != NULL) {
        str_ncpy((char *)dest,(const char *)src,len);
    }
    return len;
}

void process_id3(audio_decoder *a, jpr_file *f) {
    /* assumption: the file has already read the 'ID3' bytes */
    /* does not close out the file, that's the responsibility of the
     * calling function */
    char buffer[10];
    buffer[0] = 0;
    str_alloc buffer2 = STR_ALLOC_ZERO;
    str_alloc buffer3 = STR_ALLOC_ZERO;
    unsigned int (*text_func)(uint8_t *, const uint8_t *, unsigned int) = NULL;

    unsigned int id3_size = 0;
    unsigned int header_size = 0;
    unsigned int frame_size = 0;
    unsigned int dec_len = 0;
    uint8_t id3_ver = 0;

    if(file_read(f,buffer,10) != 10) return;
    if(str_ncmp(buffer,"ID3",3) != 0) return;
    id3_ver = (uint8_t)buffer[3];
    id3_size =
      (((unsigned int)buffer[6]) << 21) +
      (((unsigned int)buffer[7]) << 14) +
      (((unsigned int)buffer[8]) << 7 ) +
      (((unsigned int)buffer[9]));

    switch(id3_ver) {
        case 2: header_size = 6; break;
        case 3: header_size = 10; break;
        case 4: header_size = 10; break;
        default: return;
    }


    if(buffer[5] & 0x20) {
        if(file_read(f,buffer,6) != 6) return;
        frame_size =
          (((unsigned int)buffer[0]) << 21) +
          (((unsigned int)buffer[1]) << 14) +
          (((unsigned int)buffer[2]) << 7 ) +
          (((unsigned int)buffer[3]));
        frame_size -= 6;
        file_seek(f,frame_size,JPR_FILE_CUR);
    }

    while(id3_size > 0) {
        if(file_read(f,buffer,header_size) != header_size) goto id3_done;
        if(mem_cmp((const uint8_t *)buffer,allzero,header_size) == 0) goto id3_done;

        id3_size -= header_size;
        if(header_size == 10) {
            frame_size = unpack_uint32be((uint8_t *)buffer + 4);
            buffer[4] = 0;
        } else {
            frame_size = unpack_uint24be((uint8_t *)buffer+3);
            buffer[3] = 0;
        }
        id3_size -= frame_size;

        if(str_alloc_resize(&buffer2,frame_size)) goto id3_done;

        if(file_read(f,buffer2.s,frame_size) != frame_size) goto id3_done;

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

        dec_len = text_func(NULL,(uint8_t *)buffer2.s+1,frame_size-1) + 1;

        if(str_alloc_resize(&buffer3,dec_len)) goto id3_done;
        buffer3.s[text_func((uint8_t *)buffer3.s,(uint8_t *)buffer2.s+1,frame_size-1)] = 0;

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

    if(buffer2.s != NULL) mem_free(buffer2.s);
    if(buffer3.s != NULL) mem_free(buffer3.s);

    return;
}

