/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_bit_window.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static void unaligned_and_nested(void) {
    const uint8_t data[]={0xb2u,0x63u,0x5au};
    MusaDwg1032Cursor src;
    MusaDwg1032BitWindow w, child;
    uint32_t value=0;
    size_t remaining=0;
    assert(musa_dwg1032_cursor_init(&src,data,sizeof data));
    assert(musa_dwg1032_bit_window_init(&w,&src,3u,18u));
    assert(musa_dwg1032_bit_window_take(&w,5u,&child));
    assert(w.cursor.bit_position==8u && child.end_bit==8u);
    assert(musa_dwg1032_bit_window_read32(&child,5u,&value) && value==18u);
    assert(!musa_dwg1032_bit_window_read32(&child,1u,&value));
    assert(value==18u && child.cursor.bit_position==8u);
    assert(musa_dwg1032_bit_window_read32(&w,8u,&value) && value==0x63u);
    assert(musa_dwg1032_bit_window_take(&w,5u,&child));
    assert(musa_dwg1032_bit_window_read32(&child,5u,&value) && value==11u);
    assert(musa_dwg1032_bit_window_remaining(&w,&remaining) && remaining==0u);
    assert(src.bit_position==0u);
}
static void rollback_and_aliases(void) {
    const uint8_t data[]={0xffu,0x12u,0x34u};
    MusaDwg1032Cursor src;
    MusaDwg1032BitWindow w, child, snapshot;
    uint32_t value=987u;
    size_t remain=77u;
    assert(musa_dwg1032_cursor_init(&src,data,sizeof data));
    assert(musa_dwg1032_bit_window_init(&w,&src,1u,7u));
    snapshot=w;
    assert(!musa_dwg1032_bit_window_read32(&w,8u,&value));
    assert(!musa_dwg1032_bit_window_read32(&w,0u,&value));
    assert(!musa_dwg1032_bit_window_read32(&w,33u,&value));
    assert(!musa_dwg1032_bit_window_read32(&w,1u,(uint32_t *)&w));
    assert(!musa_dwg1032_bit_window_skip(&w,SIZE_MAX));
    assert(!musa_dwg1032_bit_window_take(&w,8u,&child));
    assert(!musa_dwg1032_bit_window_take(&w,1u,&w));
    assert(!musa_dwg1032_bit_window_init(&w,&src,23u,2u));
    assert(!musa_dwg1032_bit_window_init(&w,&src,SIZE_MAX,1u));
    assert(!musa_dwg1032_bit_window_init(&w,&src,0u,SIZE_MAX));
    assert(w.cursor.bit_position==snapshot.cursor.bit_position &&
           w.end_bit==snapshot.end_bit && value==987u);
    assert(musa_dwg1032_bit_window_remaining(&w,&remain) && remain==7u);
    assert(!musa_dwg1032_bit_window_remaining(&w,(size_t *)&w));
    assert(musa_dwg1032_bit_window_skip(&w,7u));
    assert(!musa_dwg1032_bit_window_skip(&w,1u));
    assert(w.cursor.bit_position==8u);
    assert(musa_dwg1032_bit_window_take(&w,0u,&child));
    assert(child.cursor.bit_position==8u && child.end_bit==8u);
    assert(musa_dwg1032_bit_window_init(&w,&src,24u,0u));
    assert(!musa_dwg1032_bit_window_read32(&w,1u,&value));
    assert(musa_dwg1032_bit_window_init(&w,&src,0u,24u));
    assert(!musa_dwg1032_bit_window_read32(&w,8u,(uint32_t *)data));
}
static void full_width_and_corruption(void) {
    const uint8_t data[]={1u,0x23u,0x45u,0x67u,0x89u,0xabu,0xcdu,0xefu};
    MusaDwg1032Cursor src;
    MusaDwg1032BitWindow w;
    uint64_t value=0;
    assert(musa_dwg1032_cursor_init(&src,data,sizeof data));
    assert(musa_dwg1032_bit_window_init(&w,&src,0u,64u));
    assert(musa_dwg1032_bit_window_read64(&w,64u,&value));
    assert(value==UINT64_C(0x0123456789abcdef));
    assert(w.cursor.bit_position==64u);
    assert(!musa_dwg1032_bit_window_read64(&w,1u,&value));
    w.end_bit=65u;
    assert(!musa_dwg1032_bit_window_skip(&w,0u));
    w.end_bit=63u;
    assert(!musa_dwg1032_bit_window_skip(&w,0u));
    assert(musa_dwg1032_cursor_init(&src,NULL,0u));
    assert(musa_dwg1032_bit_window_init(&w,&src,0u,0u));
    assert(musa_dwg1032_bit_window_skip(&w,0u));
    assert(!musa_dwg1032_bit_window_read64(&w,1u,&value));
}
int main(void) {
    unaligned_and_nested();
    rollback_and_aliases();
    full_width_and_corruption();
    return 0;
}
