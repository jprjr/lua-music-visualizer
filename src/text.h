#ifndef JPR_TEXT_H
#define JPR_TEXT_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include "norm.h"
#include "attr.h"
#include "int.h"

/* text-processor concept, reads lines of text (accepts \n, \r\n, and plain \r) */

typedef struct jpr_text_s jpr_text;

struct jpr_text_s {
    jpr_uint8 *data;
    jpr_uint8 *c;
    size_t len;
    size_t pos;
};

#ifdef __cplusplus
extern "C" {
#endif

jpr_text *jpr_text_open(const char * RESTRICT filename);
jpr_text *jpr_text_create(const jpr_uint8 *data, size_t len);
const char *jpr_text_line(jpr_text *);
void jpr_text_close(jpr_text *);

#ifdef __cplusplus
}
#endif

#endif
