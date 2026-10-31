/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_modular.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
static void unsigned_cases(void) {
    const uint8_t bytes[]={0,0x7f,0x80,0x01,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0x01};
    MusaDwg1032Cursor c;
    uint64_t v=99;
    assert(musa_dwg1032_cursor_init(&c,bytes,sizeof bytes));
    assert(musa_dwg1032_modular_unsigned(&c,&v) && v==0);
    assert(musa_dwg1032_modular_unsigned(&c,&v) && v==127);
    assert(musa_dwg1032_modular_unsigned(&c,&v) && v==128);
    assert(musa_dwg1032_modular_unsigned(&c,&v) && v==UINT64_MAX);
    assert(c.bit_position==sizeof(bytes)*8u);
    const uint8_t malformed[][3]={{0x80,0,0},{0x80,0x80,0},
                                  {0x80,0x80,0x80}};
    for (size_t i=0;i<3u;++i) {
        assert(musa_dwg1032_cursor_init(&c,malformed[i],3));
        v=42;
        assert(!musa_dwg1032_modular_unsigned(&c,&v));
        assert(v==42 && c.bit_position==0);
    }
    const uint8_t overflow[]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                              0xff,0xff,0x02};
    assert(musa_dwg1032_cursor_init(&c,overflow,sizeof overflow));
    assert(!musa_dwg1032_modular_unsigned(&c,&v));
    assert(c.bit_position==0);
}
static void signed_cases(void) {
    const uint8_t bytes[]={0x00,0x3f,0x41,0x85,0x4b,0x80,0x01,
                           0xc0,0x00,0x80,0xc0,0x40};
    MusaDwg1032Cursor c;
    int64_t v=99;
    assert(musa_dwg1032_cursor_init(&c,bytes,sizeof bytes));
    assert(musa_dwg1032_modular_signed(&c,&v) && v==0);
    assert(musa_dwg1032_modular_signed(&c,&v) && v==63);
    assert(musa_dwg1032_modular_signed(&c,&v) && v==-1);
    assert(musa_dwg1032_modular_signed(&c,&v) && v==-1413);
    assert(musa_dwg1032_modular_signed(&c,&v) && v==128);
    assert(musa_dwg1032_modular_signed(&c,&v) && v==64);
    assert(musa_dwg1032_modular_signed(&c,&v) && v==-8192);
    const uint8_t invalid[][2]={{0x40,0},{0x80,0},{0x80,0x40}};
    for (size_t i=0;i<3u;++i) {
        assert(musa_dwg1032_cursor_init(&c,invalid[i],i==0?1u:2u));
        v=42;
        assert(!musa_dwg1032_modular_signed(&c,&v));
        assert(v==42 && c.bit_position==0);
    }
}
/* The 10-byte signed MC terminal group can encode exactly -2^63.
 * Positive 2^63 and negative magnitudes beyond 2^63 must be rejected. */
static void signed_64bit_boundaries(void) {
    const uint8_t negative_min[]={
        0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x41
    };
    const uint8_t positive_overflow[]={
        0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x01
    };
    const uint8_t negative_overflow[]={
        0x81,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x41
    };
    MusaDwg1032Cursor c;
    int64_t value=123;
    assert(musa_dwg1032_cursor_init(&c,negative_min,sizeof(negative_min)));
    assert(musa_dwg1032_modular_signed(&c,&value));
    assert(value==INT64_MIN && c.bit_position==80u);
    const uint8_t *bad[]={positive_overflow,negative_overflow};
    for (size_t i=0;i<2u;++i) {
        assert(musa_dwg1032_cursor_init(&c,bad[i],10u));
        value=123;
        assert(!musa_dwg1032_modular_signed(&c,&value));
        assert(value==123 && c.bit_position==0u);
    }
}
static void short_cases(void) {
    const uint8_t bytes[]={0,0,0xff,0x7f,0x00,0x80,0x01,0,
                           0xff,0xff,0xff,0x7f};
    MusaDwg1032Cursor c;
    uint32_t v=99;
    assert(musa_dwg1032_cursor_init(&c,bytes,sizeof bytes));
    assert(musa_dwg1032_modular_short(&c,&v) && v==0);
    assert(musa_dwg1032_modular_short(&c,&v) && v==32767);
    assert(musa_dwg1032_modular_short(&c,&v) && v==32768);
    assert(musa_dwg1032_modular_short(&c,&v) && v==0x3fffffffu);
    assert(c.bit_position==sizeof bytes*8u);
    const uint8_t invalid[][4]={{0,0x80,0,0},{0,0x80,0,0x80},
                                {0,0x80,0,0}};
    for (size_t i=0;i<3u;++i) {
        assert(musa_dwg1032_cursor_init(&c,invalid[i],i==2?2u:4u));
        v=42;
        assert(!musa_dwg1032_modular_short(&c,&v));
        assert(v==42 && c.bit_position==0);
    }
}
int main(void) {
    unsigned_cases();signed_cases();signed_64bit_boundaries();short_cases();
    const uint8_t data[]={0x01,0x00,0x00,0x00};
    MusaDwg1032Cursor c;
    uint64_t u=7;
    assert(musa_dwg1032_cursor_init(&c,data,sizeof data));
    assert(musa_dwg1032_cursor_skip_bits(&c,1));
    assert(!musa_dwg1032_modular_unsigned(&c,&u) && u==7 && c.bit_position==1);
    assert(!musa_dwg1032_modular_unsigned(&c,(uint64_t *)&c));
    return 0;
}
