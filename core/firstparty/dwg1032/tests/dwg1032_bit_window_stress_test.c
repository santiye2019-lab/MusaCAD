/* SPDX-License-Identifier: MIT
 * Synthetic differential and stress checks; not a real AC1032 fixture.
 */
#include "musa_dwg1032_bit_window.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

static uint64_t reference_bits(const uint8_t *data, size_t start, unsigned n) {
    uint64_t result=0;
    for (unsigned i=0;i<n;++i) {
        const size_t bit=start+i;
        result=(result<<1u)|((data[bit/8u]>>(7u-bit%8u))&1u);
    }
    return result;
}
int main(void) {
    uint8_t data[257];
    for (size_t i=0;i<sizeof data;++i)
        data[i]=(uint8_t)((i*73u+19u)&0xffu);
    MusaDwg1032Cursor source;
    assert(musa_dwg1032_cursor_init(&source,data,sizeof data));
    const size_t total=sizeof data*8u;
    for (size_t i=0;i<20000u;++i) {
        const size_t start=(i*17u)%total;
        const unsigned n=(unsigned)((total-start)<32u?(total-start):32u);
        MusaDwg1032BitWindow window, nested;
        uint32_t result=UINT32_MAX;
        assert(musa_dwg1032_bit_window_init(&window,&source,start,n));
        assert(musa_dwg1032_bit_window_take(&window,n,&nested));
        assert(musa_dwg1032_bit_window_read32(&nested,n,&result));
        assert(result==(uint32_t)reference_bits(data,start,n));
        assert(nested.cursor.bit_position==start+n);
        assert(window.cursor.bit_position==start+n);
        assert(!musa_dwg1032_bit_window_read32(&nested,1u,&result));
        assert(result==(uint32_t)reference_bits(data,start,n));
        assert(source.bit_position==0u);
    }
    for (size_t i=0;i<4096u;++i) {
        const size_t start=(i*11u)%(total-64u+1u);
        MusaDwg1032BitWindow window;
        uint64_t value=0;
        assert(musa_dwg1032_bit_window_init(&window,&source,start,64u));
        assert(musa_dwg1032_bit_window_read64(&window,64u,&value));
        assert(value==reference_bits(data,start,64u));
    }
    return 0;
}
