/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_bitcode.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct Bits { uint8_t data[96]; size_t count; } Bits;
static void bits(Bits *b, uint64_t v, unsigned n) {
    for (unsigned i=0;i<n;++i) {
        const unsigned bit=(unsigned)((v>>(n-1u-i))&1u);
        const size_t at=b->count++;
        assert(at<sizeof(b->data)*8u);
        if (bit) b->data[at/8u]|=(uint8_t)(1u<<(7u-at%8u));
    }
}
static void le(Bits *b, uint64_t v, unsigned bytes) {
    for (unsigned i=0;i<bytes;++i) bits(b,(v>>(8u*i))&255u,8u);
}
static void numeric_fields(void) {
    Bits b={{0},0};
    bits(&b,0,2); le(&b,257,2);
    bits(&b,2,2);
    bits(&b,3,2);
    bits(&b,1,2); le(&b,15,1);
    bits(&b,0,2); le(&b,0xffff,2);
    bits(&b,0,2); le(&b,0x12345678u,4);
    bits(&b,1,2); le(&b,254,1);
    bits(&b,2,2);
    bits(&b,0,2); le(&b,0xffffffffu,4);
    MusaDwg1032Cursor c;
    uint16_t s=0;
    uint32_t l=0;
    assert(musa_dwg1032_cursor_init(&c,b.data,(b.count+7u)/8u));
    assert(musa_dwg1032_bitcode_short(&c,&s) && s==257);
    assert(musa_dwg1032_bitcode_short(&c,&s) && s==0);
    assert(musa_dwg1032_bitcode_short(&c,&s) && s==256);
    assert(musa_dwg1032_bitcode_short(&c,&s) && s==15);
    assert(musa_dwg1032_bitcode_short(&c,&s) && s==65535);
    assert(musa_dwg1032_bitcode_long(&c,&l) && l==0x12345678u);
    assert(musa_dwg1032_bitcode_long(&c,&l) && l==254);
    assert(musa_dwg1032_bitcode_long(&c,&l) && l==0);
    assert(musa_dwg1032_bitcode_long(&c,&l) && l==0xffffffffu);
    assert(c.bit_position==b.count);
    s=999; l=999;
    assert(!musa_dwg1032_bitcode_short(&c,&s) && s==999);
    assert(!musa_dwg1032_bitcode_long(&c,&l) && l==999);
    assert(c.bit_position==b.count);
}
static void doubles_and_types(void) {
    Bits b={{0},0};
    bits(&b,0,2); le(&b,UINT64_C(0x400c000000000000),8);
    bits(&b,1,2); bits(&b,2,2);
    bits(&b,0,2); le(&b,0xa1,1);
    bits(&b,1,2); le(&b,3,1);
    bits(&b,2,2); le(&b,0x3412,2);
    MusaDwg1032Cursor c;
    double d=0;
    uint16_t t=0;
    assert(musa_dwg1032_cursor_init(&c,b.data,(b.count+7u)/8u));
    assert(musa_dwg1032_bitcode_double(&c,&d) && d==3.5);
    assert(musa_dwg1032_bitcode_double(&c,&d) && d==1.0);
    assert(musa_dwg1032_bitcode_double(&c,&d) && d==0.0);
    assert(musa_dwg1032_bitcode_object_type(&c,&t) && t==0xa1);
    assert(musa_dwg1032_bitcode_object_type(&c,&t) && t==0x1f3);
    assert(musa_dwg1032_bitcode_object_type(&c,&t) && t==0x3412);
    assert(c.bit_position==b.count);
}
static void rejected_and_rollback(void) {
    const uint8_t reserved[]={0xc0};
    MusaDwg1032Cursor c;
    uint32_t l=77;
    double d=7.0;
    uint16_t t=77;
    assert(musa_dwg1032_cursor_init(&c,reserved,sizeof reserved));
    assert(!musa_dwg1032_bitcode_long(&c,&l) && l==77);
    assert(!musa_dwg1032_bitcode_double(&c,&d) && d==7.0);
    assert(!musa_dwg1032_bitcode_object_type(&c,&t) && t==77);
    assert(c.bit_position==0);
    const uint8_t short_input[]={0,0x11};
    assert(musa_dwg1032_cursor_init(&c,short_input,sizeof short_input));
    assert(!musa_dwg1032_bitcode_long(&c,&l) && l==77);
    assert(!musa_dwg1032_bitcode_short(&c,&t) && t==77);
    assert(c.bit_position==0);
    assert(!musa_dwg1032_bitcode_short(&c,(uint16_t *)&c));
    assert(!musa_dwg1032_bitcode_long(NULL,&l));
    assert(!musa_dwg1032_bitcode_double(&c,NULL));
    Bits nonfinite={{0},0};
    bits(&nonfinite,0,2); le(&nonfinite,UINT64_C(0x7ff0000000000000),8);
    assert(musa_dwg1032_cursor_init(&c,nonfinite.data,(nonfinite.count+7u)/8u));
    assert(!musa_dwg1032_bitcode_double(&c,&d) && d==7.0);
    assert(c.bit_position==0);
    const uint8_t full[]={0,0x48,0xd1,0x59,0xe0};
    for (size_t n=0;n<sizeof full;++n) {
        assert(musa_dwg1032_cursor_init(&c,full,n));
        l=77;
        assert(!musa_dwg1032_bitcode_long(&c,&l));
        assert(l==77 && c.bit_position==0);
    }
}
int main(void) {
    numeric_fields(); doubles_and_types(); rejected_and_rollback();
    return 0;
}
