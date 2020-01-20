#include "attr.h"
#include "bdf.h"
#include "text.h"
#include "str.h"
#include "scan.h"
#include <stdlib.h>

#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

static attr_inline const char *skip_whitespace(const char *p) {
    while(*p) {
        switch(*p) {
            case ' ': /* fall-through */
            case '\t': {
                p++;
                break;
            }
            default: return p;
        }
    }
    return p;
}

void bdf_free(bdf *font) {
    unsigned int i;
    if(font->glyphs != NULL) {
        for(i=0;i<=font->max_glyph;i++) {
            if(font->glyphs[i] != NULL) {
                free(font->glyphs[i]);
            }
        }
        free(font->glyphs);
    }
    if(font->widths != NULL) free(font->widths);
    free(font);
}

bdf *bdf_load(const char * RESTRICT filename) {
    jpr_text *text;
    bdf *font;
    const char *line;
    const char *l;
    unsigned int i = 0;

    unsigned int width, height, cur_width, cur_height, cur_glyph, dwidth, reading_bitmap, bitmap_line, rel_line, old_max;
    int bbx, bby, cur_bbx, cur_bby;
    void *t;

    text = jpr_text_open(filename);
    if(text == NULL) return NULL;

    font = (bdf *)malloc(sizeof(bdf));
    if(UNLIKELY(font == NULL)) {
        jpr_text_close(text);
        return NULL;
    }
    font->glyphs = NULL;
    font->widths = NULL;
    font->max_glyph = 127;
    font->total_glyphs = 0;
    reading_bitmap = 0;
    bitmap_line = 0;

    while((line = jpr_text_line(text)) != NULL) {
        i++;
        l = line;
        if(str_starts(line,"FONTBOUNDINGBOX")) {
            /* Pattern: _,_,width,height,bbx,bby = find(line,"^FONTBOUNDINGBOX%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)") */
            l += 15;
            l = skip_whitespace(l);
            l += scan_uint(l,&width);
            l = skip_whitespace(l);
            l += scan_uint(l,&height);
            l = skip_whitespace(l);
            l += scan_int(l,&bbx);
            l = skip_whitespace(l);
            l += scan_int(l,&bby);
            font->width = width;
            font->height = height;
            font->glyphs = (jpr_uint32 **)malloc(sizeof(jpr_uint32 *) * (font->max_glyph + 1));
            if(UNLIKELY(font->glyphs == NULL)) {
                bdf_free(font);
                return NULL;
            }
            memset(font->glyphs,0,sizeof(jpr_uint32 *) * (font->max_glyph + 1));

            font->widths = (unsigned int *)malloc(sizeof(unsigned int) * (font->max_glyph + 1));
            if(UNLIKELY(font->widths == NULL)) {
                bdf_free(font);
                return NULL;
            }
            memset(font->widths,0,sizeof(unsigned int) * (font->max_glyph + 1));
        }
        else if(str_starts(line,"ENCODING")) {
            l += 8;
            l = skip_whitespace(l);
            scan_uint(l,&cur_glyph);
            if(cur_glyph > font->max_glyph) {
                old_max = font->max_glyph;
                while(font->max_glyph < cur_glyph) {
                    font->max_glyph = ((font->max_glyph + 1) * 2) - 1;
                }
                t = (jpr_uint32 **)realloc(font->glyphs,sizeof(jpr_uint32 *) * (font->max_glyph + 1));
                if(UNLIKELY(t == NULL)) {
                    bdf_free(font);
                    return NULL;
                }
                font->glyphs = t;
                memset(&(font->glyphs[old_max+1]),0,sizeof(jpr_uint32 *) * (font->max_glyph - old_max));

                t = (unsigned int *)realloc(font->widths,sizeof(unsigned int) * (font->max_glyph + 1));
                if(UNLIKELY(t == NULL)) {
                    bdf_free(font);
                    return NULL;
                }
                font->widths = t;
                memset(&(font->widths[old_max+1]),0,sizeof(unsigned int) * (font->max_glyph - old_max));
            }
            font->glyphs[cur_glyph] = (jpr_uint32 *)malloc(sizeof(jpr_uint32) * height);
            memset(font->glyphs[cur_glyph],0,sizeof(jpr_uint32) * height);
        }
        else if(str_starts(line,"DWIDTH")) {
            l += 6;
            l = skip_whitespace(l);
            scan_uint(l,&dwidth);
        }
        else if(str_starts(line,"BBX")) {
            l += 3;
            l = skip_whitespace(l);
            l += scan_uint(l,&cur_width);
            l = skip_whitespace(l);
            l += scan_uint(l,&cur_height);
            l = skip_whitespace(l);
            l += scan_int(l,&cur_bbx);
            l = skip_whitespace(l);
            l += scan_int(l,&cur_bby);
         }
         else if(str_starts(line,"BITMAP")) {
             reading_bitmap = 1;
             bitmap_line = 0;
             if (cur_bbx < 0) {
                 dwidth = dwidth - cur_bbx;
                 cur_bbx = 0;
             }
             if (cur_bbx + cur_width > dwidth) {
                 dwidth = cur_bbx + cur_width;
             }
             font->widths[cur_glyph] = dwidth;
         }
         else if(str_starts(line,"ENDCHAR")) {
             reading_bitmap = 0;
             bitmap_line = 0;
         } else if(reading_bitmap) {
             rel_line = height - (cur_height - (bitmap_line - 1) - bby + cur_bby - 1);
             scan_uint32_base16(line,&(font->glyphs[cur_glyph][rel_line]));
             bitmap_line++;

         }
    }

    jpr_text_close(text);
    return font;
}

