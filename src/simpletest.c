#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "int.h"
#include "char.h"
#include "pack.h"
#include "unpack.h"
#include "str.h"
#include "utf.h"
#include "fmt.h"
#include "scan.h"
#include "file.h"
#include "mem.h"
#include "dir.h"
#include "norm.h"
#include "util.h"
#include "path.h"

void my_quit(unsigned int lineno) {
    fprintf(stderr,"bailing on line %u\n",lineno);
    fflush(stderr);
#ifdef JPR_WINDOWS
    ExitProcess(1);
#else
    exit(1);
#endif
}

#define QUIT(x) my_quit(x)

/* Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/ */
#ifdef JPR_WINDOWS
#define STRCASECMP(x,y) _stricmp(x,y)
#define STRNCASECMP(x,y,n) _strnicmp(x,y,n)
#define STRDUP(x) _strdup(x)
#else
#define STRCASECMP(x,y) strcasecmp(x,y)
#define STRNCASECMP(x,y,n) strncasecmp(x,y,n)
#define STRDUP(x) strdup(x)
#endif

#define BUF_SIZE 100


#define ASSERT_TRUE(x) if(!(x)) QUIT(__LINE__)
#define ASSERT_FALSE(x) if((x)) QUIT(__LINE__)
#define ASSERT_EQ(x,y) if((x) != (y)) QUIT(__LINE__)

#define dirname(x) x
#define basename(x) x


void utest_str_memchr(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    char *memchr_test;
    static const char *memchr_data = "abcd";
    memchr_test = memchr(memchr_data,'c',4);
    ASSERT_TRUE(&memchr_data[2] == memchr_test);
    memchr_test = mem_chr(memchr_data,'c',4);
    ASSERT_TRUE(&memchr_data[2] == memchr_test);
}

void utest_str_memset(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    ASSERT_TRUE(memset(t,0,BUF_SIZE) == mem_set(t,0,BUF_SIZE));
    free(t); free(s); free(q);
}

void utest_str_cmp_eq(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(strcmp("abcd","abcd") == str_cmp("abcd","abcd"));
}
void utest_str_cmp_case_lt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(strcmp("ABCD","abcd") < 0 && str_cmp("ABCD","abcd") < 0);
}
void utest_str_cmp_case_gt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(strcmp("abcd","ABCD") > 0 && str_cmp("abcd","ABCD") > 0);
}
void utest_str_cmp_diff_gt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(strcmp("efgh","abcd") > 0 && str_cmp("efgh","abcd") > 0);
}
void utest_str_cmp_diff_lt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(strcmp("EFGH","abcd") < 0 && str_cmp("EFGH","abcd") < 0);
}
void utest_str_cmp_diff_case_gt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(strcmp("efgh","ABCD") > 0 && str_cmp("efgh","ABCD") > 0);
}
void utest_str_cmp_len_lt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(strcmp("abc","abcd")  < 0 && str_cmp("abc","abcd")  < 0);
}
void utest_str_cmp_len_gt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(strcmp("abcd","abc")  > 0 && str_cmp("abcd","abc")  > 0);
}

void utest_str_icmp_eq(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRCASECMP("abcd","abcd") == str_icmp("abcd","abcd"));
    ASSERT_TRUE(STRCASECMP("abcd","abcD") == str_icmp("abcd","abcD"));
    ASSERT_TRUE(STRCASECMP("abcd","defg") < 0);
    ASSERT_TRUE(str_icmp("abcd","defg") < 0);
    ASSERT_TRUE(STRCASECMP("defg","abcd") > 0);
    ASSERT_TRUE(str_icmp("defg","abcd") > 0);
}
void utest_str_icmp_case_eq(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRCASECMP("ABCD","abcd") == str_icmp("ABCD","abcd"));
}
void utest_str_icmp_case_swap_eq(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRCASECMP("abcd","ABCD") == str_icmp("abcd","ABCD"));
}

void utest_str_icmp_diff_gt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRCASECMP("efgh","abcd") > 0 && str_icmp("efgh","abcd") > 0);
}
void utest_str_icmp_diff_case_gt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRCASECMP("EFGH","abcd") > 0 && str_icmp("EFGH","abcd") > 0);
}
void utest_str_icmp_diff_case_swap_gt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRCASECMP("efgh","ABCD") > 0 && str_icmp("efgh","ABCD") > 0);
}
void utest_str_icmp_diff_lt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRCASECMP("abcd","efgh") < 0 && str_icmp("abcd","efgh") < 0);
}
void utest_str_icmp_diff_case_lt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRCASECMP("ABCD","efgh") < 0 && str_icmp("ABCD","efgh") < 0);
}
void utest_str_icmp_diff_case_swap_lt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRCASECMP("abcd","EFGH") < 0 && str_icmp("abcd","EFGH") < 0);
}
void utest_str_icmp_len_lt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRCASECMP("abc","abcd") < 0 && str_icmp("abc","abcd") < 0);
}
void utest_str_icmp_len_gt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRCASECMP("abcd","abc") > 0 && str_icmp("abcd","abc") > 0);
}

void utest_str_incmp_diff_gt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRNCASECMP("efgh","abcd",3) > 0 && str_incmp("efgh","abcd",3) > 0);
    ASSERT_TRUE(STRNCASECMP("abcd","efgh",3) < 0 && str_incmp("abcd","afgh",3) < 0);
    ASSERT_TRUE(STRNCASECMP("abcd","abcd",3) == 0 && str_incmp("abcd","abcd",3) == 0);
    ASSERT_TRUE(STRNCASECMP("abcd","ABCD",3) == 0 && str_incmp("abcd","ABCD",3) == 0);
    ASSERT_TRUE(STRNCASECMP("abcd","ABCD",0) == 0 && str_incmp("abcd","ABCD",0) == 0);
    ASSERT_TRUE(STRNCASECMP("abc","ABCD",4) < 0);
    ASSERT_TRUE(str_incmp("abc","ABCD",4) < 0);
    ASSERT_TRUE(STRNCASECMP("abcd","ABC",4) > 0);
    ASSERT_TRUE(str_incmp("abcd","ABC",4) > 0);
}
void utest_str_incmp_diff_case_gt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRNCASECMP("EFGH","abcd",3) > 0 && str_incmp("EFGH","abcd",3) > 0);
}
void utest_str_incmp_diff_case_swap_gt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRNCASECMP("efgh","ABCD",3) > 0 && str_incmp("efgh","ABCD",3) > 0);
}
void utest_str_incmp_diff_lt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRNCASECMP("abcd","efgh",3) < 0 && str_incmp("abcd","efgh",3) < 0);
}
void utest_str_incmp_diff_case_lt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRNCASECMP("ABCD","efgh",3) < 0 && str_incmp("ABCD","efgh",3) < 0);
}
void utest_str_incmp_diff_case_swap_lt(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRNCASECMP("abcd","EFGH",3) < 0 && str_incmp("abcd","EFGH",3) < 0);
}

void utest_str_len_eq(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(strlen("abcd") == str_len("abcd"));
}

void utest_str_len_null_eq(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(strlen("ab\0d") == str_len("ab\0d"));
}

void utest_str_wlen(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    wchar_t str[10];
    memset(str,0,sizeof(wchar_t) * 10);
    str[0] = 'H';
    str[1] = 'e';
    str[2] = 'l';
    str[3] = 'l';
    str[4] = 'o';
    str[5] = '\0';
    ASSERT_EQ(wcslen(str),wstr_len(str));
    ASSERT_EQ(5,wstr_len(str));
}

void utest_str_nlen_eq(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(strnlen("abcdef",7) == str_nlen("abcdef",7));
    ASSERT_TRUE(strnlen("abcdef",3) == str_nlen("abcdef",3));
}

void utest_str_ncmp_eq(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(strncmp("abcd","abcd",0) == 0);
    ASSERT_TRUE(str_ncmp("abcd","abcd",0) == 0);
    ASSERT_TRUE(strncmp("abcd","abcd",3) == 0);
    ASSERT_TRUE(str_ncmp("abcd","abcd",3) == 0);
    ASSERT_TRUE(strncmp("abcd","aBcd",3) > 0);
    ASSERT_TRUE(str_ncmp("abcd","aBcd",3) > 0);
    ASSERT_TRUE(strncmp("aBcd","abcd",3) < 0);
    ASSERT_TRUE(str_ncmp("aBcd","abcd",3) < 0);
    ASSERT_TRUE(strncmp("abcd","abc",4) > 0);
    ASSERT_TRUE(str_ncmp("abcd","abc",4) > 0);
    ASSERT_TRUE(strncmp("abc","abcd",4) < 0);
    ASSERT_TRUE(str_ncmp("abc","abcd",4) < 0);
}

void utest_str_incmp_eq(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(STRNCASECMP("ABCD","abcd",0) == str_incmp("ABCD","abcd",0));
    ASSERT_TRUE(STRNCASECMP("ABCD","abcd",3) == str_incmp("ABCD","abcd",3));
}

void utest_str_memcpy(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;

    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);

    memset(t,0xFF,BUF_SIZE);
    memset(s,0,BUF_SIZE);
    t[2] = 0x00;
    t[3] = 0x00;
    ASSERT_TRUE(mem_cpy(s,t,BUF_SIZE) == s);
    ASSERT_EQ(s[0],0xFF);
    ASSERT_EQ(s[1],0xFF);
    ASSERT_EQ(s[2],0x00);
    ASSERT_EQ(s[3],0x00);
    ASSERT_EQ(s[4],0xFF);
    ASSERT_EQ(s[5],0xFF);
    ASSERT_EQ(s[6],0xFF);
    ASSERT_EQ(s[7],0xFF);
    free(t); free(s); free(q);

}

void utest_unpack_int64le_eq_negone(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
    ASSERT_EQ(unpack_int64le(t),-1);
    free(t); free(s); free(q);
}

void utest_unpack_int64be_eq_negone(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
    ASSERT_EQ(unpack_int64be(t),-1);
    free(t); free(s); free(q);
}

void utest_unpack_int32le_eq_negone(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);

    memset(t,0xFF,8);
    ASSERT_EQ(unpack_int32le(t),-1);
    free(t); free(s); free(q);
}

void utest_unpack_int32be_eq_negone(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);

    memset(t,0xFF,8);
    ASSERT_EQ(unpack_int32be(t),-1);
    free(t); free(s); free(q);
}

void utest_unpack_uint24(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
    ASSERT_EQ(unpack_uint24le(t),16777215);
    ASSERT_EQ(unpack_uint24be(t),16777215);
    memset(t,0x00,8);
    ASSERT_EQ(unpack_uint24le(t),0);
    ASSERT_EQ(unpack_uint24be(t),0);
    t[0] = 0x01;
    ASSERT_EQ(unpack_uint24le(t),1);
    ASSERT_EQ(unpack_uint24be(t),0x10000);
    t[0] = 0x00;
    t[2] = 0x01;
    ASSERT_EQ(unpack_uint24le(t),0x10000);
    ASSERT_EQ(unpack_uint24be(t),1);
    free(t); free(s); free(q);
}

void utest_unpack_int24(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
    ASSERT_EQ(unpack_int24le(t),-1);
    ASSERT_EQ(unpack_int24be(t),-1);

    memset(t,0x00,8);
    ASSERT_EQ(unpack_int24le(t),0);
    ASSERT_EQ(unpack_int24be(t),0);

    t[0] = 0x01;
    t[1] = 0x00;
    t[2] = 0x00;
    ASSERT_EQ(unpack_int24le(t),1);
    ASSERT_EQ(unpack_int24be(t),0x10000);

    t[0] = 0x80;
    t[1] = 0x00;
    t[2] = 0x00;
    ASSERT_EQ(unpack_int24le(t),0x80);
    ASSERT_EQ(unpack_int24be(t),-8388608);

    t[0] = 0x00;
    t[1] = 0x00;
    t[2] = 0x80;
    ASSERT_EQ(unpack_int24le(t),-8388608);
    ASSERT_EQ(unpack_int24be(t),0x80);

    t[0] = 0x00;
    t[1] = 0x00;
    t[2] = 0x01;
    ASSERT_EQ(unpack_int24le(t),0x10000);
    ASSERT_EQ(unpack_int24be(t),1);
    free(t); free(s); free(q);
}

void utest_unpack_int16le_eq_negone(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
    ASSERT_EQ(unpack_int16le(t),-1);
    free(t); free(s); free(q);
}

void utest_unpack_int16be_eq_negone(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
    ASSERT_EQ(unpack_int16be(t),-1);
    free(t); free(s); free(q);
}

void utest_unpack_uint64le_eq_max(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
#if __STDC_VERSION__ >= 199901L
    ASSERT_EQ(unpack_uint64le(t),UINT64_MAX);
#endif
    free(t); free(s); free(q);
}

void utest_unpack_uint64be_eq_max(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
#if __STDC_VERSION__ >= 199901L
    ASSERT_EQ(unpack_uint64be(t),UINT64_MAX);
#endif
    free(t); free(s); free(q);
}

void utest_unpack_uint32le_eq_max(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
#if __STDC_VERSION__ >= 199901L
    ASSERT_EQ(unpack_uint32le(t),UINT32_MAX);
#endif
    free(t); free(s); free(q);
}

void utest_unpack_uint32be_eq_max(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
#if __STDC_VERSION__ >= 199901L
    ASSERT_EQ(unpack_uint32be(t),UINT32_MAX);
#endif
    free(t); free(s); free(q);
}

void utest_unpack_uint16le_eq_max(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
#if __STDC_VERSION__ >= 199901L
    ASSERT_EQ(unpack_uint16le(t),UINT16_MAX);
#endif
    free(t); free(s); free(q);
}

void utest_unpack_uint16be_eq_max(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
#if __STDC_VERSION__ >= 199901L
    ASSERT_EQ(unpack_uint16be(t),UINT16_MAX);
#endif
    free(t); free(s); free(q);
}


void utest_unpack_int32lebe(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    t[0] = 0xFF;
    t[1] = 0xFF;
    t[2] = 0xFF;
    t[3] = 0x7F;
    ASSERT_EQ(unpack_int32le(t),2147483647);
    ASSERT_EQ(unpack_int32be(t),-129);

    memset(t,0,BUF_SIZE);
    t[0] = 0x7F;
    t[1] = 0xFF;
    t[2] = 0xFF;
    t[3] = 0xFF;
    ASSERT_EQ(unpack_int32le(t),-129);
    ASSERT_EQ(unpack_int32be(t),2147483647);
    free(t); free(s); free(q);
}

void utest_unpack_int16lebe(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    t[0] = 0xFF;
    t[1] = 0xFE;
    ASSERT_EQ(unpack_int16le(t),-257);
    ASSERT_EQ(unpack_int16be(t),-2);

    memset(t,0,BUF_SIZE);
    t[0] = 0xFE;
    t[1] = 0xFF;
    ASSERT_EQ(unpack_int16le(t),-2);
    ASSERT_EQ(unpack_int16be(t),-257);
    free(t); free(s); free(q);
}

void utest_pack_uint64le(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(s,0xFF,8);
    memset(t,0,BUF_SIZE);
#if __STDC_VERSION__ >= 199901L
    ASSERT_EQ(pack_uint64le(NULL,UINT64_MAX),8);
    ASSERT_EQ(pack_uint64le(t,UINT64_MAX),8);
    ASSERT_EQ(mem_cmp(t,s,8),0);
#endif
    free(t); free(s); free(q);
}

void utest_pack_uint64be(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(s,0xFF,8);
    memset(t,0,BUF_SIZE);

#if __STDC_VERSION__ >= 199901L
    ASSERT_EQ(pack_uint64be(NULL,UINT64_MAX),8);
    ASSERT_EQ(pack_uint64be(t,UINT64_MAX),8);
    ASSERT_EQ(mem_cmp(t,s,8),0);
#endif

    free(t); free(s); free(q);
}

void utest_pack_uint32le(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(s,0xFF,8);
    memset(t,0,BUF_SIZE);

#if __STDC_VERSION__ >= 199901L
    ASSERT_EQ(pack_uint32le(NULL,UINT32_MAX),4);
    ASSERT_EQ(pack_uint32le(t,UINT32_MAX),4);
    ASSERT_EQ(mem_cmp(t,s,4),0);
#endif

    free(t); free(s); free(q);
}

void utest_pack_uint32be(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(s,0xFF,8);
    memset(t,0,BUF_SIZE);
#if __STDC_VERSION__ >= 199901L
    ASSERT_EQ(pack_uint32be(NULL,UINT32_MAX),4);
    ASSERT_EQ(pack_uint32be(t,UINT32_MAX),4);
    ASSERT_EQ(mem_cmp(t,s,4),0);
#endif
    free(t); free(s); free(q);
}

void utest_pack_uint16le(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(s,0xFF,8);
    memset(t,0,BUF_SIZE);
#if __STDC_VERSION__ >= 199901L
    ASSERT_EQ(pack_uint16le(NULL,UINT16_MAX),2);
    ASSERT_EQ(pack_uint16le(t,UINT16_MAX),2);
    ASSERT_EQ(mem_cmp(t,s,2),0);
#endif
    free(t); free(s); free(q);
}

void utest_pack_uint16be(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(s,0xFF,8);
    memset(t,0,BUF_SIZE);

#if __STC_VERSION >= 199901L
    ASSERT_EQ(pack_uint16be(NULL,UINT16_MAX),2);
    ASSERT_EQ(pack_uint16be(t,UINT16_MAX),2);
    ASSERT_EQ(mem_cmp(t,s,2),0);
#endif
    free(t); free(s); free(q);
}

void utest_pack_int64(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(s,0,8);
    memset(q,0,8);
    s[7] = 0x80;
    q[0] = 0x80;

#if __STC_VERSION >= 199901L
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(pack_int64le(NULL,INT64_MIN),8);
    ASSERT_EQ(pack_int64le(t,INT64_MIN),8);
    ASSERT_EQ(mem_cmp(t,s,8),0);

    memset(t,0,BUF_SIZE);
    ASSERT_EQ(pack_int64be(NULL,INT64_MIN),8);
    ASSERT_EQ(pack_int64be(t,INT64_MIN),8);
    ASSERT_EQ(mem_cmp(t,q,8),0);
#endif

    free(t); free(s); free(q);
}

void utest_pack_int32(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(s,0,8);
    memset(q,0,8);
    s[3] = 0x80;
    q[0] = 0x80;

#if __STC_VERSION >= 199901L
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(pack_int32le(NULL,INT32_MIN),4);
    ASSERT_EQ(pack_int32le(t,INT32_MIN),4);
    ASSERT_EQ(mem_cmp(t,s,4),0);

    memset(t,0,BUF_SIZE);
    ASSERT_EQ(pack_int32be(NULL,INT32_MIN),4);
    ASSERT_EQ(pack_int32be(t,INT32_MIN),4);
    ASSERT_EQ(mem_cmp(t,q,4),0);
#endif
    free(t); free(s); free(q);
}

void utest_pack_int16(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(s,0,8);
    memset(q,0,8);
    s[1] = 0x80;
    q[0] = 0x80;

#if __STC_VERSION >= 199901L
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(pack_int16le(NULL,INT16_MIN),2);
    ASSERT_EQ(pack_int16le(t,INT16_MIN),2);
    ASSERT_EQ(mem_cmp(t,s,2),0);

    memset(t,0,BUF_SIZE);
    ASSERT_EQ(pack_int16be(NULL,INT16_MIN),2);
    ASSERT_EQ(pack_int16be(t,INT16_MIN),2);
    ASSERT_EQ(mem_cmp(t,q,2),0);
#endif
    free(t); free(s); free(q);
}

void utest_str_memcmp(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,BUF_SIZE);
    memset(s,0xFF,BUF_SIZE);
    s[BUF_SIZE-1] = 0;
    ASSERT_TRUE(memcmp(t,s,BUF_SIZE) > 0);
    ASSERT_TRUE(mem_cmp(t,s,BUF_SIZE) > 0);
    ASSERT_TRUE(memcmp(s,t,BUF_SIZE) < 0);
    ASSERT_TRUE(mem_cmp(s,t,BUF_SIZE) < 0);
}


void utest_str_cpy(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
    str_cpy((char *)t,"abcdefg");
    ASSERT_EQ(mem_cmp(t,"abcdefg\0",8),0);
    str_cpy((char *)t,"He\\o");
    ASSERT_EQ(mem_cmp(t,"He\\o\0",5),0);
    free(t); free(s); free(q);
}

void utest_str_ncpy_long(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
    str_ncpy((char *)t,"abcdefgh",3);
    ASSERT_EQ(mem_cmp(t,"abc\xFF",4),0);
    free(t); free(s); free(q);
}

void utest_str_ncpy_short(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0xFF,8);
    str_ncpy((char *)t,"ab",3);
    ASSERT_EQ(mem_cmp(t,"ab\0",3),0);
    free(t); free(s); free(q);
}

void utest_str_lower(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(str_lower((char *)t,"AbCdEfG"),7);
    ASSERT_EQ(mem_cmp(t,"abcdefg",8),0);

    t[0] = 'H';
    t[1] = 'e';
    t[2] = 'l';
    t[3] = 'l';
    t[4] = 'o';
    t[5] = '\0';

    ASSERT_EQ(str_lower( (char *)t, (char *)t),5);
    ASSERT_EQ(mem_cmp(t,"hello\0",6),0);
    free(t); free(s); free(q);
}

void utest_str_nlower(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(str_nlower((char *)t,"AbCdEfG",3),3);
    ASSERT_EQ(mem_cmp(t,"abcdefg",3),0);

    memset(t,0,BUF_SIZE);
    ASSERT_EQ(str_nlower((char *)t,"AbCdEfG",99),7);
    ASSERT_EQ(mem_cmp(t,"abcdefg",7),0);
    free(t); free(s); free(q);
}

void utest_str_ecat(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    memset(s,0,8);
    t[0] = 'h';
    t[1] = 'e';
    t[2] = '\\';
    t[3] = 'o';
    ASSERT_EQ(str_ecat(NULL,(const char *)t,"\\",'\\'),5);
    ASSERT_EQ(str_ecat((char *)s,(const char *)t,"\\",'\\'),5);
    free(t); free(s); free(q);
}

void utest_str_necat(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    memset(s,0,8);
    t[0] = 'h';
    t[1] = 'e';
    t[2] = '\\';
    t[3] = 'o';
    ASSERT_EQ(str_necat(NULL,(const char *)t,3,"\\",'\\'),4);
    ASSERT_EQ(str_necat((char *)s,(const char *)t,3,"\\",'\\'),4);
    free(t); free(s); free(q);
}

void utest_str_chr(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    const char *test_str1 = "abcdefg";
    const char *test_str2 = "gabcdefg";

    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    ASSERT_TRUE(str_chr(test_str1,'g') == &test_str1[6]);
    ASSERT_TRUE(str_chr(test_str1,'z') == NULL);
    ASSERT_TRUE(str_chr(test_str2,'g') == &test_str2[0]);
    ASSERT_TRUE(str_rchr(test_str2,'g') == &test_str2[7]);
    ASSERT_TRUE(str_rchr(test_str2,'h') == NULL);
    free(t); free(s); free(q);
}

void utest_str_starts(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(str_starts("AbCd","AbC"));
    ASSERT_FALSE(str_starts("DeF","AbC"));
    ASSERT_FALSE(str_starts("AbCd","abc"));
}

void utest_str_istarts(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(str_istarts("AbCd","abc"));
    ASSERT_FALSE(str_istarts("def","abc"));
}

void utest_str_ends(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(str_ends("AbCd","bCd"));
}

void utest_str_iends(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    ASSERT_TRUE(str_iends("AbCd","bcd"));
}

void utest_str_cat(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    str_cat((char *)t,"abc");
    ASSERT_EQ(str_len((const char *)t),3);
    str_cat((char *)t,"def");
    ASSERT_EQ(str_len((const char *)t),6);
    ASSERT_EQ(mem_cmp(t,"abcdef\0",7),0);
    free(t); free(s); free(q);
}

void utest_str_ncat(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    str_ncat((char *)t,"abcdefg",6);
    ASSERT_EQ(mem_cmp(t,"abcdef\0",7),0);

    memset(t,0,BUF_SIZE);
    str_ncat((char *)t,"abc",6);
    ASSERT_EQ(mem_cmp(t,"abc\0",4),0);
    free(t); free(s); free(q);
}

void utest_str_str(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    const char *test_str1 = "garbage";
    const char *test_str2 = "";

    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    ASSERT_TRUE(str_str(test_str1,"age") == &test_str1[4]);
    ASSERT_TRUE(str_str(test_str1,"bonkers") == NULL);
    ASSERT_TRUE(str_str(test_str1,"") == &test_str1[0]);
    ASSERT_TRUE(str_str(test_str2,"garbage") == NULL);
    free(t); free(s); free(q);
}

void utest_utf_conv_utf16w(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    wchar_t str[20];
    wchar_t str2[20];
#ifdef JPR_WINDOWS
    jpr_uint32 i;
#endif

    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);

#ifdef JPR_WINDOWS
    i = 0;
#endif
    memset(str,0,sizeof(wchar_t) * 20);
    memset(str2,0,sizeof(wchar_t) * 20);
    memset(t,0,BUF_SIZE);
    str[0] = 'H';
    str[1] = 'e';
    str[2] = 'l';
    str[3] = 'l';
    str[4] = 'o';
    str[5] = '\0';
    ASSERT_EQ(5,wstr_len(str));
    ASSERT_EQ(wcslen(str),wstr_len(str));
    ASSERT_EQ(5,utf_conv_utf16w_utf8(NULL,str,0));
    ASSERT_EQ(wstr_len(str),utf_conv_utf16w_utf8(NULL,str,0));
    ASSERT_EQ(wstr_len(str),utf_conv_utf16w_utf8(t,str,0));
    ASSERT_EQ(5,strlen((const char *)t));
    ASSERT_EQ(2,utf_conv_utf16w_utf8(NULL,str,2));
    str[1] = 0xD83C;
    ASSERT_EQ(0,utf_conv_utf16w_utf8(NULL,str,0));
    str[2] = 0xDF7A;
    ASSERT_EQ(7,utf_conv_utf16w_utf8(NULL,str,0));
    str[0] = 0xFFFF;
    ASSERT_EQ(9,utf_conv_utf16w_utf8(NULL,str,0));
    str[0] = 0x110000;
    ASSERT_EQ(0,utf_conv_utf16w_utf8(NULL,str,0));
    memset(str,0,sizeof(wchar_t) * 10);
    memset(t,0,BUF_SIZE);
    str[0] = 'H';
    str[1] = 'e';
    str[2] = 'l';
    str[3] = 'l';
    str[4] = 'o';
    str[5] = '\0';
    ASSERT_EQ(wstr_len(str),utf_conv_utf16w_utf8(t,str,0));
    memset(str,0,sizeof(wchar_t) * 10);
    ASSERT_EQ(str_len((const char *)t),utf_conv_utf8_utf16w(NULL,t,0));
    ASSERT_EQ(str_len((const char *)t),utf_conv_utf8_utf16w(str,t,0));
    ASSERT_EQ(str[0],'H');
    ASSERT_EQ(str[1],'e');
    ASSERT_EQ(str[2],'l');
    ASSERT_EQ(str[3],'l');
    ASSERT_EQ(str[4],'o');
    memset(str,0,sizeof(wchar_t) * 10);
    memset(t,0,BUF_SIZE);
    str[0] = 'H';
    str[1] = 0xD83C;
    str[2] = 0xDF7A;
    str[3] = 'l';
    str[4] = 'o';
    str[5] = '\0';
    ASSERT_EQ(7,utf_conv_utf16w_utf8(t,str,0));
    memset(str,0,sizeof(wchar_t) * 10);
    ASSERT_EQ(5,utf_conv_utf8_utf16w(NULL,t,0));
    ASSERT_EQ(5,utf_conv_utf8_utf16w(str,t,0));
    ASSERT_EQ(str[0],'H');
    ASSERT_EQ(str[1],0xD83C);
    ASSERT_EQ(str[2],0xDF7A);
    ASSERT_EQ(str[3],'l');
    ASSERT_EQ(str[4],'o');
    ASSERT_EQ(str[5],'\0');

#ifdef JPR_WINDOWS
/* verify we match the behavior of MultiByteToWideChar */
    memset(t,0,BUF_SIZE);
    memset(str,0,sizeof(wchar_t) * 20);
    memset(str2,0,sizeof(wchar_t) * 20);
    t[0] = 'P';
    t[1] = 'o';
    t[2] = 'k';
    t[3] = 0xC3;
    t[4] = 0xA9;
    t[5] = 'm';
    t[6] = 'o';
    t[7] = 'n';
    /* we report 7, MultiByteToWideChar reports 8 */
    ASSERT_EQ(8,MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,t,-1,NULL,0));
    MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,t,-1,str2,8);
    ASSERT_EQ(7,utf_conv_utf8_utf16w(str,t,0));
    ASSERT_TRUE(memcmp(str,str2,8 * sizeof(wchar_t)) == 0);

    memset(t,0,BUF_SIZE);
    memset(str,0,sizeof(wchar_t) * 20);
    memset(str2,0,sizeof(wchar_t) * 20);
    t[0] = 0xF0;
    t[1] = 0x9F;
    t[2] = 0x8D;
    t[3] = 0xBA;
    t[4] = '.';
    t[5] = 't';
    t[6] = 'x';
    t[7] = 't';
    ASSERT_EQ(7,MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,t,-1,NULL,0));
    ASSERT_EQ(6,utf_conv_utf8_utf16w(str,t,0));
    MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,t,-1,str2,7);
    for(i=0;i<8;i++) {
        ASSERT_EQ(str2[i],str[i]);
    }
#endif
    free(t); free(s); free(q);
}

void utest_utf_dec_utf16le(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    cp = 0;
    memset(t,0,BUF_SIZE);
    t[0] = 0x3C;
    t[1] = 0xD8;
    t[2] = 0x7A;
    t[3] = 0xDF;
    ASSERT_EQ(utf_dec_utf16le(NULL,t),4);
    ASSERT_EQ(utf_dec_utf16le(&cp,t),4);
    ASSERT_EQ(cp,0x1f37a);

    memset(t,0,BUF_SIZE);
    t[0] = 0xFF;
    t[1] = 0xFF;
    ASSERT_EQ(utf_dec_utf16le(NULL,t),2);

    /* only half a surrogate */
    memset(t,0,BUF_SIZE);
    t[0] = 0x3C;
    t[1] = 0xD8;
    ASSERT_EQ(utf_dec_utf16le(NULL,t),0);
    
    /* add second "surrogate" outside range */
    t[2] = 0xFF;
    t[3] = 0xFF;
    ASSERT_EQ(utf_dec_utf16le(NULL,t),0);
    free(t); free(s); free(q);

}

void utest_utf_dec_utf16be(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    cp = 0;
    memset(t,0,BUF_SIZE);
    t[0] = 0xD8;
    t[1] = 0x3C;
    t[2] = 0xDF;
    t[3] = 0x7A;
    ASSERT_EQ(utf_dec_utf16be(NULL,t),4);
    ASSERT_EQ(utf_dec_utf16be(&cp,t),4);
    ASSERT_EQ(cp,0x1f37a);

    /* only half a surrogate */
    memset(t,0,BUF_SIZE);
    t[0] = 0xD8;
    t[1] = 0x3C;
    ASSERT_EQ(utf_dec_utf16be(NULL,t),0);
    free(t); free(s); free(q);
}

void utest_utf_dec_utf32le(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    cp = 0;
    memset(t,0,BUF_SIZE);
    t[0] = 0x7A;
    t[1] = 0xF3;
    t[2] = 0x01;
    t[3] = 0x00;
    ASSERT_EQ(utf_dec_utf32le(NULL,t),4);
    ASSERT_EQ(utf_dec_utf32le(&cp,t),4);
    ASSERT_EQ(cp,0x1F37A);

    cp = 0;
    memset(t,0,BUF_SIZE);
    t[0] = 0x3C;
    t[1] = 0xD8;
    t[2] = 0x00;
    t[3] = 0x00;
    ASSERT_EQ(utf_dec_utf32le(NULL,t),0);
    free(t); free(s); free(q);
}

void utest_utf_dec_utf32be(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    cp = 0;
    memset(t,0,BUF_SIZE);
    t[0] = 0x00;
    t[1] = 0x01;
    t[2] = 0xF3;
    t[3] = 0x7A;
    ASSERT_EQ(utf_dec_utf32be(NULL,t),4);
    ASSERT_EQ(utf_dec_utf32be(&cp,t),4);
    ASSERT_EQ(cp,0x1F37A);

    cp = 0;
    memset(t,0,BUF_SIZE);
    t[0] = 0x00;
    t[1] = 0x00;
    t[2] = 0xD8;
    t[3] = 0x3C;
    ASSERT_EQ(utf_dec_utf32be(NULL,t),0);
    free(t); free(s); free(q);
}

void utest_utf_enc_utf32le(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_enc_utf32le(NULL,0x1F37A),4);
    ASSERT_EQ(utf_enc_utf32le(t,0x1F37A),4);
    ASSERT_EQ(t[0],0x7A);
    ASSERT_EQ(t[1],0xF3);
    ASSERT_EQ(t[2],0x01);
    ASSERT_EQ(t[3],0x00);
    ASSERT_EQ(utf_enc_utf32le(NULL,0xD800),0);
    ASSERT_EQ(utf_enc_utf32le(NULL,0xDFFF),0);
    ASSERT_EQ(utf_enc_utf32le(NULL,0x110000),0);
    free(t); free(s); free(q);
}

void utest_utf_enc_utf32be(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_enc_utf32be(NULL,0x1F37A),4);
    ASSERT_EQ(utf_enc_utf32be(t,0x1F37A),4);
    ASSERT_EQ(t[0],0x00);
    ASSERT_EQ(t[1],0x01);
    ASSERT_EQ(t[2],0xF3);
    ASSERT_EQ(t[3],0x7A);
    ASSERT_EQ(utf_enc_utf32be(NULL,0xD800),0);
    ASSERT_EQ(utf_enc_utf32be(NULL,0xDFFF),0);
    ASSERT_EQ(utf_enc_utf32be(NULL,0x110000),0);
    free(t); free(s); free(q);
}

void utest_utf_enc_utf16le(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_enc_utf16le(NULL,0x1F37A),4);
    ASSERT_EQ(utf_enc_utf16le(t,0x1F37A),4);
    ASSERT_EQ(t[0],0x3C);
    ASSERT_EQ(t[1],0xD8);
    ASSERT_EQ(t[2],0x7A);
    ASSERT_EQ(t[3],0xDF);
    ASSERT_EQ(utf_enc_utf16le(NULL,0xD800),0);
    ASSERT_EQ(utf_enc_utf16le(NULL,0xDFFF),0);
    ASSERT_EQ(utf_enc_utf16le(NULL,0x110000),0);
    free(t); free(s); free(q);
}

void utest_utf_enc_utf16be(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_enc_utf16be(NULL,0x1F37A),4);
    ASSERT_EQ(utf_enc_utf16be(t,0x1F37A),4);
    ASSERT_EQ(t[0],0xD8);
    ASSERT_EQ(t[1],0x3C);
    ASSERT_EQ(t[2],0xDF);
    ASSERT_EQ(t[3],0x7A);
    ASSERT_EQ(utf_enc_utf16be(NULL,0xD800),0);
    ASSERT_EQ(utf_enc_utf16be(NULL,0xDFFF),0);
    ASSERT_EQ(utf_enc_utf16be(NULL,0x110000),0);
    free(t); free(s); free(q);
}

void utest_utf_enc_utf8(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);

    /* surrogate pair */
    ASSERT_EQ(utf_enc_utf8(NULL,0xD800),0);
    ASSERT_EQ(utf_enc_utf8(NULL,0xDFFF),0);
    /* outside the range */
    ASSERT_EQ(utf_enc_utf8(NULL,0x110000),0);

    /* just above surrogate pair */
    ASSERT_EQ(utf_enc_utf8(NULL,0xFFFF),3);

    ASSERT_EQ(utf_enc_utf8(NULL,0x1F37A),4);
    ASSERT_EQ(utf_enc_utf8(t,0x1F37A),4);
    ASSERT_EQ(t[0],0xF0);
    ASSERT_EQ(t[1],0x9F);
    ASSERT_EQ(t[2],0x8D);
    ASSERT_EQ(t[3],0xBA);

    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_enc_utf8(NULL,0xE9),2);
    ASSERT_EQ(utf_enc_utf8(t,0xE9),2);
    ASSERT_EQ(t[0],0xC3);
    ASSERT_EQ(t[1],0xA9);

    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_enc_utf8(NULL,0x3050),3);
    ASSERT_EQ(utf_enc_utf8(t,0x3050),3);
    ASSERT_EQ(t[0],0xE3);
    ASSERT_EQ(t[1],0x81);
    ASSERT_EQ(t[2],0x90);

    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_enc_utf8(NULL,0x65),1);
    ASSERT_EQ(utf_enc_utf8(t,0x65),1);
    ASSERT_EQ(t[0],0x65);
    free(t); free(s); free(q);
}

void utest_utf_enc_iso88591(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_enc_iso88591(NULL,0xC9),1);
    ASSERT_EQ(utf_enc_iso88591(t,0xC9),1);
    ASSERT_EQ(t[0],0xC9);
    ASSERT_EQ(utf_enc_iso88591(NULL,0x1F37A),0);
    ASSERT_EQ(utf_enc_iso88591(NULL,0xD800),0);
    free(t); free(s); free(q);
}

void utest_utf_dec_iso88591(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    t[0] = 0xC9;
    ASSERT_EQ(utf_dec_iso88591(NULL,t),1);
    ASSERT_EQ(utf_dec_iso88591(&cp,t),1);
    ASSERT_EQ(cp,0xC9);
    free(t); free(s); free(q);
}

void utest_utf_dec_utf8(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    t[0] = 0xF0;
    t[1] = 0x9F;
    t[2] = 0x8D;
    t[3] = 0xBA;

    ASSERT_EQ(utf_dec_utf8(NULL,t),4);
    ASSERT_EQ(utf_dec_utf8(&cp,t),4);
    ASSERT_EQ(cp,0x1F37A);

    t[0] = 0xF0;
    t[1] = 0x9F;
    t[2] = 0x8D;
    t[3] = 0xBA;
    t[1] = 0x00;
    ASSERT_EQ(utf_dec_utf8(NULL,t),0);

    t[0] = 0xF0;
    t[1] = 0x9F;
    t[2] = 0x8D;
    t[3] = 0xBA;
    t[2] = 0x00;
    ASSERT_EQ(utf_dec_utf8(NULL,t),0);

    t[0] = 0xF0;
    t[1] = 0x9F;
    t[2] = 0x8D;
    t[3] = 0xBA;
    t[3] = 0x00;
    ASSERT_EQ(utf_dec_utf8(NULL,t),0);

    t[0] = 0xFF;
    t[1] = 0x9F;
    t[2] = 0x8D;
    t[3] = 0xBA;
    ASSERT_EQ(utf_dec_utf8(NULL,t),0);


    memset(t,0,BUF_SIZE);
    t[0] = 0x65;

    ASSERT_EQ(utf_dec_utf8(NULL,t),1);
    ASSERT_EQ(utf_dec_utf8(&cp,t),1);
    ASSERT_EQ(cp,0x65);

    memset(t,0,BUF_SIZE);
    t[0] = 0xC3;
    t[1] = 0xA9;

    ASSERT_EQ(utf_dec_utf8(NULL,t),2);
    ASSERT_EQ(utf_dec_utf8(&cp,t),2);
    ASSERT_EQ(cp,0xE9);

    t[1] = 0x00;
    ASSERT_EQ(utf_dec_utf8(NULL,t),0);

    memset(t,0,BUF_SIZE);
    t[0] = 0xE3;
    t[1] = 0x81;
    t[2] = 0x90;

    ASSERT_EQ(utf_dec_utf8(NULL,t),3);
    ASSERT_EQ(utf_dec_utf8(&cp,t),3);
    ASSERT_EQ(cp,0x3050);

    t[2] = 0x00;
    ASSERT_EQ(utf_dec_utf8(NULL,t),0);
    t[1] = 0x00;
    ASSERT_EQ(utf_dec_utf8(NULL,t),0);

    /* invalid byte sequence */
    memset(t,0,BUF_SIZE);
    t[0] = 0xC3;
    ASSERT_EQ(utf_dec_utf8(NULL,t),0);

    /* valid byte sequence, decodes to d800 */
    memset(t,0,BUF_SIZE);
    t[0] = 0xED;
    t[1] = 0xA0;
    t[2] = 0x80;
    ASSERT_EQ(utf_dec_utf8(NULL,t),0);

    /* valid byte sequence, decodes to dfff */
    memset(t,0,BUF_SIZE);
    t[0] = 0xED;
    t[1] = 0xBF;
    t[2] = 0xBF;
    ASSERT_EQ(utf_dec_utf8(NULL,t),0);
    free(t); free(s); free(q);
}

void utest_utf_conv(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_enc_utf16le(NULL,0x1F37A),4);
    ASSERT_EQ(utf_enc_utf16le(t,0x1F37A),4);
    t[4] = '!';
    t[5] = 0;
    t[6] = 0;
    t[7] = 0;

    ASSERT_EQ(utf_conv_utf16le_utf8(NULL,t,0),5);
    ASSERT_EQ(utf_conv_utf16le_utf8(NULL,t,4),4);
    ASSERT_EQ(utf_conv_utf16le_utf8(t,t,0),5);

    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_enc_utf8(t,0x1F37A),4);
    t[1] = 0;
    ASSERT_EQ(utf_conv_utf8_utf16(NULL,t,4),0);

    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_enc_utf16be(NULL,0x1F37A),4);
    ASSERT_EQ(utf_enc_utf16be(t,0x1F37A),4);
    t[4] = 0;
    t[5] = '!';
    t[6] = 0;
    t[7] = 0;

    ASSERT_EQ(utf_conv_utf16be_utf8(NULL,t,0),5);
    ASSERT_EQ(utf_conv_utf16be_utf8(t,t,0),5);

    t[5] = 0;
    memset(s,0,16);
    ASSERT_EQ(utf_conv_utf8_utf16(NULL,t,0),8);
    ASSERT_EQ(utf_conv_utf8_utf16le(NULL,t,0),6);
    ASSERT_EQ(utf_conv_utf8_utf16be(NULL,t,0),6);

    ASSERT_EQ(utf_conv_utf8_utf32(NULL,t,0),12);
    ASSERT_EQ(utf_conv_utf8_utf32le(NULL,t,0),8);
    ASSERT_EQ(utf_conv_utf8_utf32be(NULL,t,0),8);

    ASSERT_EQ(utf_conv_utf8_utf16(s,t,0),8);
    ASSERT_EQ(utf_conv_utf8_utf16le(s,t,0),6);
    ASSERT_EQ(utf_conv_utf8_utf16be(s,t,0),6);

    ASSERT_EQ(utf_conv_utf8_utf32(s,t,0),12);
    ASSERT_EQ(utf_conv_utf8_utf32le(s,t,0),8);
    ASSERT_EQ(utf_conv_utf8_utf32be(s,t,0),8);

    memset(s,0,16);
    ASSERT_EQ(utf_conv_utf8_utf16(s,t,0),8);

    /* intentionally break t */
    t[1] = 0;
    ASSERT_EQ(utf_conv_utf8_utf16(s,t,0),0);
    ASSERT_EQ(utf_conv_utf8_utf32(s,t,0),0);

    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_conv_utf16_utf8(t,s,0),5);
    ASSERT_EQ(t[0],0xF0);
    ASSERT_EQ(t[1],0x9F);
    ASSERT_EQ(t[2],0x8D);
    ASSERT_EQ(t[3],0xBA);
    ASSERT_EQ(t[4],'!');

    ASSERT_EQ(utf_conv_utf16_utf8(t,s,1),0);
    ASSERT_EQ(utf_conv_utf16_utf8(t,s,2),0);
    ASSERT_EQ(utf_conv_utf16_utf8(t,s,8),5);

    memset(s,0,BUF_SIZE);
    memset(t,0,BUF_SIZE);
    t[0] = 0xF0;
    t[1] = 0x9F;
    t[2] = 0x8D;
    t[3] = 0xBA;
    t[4] = '!';
    s[0] = 0xFE;
    s[1] = 0xFF;

    ASSERT_EQ(utf_conv_utf8_utf16be(s+2,t,0),6);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_conv_utf16_utf8(t,s,0),5);
    ASSERT_EQ(t[0],0xF0);
    ASSERT_EQ(t[1],0x9F);
    ASSERT_EQ(t[2],0x8D);
    ASSERT_EQ(t[3],0xBA);
    ASSERT_EQ(t[4],'!');

    ASSERT_EQ(utf_conv_utf8_utf16be(s,t,0),6);
    ASSERT_EQ(utf_conv_utf16_utf8(NULL,s,0),0);

    memset(s,0,BUF_SIZE);
    memset(t,0,BUF_SIZE);
    t[0] = 0xF0;
    t[1] = 0x9F;
    t[2] = 0x8D;
    t[3] = 0xBA;
    t[4] = '!';
    ASSERT_EQ(utf_conv_utf8_utf32(s,t,0),12);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_conv_utf32_utf8(NULL,s,0),5);
    ASSERT_EQ(utf_conv_utf32_utf8(NULL,s,1),0);
    ASSERT_EQ(utf_conv_utf32_utf8(NULL,s,2),0);
    ASSERT_EQ(utf_conv_utf32_utf8(NULL,s,3),0);
    ASSERT_EQ(utf_conv_utf32_utf8(NULL,s,4),0);
    ASSERT_EQ(utf_conv_utf32_utf8(NULL,s,12),5);
    ASSERT_EQ(utf_conv_utf32_utf8(t,s,0),5);

    memset(s,0,BUF_SIZE);
    memset(t,0,BUF_SIZE);
    t[0] = 0xF0;
    t[1] = 0x9F;
    t[2] = 0x8D;
    t[3] = 0xBA;
    t[4] = '!';
    s[0] = 0x00;
    s[1] = 0x00;
    s[2] = 0xFE;
    s[3] = 0xFF;
    ASSERT_EQ(utf_conv_utf8_utf32be(s+4,t,0),8);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_conv_utf32_utf8(NULL,s,0),5);
    ASSERT_EQ(utf_conv_utf32_utf8(t,s,0),5);

    memset(s,0,BUF_SIZE);
    memset(t,0,BUF_SIZE);
    t[0] = 0xF0;
    t[1] = 0x9F;
    t[2] = 0x8D;
    t[3] = 0xBA;
    t[4] = '!';
    ASSERT_EQ(utf_conv_utf8_utf32le(s,t,0),8);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(utf_conv_utf32_utf8(NULL,s,0),0);

    memset(t,0,BUF_SIZE);
    memset(s,0,16);
    ASSERT_EQ(utf_enc_utf32le(NULL,0x1F37A),4);
    ASSERT_EQ(utf_enc_utf32le(t,0x1F37A),4);
    ASSERT_EQ(utf_enc_utf32le(t+4,'!'),4);
    ASSERT_EQ(utf_conv_utf32le_utf8(NULL,t,0),5);
    ASSERT_EQ(utf_conv_utf32le_utf8(s,t,0),5);

    ASSERT_EQ(utf_enc_utf32be(t,0x1F37A),4);
    ASSERT_EQ(utf_enc_utf32be(t+4,'!'),4);
    ASSERT_EQ(utf_conv_utf32be_utf8(s,t,0),5);

    memset(t,0,BUF_SIZE);
    t[0] = 0xA3;
    t[1] = '!';
    t[2] = 0;
    ASSERT_EQ(utf_conv_iso88591_utf8(NULL,t,0),3);
    ASSERT_EQ(utf_conv_iso88591_utf8(s,t,0),3);
    memset(t,0,BUF_SIZE);
    t[0] = 0xE2;
    t[1] = 0x84;
    t[2] = 0xA2;
    t[3] = 0;
    ASSERT_EQ(utf_conv_utf8_iso88591(NULL,t,0),0);
    ASSERT_EQ(utf_conv_utf8_iso88591(s,t,0),0);
    free(t); free(s); free(q);
}


void utest_scan_uint(void) {
    jpr_uint64 c;
	const char *u_str = "U1925";
	const char *p_str = "(1925";
	const char *r_str = "1925";
	const char *e_str = "1925e";
    c = 0;
    ASSERT_EQ(scan_uint(u_str,&c),0);
    ASSERT_EQ(c,0);
    ASSERT_EQ(scan_uint(p_str,&c),0);
    ASSERT_EQ(c,0);

    ASSERT_EQ(scan_uint(r_str,&c),4);
    ASSERT_EQ(c,1925);
    c = 0;
    ASSERT_EQ(scan_uint(e_str,&c),4);
    ASSERT_EQ(c,1925);
}

void utest_scan_int(void) {
    jpr_int64 d;
	jpr_int64 t;
    ASSERT_EQ(scan_int("-2147483648",&d),str_len("-2147483648"));
	t = 2147483648;
	t *= -1;
    ASSERT_EQ(d,t);
    
    ASSERT_EQ(scan_int("2147483647",&d),str_len("2147483647"));
    t = 2147483647;
    ASSERT_EQ(d,t);
}


void utest_fmt_uint(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    ASSERT_TRUE(t != NULL);
    memset(t,0,BUF_SIZE);
    ASSERT_EQ(fmt_uint((char *)t,1925),4);
    ASSERT_EQ(str_ncmp((const char *)t,"1925",4),0);
    free(t); free(s); free(q);
}

void utest_fmt_int(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
	jpr_int64 temp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    ASSERT_TRUE(t != NULL);
    memset(t,0,BUF_SIZE);
	temp = -1925;
    ASSERT_EQ(fmt_int(NULL,temp),5);
    ASSERT_EQ(fmt_int((char *)t,temp),5);
    ASSERT_EQ(str_ncmp((const char *)t,"-1925",5),0);

    memset(t,0,BUF_SIZE);
#ifdef _MSC_VER
	temp = -2147483648LL;
#else
	temp = -2147483648;
#endif

    ASSERT_EQ(fmt_int(NULL,temp),str_len("-2147483648"));
    ASSERT_EQ(fmt_int((char *)t,temp),str_len("-2147483648"));
    ASSERT_EQ(str_ncmp((const char *)t,"-2147483648",str_len("-2147483648")),0);


    memset(t,0,BUF_SIZE);
    ASSERT_EQ(fmt_int(NULL,1925),4);
    ASSERT_EQ(fmt_int((char *)t,1925),4);
    ASSERT_EQ(str_ncmp((const char *)t,"1925",4),0);
    free(t); free(s); free(q);
}

void utest_char_char(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    c = 0;
    d = 0;
    while(c < 256 ) {
        ASSERT_EQ(islower(c) == 0,char_islower(c) == 0);
        ASSERT_EQ(isupper(c) == 0,char_isupper(c) == 0);
        ASSERT_TRUE(tolower(c) == char_lower(c));
        ASSERT_TRUE(toupper(c) == char_upper(c));
        c++;
    }

    for(c=0;c<256;c++) {
        ASSERT_EQ(char_isdigit(c) == 0,isdigit(c) == 0);
    }
}

void utest_file_open(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    jpr_dir *dir;
    jpr_dire *entry;
    jpr_file *f;

    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    memset(t,0,BUF_SIZE);
    t[0] = 0xC3;
    t[1] = 0xA9;
    t[2] = '.';
    t[3] = 't';
    t[4] = 'x';
    t[5] = 't';
    ASSERT_TRUE(file_open(NULL,NULL) == NULL);

    f = file_open("garbage.txt","r");
    ASSERT_TRUE(f == NULL);
    f = file_open((const char *)t,"w");
    ASSERT_TRUE(f != NULL);
    ASSERT_EQ(0,file_tell(f));
    ASSERT_EQ(6,file_write(f,t,6));
    ASSERT_EQ(6,file_tell(f));
    ASSERT_EQ(0,file_close(f));
    file_free(f);
    f = file_open((const char *)t,"a");
    ASSERT_TRUE(f != NULL);
    ASSERT_EQ(6,file_tell(f));
    ASSERT_EQ(0,file_close(f));
    file_free(f);
    f = file_open((const char *)t,"r");
    ASSERT_TRUE(f != NULL);
    ASSERT_EQ(6,file_read(f,s,6));
    ASSERT_TRUE(mem_cmp(s,t,6) == 0);
    ASSERT_EQ(0,file_read(f,s,6));
    ASSERT_TRUE(file_eof(f));
    ASSERT_EQ(1,file_seek(f,1,0));
    ASSERT_FALSE(file_eof(f));
    ASSERT_EQ(99,file_seek(f,99,0));
    ASSERT_FALSE(file_eof(f));
    ASSERT_EQ(0,file_close(f));
    file_free(f);
    ASSERT_TRUE(file_open((const char *)t,NULL) == NULL);
    ASSERT_TRUE(file_open((const char *)t,"crap") == NULL);

    memset(t,0,BUF_SIZE);
    t[0] = 0xF0;
    t[1] = 0x9F;
    t[2] = 0x8D;
    t[3] = 0xBA;
    t[4] = '.';
    t[5] = 't';
    t[6] = 'x';
    t[7] = 't';

    memset(s,0,BUF_SIZE);
    s[0] = 0xC3;
    s[1] = 0xA9;
    s[2] = '.';
    s[3] = 't';
    s[4] = 'x';
    s[5] = 't';

    f = file_open((const char *)t,"w");
    ASSERT_TRUE(f != NULL);
    file_close(f);
    file_free(f);

    /* not really part of the file tests, just making sure
     * it runs after creating the txt files */

    dir = dir_open(".");
    ASSERT_TRUE(dir != NULL);

    while( (entry = dir_read(dir)) != NULL) {
      if(str_cmp(entry->filename,(const char *)t) == 0 || str_cmp(entry->filename,(const char *)s) == 0) {
      }
      dire_free(entry);
    }

    dir_close(dir);

    ASSERT_EQ(1,path_exists((const char *)t));
    ASSERT_EQ(0,path_exists("garbage"));
    free(t); free(s); free(q);

}

void utest_dir_dir(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    jpr_dir *dir;
    jpr_dire *entry;

    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    dir = dir_open(".");
    ASSERT_TRUE(dir != NULL);

    while( (entry = dir_read(dir)) != NULL) {
      dire_free(entry);
    }

    dir_close(dir);

    dir = dir_open("garbage");
    ASSERT_TRUE(dir == NULL);
    free(t); free(s); free(q);
}

void utest_path_basename(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    char *my_basename;
    char *sys_basename;
    const char *full_path;
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);

#ifdef JPR_WINDOWS
    full_path = "C:\\windows";
    my_basename = path_basename(full_path);
    mem_free(my_basename);

    full_path = "C:\\windows/system32";
    my_basename = path_basename(full_path);
    mem_free(my_basename);

    full_path = "C:\\windows\\///system32\\/";
    my_basename = path_basename(full_path);
    mem_free(my_basename);
#endif

    full_path = "/";
    my_basename = path_basename(full_path);
    mem_free(my_basename);

    full_path = "";
    my_basename = path_basename(full_path);
    mem_free(my_basename);

    full_path = "/file/something";
    my_basename = path_basename(full_path);
    mem_free(my_basename);

    full_path = "/file/something/";
    my_basename = path_basename(full_path);
    mem_free(my_basename);

    full_path = "something";
    my_basename = path_basename(full_path);
    mem_free(my_basename);

    free(t); free(s); free(q);
}

void utest_path_dirname(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    char *my_dirname;
    char *sys_dirname;
    const char *full_path;
    char *b;

    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);

#ifdef JPR_WINDOWS
    full_path = "C:\\windows";
    my_dirname = path_dirname(full_path);
    ASSERT_TRUE(strcmp(my_dirname,"C:") == 0);
    mem_free(my_dirname);

    full_path = "C:\\windows/system32";
    my_dirname = path_dirname(full_path);
    ASSERT_TRUE(strcmp(my_dirname,"C:\\windows") == 0);
    mem_free(my_dirname);

    full_path = "C:\\windows\\///system32\\/";
    my_dirname = path_dirname(full_path);
    mem_free(my_dirname);
#endif

    full_path = "/";
    my_dirname = path_dirname(full_path);
    mem_free(my_dirname);

    full_path = "";
    my_dirname = path_dirname(full_path);
    mem_free(my_dirname);

    full_path = "/file/something";
    my_dirname = path_dirname(full_path);
    mem_free(my_dirname);

    full_path = "/file/something/";
    my_dirname = path_dirname(full_path);;
    mem_free(my_dirname);

    free(t); free(s); free(q);
}

void utest_path_cwd(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    char *my_cwd = path_getcwd();
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    ASSERT_TRUE(my_cwd != NULL);
    ASSERT_EQ(path_setcwd(my_cwd),0);
}

void utest_path_absolute(void) {
    size_t c;
    jpr_int64 d;
    jpr_uint32 cp;
    jpr_uint8 *t;
    jpr_uint8 *s;
    jpr_uint8 *q;
    char *my_abs = path_absolute("/usr");
    t = malloc(BUF_SIZE);
    s = malloc(BUF_SIZE);
    q = malloc(BUF_SIZE);
    ASSERT_TRUE(str_cmp(my_abs,"/usr") == 0);
    mem_free(my_abs);

#ifdef JPR_WINDOWS
    my_abs = path_absolute("\\\\server\\share");
    ASSERT_TRUE(str_cmp(my_abs,"\\\\server\\share") == 0);
    mem_free(my_abs);

    my_abs = path_absolute("C:\\windows\\system32");
    ASSERT_TRUE(str_cmp(my_abs,"C:\\windows\\system32") == 0);
    mem_free(my_abs);
#endif

    my_abs = path_absolute("folder/file.txt");
    ASSERT_TRUE(str_cmp(my_abs,"folder/file.txt") != 0);
    mem_free(my_abs);

    free(t); free(s); free(q);
}

void run_tests(void) {
utest_str_memchr();
utest_str_memset();
utest_str_cmp_eq();
utest_str_cmp_case_lt();
utest_str_cmp_case_gt();
utest_str_cmp_diff_gt();
utest_str_cmp_diff_lt();
utest_str_cmp_diff_case_gt();
utest_str_cmp_len_lt();
utest_str_cmp_len_gt();
utest_str_icmp_eq();
utest_str_icmp_case_eq();
utest_str_icmp_case_swap_eq();
utest_str_icmp_diff_gt();
utest_str_icmp_diff_case_gt();
utest_str_icmp_diff_case_swap_gt();
utest_str_icmp_diff_lt();
utest_str_icmp_diff_case_lt();
utest_str_icmp_diff_case_swap_lt();
utest_str_icmp_len_lt();
utest_str_icmp_len_gt();
utest_str_incmp_diff_gt();
utest_str_incmp_diff_case_gt();
utest_str_incmp_diff_case_swap_gt();
utest_str_incmp_diff_lt();
utest_str_incmp_diff_case_lt();
utest_str_incmp_diff_case_swap_lt();
utest_str_len_eq();
utest_str_len_null_eq();
utest_str_wlen();
utest_str_nlen_eq();
utest_str_ncmp_eq();
utest_str_incmp_eq();
utest_str_memcpy();
utest_unpack_int64le_eq_negone();
utest_unpack_int64be_eq_negone();
utest_unpack_int32le_eq_negone();
utest_unpack_int32be_eq_negone();
utest_unpack_uint24();
utest_unpack_int24();
utest_unpack_int16le_eq_negone();
utest_unpack_int16be_eq_negone();
utest_unpack_uint64le_eq_max();
utest_unpack_uint64be_eq_max();
utest_unpack_uint32le_eq_max();
utest_unpack_uint32be_eq_max();
utest_unpack_uint16le_eq_max();
utest_unpack_uint16be_eq_max();
utest_unpack_int32lebe();
utest_unpack_int16lebe();
utest_pack_uint64le();
utest_pack_uint64be();
utest_pack_uint32le();
utest_pack_uint32be();
utest_pack_uint16le();
utest_pack_uint16be();
utest_pack_int64();
utest_pack_int32();
utest_pack_int16();
utest_str_memcmp();
utest_str_cpy();
utest_str_ncpy_long();
utest_str_ncpy_short();
utest_str_lower();
utest_str_nlower();
utest_str_ecat();
utest_str_necat();
utest_str_chr();
utest_str_starts();
utest_str_istarts();
utest_str_ends();
utest_str_iends();
utest_str_cat();
utest_str_ncat();
utest_str_str();
utest_utf_conv_utf16w();
utest_utf_dec_utf16le();
utest_utf_dec_utf16be();
utest_utf_dec_utf32le();
utest_utf_dec_utf32be();
utest_utf_enc_utf32le();
utest_utf_enc_utf32be();
utest_utf_enc_utf16le();
utest_utf_enc_utf16be();
utest_utf_enc_utf8();
utest_utf_enc_iso88591();
utest_utf_dec_iso88591();
utest_utf_dec_utf8();
utest_utf_conv();
utest_scan_uint();
utest_scan_int();
utest_fmt_uint();
utest_fmt_int();
utest_char_char();
utest_file_open();
utest_dir_dir();
utest_path_basename();
utest_path_dirname();
utest_path_cwd();
utest_path_absolute();
}

#ifdef JPR_WINDOWS
int wmain(int argc, const wchar_t *argv[]) {
#else
int main(int argc, const char *argv[]) {
#endif
    int r;
    const char *fake_argv[] = { "test", NULL };

    (void)argc;
    (void)argv;

    r = 0;

    run_tests();

    return r;
}

