/* SPDX-License-Identifier: MIT
 * Independent synthetic UTF-16LE terminated-field regressions.
 * These fixtures do not claim to represent any AC1032 entity layout.
 */
#include "musa_dwg1032_text_utf16le.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static void bounded_sequence(void) {
    const uint8_t record[]={
        'A',0,0x3d,0xd8,0x00,0xde,0,0,
        'B',0,0,0,0x5a
    };
    const char expected[]="A\xf0\x9f\x98\x80";
    MusaDwg1032Cursor cursor;
    assert(musa_dwg1032_cursor_init(&cursor,record,sizeof(record)));
    size_t required=101u, consumed=102u, written=103u;
    char output[32];
    memset(output,0x5a,sizeof(output));
    assert(musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,sizeof(record),&required,&consumed));
    assert(required==sizeof(expected) && consumed==8u);
    assert(cursor.bit_position==0u);
    assert(!musa_dwg1032_text_utf16le_take_cstr(
        &cursor,7u,output,sizeof(output),&written));
    assert(!musa_dwg1032_text_utf16le_take_cstr(
        &cursor,8u,output,sizeof(expected)-1u,&written));
    assert(cursor.bit_position==0u && written==103u && output[0]=='Z');
    assert(musa_dwg1032_text_utf16le_take_cstr(
        &cursor,8u,output,sizeof(expected),&written));
    assert(cursor.bit_position==64u && written==sizeof(expected)-1u);
    assert(memcmp(output,expected,sizeof(expected))==0);
    assert(musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,SIZE_MAX,&required,&consumed));
    assert(required==2u && consumed==4u && cursor.bit_position==64u);
    assert(musa_dwg1032_text_utf16le_take_cstr(
        &cursor,4u,output,sizeof(output),&written));
    assert(cursor.bit_position==96u && written==1u &&
           output[0]=='B' && output[1]==0);
    assert(!musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,SIZE_MAX,&required,&consumed));
    assert(required==2u && consumed==4u && cursor.bit_position==96u);
}

static void invalid_and_rollback(void) {
    const uint8_t missing[]={'A',0,'B',0};
    const uint8_t malformed[]={'A',0,0x3d,0xd8,0,0};
    const uint8_t empty[]={0,0,'Q',0};
    const uint8_t odd[]={'A',0,0,0,'X'};
    MusaDwg1032Cursor cursor;
    size_t required=81u, consumed=82u, written=83u;
    char output[8];
    memset(output,0x5a,sizeof(output));
    assert(musa_dwg1032_cursor_init(&cursor,missing,sizeof(missing)));
    assert(!musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,sizeof(missing),&required,&consumed));
    assert(!musa_dwg1032_text_utf16le_take_cstr(
        &cursor,sizeof(missing),output,sizeof(output),&written));
    assert(cursor.bit_position==0u && required==81u &&
           consumed==82u && written==83u && output[0]=='Z');
    assert(musa_dwg1032_cursor_init(&cursor,malformed,sizeof(malformed)));
    assert(!musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,sizeof(malformed),&required,&consumed));
    assert(!musa_dwg1032_text_utf16le_take_cstr(
        &cursor,sizeof(malformed),output,sizeof(output),&written));
    assert(cursor.bit_position==0u && required==81u &&
           consumed==82u && written==83u && output[0]=='Z');
    assert(musa_dwg1032_cursor_init(&cursor,odd,sizeof(odd)));
    assert(!musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,3u,&required,&consumed));
    assert(musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,4u,&required,&consumed));
    assert(required==2u && consumed==4u);
    assert(musa_dwg1032_cursor_init(&cursor,empty,sizeof(empty)));
    assert(musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,sizeof(empty),&required,&consumed));
    assert(required==1u && consumed==2u);
    assert(musa_dwg1032_text_utf16le_take_cstr(
        &cursor,2u,output,1u,&written));
    assert(written==0u && output[0]==0 && cursor.bit_position==16u);
    assert(musa_dwg1032_cursor_skip_bits(&cursor,1u));
    assert(!musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,sizeof(empty),&required,&consumed));
    assert(!musa_dwg1032_text_utf16le_take_cstr(
        &cursor,sizeof(empty),output,sizeof(output),&written));
    assert(cursor.bit_position==17u && written==0u);
}

static void reject_aliasing(void) {
    _Alignas(size_t) uint8_t bytes[32]={0};
    bytes[0]='A'; bytes[2]=0; bytes[3]=0;
    MusaDwg1032Cursor cursor;
    assert(musa_dwg1032_cursor_init(&cursor,bytes,sizeof(bytes)));
    size_t required=19u, consumed=29u, written=39u;
    assert(!musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,4u,&cursor.bit_position,&consumed));
    assert(!musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,4u,&required,&required));
    assert(!musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,4u,(size_t *)(void *)bytes,&consumed));
    assert(!musa_dwg1032_text_utf16le_take_cstr(
        &cursor,4u,(char *)(void *)bytes,sizeof(bytes),&written));
    assert(!musa_dwg1032_text_utf16le_take_cstr(
        &cursor,4u,(char *)(void *)&cursor,sizeof(cursor),&written));
    char output[8];
    assert(!musa_dwg1032_text_utf16le_take_cstr(
        &cursor,4u,output,sizeof(output),&cursor.bit_position));
    assert(cursor.bit_position==0u && required==19u &&
           consumed==29u && written==39u);
    cursor.bytes=(const uint8_t *)(const void *)&cursor;
    cursor.byte_count=sizeof(cursor);
    assert(!musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,4u,&required,&consumed));
    assert(!musa_dwg1032_text_utf16le_take_cstr(
        &cursor,4u,output,sizeof(output),&written));
    assert(required==19u && consumed==29u && written==39u);
}

static void large_bounded_field(void) {
    enum { N=20000 };
    static uint8_t record[2u*N+4u];
    static char output[3u*N+1u];
    for (size_t i=0;i<N;++i) {
        record[2u*i]=0xacu; record[2u*i+1u]=0x20u;
    }
    record[2u*N]=0; record[2u*N+1u]=0;
    record[2u*N+2u]='X'; record[2u*N+3u]=0;
    MusaDwg1032Cursor cursor;
    assert(musa_dwg1032_cursor_init(&cursor,record,sizeof(record)));
    size_t required=0u, consumed=0u, written=17u;
    assert(musa_dwg1032_text_utf16le_peek_cstr(
        &cursor,sizeof(record),&required,&consumed));
    assert(required==3u*N+1u && consumed==2u*N+2u);
    assert(!musa_dwg1032_text_utf16le_take_cstr(
        &cursor,2u*N+1u,output,sizeof(output),&written));
    assert(cursor.bit_position==0u && written==17u);
    assert(musa_dwg1032_text_utf16le_take_cstr(
        &cursor,sizeof(record),output,sizeof(output),&written));
    assert(written==3u*N && output[written]==0);
    assert(cursor.bit_position==(2u*N+2u)*8u);
    for (size_t i=0;i<N;++i) {
        assert((unsigned char)output[3u*i]==0xe2u);
        assert((unsigned char)output[3u*i+1u]==0x82u);
        assert((unsigned char)output[3u*i+2u]==0xacu);
    }
    record[2u*N-1u]=0xd8u;
    assert(musa_dwg1032_cursor_init(&cursor,record,sizeof(record)));
    assert(!musa_dwg1032_text_utf16le_take_cstr(
        &cursor,sizeof(record),output,sizeof(output),&written));
    assert(cursor.bit_position==0u && written==3u*N &&
           output[0]==(char)0xe2u && output[3u*N]==0);
}
int main(void) {
    bounded_sequence();
    invalid_and_rollback();
    reject_aliasing();
    large_bounded_field();
    return 0;
}
