/* SPDX-License-Identifier: MIT
 * Synthetic Unicode text payloads, not extracted AC1032 entity fixtures.
 */
#include "musa_dwg1032_text_utf16le.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static void multilingual_and_terminator(void) {
    /* U+00C7, "izim", space, U+03A9, space, U+1F600. */
    const uint8_t input[]={
        0xc7,0x00, 'i',0, 'z',0, 'i',0, 'm',0, ' ',0,
        0xa9,0x03, ' ',0, 0x3d,0xd8, 0x00,0xde
    };
    const char expected[]="\xc3\x87izim \xce\xa9 \xf0\x9f\x98\x80";
    char out[64];
    memset(out,0xa5,sizeof(out));
    size_t written=SIZE_MAX;
    assert(musa_dwg1032_text_utf16le_to_utf8(
        input,sizeof(input),out,sizeof(out),&written));
    assert(written==sizeof(expected)-1u);
    assert(memcmp(out,expected,sizeof(expected))==0);
    assert((unsigned char)out[sizeof(expected)]==0xa5u);
    memset(out,0xa5,sizeof(out));
    written=77u;
    assert(!musa_dwg1032_text_utf16le_to_utf8(
        input,sizeof(input),out,sizeof(expected)-1u,&written));
    assert(written==77u);
    for (size_t i=0;i<sizeof(out);++i)
        assert((unsigned char)out[i]==0xa5u);
    assert(musa_dwg1032_text_utf16le_to_utf8(
        input,sizeof(input),out,sizeof(expected),&written));
    assert(written==sizeof(expected)-1u);
    assert(musa_dwg1032_text_utf16le_to_utf8(
        NULL,0u,out,1u,&written));
    assert(written==0u && out[0]=='\0');
}

static void reject_malformed_without_publication(void) {
    const uint8_t lone_high[]={0x3d,0xd8};
    const uint8_t lone_low[]={0x00,0xde};
    const uint8_t bad_pair[]={0x3d,0xd8,'A',0};
    const uint8_t nul_middle[]={'A',0,0,0,'B',0};
    const uint8_t odd[]={'A',0,'B'};
    const uint8_t valid[]={'A',0};
    const struct { const uint8_t *bytes; size_t length; } bad[]={
        {lone_high,sizeof(lone_high)},
        {lone_low,sizeof(lone_low)},
        {bad_pair,sizeof(bad_pair)},
        {nul_middle,sizeof(nul_middle)},
        {odd,sizeof(odd)}
    };
    char out[32];
    size_t written=123u;
    for (size_t i=0;i<sizeof(bad)/sizeof(bad[0]);++i) {
        memset(out,0x5a,sizeof(out));
        assert(!musa_dwg1032_text_utf16le_to_utf8(
            bad[i].bytes,bad[i].length,out,sizeof(out),&written));
        assert(written==123u);
        for (size_t j=0;j<sizeof(out);++j) assert(out[j]=='Z');
    }
    assert(!musa_dwg1032_text_utf16le_to_utf8(
        NULL,2u,out,sizeof(out),&written));
    assert(!musa_dwg1032_text_utf16le_to_utf8(
        valid,sizeof(valid),NULL,sizeof(out),&written));
    assert(!musa_dwg1032_text_utf16le_to_utf8(
        valid,sizeof(valid),out,sizeof(out),NULL));
    assert(!musa_dwg1032_text_utf16le_to_utf8(
        valid,sizeof(valid),out,0u,&written));
    assert(!musa_dwg1032_text_utf16le_to_utf8(
        valid,SIZE_MAX,out,sizeof(out),&written));
    assert(!musa_dwg1032_text_utf16le_to_utf8(
        valid,sizeof(valid),out,SIZE_MAX,&written));
    assert(!musa_dwg1032_text_utf16le_to_utf8(
        valid,sizeof(valid),(char *)(void *)valid,sizeof(valid),&written));
    _Alignas(size_t) char aligned_out[32]={0};
    assert(!musa_dwg1032_text_utf16le_to_utf8(
        valid,sizeof(valid),aligned_out,sizeof(aligned_out),
        (size_t *)(void *)aligned_out));
    assert(written==123u);
}

static void large_text_and_late_failure(void) {
    enum { N=20000 };
    static uint8_t input[N*2u];
    static char output[N*3u+1u];
    for (size_t i=0;i<N;++i) {
        input[2u*i]=0xacu;
        input[2u*i+1u]=0x20u; /* U+20AC EURO SIGN */
    }
    size_t written=0u;
    assert(musa_dwg1032_text_utf16le_to_utf8(
        input,sizeof(input),output,sizeof(output),&written));
    assert(written==N*3u && output[written]=='\0');
    for (size_t i=0;i<N;++i) {
        assert((unsigned char)output[3u*i]==0xe2u);
        assert((unsigned char)output[3u*i+1u]==0x82u);
        assert((unsigned char)output[3u*i+2u]==0xacu);
    }
    input[2u*(N-1u)+1u]=0xd8u; /* Late unpaired high surrogate. */
    assert(!musa_dwg1032_text_utf16le_to_utf8(
        input,sizeof(input),output,sizeof(output),&written));
    assert(written==N*3u && output[written]=='\0');
    assert((unsigned char)output[0]==0xe2u);
    assert((unsigned char)output[3u*(N-1u)+2u]==0xacu);
}
int main(void) {
    multilingual_and_terminator();
    reject_malformed_without_publication();
    large_text_and_late_failure();
    return 0;
}
