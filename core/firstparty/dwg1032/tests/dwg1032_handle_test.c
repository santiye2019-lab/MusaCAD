/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_handle.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct Bits { uint8_t bytes[64]; size_t bits; } Bits;
static void put(Bits *b, uint64_t value, unsigned count) {
    for (unsigned i=0;i<count;++i) {
        size_t at=b->bits++;
        assert(at<sizeof(b->bytes)*8u);
        if ((value>>(count-i-1u))&1u)
            b->bytes[at/8u]|=(uint8_t)(1u<<(7u-at%8u));
    }
}
static void handle_bits(Bits *b, uint8_t code, uint8_t count,
                        uint64_t payload) {
    put(b,code,4u); put(b,count,4u);
    for (unsigned i=count;i>0u;--i)
        put(b,(payload>>(8u*(i-1u)))&255u,8u);
}
static void absolute_and_relative(void) {
    Bits b={{0},0};
    put(&b,5u,3u); /* unaligned handle stream */
    handle_bits(&b,5u,2u,0x5e7u);
    handle_bits(&b,3u,1u,42u);
    handle_bits(&b,6u,0u,0u);
    handle_bits(&b,8u,0u,0u);
    handle_bits(&b,10u,1u,5u);
    handle_bits(&b,12u,1u,5u);
    MusaDwg1032Cursor c;
    MusaDwg1032Handle h;
    uint64_t resolved=0;
    assert(musa_dwg1032_cursor_init(&c,b.bytes,(b.bits+7u)/8u));
    uint32_t skip=0;
    assert(musa_dwg1032_cursor_read_bits(&c,3u,&skip) && skip==5u);
    assert(musa_dwg1032_handle_read(&c,&h));
    assert(h.code==5u && h.length==2u && h.payload==0x5e7u);
    assert(musa_dwg1032_handle_resolve(&h,0u,&resolved) && resolved==0x5e7u);
    assert(musa_dwg1032_handle_read(&c,&h) && h.code==3u);
    assert(musa_dwg1032_handle_resolve(&h,0u,&resolved) && resolved==42u);
    assert(musa_dwg1032_handle_read(&c,&h) && h.code==6u);
    assert(musa_dwg1032_handle_resolve(&h,10u,&resolved) && resolved==11u);
    assert(!musa_dwg1032_handle_resolve(&h,UINT64_MAX,&resolved));
    assert(musa_dwg1032_handle_read(&c,&h) && h.code==8u);
    assert(musa_dwg1032_handle_resolve(&h,10u,&resolved) && resolved==9u);
    assert(!musa_dwg1032_handle_resolve(&h,0u,&resolved));
    assert(musa_dwg1032_handle_read(&c,&h) && h.code==10u);
    assert(musa_dwg1032_handle_resolve(&h,10u,&resolved) && resolved==15u);
    assert(!musa_dwg1032_handle_resolve(&h,UINT64_MAX,&resolved));
    assert(musa_dwg1032_handle_read(&c,&h) && h.code==12u);
    assert(musa_dwg1032_handle_resolve(&h,10u,&resolved) && resolved==5u);
    assert(!musa_dwg1032_handle_resolve(&h,4u,&resolved));
    assert(c.bit_position==b.bits);
}
static void malformed_and_rollback(void) {
    const uint8_t truncated[]={0x52,0x05};
    MusaDwg1032Cursor c;
    MusaDwg1032Handle h={17u,3u,1u};
    for (size_t n=0;n<=sizeof truncated;++n) {
        assert(musa_dwg1032_cursor_init(&c,truncated,n));
        assert(!musa_dwg1032_handle_read(&c,&h));
        assert(c.bit_position==0 && h.payload==17u && h.code==3u);
    }
    const uint8_t bad_codes[]={0xf0,0x69,0xa0,0x00};
    for (size_t i=0;i<sizeof bad_codes;++i) {
        assert(musa_dwg1032_cursor_init(&c,bad_codes+i,1u));
        assert(!musa_dwg1032_handle_read(&c,&h));
        assert(c.bit_position==0 && h.payload==17u);
    }
    assert(!musa_dwg1032_handle_read(NULL,&h));
    assert(!musa_dwg1032_handle_read(&c,NULL));
    assert(!musa_dwg1032_handle_read(&c,(MusaDwg1032Handle *)&c));
    h=(MusaDwg1032Handle){7u,6u,0u};
    uint64_t result=999u;
    assert(!musa_dwg1032_handle_resolve(&h,1u,&result));
    assert(result==999u);
    h=(MusaDwg1032Handle){0u,5u,9u};
    assert(!musa_dwg1032_handle_resolve(&h,1u,&result));
    assert(result==999u);
}
int main(void) { absolute_and_relative(); malformed_and_rollback(); return 0; }
