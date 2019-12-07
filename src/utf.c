#include "utf.h"
#include "unpack.h"
#include "pack.h"
#include "str.h"
#include <stddef.h>

#include <stdio.h>

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

static const uint8_t null[4] = { 0, 0, 0, 0 };
static const uint8_t bom_le_16[2] = { 0xFF, 0xFE };
static const uint8_t bom_be_16[2] = { 0xFE, 0xFF };
static const uint8_t bom_le_32[4] = { 0xFF, 0xFE, 0x00, 0x00 };
static const uint8_t bom_be_32[4] = { 0x00, 0x00, 0xFE, 0xFF };

uint8_t utf_dec_iso88591(uint32_t *cp, const uint8_t *s) {
    if(cp != NULL) {
        *cp = *s;
    }
    return 1;
}

uint8_t utf_dec_utf8(uint32_t *cp, const uint8_t *s) {
    uint8_t r = 0;
    uint32_t t;

    if(s[0] < 0x80) {
        t = (uint32_t)s[0];
        r = 1;
        goto utf_dec_utf8_validate;
    }
    if( (s[0] & 0xE0) == 0xC0) {
        if ((s[1] & 0x80) != 0x80) return 0;
        t = ((uint32_t)(s[0] & 0x1F) << 6) |
            ((uint32_t)(s[1] & 0x3F));
        r = 2;
        goto utf_dec_utf8_validate;
    }
    if( (s[0] & 0xF0) == 0xE0 ) {
        if ((s[1] & 0x80) != 0x80) return 0;
        if ((s[2] & 0x80) != 0x80) return 0;
        t = ((uint32_t)(s[0] & 0x0F) << 12) |
            ((uint32_t)(s[1] & 0x3F) << 6 ) |
            ((uint32_t)(s[2] & 0x3F));
        r = 3;
        goto utf_dec_utf8_validate;
    }
    if( ((s[0] & 0xF8) == 0xF0) ) {
        if ((s[1] & 0x80) != 0x80) return 0;
        if ((s[2] & 0x80) != 0x80) return 0;
        if ((s[3] & 0x80) != 0x80) return 0;
        t = ((uint32_t)(s[0] & 0x07) << 18) |
            ((uint32_t)(s[1] & 0x3F) << 12) |
            ((uint32_t)(s[2] & 0x3F) << 6 ) |
            ((uint32_t)(s[3] & 0x3F));
        r = 4;
        goto utf_dec_utf8_validate;
    }

    return 0;

    utf_dec_utf8_validate:
    if(t >= 0xD800 && t <= 0xDFFF) {
        return 0;
    }
    if(cp != NULL) *cp = t;
    return r;
}

static uint32_t surrogate_pair(uint32_t cp1, uint32_t cp2) {
    if(cp2 < 0xD800 || cp2 > 0xDFFF) {
        return 0;
    }

    cp1 -= 0xD800;
    cp1 <<= 10;
    cp2 -= 0xDC00;
    return cp1 + cp2 + 0x10000;
}

static uint8_t utf_dec_utf16(uint32_t *cp, const uint8_t *s, uint16_t (*f)(const uint8_t *)) {
    uint32_t cp1, cp2;
    uint8_t r = 0;

    cp1 = f(s);
    if(cp1 >= 0xD800 && cp1 <= 0xDFFF) {
        cp2 = f(s+2);
        cp1 = surrogate_pair(cp1,cp2);
        if(cp1 != 0) r = 4;
    }
    else {
        r = 2;
    }
    if(cp != NULL) *cp = cp1;
    return r;
}

uint8_t utf_dec_utf16le(uint32_t *cp, const uint8_t *s) {
    return utf_dec_utf16(cp,s,unpack_uint16le);
}

uint8_t utf_dec_utf16be(uint32_t *cp, const uint8_t *s) {
    return utf_dec_utf16(cp,s,unpack_uint16be);
}

uint8_t utf_dec_utf32le(uint32_t *cp, const uint8_t *s) {
    uint32_t t = unpack_uint32le(s);
    if(t >= 0xD800 && t <= 0xDFFF) return 0;
    if(cp != NULL) *cp = t;
    return 4;
}

uint8_t utf_dec_utf32be(uint32_t *cp, const uint8_t *s) {
    uint32_t t = unpack_uint32be(s);
    if(t >= 0xD800 && t <= 0xDFFF) return 0;
    if(cp != NULL) *cp = t;
    return 4;
}

uint8_t utf_enc_iso88591(uint8_t *d, uint32_t cp) {
    if(cp < 0x100) {
        if(d != NULL) *d = (uint8_t) cp;
        return 1;
    }
    return 0;
}

static uint8_t utf_enc_utf16(uint8_t *d, uint32_t cp, uint8_t (*f)(uint8_t *,uint16_t)) {
    uint32_t cp1, cp2;
    if(cp > 0x10FFFF) return 0;
    if(cp >= 0xD800 && cp <= 0xDFFF) return 0;

    if(cp < 0x010000) {
        return f(d,cp);
    }

    cp -= 0x10000;
    cp1 = (cp >> 10) + 0xD800;
    cp2 = (cp & 0x000003FF) + 0xDC00;
    return f(d,cp1) + f( (d == NULL ? NULL : d+2 ) ,cp2);
}


uint8_t utf_enc_utf16le(uint8_t *d, uint32_t cp) {
    return utf_enc_utf16(d,cp,pack_uint16le);
}

uint8_t utf_enc_utf16be(uint8_t *d, uint32_t cp) {
    return utf_enc_utf16(d,cp,pack_uint16be);
}

uint8_t utf_enc_utf32le(uint8_t *d, uint32_t cp) {
    if(cp > 0x10FFFF) return 0;
    if(cp < 0xD800 || cp > 0xDFFF) return pack_uint32le(d,cp);
    return 0;
}

uint8_t utf_enc_utf32be(uint8_t *d, uint32_t cp) {
    if(cp > 0x10FFFF) return 0;
    if(cp < 0xD800 || cp > 0xDFFF) return pack_uint32be(d,cp);
    return 0;
}

uint8_t utf_enc_utf8(uint8_t *d, uint32_t cp) {
    if(cp < 0x80) {
        if(d != NULL) *d = (uint8_t)cp;
        return 1;
    }
    if(cp < 0x800) {
        if(d != NULL) {
          *d++ = ((cp >> 6  )        ) | 0xC0;
          *d   = ((cp       ) & 0x3F ) | 0x80;
        }
        return 2;
    }
    if(cp < 0x10000) {
        if(cp >= 0xD800 && cp <= 0xDFFF) return 0;
        if(d != NULL) {
          *d++ = ((cp >> 12 )        ) | 0xE0;
          *d++ = ((cp >> 6  ) & 0x3F ) | 0x80;
          *d   = ((cp       ) & 0x3F ) | 0x80;
        }
        return 3;
    }
    if(cp < 0x110000) {
        if(d != NULL) {
          *d++ = ((cp >> 18 )        ) | 0xF0;
          *d++ = ((cp >> 12 ) & 0x3F ) | 0x80;
          *d++ = ((cp >> 6  ) & 0x3F ) | 0x80;
          *d   = ((cp       ) & 0x3F ) | 0x80;
        }
        return 4;
    }
    return 0;
}

static unsigned int get_utf16_len(const uint8_t *src) {
    const uint8_t *s = src;
    while(mem_cmp(s,null,2)) {
        s+= 2;
    }
    return s - src;
}

static unsigned int get_utf32_len(const uint8_t *src) {
    const uint8_t *s = src;
    while(mem_cmp(s,null,4)) {
        s+= 4;
    }
    return s - src;
}

static unsigned int get_utf8_len(const uint8_t *src) {
    return str_len((const char *)src);
}

typedef unsigned int (*len_func)(const uint8_t *);
typedef uint8_t (*dec_func)(uint32_t *, const uint8_t *);
typedef uint8_t (*enc_func)(uint8_t *, uint32_t);

static unsigned int utf_conv(uint8_t *dest, const uint8_t *src, unsigned int len, dec_func dec, enc_func enc, len_func _len) {
    const uint8_t *s = src;
    uint8_t *d = dest;
    uint32_t cp = 0;
    unsigned int r = 0;
    unsigned int n = 0;

    if(len == 0) {
        len = _len(src);
    }

    while(len > 0) {
        if(( r = dec(&cp,s)) == 0) break;
        len-=r;
        s += r;
        if( (r = enc(d,cp)) == 0 ) break;
        if(d != NULL) d += r;
        n += r;
    }
    return n;

}

unsigned int utf_conv_iso88591_utf8(uint8_t *dest, const uint8_t *src, unsigned int len) {
    return utf_conv(dest,src,len, utf_dec_iso88591, utf_enc_utf8, get_utf8_len);
}

unsigned int utf_conv_utf16le_utf8(uint8_t *dest, const uint8_t *src, unsigned int len) {
    return utf_conv(dest,src,len, utf_dec_utf16le, utf_enc_utf8, get_utf16_len);
}

unsigned int utf_conv_utf16be_utf8(uint8_t *dest, const uint8_t *src, unsigned int len) {
    return utf_conv(dest,src,len, utf_dec_utf16be, utf_enc_utf8, get_utf16_len);
}

unsigned int utf_conv_utf32le_utf8(uint8_t *dest, const uint8_t *src, unsigned int len) {
    return utf_conv(dest,src,len, utf_dec_utf32le, utf_enc_utf8, get_utf32_len);
}

unsigned int utf_conv_utf32be_utf8(uint8_t *dest, const uint8_t *src, unsigned int len) {
    return utf_conv(dest,src,len, utf_dec_utf32be, utf_enc_utf8, get_utf32_len);
}

unsigned int utf_conv_utf8_iso88591(uint8_t *dest, const uint8_t *src, unsigned int len) {
    return utf_conv(dest,src,len,utf_dec_utf8,utf_enc_iso88591,get_utf8_len);
}

unsigned int utf_conv_utf8_utf16le(uint8_t *dest, const uint8_t *src, unsigned int len) {
    return utf_conv(dest,src,len,utf_dec_utf8,utf_enc_utf16le,get_utf8_len);
}

unsigned int utf_conv_utf8_utf16be(uint8_t *dest, const uint8_t *src, unsigned int len) {
    return utf_conv(dest,src,len,utf_dec_utf8,utf_enc_utf16be,get_utf8_len);
}

unsigned int utf_conv_utf8_utf32le(uint8_t *dest, const uint8_t *src, unsigned int len) {
    return utf_conv(dest,src,len,utf_dec_utf8,utf_enc_utf32le,get_utf8_len);
}

unsigned int utf_conv_utf8_utf32be(uint8_t *dest, const uint8_t *src, unsigned int len) {
    return utf_conv(dest,src,len,utf_dec_utf8,utf_enc_utf32be,get_utf8_len);
}

unsigned int utf_conv_utf8_utf16(uint8_t *dest, const uint8_t *src, unsigned int len) {
    unsigned int r = utf_conv( (dest == NULL ? NULL : dest + 2) ,src,len,utf_dec_utf8,utf_enc_utf16le,get_utf8_len);
    if(r > 0) {
        r += 2;
        if(dest != NULL) {
          dest[0] = 0xFF;
          dest[1] = 0xFE;
        }
    }
    return r;
}

unsigned int utf_conv_utf8_utf32(uint8_t *dest, const uint8_t *src, unsigned int len) {
    unsigned int r = utf_conv( ( dest == NULL ? NULL : dest + 4),src,len,utf_dec_utf8,utf_enc_utf32le,get_utf8_len);
    if(r > 0) {
        r += 4;
        if(dest != NULL) {
          dest[0] = 0xFF;
          dest[1] = 0xFE;
          dest[2] = 0x00;
          dest[3] = 0x00;
        }
    }
    return r;
}

unsigned int utf_conv_utf16_utf8(uint8_t *dest, const uint8_t *src, unsigned int len) {
    if(len % 2) return 0;
    if(len == 2) return 0;
    if(len) len -= 2;
    if(mem_cmp(src,bom_le_16,2) == 0)
        return utf_conv(dest,src+2,len, utf_dec_utf16le, utf_enc_utf8, get_utf16_len);
    if(mem_cmp(src,bom_be_16,2) == 0)
        return utf_conv(dest,src+2,len, utf_dec_utf16be, utf_enc_utf8, get_utf16_len);
    return 0;
}

unsigned int utf_conv_utf32_utf8(uint8_t *dest, const uint8_t *src, unsigned int len) {
    if(len % 4) return 0;
    if(len == 4) return 0;
    if(len) len -= 4;
    if(mem_cmp(src,bom_le_32,4) == 0)
        return utf_conv(dest,src+4,len, utf_dec_utf32le, utf_enc_utf8, get_utf32_len);
    if(mem_cmp(src,bom_be_32,4) == 0)
        return utf_conv(dest,src+4,len, utf_dec_utf32be, utf_enc_utf8, get_utf32_len);
    return 0;
}
