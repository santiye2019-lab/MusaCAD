/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_cursor.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

int main(void) {
    const uint8_t data[]={0xa5,0x5a,0x81,0x7e,0x00,0xff,0x10,0x90,0x03};
    MusaDwg1032Cursor c;
    uint64_t value=0;
    assert(musa_dwg1032_cursor_init(&c,data,sizeof data));
    assert(musa_dwg1032_cursor_read_bits64(&c,64,&value));
    assert(value==UINT64_C(0xa55a817e00ff1090));
    assert(!musa_dwg1032_cursor_read_bits64(&c,9,&value));
    assert(value==UINT64_C(0xa55a817e00ff1090) && c.bit_position==64);
    assert(musa_dwg1032_cursor_read_bits64(&c,8,&value) && value==3);
    assert(!musa_dwg1032_cursor_read_bits64(&c,0,&value));
    assert(!musa_dwg1032_cursor_read_bits64(&c,65,&value));
    assert(!musa_dwg1032_cursor_read_bits64(&c,1,&value));
    assert(c.bit_position==72 && value==3);
    assert(musa_dwg1032_cursor_init(&c,data,sizeof data));
    assert(musa_dwg1032_cursor_read_u64le(&c,&value));
    assert(value==UINT64_C(0x9010ff007e815aa5));
    assert(!musa_dwg1032_cursor_read_u64le(&c,&value));
    assert(c.bit_position==64 && value==UINT64_C(0x9010ff007e815aa5));
    assert(!musa_dwg1032_cursor_read_bits64(&c,1,(uint64_t *)&c));
    assert(!musa_dwg1032_cursor_read_u64le(&c,(uint64_t *)&c));
    for (size_t n=0;n<=sizeof data;++n) {
        for (size_t offset=0;offset<=n*8u;++offset) {
            for (unsigned width=1;width<=64;++width) {
                assert(musa_dwg1032_cursor_init(&c,data,n));
                assert(musa_dwg1032_cursor_skip_bits(&c,offset));
                value=UINT64_MAX;
                const int ok=musa_dwg1032_cursor_read_bits64(&c,width,&value);
                assert(ok==(width<=n*8u-offset));
                if (!ok) assert(value==UINT64_MAX && c.bit_position==offset);
                else {
                    uint64_t expected=0;
                    for (unsigned i=0;i<width;++i) {
                        const size_t bit=offset+i;
                        expected=(expected<<1u)|((data[bit/8u]>>(7u-bit%8u))&1u);
                    }
                    assert(value==expected && c.bit_position==offset+width);
                }
            }
        }
    }
    return 0;
}
