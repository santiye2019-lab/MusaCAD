/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_point.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef struct Bits { uint8_t bytes[128]; size_t count; } Bits;
static void push(Bits *b, uint64_t value, unsigned count) {
    for (unsigned i=0;i<count;++i) {
        size_t pos=b->count++;
        assert(pos<sizeof(b->bytes)*8u);
        if ((value>>(count-1u-i))&1u)
            b->bytes[pos/8u]|=(uint8_t)(1u<<(7u-pos%8u));
    }
}
static void raw(Bits *b, uint64_t bits) {
    for (unsigned i=0;i<8u;++i) push(b,(bits>>(i*8u))&255u,8u);
}
static void bd_raw(Bits *b, uint64_t bits) {
    push(b,0u,2u); raw(b,bits);
}
static void positive(void) {
    Bits b={{0},0};
    push(&b,5u,3u); /* Fields need not be byte aligned. */
    raw(&b,UINT64_C(0x3ff4000000000000));
    raw(&b,UINT64_C(0xc004000000000000));
    bd_raw(&b,UINT64_C(0x400c000000000000));
    push(&b,1u,2u); push(&b,2u,2u);
    raw(&b,UINT64_C(0x3ff0000000000000));
    raw(&b,UINT64_C(0x4000000000000000));
    raw(&b,UINT64_C(0x4008000000000000));
    push(&b,1u,2u); push(&b,2u,2u); push(&b,1u,2u);
    MusaDwg1032Cursor c;
    MusaDwg1032Point2 a={0,0};
    MusaDwg1032Point3 p={0,0,0};
    assert(musa_dwg1032_cursor_init(&c,b.bytes,(b.count+7u)/8u));
    assert(musa_dwg1032_cursor_skip_bits(&c,3u));
    assert(musa_dwg1032_point2rd(&c,&a) && a.x==1.25 && a.y==-2.5);
    assert(musa_dwg1032_point3bd(&c,&p) && p.x==3.5 && p.y==1.0 && p.z==0.0);
    assert(musa_dwg1032_point3rd(&c,&p) && p.x==1.0 && p.y==2.0 && p.z==3.0);
    assert(musa_dwg1032_point3bd(&c,&p) && p.x==1.0 && p.y==0.0 && p.z==1.0);
    assert(c.bit_position==b.count);
}
static void rollback(void) {
    Bits b={{0},0};
    bd_raw(&b,UINT64_C(0x3ff0000000000000));
    push(&b,3u,2u); /* Reserved BD opcode for the second coordinate. */
    MusaDwg1032Cursor c;
    MusaDwg1032Point2 p={7.0,9.0};
    assert(musa_dwg1032_cursor_init(&c,b.bytes,(b.count+7u)/8u));
    assert(!musa_dwg1032_point2bd(&c,&p));
    assert(c.bit_position==0 && p.x==7.0 && p.y==9.0);
    Bits bad={{0},0};
    raw(&bad,UINT64_C(0x7ff8000000000000));
    raw(&bad,UINT64_C(0x3ff0000000000000));
    assert(musa_dwg1032_cursor_init(&c,bad.bytes,(bad.count+7u)/8u));
    assert(!musa_dwg1032_point2rd(&c,&p));
    assert(c.bit_position==0 && p.x==7.0 && p.y==9.0);
    Bits full={{0},0};
    raw(&full,UINT64_C(0x3ff0000000000000));
    raw(&full,UINT64_C(0x4000000000000000));
    raw(&full,UINT64_C(0x4008000000000000));
    for (size_t n=0;n<24u;++n) {
        assert(musa_dwg1032_cursor_init(&c,full.bytes,n));
        MusaDwg1032Point3 v={11.0,12.0,13.0};
        assert(!musa_dwg1032_point3rd(&c,&v));
        assert(c.bit_position==0 && v.x==11.0 && v.y==12.0 && v.z==13.0);
    }
    assert(musa_dwg1032_cursor_init(&c,full.bytes,24u));
    assert(!musa_dwg1032_point2rd(&c,(MusaDwg1032Point2 *)(void *)full.bytes));
    assert(!musa_dwg1032_point2bd(&c,(MusaDwg1032Point2 *)(void *)&c));
    assert(!musa_dwg1032_point3bd(NULL,&(MusaDwg1032Point3){0}));
}
int main(void) { positive(); rollback(); return 0; }
