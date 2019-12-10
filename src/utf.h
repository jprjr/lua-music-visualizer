#ifndef UTF_H
#define UTF_H

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* read bytes from s, writes codepoint to cp */
/* returns: the number of bytes decoded, or 0 on error */

uint8_t utf_dec_utf8(uint32_t *cp, const uint8_t *s);
uint8_t utf_dec_utf16le(uint32_t *cp, const uint8_t *s);
uint8_t utf_dec_utf16be(uint32_t *cp, const uint8_t *s);
uint8_t utf_dec_utf32le(uint32_t *cp, const uint8_t *s);
uint8_t utf_dec_utf32be(uint32_t *cp, const uint8_t *s);
uint8_t utf_dec_iso88591(uint32_t *cp, const uint8_t *s);

/* encodes a codepoint (cp) into a buffer (d) */
/* returns: the number of bytes used to encode, or 0 on error */
uint8_t utf_enc_iso88591(uint8_t *d, uint32_t cp);
uint8_t utf_enc_utf8(uint8_t *d, uint32_t cp);
uint8_t utf_enc_utf16le(uint8_t *d, uint32_t cp);
uint8_t utf_enc_utf16be(uint8_t *d, uint32_t cp);
uint8_t utf_enc_utf32le(uint8_t *d, uint32_t cp);
uint8_t utf_enc_utf32be(uint8_t *d, uint32_t cp);

/* convert entire strings between representations */
/* len is optional, if left out its assumed the string is null-terminated */
/* returns: the number of bytes used to encode the new string */

unsigned int utf_conv_iso88591_utf8(uint8_t *s, const uint8_t *src, unsigned int len);

/* convert from utf16le (etc) to utf8 */
unsigned int utf_conv_utf16le_utf8(uint8_t *d, const uint8_t *src, unsigned int len);
unsigned int utf_conv_utf16be_utf8(uint8_t *d, const uint8_t *src, unsigned int len);
unsigned int utf_conv_utf32le_utf8(uint8_t *d, const uint8_t *src, unsigned int len);
unsigned int utf_conv_utf32be_utf8(uint8_t *d, const uint8_t *src, unsigned int len);

unsigned int utf_conv_utf16w_utf8(uint8_t *d, const wchar_t *src, unsigned int len);
unsigned int utf_conv_utf8_utf16w(wchar_t *d, const uint8_t *src, unsigned int len);

/* same as above but checks for a byte order marker */
unsigned int utf_conv_utf16_utf8(uint8_t *d, const uint8_t *src, unsigned int len);
unsigned int utf_conv_utf32_utf8(uint8_t *d, const uint8_t *src, unsigned int len);

unsigned int utf_conv_utf8_iso88591(uint8_t *s, const uint8_t *src, unsigned int len);

/* convert from utf8 to utf16le (etc) */
unsigned int utf_conv_utf8_utf16le(uint8_t *d, const uint8_t *src, unsigned int len);
unsigned int utf_conv_utf8_utf16be(uint8_t *d, const uint8_t *src, unsigned int len);
unsigned int utf_conv_utf8_utf32le(uint8_t *d, const uint8_t *src, unsigned int len);
unsigned int utf_conv_utf8_utf32be(uint8_t *d, const uint8_t *src, unsigned int len);

/* same as above but inserts a byte order marker (little endian) */
unsigned int utf_conv_utf8_utf16(uint8_t *d, const uint8_t *src, unsigned int len);
unsigned int utf_conv_utf8_utf32(uint8_t *d, const uint8_t *src, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif
