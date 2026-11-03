/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_point.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>

static void bulk_bitcoded(void) {
    /* Every 2BD pair is two 2-bit constant opcodes: 1.0, 0.0. */
    uint8_t packed[10000];
    memset(packed,0x66,sizeof packed); /* 01 10 01 10 */
    MusaDwg1032Cursor c;
    MusaDwg1032Point2 p={-1.0,-1.0};
    assert(musa_dwg1032_cursor_init(&c,packed,sizeof packed));
    for (size_t i=0;i<20000u;++i) {
        assert(musa_dwg1032_point2bd(&c,&p));
        assert(p.x==1.0 && p.y==0.0);
    }
    assert(c.bit_position==sizeof packed*8u);
    p.x=9.0; p.y=8.0;
    assert(!musa_dwg1032_point2bd(&c,&p));
    assert(c.bit_position==sizeof packed*8u && p.x==9.0 && p.y==8.0);
}
static void invalid_last_coordinate(void) {
    /* Three raw doubles, with non-finite last value; output is atomic. */
    const uint8_t bad[24]={
        0,0,0,0,0,0,0xf0,0x3f,
        0,0,0,0,0,0,0,0x40,
        0,0,0,0,0,0,0xf0,0x7f
    };
    MusaDwg1032Cursor c;
    MusaDwg1032Point3 p={4.0,5.0,6.0};
    assert(musa_dwg1032_cursor_init(&c,bad,sizeof bad));
    assert(!musa_dwg1032_point3rd(&c,&p));
    assert(c.bit_position==0 && p.x==4.0 && p.y==5.0 && p.z==6.0);
    assert(!musa_dwg1032_point3rd(&c,(MusaDwg1032Point3 *)(void *)&c));
    assert(c.bit_position==0);
    assert(!musa_dwg1032_point3rd(&c,(MusaDwg1032Point3 *)(void *)bad));
    assert(c.bit_position==0);
}
int main(void) { bulk_bitcoded(); invalid_last_coordinate(); return 0; }
