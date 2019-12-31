#ifndef JPR_UTF_H
#define JPR_UTF_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include "int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* read bytes from s, writes codepoint to cp */
/* returns: the number of bytes decoded, or 0 on error */

jpr_uint8 utf_dec_utf8(jpr_uint32 *cp, const jpr_uint8 *s);
jpr_uint8 utf_dec_utf16le(jpr_uint32 *cp, const jpr_uint8 *s);
jpr_uint8 utf_dec_utf16be(jpr_uint32 *cp, const jpr_uint8 *s);
jpr_uint8 utf_dec_utf32le(jpr_uint32 *cp, const jpr_uint8 *s);
jpr_uint8 utf_dec_utf32be(jpr_uint32 *cp, const jpr_uint8 *s);
jpr_uint8 utf_dec_iso88591(jpr_uint32 *cp, const jpr_uint8 *s);

/* encodes a codepoint (cp) into a buffer (d) */
/* returns: the number of bytes used to encode, or 0 on error */
jpr_uint8 utf_enc_iso88591(jpr_uint8 *d, jpr_uint32 cp);
jpr_uint8 utf_enc_utf8(jpr_uint8 *d, jpr_uint32 cp);
jpr_uint8 utf_enc_utf16le(jpr_uint8 *d, jpr_uint32 cp);
jpr_uint8 utf_enc_utf16be(jpr_uint8 *d, jpr_uint32 cp);
jpr_uint8 utf_enc_utf32le(jpr_uint8 *d, jpr_uint32 cp);
jpr_uint8 utf_enc_utf32be(jpr_uint8 *d, jpr_uint32 cp);

/* convert entire strings between representations */
/* len is optional, if left out its assumed the string is null-terminated */
/* returns: the number of bytes used to encode the new string */

size_t utf_conv_iso88591_utf8(jpr_uint8 *s, const jpr_uint8 *src, size_t len);

/* convert from utf16le (etc) to utf8 */
size_t utf_conv_utf16le_utf8(jpr_uint8 *d, const jpr_uint8 *src, size_t len);
size_t utf_conv_utf16be_utf8(jpr_uint8 *d, const jpr_uint8 *src, size_t len);
size_t utf_conv_utf32le_utf8(jpr_uint8 *d, const jpr_uint8 *src, size_t len);
size_t utf_conv_utf32be_utf8(jpr_uint8 *d, const jpr_uint8 *src, size_t len);

size_t utf_conv_utf16w_utf8(jpr_uint8 *d, const wchar_t *src, size_t len);
size_t utf_conv_utf8_utf16w(wchar_t *d, const jpr_uint8 *src, size_t len);

/* same as above but checks for a byte order marker */
size_t utf_conv_utf16_utf8(jpr_uint8 *d, const jpr_uint8 *src, size_t len);
size_t utf_conv_utf32_utf8(jpr_uint8 *d, const jpr_uint8 *src, size_t len);

size_t utf_conv_utf8_iso88591(jpr_uint8 *s, const jpr_uint8 *src, size_t len);

/* convert from utf8 to utf16le (etc) */
size_t utf_conv_utf8_utf16le(jpr_uint8 *d, const jpr_uint8 *src, size_t len);
size_t utf_conv_utf8_utf16be(jpr_uint8 *d, const jpr_uint8 *src, size_t len);
size_t utf_conv_utf8_utf32le(jpr_uint8 *d, const jpr_uint8 *src, size_t len);
size_t utf_conv_utf8_utf32be(jpr_uint8 *d, const jpr_uint8 *src, size_t len);

/* same as above but inserts a byte order marker (little endian) */
size_t utf_conv_utf8_utf16(jpr_uint8 *d, const jpr_uint8 *src, size_t len);
size_t utf_conv_utf8_utf32(jpr_uint8 *d, const jpr_uint8 *src, size_t len);

#ifdef __cplusplus
}
#endif

#endif
