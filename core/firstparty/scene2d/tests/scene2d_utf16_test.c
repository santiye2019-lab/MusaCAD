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
int main(void) {
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
