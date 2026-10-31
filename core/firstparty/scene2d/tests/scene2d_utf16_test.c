/* SPDX-License-Identifier: MIT */
#include "musa_scene2d_utf16.h"
#include "musa_scene2d_text.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void invalid(const uint8_t *input, size_t length) {
    uint8_t output[24];
    memset(output,0xa5,sizeof output);
    size_t written=777;
    assert(!musa_scene2d_utf16le_to_utf8(input,length,
                                          output,sizeof output,&written));
    assert(written==777);
    for (size_t i=0;i<sizeof output;++i) assert(output[i]==0xa5);
    assert(!musa_scene2d_utf16le_to_utf8(input,length,NULL,0,&written));
    assert(written==777);
}
static void boundary_scalars(void) {
    const uint8_t utf16[]={
        0x7f,0, 0x80,0, 0xff,0x07, 0,0x08,
        0xff,0xd7, 0,0xe0, 0xff,0xff,
        0,0xd8,0,0xdc, 0xff,0xdb,0xff,0xdf
    };
    const uint8_t expected[]={
        0x7f, 0xc2,0x80, 0xdf,0xbf, 0xe0,0xa0,0x80,
        0xed,0x9f,0xbf, 0xee,0x80,0x80, 0xef,0xbf,0xbf,
        0xf0,0x90,0x80,0x80, 0xf4,0x8f,0xbf,0xbf
    };
    uint8_t out[sizeof(expected)+1];
    size_t written=0, count=0;
    assert(musa_scene2d_utf16le_to_utf8(utf16,sizeof utf16,
                                         out,sizeof out,&written));
    assert(written==sizeof expected && out[written]==0);
    assert(memcmp(out,expected,sizeof expected)==0);
    assert(musa_scene2d_utf8_count(out,written,&count) && count==9);
}
static void terminated_fields(void) {
    /* UTF-16LE 'A' + U+1F600 + NUL, followed by an unrelated odd byte. */
    const uint8_t record[]={0x41,0,0x3d,0xd8,0x00,0xde,0,0,0xff};
    const uint8_t expected[]={0x41,0xf0,0x9f,0x98,0x80};
    uint8_t output[16];
    size_t written=123, consumed=456;
    assert(musa_scene2d_utf16le_cstr_to_utf8(record,sizeof record,
                                               NULL,0,&written,&consumed));
    assert(written==sizeof expected && consumed==8);
    memset(output,0xa5,sizeof output);
    written=123; consumed=456;
    assert(musa_scene2d_utf16le_cstr_to_utf8(record,sizeof record,
                                               output,sizeof output,
                                               &written,&consumed));
    assert(written==sizeof expected && consumed==8);
    assert(memcmp(output,expected,sizeof expected)==0 &&
           output[written]==0 && output[written+1]==0xa5);
    memset(output,0xa5,sizeof output);
    written=123; consumed=456;
    assert(!musa_scene2d_utf16le_cstr_to_utf8(record,sizeof record,
                                                output,sizeof expected,
                                                &written,&consumed));
    assert(written==123 && consumed==456);
    for (size_t i=0;i<sizeof output;++i) assert(output[i]==0xa5);
    const uint8_t no_nul[]={0x41,0,0x42,0,0x7f};
    const uint8_t bad_pair[]={0x00,0xd8,0,0,0x7f};
    const uint8_t empty[]={0,0,0x7f};
    written=123; consumed=456;
    assert(!musa_scene2d_utf16le_cstr_to_utf8(no_nul,sizeof no_nul,
                                                output,sizeof output,
                                                &written,&consumed));
    assert(!musa_scene2d_utf16le_cstr_to_utf8(bad_pair,sizeof bad_pair,
                                                output,sizeof output,
                                                &written,&consumed));
    assert(written==123 && consumed==456 && output[0]==0xa5);
    assert(musa_scene2d_utf16le_cstr_to_utf8(empty,sizeof empty,
                                               output,sizeof output,
                                               &written,&consumed));
    assert(written==0 && consumed==2 && output[0]==0);
    written=123; consumed=456;
    assert(!musa_scene2d_utf16le_cstr_to_utf8(NULL,2,output,
                                                sizeof output,
                                                &written,&consumed));
    assert(!musa_scene2d_utf16le_cstr_to_utf8(record,1,output,
                                                sizeof output,
                                                &written,&consumed));
    assert(!musa_scene2d_utf16le_cstr_to_utf8(record,SIZE_MAX,NULL,0,
                                                &written,&consumed));
    assert(!musa_scene2d_utf16le_cstr_to_utf8(record,sizeof record,
                                                (uint8_t *)(void *)record,
                                                sizeof record,
                                                &written,&consumed));
    assert(!musa_scene2d_utf16le_cstr_to_utf8(record,sizeof record,
                                                output,sizeof output,
                                                &written,&written));
    assert(!musa_scene2d_utf16le_cstr_to_utf8(record,sizeof record,
                                                output,sizeof output,
                                                (size_t *)(void *)output,
                                                &consumed));
    assert(!musa_scene2d_utf16le_cstr_to_utf8(record,sizeof record,
                                                output,sizeof output,
                                                &written,
                                                (size_t *)(void *)output));
    assert(written==123 && consumed==456);
}
int main(void) {
    boundary_scalars();
    terminated_fields();
    /* A, c-cedilla, euro, CJK, supplementary smile, combining acute. */
    const uint8_t input[]={0x41,0,0xe7,0,0xac,0x20,0x2d,0x4e,
                           0x3d,0xd8,0x00,0xde,0x01,0x03};
    const uint8_t expected[]={0x41,0xc3,0xa7,0xe2,0x82,0xac,
                              0xe4,0xb8,0xad,0xf0,0x9f,0x98,0x80,
                              0xcc,0x81};
    size_t n=777;
    assert(musa_scene2d_utf16le_to_utf8(input,sizeof input,NULL,0,&n));
    assert(n==sizeof expected);
    uint8_t output[32];
    memset(output,0xa5,sizeof output);
    n=777;
    assert(!musa_scene2d_utf16le_to_utf8(input,sizeof input,
                                          output,sizeof expected,&n));
    assert(n==777 && output[0]==0xa5 && output[sizeof expected]==0xa5);
    assert(musa_scene2d_utf16le_to_utf8(input,sizeof input,
                                         output,sizeof output,&n));
    assert(n==sizeof expected && output[n]==0 &&
           memcmp(output,expected,n)==0 && output[n+1]==0xa5);
    size_t codepoints=0;
    assert(musa_scene2d_utf8_count(output,n,&codepoints) && codepoints==6);
    n=777;
    assert(musa_scene2d_utf16le_to_utf8(NULL,0,output,sizeof output,&n));
    assert(n==0 && output[0]==0);
    assert(musa_scene2d_utf16le_to_utf8(NULL,0,NULL,0,&n) && n==0);
    const uint8_t odd[]={0x41};
    const uint8_t nul[]={0,0};
    const uint8_t lone_high[]={0x00,0xd8};
    const uint8_t lone_low[]={0x00,0xdc};
    const uint8_t bad_pair[]={0x00,0xd8,0x41,0};
    const uint8_t wrong_pair[]={0x00,0xd8,0x00,0xd8};
    const uint8_t embedded_nul[]={0x41,0,0,0,0x42,0};
    invalid(odd,sizeof odd);
    invalid(nul,sizeof nul);
    invalid(lone_high,sizeof lone_high);
    invalid(lone_low,sizeof lone_low);
    invalid(bad_pair,sizeof bad_pair);
    invalid(wrong_pair,sizeof wrong_pair);
    invalid(embedded_nul,sizeof embedded_nul);
    n=777;
    assert(!musa_scene2d_utf16le_to_utf8(NULL,2,output,sizeof output,&n));
    assert(!musa_scene2d_utf16le_to_utf8(input,sizeof input,
                                         output,sizeof output,NULL));
    assert(!musa_scene2d_utf16le_to_utf8(input,sizeof input,output,0,&n));
    assert(!musa_scene2d_utf16le_to_utf8(input,sizeof input,NULL,1,&n));
    assert(!musa_scene2d_utf16le_to_utf8(input,SIZE_MAX,NULL,0,&n));
    assert(!musa_scene2d_utf16le_to_utf8(input,sizeof input,
                                         (uint8_t *)(void *)input,32,&n));
    assert(!musa_scene2d_utf16le_to_utf8(input,sizeof input,
                                         output,sizeof output,
                                         (size_t *)(void *)output));
    assert(!musa_scene2d_utf16le_to_utf8(input,sizeof input,NULL,0,
                                         (size_t *)(void *)input));
    /* Large predecoded layer-name/text payload with predictable length. */
    const size_t count=20000;
    uint8_t *long_input=malloc(count*2);
    uint8_t *long_output=malloc(count*3+1);
    assert(long_input && long_output);
    for (size_t i=0;i<count;++i) {
        long_input[2*i]=0xac; long_input[2*i+1]=0x20; /* euro */
    }
    assert(musa_scene2d_utf16le_to_utf8(long_input,count*2,NULL,0,&n));
    assert(n==count*3);
    assert(musa_scene2d_utf16le_to_utf8(long_input,count*2,
                                         long_output,count*3+1,&n));
    assert(n==count*3 && long_output[n]==0);
    assert(musa_scene2d_utf8_count(long_output,n,&codepoints));
    assert(codepoints==count);
    free(long_input); free(long_output);
    return 0;
}
