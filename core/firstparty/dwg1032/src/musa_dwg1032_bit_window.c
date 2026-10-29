/* SPDX-License-Identifier: MIT
 * Original C11 bit-range boundary enforcement; no external CAD code.
 */
#include "musa_dwg1032_bit_window.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int valid(const MusaDwg1032BitWindow *w) {
    if (!w || w->cursor.byte_count>PTRDIFF_MAX/8u ||
        (w->cursor.byte_count && !w->cursor.bytes)) return 0;
    const size_t total=w->cursor.byte_count*8u;
    return w->cursor.bit_position<=w->end_bit && w->end_bit<=total &&
           !overlaps(w,sizeof(*w),w->cursor.bytes,w->cursor.byte_count);
}
static int safe_output(const MusaDwg1032BitWindow *w,
                       const void *out, size_t count) {
    return out && !overlaps(w,sizeof(*w),out,count) &&
           !overlaps(w->cursor.bytes,w->cursor.byte_count,out,count);
}
int musa_dwg1032_bit_window_init(MusaDwg1032BitWindow *window,
                                 const MusaDwg1032Cursor *source,
                                 size_t start_bit, size_t bit_length) {
    if (!window || !source || source->byte_count>PTRDIFF_MAX/8u ||
        (source->byte_count && !source->bytes) ||
        overlaps(window,sizeof(*window),source,sizeof(*source)) ||
        overlaps(window,sizeof(*window),source->bytes,source->byte_count) ||
        overlaps(source,sizeof(*source),source->bytes,source->byte_count))
        return 0;
    const size_t total=source->byte_count*8u;
    if (source->bit_position>total || start_bit>total ||
        bit_length>total-start_bit) return 0;
    MusaDwg1032BitWindow next={*source,start_bit+bit_length};
    next.cursor.bit_position=start_bit;
    *window=next;
    return 1;
}
int musa_dwg1032_bit_window_remaining(const MusaDwg1032BitWindow *window,
                                      size_t *remaining) {
    if (!valid(window) || !safe_output(window,remaining,sizeof(*remaining)))
        return 0;
    *remaining=window->end_bit-window->cursor.bit_position;
    return 1;
}
int musa_dwg1032_bit_window_read32(MusaDwg1032BitWindow *window,
                                   unsigned bits, uint32_t *value) {
    if (!valid(window) || !safe_output(window,value,sizeof(*value)) ||
        !bits || bits>32u ||
        (size_t)bits>window->end_bit-window->cursor.bit_position) return 0;
    MusaDwg1032Cursor trial=window->cursor;
    uint32_t decoded=0;
    if (!musa_dwg1032_cursor_read_bits(&trial,bits,&decoded)) return 0;
    *value=decoded;
    window->cursor=trial;
    return 1;
}
int musa_dwg1032_bit_window_read64(MusaDwg1032BitWindow *window,
                                   unsigned bits, uint64_t *value) {
    if (!valid(window) || !safe_output(window,value,sizeof(*value)) ||
        !bits || bits>64u ||
        (size_t)bits>window->end_bit-window->cursor.bit_position) return 0;
    MusaDwg1032Cursor trial=window->cursor;
    uint64_t decoded=0;
    if (!musa_dwg1032_cursor_read_bits64(&trial,bits,&decoded)) return 0;
    *value=decoded;
    window->cursor=trial;
    return 1;
}
int musa_dwg1032_bit_window_skip(MusaDwg1032BitWindow *window,
                                 size_t bits) {
    if (!valid(window) ||
        bits>window->end_bit-window->cursor.bit_position) return 0;
    window->cursor.bit_position+=bits;
    return 1;
}
int musa_dwg1032_bit_window_take(MusaDwg1032BitWindow *parent,
                                 size_t bits, MusaDwg1032BitWindow *child) {
    if (!valid(parent) || !child ||
        overlaps(parent,sizeof(*parent),child,sizeof(*child)) ||
        overlaps(parent->cursor.bytes,parent->cursor.byte_count,
                 child,sizeof(*child)) ||
        bits>parent->end_bit-parent->cursor.bit_position) return 0;
    MusaDwg1032BitWindow next=*parent;
    next.end_bit=parent->cursor.bit_position+bits;
    *child=next;
    parent->cursor.bit_position=next.end_bit;
    return 1;
}
