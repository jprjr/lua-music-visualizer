#include "text.h"
#include "file.h"
#include "str.h"
#include <stdlib.h>

jpr_text *jpr_text_open(const char * RESTRICT filename) {
    jpr_text *text;
    jpr_uint8 *data;
    jpr_uint64 len;

    data = file_slurp(filename,&len);
    if(data == NULL) return NULL;
    text = jpr_text_create(data,len);
    free(data);
    return text;
}

jpr_text *jpr_text_create(const jpr_uint8 *data, jpr_uint64 len) {
    jpr_text *text;
    jpr_uint8 *data_copy;

    data_copy = (jpr_uint8 *)malloc(len + 1);
    if(UNLIKELY(data_copy == NULL)) {
        return NULL;
    }
    mem_cpy(data_copy,data,len);
    data_copy[len] = '\0';

    text = (jpr_text *)malloc(sizeof(jpr_text));
    if(UNLIKELY(text == NULL)) {
        free(data_copy);
        return NULL;
    }

    text->data = data_copy;
    text->c = &(text->data[0]);
    text->len = len;
    text->pos = 0;

    return text;
}

void jpr_text_close(jpr_text *text) {
    free(text->data);
    free(text);
}

const char *jpr_text_line(jpr_text *text) {
    jpr_uint8 *p;

    if(text->pos == text->len) return NULL;
    text->c = &(text->data[text->pos]);

    p = mem_chr(text->c,'\n',text->len - text->pos);
    if(p == NULL) {
        p = mem_chr(text->c,'\r',text->len - text->pos);
        if(p == NULL) {
            /* file has no final line ending */
            text->pos = text->len;
            return (const char *)text->c;
        }
    }

    if(p[0] == '\n') {
        if( &p[-1] >= text->c && p[-1] == '\r') {
            p[-1] = '\0';
        }
        p[0] = '\0';
    }

    text->pos += p - text->c + 1;
    return (const char *)text->c;
}
