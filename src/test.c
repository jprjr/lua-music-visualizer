#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "char.h"
#include <ctype.h>

#include "pack.h"
#include "unpack.h"

#include "str.h"
#include "utf.h"


#define test(r) bail(r,__FILE__,__LINE__)

static void bail(int r,char *f, int line) {
    if(!r) {
        fprintf(stderr,"Error - see %s:%d\n",f,line);
        exit(1);
    }
}

static void test_char(unsigned char i, unsigned char r, unsigned char e) {
    if(r != e) {
        fprintf(stderr,"Error: input %u, received %u, expected %u\n",i,r,e);
        exit(1);
    }
}

static void test_uint64(uint64_t r, uint64_t e) {
    if(r != e ) {
        fprintf(stderr,"Error: received %llu, expected %llu\n",r,e);
        exit(1);
    }
}

static void test_int64(int64_t r, int64_t e) {
    if(r != e ) {
        fprintf(stderr,"Error: received %lld, expected %lld\n",r,e);
        exit(1);
    }
}

static void test_uint32(uint32_t r, uint32_t e) {
    if(r != e ) {
        fprintf(stderr,"Error: received %d, expected %d\n",r,e);
        exit(1);
    }
}

static void test_int32(int32_t r, int32_t e) {
    if(r != e ) {
        fprintf(stderr,"Error: received %d, expected %d\n",r,e);
        exit(1);
    }
}

static void test_uint16(uint16_t r, uint16_t e) {
    if(r != e ) {
        fprintf(stderr,"Error: received %hd, expected %hd\n",r,e);
        exit(1);
    }
}

static void test_int16(int16_t r, int16_t e) {
    if(r != e ) {
        fprintf(stderr,"Error: received %hd, expected %hd\n",r,e);
        exit(1);
    }
}


int main(void) {
    unsigned int c = 0;
    while(c < 256 ) {
        test_char((unsigned char)c, char_lower((unsigned char)c) , tolower((unsigned char)c));
        test_char((unsigned char)c, char_upper((unsigned char)c) , toupper((unsigned char)c));
        c++;
    }

    char *alphanum = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789{}[]";
    for(c=0;c<strlen(alphanum);c++) {
        test_char(c,isdigit(c),char_isdigit(c));
    }

    uint8_t t[8];
    uint8_t s[16];
    uint8_t q[8];
    uint32_t cp;

    memset(t,0xFF,8);

    test_int64(unpack_int64le(t),-1);
    test_int64(unpack_int64be(t),-1);
    test_int32(unpack_int32le(t),-1);
    test_int32(unpack_int32be(t),-1);
    test_int16(unpack_int16le(t),-1);
    test_int16(unpack_int16be(t),-1);

    test_uint64(unpack_uint64le(t),UINT64_MAX);
    test_uint64(unpack_uint64be(t),UINT64_MAX);
    test_uint32(unpack_uint32le(t),UINT32_MAX);
    test_uint32(unpack_uint32be(t),UINT32_MAX);
    test_uint16(unpack_uint16le(t),UINT16_MAX);
    test_uint16(unpack_uint16be(t),UINT16_MAX);

    memset(t,0,8);
    t[0] = 0xFF;
    t[1] = 0xFF;
    t[2] = 0xFF;
    t[3] = 0x7F;
    test_int32(unpack_int32le(t),2147483647);
    test_int32(unpack_int32be(t),-129);

    t[0] = 0x7F;
    t[3] = 0xFF;

    test_int32(unpack_int32le(t),-129);
    test_int32(unpack_int32be(t),2147483647);

    t[0] = 0xFF;
    t[1] = 0xFE;

    test_int16(unpack_int16le(t),-257);
    test_int16(unpack_int16be(t),-2);

    memset(s,0xFF,8);

    memset(t,0,8);
    test(pack_uint64le(NULL,UINT64_MAX) == 8);
    test(pack_uint64le(t,UINT64_MAX) == 8);
    test(memcmp(t,s,8) == 0);

    memset(t,0,8);
    test(pack_uint64be(NULL,UINT64_MAX) == 8);
    test(pack_uint64be(t,UINT64_MAX) == 8);
    test(memcmp(t,s,8) == 0);

    memset(t,0,8);
    test(pack_uint32le(NULL,UINT32_MAX) == 4);
    test(pack_uint32le(t,UINT32_MAX) == 4);
    test(memcmp(t,s,4) == 0);

    memset(t,0,8);
    test(pack_uint32be(NULL,UINT32_MAX) == 4);
    test(pack_uint32be(t,UINT32_MAX) == 4);
    test(memcmp(t,s,4) == 0);

    memset(t,0,8);
    test(pack_uint16le(NULL,UINT16_MAX) == 2);
    test(pack_uint16le(t,UINT16_MAX) == 2);
    test(memcmp(t,s,2) == 0);

    memset(t,0,8);
    test(pack_uint16be(NULL,UINT16_MAX) == 2);
    test(pack_uint16be(t,UINT16_MAX) == 2);
    test(memcmp(t,s,2) == 0);

    memset(s,0,8);
    memset(q,0,8);
    s[7] = 0x80;
    q[0] = 0x80;

    memset(t,0,8);
    pack_int64le(t,INT64_MIN);
    test(memcmp(t,s,8) == 0);

    memset(t,0,8);
    pack_int64be(t,INT64_MIN);
    test(memcmp(t,q,8) == 0);

    memset(s,0,8);
    memset(q,0,8);
    s[3] = 0x80;
    q[0] = 0x80;

    memset(t,0,8);
    pack_int32le(t,INT32_MIN);
    test(memcmp(t,s,4) == 0);

    memset(t,0,8);
    pack_int32be(t,INT32_MIN);
    test(memcmp(t,q,4) == 0);

    memset(s,0,8);
    memset(q,0,8);
    s[1] = 0x80;
    q[0] = 0x80;

    memset(t,0,8);
    pack_int16le(t,INT16_MIN);
    test(memcmp(t,s,2) == 0);

    memset(t,0,8);
    pack_int16be(t,INT16_MIN);
    test(memcmp(t,q,2) == 0);

    test(strcmp("abcd","abcd") == str_cmp("abcd","abcd"));
    test(strcmp("ABCD","abcd") < 0 && str_cmp("ABCD","abcd") < 0);
    test(strcmp("abcd","ABCD") > 0 && str_cmp("abcd","ABCD") > 0);
    test(strcmp("efgh","abcd") > 0 && str_cmp("efgh","abcd") > 0);
    test(strcmp("EFGH","abcd") < 0 && str_cmp("EFGH","abcd") < 0);
    test(strcmp("efgh","ABCD") > 0 && str_cmp("efgh","ABCD") > 0);
    test(strcmp("abc","abcd")  < 0 && str_cmp("abc","abcd")  < 0);
    test(strcmp("abcd","abc")  > 0 && str_cmp("abcd","abc")  > 0);

    test(strcasecmp("abcd","abcd") == str_icmp("abcd","abcd"));
    test(strcasecmp("ABCD","abcd") == str_icmp("ABCD","abcd"));
    test(strcasecmp("abcd","ABCD") == str_icmp("abcd","ABCD"));

    test(strcasecmp("efgh","abcd") > 0 && str_icmp("efgh","abcd") > 0);
    test(strcasecmp("EFGH","abcd") > 0 && str_icmp("EFGH","abcd") > 0);
    test(strcasecmp("efgh","ABCD") > 0 && str_icmp("efgh","ABCD") > 0);
    test(strcasecmp("abcd","efgh") < 0 && str_icmp("abcd","efgh") < 0);
    test(strcasecmp("ABCD","efgh") < 0 && str_icmp("ABCD","efgh") < 0);
    test(strcasecmp("abcd","EFGH") < 0 && str_icmp("abcd","EFGH") < 0);
    test(strcasecmp("abc","abcd") < 0 && str_icmp("abc","abcd") < 0);
    test(strcasecmp("abcd","abc") > 0 && str_icmp("abcd","abc") > 0);

    test(strncasecmp("efgh","abcd",3) > 0 && str_incmp("efgh","abcd",3) > 0);
    test(strncasecmp("EFGH","abcd",3) > 0 && str_incmp("EFGH","abcd",3) > 0);
    test(strncasecmp("efgh","ABCD",3) > 0 && str_incmp("efgh","ABCD",3) > 0);
    test(strncasecmp("abcd","efgh",3) < 0 && str_incmp("abcd","efgh",3) < 0);
    test(strncasecmp("ABCD","efgh",3) < 0 && str_incmp("ABCD","efgh",3) < 0);
    test(strncasecmp("abcd","EFGH",3) < 0 && str_incmp("abcd","EFGH",3) < 0);

    test(strlen("abcd") == str_len("abcd"));
    test(strlen("ab\0d") == str_len("ab\0d"));

    test(strncmp("abcd","abcd",3) == str_ncmp("abcd","abcd",3));
    test(strncasecmp("ABCD","abcd",3) == str_incmp("ABCD","abcd",3));

    memset(t,0xFF,8);
    test(str_cpy((char *)t,"abcdefg") == 7);
    test(memcmp(t,"abcdefg\0",8) == 0);

    memset(t,0xFF,8);
    test(str_ncpy((char *)t,"abcdefgh",3) == 3);
    test(memcmp(t,"abc\0",4) == 0);

    memset(t,0xFF,8);
    test(str_ncpy((char *)t,"ab",3) == 2);
    test(memcmp(t,"ab\0",3) == 0);

    memset(t,0,8);
    test(str_lower((char *)t,"AbCdEfG") == 7);
    test(memcmp(t,"abcdefg",8) == 0);

    memset(t,0,8);
    test(str_nlower((char *)t,"AbCdEfG",3) == 3);
    test(memcmp(t,"abcdefg",3) == 0);

    t[0] = 'H';
    t[1] = 'e';
    t[2] = 'l';
    t[3] = 'l';
    t[4] = 'o';
    t[5] = '\0';

    test(str_lower( (char *)t, (char *)t) == 5);
    test(memcmp(t,"hello\0",6) == 0);

    test(str_chr("abcdefg",'g') == 6);
    test(str_chr("abcdefg",'z') == 8);
    test(str_starts("AbCd","AbC"));
    test(str_starts("DeF","AbC") == 0);
    test(str_starts("AbCd","abc") == 0);
    test(str_istarts("AbCd","abc"));
    test(str_istarts("def","abc") == 0);
    test(str_ends("AbCd","bCd"));
    test(str_iends("AbCd","bcd"));

    memset(t,0,8);
    test(str_cat((char *)t,"abc") == 3);
    test(str_cat((char *)t,"def") == 3);
    test(memcmp(t,"abcdef\0",7) == 0);

    memset(t,0,8);
    test(str_ncat((char *)t,"abcdefg",6) == 6);
    test(memcmp(t,"abcdef\0",7) == 0);

    memset(t,0,8);
    test(str_ncat((char *)t,"abc",6) == 3);
    test(memcmp(t,"abc\0",4) == 0);

    cp = 0;
    memset(t,0,8);
    t[0] = 0x3C;
    t[1] = 0xD8;
    t[2] = 0x7A;
    t[3] = 0xDF;
    test(utf_dec_utf16le(NULL,t) == 4);
    test(utf_dec_utf16le(&cp,t) == 4);
    test(cp == 0x1f37a);

    cp = 0;
    memset(t,0,8);
    t[0] = 0xD8;
    t[1] = 0x3C;
    t[2] = 0xDF;
    t[3] = 0x7A;
    test(utf_dec_utf16be(NULL,t) == 4);
    test(utf_dec_utf16be(&cp,t) == 4);
    test(cp == 0x1f37a);

    cp = 0;
    memset(t,0,8);
    t[0] = 0x7A;
    t[1] = 0xF3;
    t[2] = 0x01;
    t[3] = 0x00;
    test(utf_dec_utf32le(NULL,t) == 4);
    test(utf_dec_utf32le(&cp,t) == 4);
    test(cp == 0x1F37A);

    cp = 0;
    memset(t,0,8);
    t[0] = 0x00;
    t[1] = 0x01;
    t[2] = 0xF3;
    t[3] = 0x7A;
    test(utf_dec_utf32be(NULL,t) == 4);
    test(utf_dec_utf32be(&cp,t) == 4);
    test(cp == 0x1F37A);

    memset(t,0,8);
    test(utf_enc_utf16le(NULL,0x1F37A) == 4);
    test(utf_enc_utf16le(t,0x1F37A) == 4);
    test(t[0] == 0x3C);
    test(t[1] == 0xD8);
    test(t[2] == 0x7A);
    test(t[3] == 0xDF);

    memset(t,0,8);
    test(utf_enc_utf16be(NULL,0x1F37A) == 4);
    test(utf_enc_utf16be(t,0x1F37A) == 4);
    test(t[0] == 0xD8);
    test(t[1] == 0x3C);
    test(t[2] == 0xDF);
    test(t[3] == 0x7A);

    memset(t,0,8);
    test(utf_enc_utf8(NULL,0x1F37A) == 4);
    test(utf_enc_utf8(t,0x1F37A) == 4);
    test(t[0] == 0xF0);
    test(t[1] == 0x9F);
    test(t[2] == 0x8D);
    test(t[3] == 0xBA);

    t[4] = 0;

    test(utf_dec_utf8(NULL,t) == 4);
    test(utf_dec_utf8(&cp,t) == 4);
    test(cp == 0x1F37A);

    memset(t,0,8);
    test(utf_enc_utf16le(NULL,0x1F37A) == 4);
    test(utf_enc_utf16le(t,0x1F37A) == 4);
    t[4] = '!';
    t[5] = 0;
    t[6] = 0;
    t[7] = 0;

    test(utf_conv_utf16le_utf8(NULL,t,0) == 5);
    test(utf_conv_utf16le_utf8(t,t,0) == 5);
    memset(s,0,16);
    test(utf_conv_utf8_utf16le(NULL,t,0) == 6);
    test(utf_conv_utf8_utf16be(NULL,t,0) == 6);
    test(utf_conv_utf8_utf32le(NULL,t,0) == 8);
    test(utf_conv_utf8_utf32be(NULL,t,0) == 8);

    test(utf_conv_utf8_utf16le(s,t,0) == 6);
    test(utf_conv_utf8_utf16be(s,t,0) == 6);
    test(utf_conv_utf8_utf32le(s,t,0) == 8);
    test(utf_conv_utf8_utf32be(s,t,0) == 8);

    memset(t,0,8);
    t[0] = 0xA3;
    t[1] = '!';
    t[2] = 0;
    test(utf_conv_iso88591_utf8(NULL,t,0) == 3);
    test(utf_conv_iso88591_utf8(s,t,0) == 3);
    memset(t,0,8);
    t[0] = 0xE2;
    t[1] = 0x84;
    t[2] = 0xA2;
    t[3] = 0;
    test(utf_conv_utf8_iso88591(NULL,t,0) == 0);
    test(utf_conv_utf8_iso88591(s,t,0) == 0);
    memset(t,0,8);
    memset(s,0,8);
    test(str_cpy((char *)t,"He\\o") == 4);
    test(str_ecat((char *)s,(const char *)t,"\\",'\\') == 5);

    return 0;
}

