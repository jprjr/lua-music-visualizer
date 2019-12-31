#include "utf.h"
#include "unpack.h"
#include "pack.h"
#include "str.h"
#include <stddef.h>

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

static const jpr_uint8 null[4] = { 0, 0, 0, 0 };
static const jpr_uint8 bom_le_16[2] = { 0xFF, 0xFE };
static const jpr_uint8 bom_be_16[2] = { 0xFE, 0xFF };
static const jpr_uint8 bom_le_32[4] = { 0xFF, 0xFE, 0x00, 0x00 };
static const jpr_uint8 bom_be_32[4] = { 0x00, 0x00, 0xFE, 0xFF };

jpr_uint8 utf_dec_iso88591(jpr_uint32 *cp, const jpr_uint8 *s) {
    if(cp != NULL) {
        *cp = *s;
    }
    return 1;
}

jpr_uint8 utf_dec_utf8(jpr_uint32 *cp, const jpr_uint8 *s) {
    jpr_uint8 r = 0;
    jpr_uint32 t;

    if(s[0] < 0x80) {
        t = (jpr_uint32)s[0];
        r = 1;
        goto utf_dec_utf8_validate;
    }
    if( (s[0] & 0xE0) == 0xC0) {
        if ((s[1] & 0x80) != 0x80) return 0;
        t = ((jpr_uint32)(s[0] & 0x1F) << 6) |
            ((jpr_uint32)(s[1] & 0x3F));
        r = 2;
        goto utf_dec_utf8_validate;
    }
    if( (s[0] & 0xF0) == 0xE0 ) {
        if ((s[1] & 0x80) != 0x80) return 0;
        if ((s[2] & 0x80) != 0x80) return 0;
        t = ((jpr_uint32)(s[0] & 0x0F) << 12) |
            ((jpr_uint32)(s[1] & 0x3F) << 6 ) |
            ((jpr_uint32)(s[2] & 0x3F));
        r = 3;
        goto utf_dec_utf8_validate;
    }
    if( ((s[0] & 0xF8) == 0xF0) ) {
        if ((s[1] & 0x80) != 0x80) return 0;
        if ((s[2] & 0x80) != 0x80) return 0;
        if ((s[3] & 0x80) != 0x80) return 0;
        t = ((jpr_uint32)(s[0] & 0x07) << 18) |
            ((jpr_uint32)(s[1] & 0x3F) << 12) |
            ((jpr_uint32)(s[2] & 0x3F) << 6 ) |
            ((jpr_uint32)(s[3] & 0x3F));
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

static jpr_uint32 surrogate_pair(jpr_uint32 cp1, jpr_uint32 cp2) {
    if(cp2 < 0xD800 || cp2 > 0xDFFF) {
        return 0;
    }

    cp1 -= 0xD800;
    cp1 <<= 10;
    cp2 -= 0xDC00;
    return cp1 + cp2 + 0x10000;
}

static jpr_uint8 utf_dec_utf16(jpr_uint32 *cp, const jpr_uint8 *s, jpr_uint16 (*f)(const jpr_uint8 *)) {
    jpr_uint32 cp1, cp2;
    jpr_uint8 r = 0;

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

jpr_uint8 utf_dec_utf16le(jpr_uint32 *cp, const jpr_uint8 *s) {
    return utf_dec_utf16(cp,s,unpack_uint16le);
}

jpr_uint8 utf_dec_utf16be(jpr_uint32 *cp, const jpr_uint8 *s) {
    return utf_dec_utf16(cp,s,unpack_uint16be);
}

jpr_uint8 utf_dec_utf32le(jpr_uint32 *cp, const jpr_uint8 *s) {
    jpr_uint32 t = unpack_uint32le(s);
    if(t >= 0xD800 && t <= 0xDFFF) return 0;
    if(cp != NULL) *cp = t;
    return 4;
}

jpr_uint8 utf_dec_utf32be(jpr_uint32 *cp, const jpr_uint8 *s) {
    jpr_uint32 t = unpack_uint32be(s);
    if(t >= 0xD800 && t <= 0xDFFF) return 0;
    if(cp != NULL) *cp = t;
    return 4;
}

jpr_uint8 utf_enc_iso88591(jpr_uint8 *d, jpr_uint32 cp) {
    if(cp < 0x100) {
        if(d != NULL) *d = (jpr_uint8) cp;
        return 1;
    }
    return 0;
}

static jpr_uint8 utf_enc_utf16(jpr_uint8 *d, jpr_uint32 cp, jpr_uint8 (*f)(jpr_uint8 *,jpr_uint16)) {
    jpr_uint32 cp1, cp2;
    if(cp > 0x10FFFF) return 0;
    if(cp >= 0xD800 && cp <= 0xDFFF) return 0;

    if(cp < 0x010000) {
        return f(d,(jpr_uint16)cp);
    }

    cp -= 0x10000;
    cp1 = (cp >> 10) + 0xD800;
    cp2 = (cp & 0x000003FF) + 0xDC00;
    return f(d,(jpr_uint16)cp1) + f( (d == NULL ? NULL : d+2 ) ,(jpr_uint16)cp2);
}

static jpr_uint8 utf_enc_utf16w(wchar_t *d, jpr_uint32 cp) {
    jpr_uint32 cp1, cp2;
    if(cp > 0x10FFFF) return 0;
    if(cp >= 0xD800 && cp <= 0xDFFF) return 0;

    if(cp < 0x010000) {
		if(d != NULL) {
			d[0] = (wchar_t)cp;
		}
        return 1;
    }

    cp -= 0x10000;
    cp1 = (cp >> 10) + 0xD800;
    cp2 = (cp & 0x000003FF) + 0xDC00;
    if(d != NULL) {
        d[0] = (wchar_t)cp1;
        d[1] = (wchar_t)cp2;
    }
    return 2;
}

jpr_uint8 utf_enc_utf16le(jpr_uint8 *d, jpr_uint32 cp) {
    return utf_enc_utf16(d,cp,pack_uint16le);
}

jpr_uint8 utf_enc_utf16be(jpr_uint8 *d, jpr_uint32 cp) {
    return utf_enc_utf16(d,cp,pack_uint16be);
}

jpr_uint8 utf_enc_utf32le(jpr_uint8 *d, jpr_uint32 cp) {
    if(cp > 0x10FFFF) return 0;
    if(cp < 0xD800 || cp > 0xDFFF) return pack_uint32le(d,cp);
    return 0;
}

jpr_uint8 utf_enc_utf32be(jpr_uint8 *d, jpr_uint32 cp) {
    if(cp > 0x10FFFF) return 0;
    if(cp < 0xD800 || cp > 0xDFFF) return pack_uint32be(d,cp);
    return 0;
}

jpr_uint8 utf_enc_utf8(jpr_uint8 *d, jpr_uint32 cp) {
    if(cp < 0x80) {
        if(d != NULL) *d = (jpr_uint8)cp;
        return 1;
    }
    if(cp < 0x800) {
        if(d != NULL) {
          *d++ = (jpr_uint8)((cp >> 6  )        ) | 0xC0;
          *d   = (jpr_uint8)((cp       ) & 0x3F ) | 0x80;
        }
        return 2;
    }
    if(cp < 0x10000) {
        if(cp >= 0xD800 && cp <= 0xDFFF) return 0;
        if(d != NULL) {
          *d++ = (jpr_uint8)((cp >> 12 )        ) | 0xE0;
          *d++ = (jpr_uint8)((cp >> 6  ) & 0x3F ) | 0x80;
          *d   = (jpr_uint8)((cp       ) & 0x3F ) | 0x80;
        }
        return 3;
    }
    if(cp < 0x110000) {
        if(d != NULL) {
          *d++ = (jpr_uint8)((cp >> 18 )        ) | 0xF0;
          *d++ = (jpr_uint8)((cp >> 12 ) & 0x3F ) | 0x80;
          *d++ = (jpr_uint8)((cp >> 6  ) & 0x3F ) | 0x80;
          *d   = (jpr_uint8)((cp       ) & 0x3F ) | 0x80;
        }
        return 4;
    }
    return 0;
}

static size_t get_utf16_len(const jpr_uint8 *src) {
    const jpr_uint8 *s = src;
    while(mem_cmp(s,null,2)) {
        s+= 2;
    }
    return s - src;
}

static size_t get_utf16w_len(const wchar_t *src) {
    return wstr_len(src);
}

static size_t get_utf32_len(const jpr_uint8 *src) {
    const jpr_uint8 *s = src;
    while(mem_cmp(s,null,4)) {
        s+= 4;
    }
    return s - src;
}

static size_t get_utf8_len(const jpr_uint8 *src) {
    return str_len((const char *)src);
}

typedef size_t (*len_func)(const jpr_uint8 *);
typedef size_t (*lenw_func)(const wchar_t *);
typedef jpr_uint8 (*dec_func)(jpr_uint32 *, const jpr_uint8 *);
typedef jpr_uint8 (*enc_func)(jpr_uint8 *, jpr_uint32);
typedef jpr_uint8 (*encw_func)(wchar_t *, jpr_uint32);

static size_t utf_conv(jpr_uint8 *dest, const jpr_uint8 *src, size_t len, dec_func dec, enc_func enc, len_func _len) {
    const jpr_uint8 *s = src;
    jpr_uint8 *d = dest;
    jpr_uint32 cp = 0;
    size_t r = 0;
    size_t n = 0;

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

static size_t utf_convw(jpr_uint8 *dest, const wchar_t *src, size_t len, enc_func enc, lenw_func _len) {
    const wchar_t *s = src;
    jpr_uint8 *d = dest;
    jpr_uint32 cp = 0;
    wchar_t cp1 = 0;
    wchar_t cp2 = 0;
    size_t r = 0;
    size_t n = 0;

    if(len == 0) {
        len = _len(src);
    }

    while(len > 0) {
        cp1 = s[0];
        if(cp1 >= 0xD800 && cp1 <= 0xDFFF) {
            cp2 = s[1];
            cp = surrogate_pair((jpr_uint32)cp1,(jpr_uint32)cp2);
            if(cp == 0) return 0;
            s++;
            len--;
        }
        else {
            cp = (jpr_uint32)cp1;
        }

        len--;
        s++;
        if( (r = enc(d,cp)) == 0 ) break;
        if(d != NULL) d += r;
        n += r;
    }

    return n;
}

static size_t utf_wconv(wchar_t *dest, const jpr_uint8 *src, size_t len, encw_func enc, dec_func dec, len_func _len) {
    const jpr_uint8 *s = src;
    wchar_t *d = dest;
    jpr_uint32 cp = 0;
    size_t r = 0;
    size_t n = 0;

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

size_t utf_conv_iso88591_utf8(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    return utf_conv(dest,src,len, utf_dec_iso88591, utf_enc_utf8, get_utf8_len);
}

size_t utf_conv_utf16le_utf8(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    return utf_conv(dest,src,len, utf_dec_utf16le, utf_enc_utf8, get_utf16_len);
}

size_t utf_conv_utf16be_utf8(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    return utf_conv(dest,src,len, utf_dec_utf16be, utf_enc_utf8, get_utf16_len);
}

size_t utf_conv_utf32le_utf8(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    return utf_conv(dest,src,len, utf_dec_utf32le, utf_enc_utf8, get_utf32_len);
}

size_t utf_conv_utf32be_utf8(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    return utf_conv(dest,src,len, utf_dec_utf32be, utf_enc_utf8, get_utf32_len);
}

size_t utf_conv_utf8_iso88591(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    return utf_conv(dest,src,len,utf_dec_utf8,utf_enc_iso88591,get_utf8_len);
}

size_t utf_conv_utf8_utf16le(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    return utf_conv(dest,src,len,utf_dec_utf8,utf_enc_utf16le,get_utf8_len);
}

size_t utf_conv_utf8_utf16be(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    return utf_conv(dest,src,len,utf_dec_utf8,utf_enc_utf16be,get_utf8_len);
}

size_t utf_conv_utf8_utf32le(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    return utf_conv(dest,src,len,utf_dec_utf8,utf_enc_utf32le,get_utf8_len);
}

size_t utf_conv_utf8_utf32be(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    return utf_conv(dest,src,len,utf_dec_utf8,utf_enc_utf32be,get_utf8_len);
}

size_t utf_conv_utf8_utf16(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    size_t r = utf_conv( (dest == NULL ? NULL : dest + 2) ,src,len,utf_dec_utf8,utf_enc_utf16le,get_utf8_len);
    if(r > 0) {
        r += 2;
        if(dest != NULL) {
          dest[0] = 0xFF;
          dest[1] = 0xFE;
        }
    }
    return r;
}

size_t utf_conv_utf8_utf32(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    size_t r = utf_conv( ( dest == NULL ? NULL : dest + 4),src,len,utf_dec_utf8,utf_enc_utf32le,get_utf8_len);
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

size_t utf_conv_utf8_utf16w(wchar_t *dest, const jpr_uint8 *src, size_t len) {
    return utf_wconv(dest,src,len, utf_enc_utf16w, utf_dec_utf8, get_utf8_len);
}

size_t utf_conv_utf16w_utf8(jpr_uint8 *dest, const wchar_t *src, size_t len) {
    return utf_convw(dest,src,len, utf_enc_utf8, get_utf16w_len);
}

size_t utf_conv_utf16_utf8(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    if(len % 2) return 0;
    if(len == 2) return 0;
    if(len) len -= 2;
    if(mem_cmp(src,bom_le_16,2) == 0)
        return utf_conv(dest,src+2,len, utf_dec_utf16le, utf_enc_utf8, get_utf16_len);
    if(mem_cmp(src,bom_be_16,2) == 0)
        return utf_conv(dest,src+2,len, utf_dec_utf16be, utf_enc_utf8, get_utf16_len);
    return 0;
}

size_t utf_conv_utf32_utf8(jpr_uint8 *dest, const jpr_uint8 *src, size_t len) {
    if(len % 4) return 0;
    if(len == 4) return 0;
    if(len) len -= 4;
    if(mem_cmp(src,bom_le_32,4) == 0)
        return utf_conv(dest,src+4,len, utf_dec_utf32le, utf_enc_utf8, get_utf32_len);
    if(mem_cmp(src,bom_be_32,4) == 0)
        return utf_conv(dest,src+4,len, utf_dec_utf32be, utf_enc_utf8, get_utf32_len);
    return 0;
}
