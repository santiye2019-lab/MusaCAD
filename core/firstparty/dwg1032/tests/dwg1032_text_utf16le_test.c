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
/* Record field lengths here are synthetic, explicitly supplied by caller;
 * no real AC1032 entity text length or schema is assumed. */
static void cursor_field_transaction(void) {
    const uint8_t input[]={
        'A',0,0xe7,0,0x3d,0xd8,0x00,0xde,'B',0
    };
    const char expected[]="A\xc3\xa7\xf0\x9f\x98\x80";
    MusaDwg1032Cursor record;
    char output[32];
    size_t written=91u;
    memset(output,0x5a,sizeof(output));
    assert(musa_dwg1032_cursor_init(&record,input,sizeof(input)));
    assert(!musa_dwg1032_text_utf16le_take(
        &record,8u,output,sizeof(expected)-1u,&written));
    assert(record.bit_position==0u && written==91u && output[0]=='Z');
    assert(!musa_dwg1032_text_utf16le_take(
        &record,7u,output,sizeof(output),&written));
    assert(!musa_dwg1032_text_utf16le_take(
        &record,sizeof(input)+2u,output,sizeof(output),&written));
    assert(record.bit_position==0u && written==91u && output[0]=='Z');
    assert(!musa_dwg1032_text_utf16le_take(
        &record,8u,(char *)(void *)input,sizeof(input),&written));
    assert(!musa_dwg1032_text_utf16le_take(
        &record,8u,(char *)(void *)&record,sizeof(record),&written));
    assert(!musa_dwg1032_text_utf16le_take(
        &record,8u,output,sizeof(output),&record.bit_position));
    assert(record.bit_position==0u && written==91u && output[0]=='Z');
    assert(musa_dwg1032_cursor_skip_bits(&record,1u));
    assert(!musa_dwg1032_text_utf16le_take(
        &record,8u,output,sizeof(output),&written));
    assert(record.bit_position==1u && written==91u && output[0]=='Z');
    /* A forged cursor borrowing its own metadata is not a valid source. */
    record.bytes=(const uint8_t *)(const void *)&record;
    record.byte_count=sizeof(record);
    record.bit_position=0u;
    assert(!musa_dwg1032_text_utf16le_take(
        &record,2u,output,sizeof(output),&written));
    assert(record.bytes==(const uint8_t *)(const void *)&record &&
           record.bit_position==0u && written==91u && output[0]=='Z');
    assert(musa_dwg1032_cursor_init(&record,input,sizeof(input)));
    assert(musa_dwg1032_text_utf16le_take(
        &record,8u,output,sizeof(output),&written));
    assert(record.bit_position==64u && written==sizeof(expected)-1u);
    assert(memcmp(output,expected,sizeof(expected))==0);
    assert(musa_dwg1032_text_utf16le_take(
        &record,2u,output,sizeof(output),&written));
    assert(record.bit_position==80u && written==1u &&
           output[0]=='B' && output[1]=='\0');
    assert(musa_dwg1032_text_utf16le_take(
        &record,0u,output,sizeof(output),&written));
    assert(record.bit_position==80u && written==0u && output[0]=='\0');
    assert(!musa_dwg1032_text_utf16le_take(
        &record,2u,output,sizeof(output),&written));
    assert(record.bit_position==80u && written==0u);
    const uint8_t malformed[]={'A',0,0,0};
    assert(musa_dwg1032_cursor_init(&record,malformed,sizeof(malformed)));
    memset(output,0x5a,sizeof(output));
    written=19u;
    assert(!musa_dwg1032_text_utf16le_take(
        &record,sizeof(malformed),output,sizeof(output),&written));
    assert(record.bit_position==0u && written==19u && output[0]=='Z');
}

static void cursor_large_text_late_failure(void) {
    enum { N=20000 };
    static uint8_t input[N*2u];
    static char output[N*3u+1u];
    for (size_t i=0;i<N;++i) {
        input[2u*i]=0xacu;
        input[2u*i+1u]=0x20u;
    }
    MusaDwg1032Cursor record;
    size_t written=0u;
    assert(musa_dwg1032_cursor_init(&record,input,sizeof(input)));
    assert(musa_dwg1032_text_utf16le_take(
        &record,sizeof(input),output,sizeof(output),&written));
    assert(written==N*3u && record.bit_position==sizeof(input)*8u);
    assert(output[written]=='\0');
    input[2u*(N-1u)+1u]=0xd8u;
    assert(musa_dwg1032_cursor_init(&record,input,sizeof(input)));
    assert(!musa_dwg1032_text_utf16le_take(
        &record,sizeof(input),output,sizeof(output),&written));
    assert(record.bit_position==0u && written==N*3u);
    assert((unsigned char)output[0]==0xe2u);
    assert((unsigned char)output[N*3u-1u]==0xacu);
}
static void exact_capacity_preflight(void) {
    const uint8_t sample[]={'A',0,0x3d,0xd8,0x00,0xde,0xac,0x20};
    const uint8_t malformed[]={'A',0,0x3d,0xd8,'B',0};
    const uint8_t embedded_nul[]={'A',0,0,0};
    size_t required=777u;
    assert(musa_dwg1032_text_utf16le_measure(
        sample,sizeof(sample),&required));
    assert(required==9u); /* ASCII(1) + emoji(4) + euro(3) + NUL. */
    char output[9];
    size_t written=0u;
    assert(musa_dwg1032_text_utf16le_to_utf8(
        sample,sizeof(sample),output,required,&written));
    assert(written+1u==required && output[written]=='\0');
    assert(musa_dwg1032_text_utf16le_measure(NULL,0u,&required));
    assert(required==1u);
    required=777u;
    assert(!musa_dwg1032_text_utf16le_measure(
        malformed,sizeof(malformed),&required));
    assert(!musa_dwg1032_text_utf16le_measure(
        embedded_nul,sizeof(embedded_nul),&required));
    assert(!musa_dwg1032_text_utf16le_measure(
        sample,sizeof(sample)-1u,&required));
    assert(!musa_dwg1032_text_utf16le_measure(NULL,2u,&required));
    assert(!musa_dwg1032_text_utf16le_measure(
        sample,SIZE_MAX,&required));
    assert(!musa_dwg1032_text_utf16le_measure(
        sample,sizeof(sample),NULL));
    assert(required==777u);
    _Alignas(size_t) uint8_t aliased[sizeof(size_t)+2u]={0};
    assert(!musa_dwg1032_text_utf16le_measure(
        aliased,sizeof(aliased),(size_t *)(void *)aliased));
}
static void large_capacity_preflight(void) {
    enum { N=20000 };
    static uint8_t input[2u*N];
    for (size_t i=0;i<N;++i) {
        input[2u*i]=0xacu;
        input[2u*i+1u]=0x20u;
    }
    size_t required=0u;
    assert(musa_dwg1032_text_utf16le_measure(
        input,sizeof(input),&required));
    assert(required==3u*N+1u);
    input[2u*(N-1u)+1u]=0xd8u;
    assert(!musa_dwg1032_text_utf16le_measure(
        input,sizeof(input),&required));
    assert(required==3u*N+1u);
}

static void cursor_nonconsuming_measure(void) {
    const uint8_t input[]={'A',0,0x3d,0xd8,0x00,0xde,'B',0};
    MusaDwg1032Cursor cursor;
    assert(musa_dwg1032_cursor_init(&cursor,input,sizeof(input)));
    size_t required=77u;
    assert(musa_dwg1032_text_utf16le_peek_measure(
        &cursor,6u,&required));
    assert(required==6u && cursor.bit_position==0u);
    char output[6];
    size_t written=0u;
    assert(musa_dwg1032_text_utf16le_take(
        &cursor,6u,output,sizeof(output),&written));
    assert(written+1u==required && cursor.bit_position==48u);
    assert(musa_dwg1032_text_utf16le_peek_measure(
        &cursor,2u,&required));
    assert(required==2u && cursor.bit_position==48u);
    assert(!musa_dwg1032_text_utf16le_peek_measure(
        &cursor,3u,&required));
    assert(!musa_dwg1032_text_utf16le_peek_measure(
        &cursor,4u,&required));
    assert(required==2u && cursor.bit_position==48u);
    assert(musa_dwg1032_cursor_skip_bits(&cursor,1u));
    assert(!musa_dwg1032_text_utf16le_peek_measure(
        &cursor,0u,&required));
    assert(required==2u && cursor.bit_position==49u);
    assert(musa_dwg1032_cursor_init(&cursor,input,sizeof(input)));
    assert(!musa_dwg1032_text_utf16le_peek_measure(
        &cursor,6u,&cursor.bit_position));
    assert(cursor.bit_position==0u);
    _Alignas(size_t) uint8_t aligned[sizeof(size_t)+2u]={0};
    assert(musa_dwg1032_cursor_init(&cursor,aligned,sizeof(aligned)));
    assert(!musa_dwg1032_text_utf16le_peek_measure(
        &cursor,2u,(size_t *)(void *)aligned));
    assert(cursor.bit_position==0u);
    assert(!musa_dwg1032_text_utf16le_peek_measure(NULL,0u,&required));
    assert(!musa_dwg1032_text_utf16le_peek_measure(
        &cursor,0u,NULL));
    assert(required==2u);
    const uint8_t malformed[]={'A',0,0x3d,0xd8};
    assert(musa_dwg1032_cursor_init(
        &cursor,malformed,sizeof(malformed)));
    assert(!musa_dwg1032_text_utf16le_peek_measure(
        &cursor,sizeof(malformed),&required));
    assert(required==2u && cursor.bit_position==0u);
}

int main(void) {
    cursor_nonconsuming_measure();
    exact_capacity_preflight();
    large_capacity_preflight();
    multilingual_and_terminator();
    reject_malformed_without_publication();
    large_text_and_late_failure();
    cursor_field_transaction();
    cursor_large_text_late_failure();
    return 0;
}
