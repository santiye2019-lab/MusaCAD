/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_object_map.h"
#include "musa_dwg1032_modular.h"
#include <stdint.h>
#include <stddef.h>
static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int parse(const uint8_t *bytes, size_t byte_count, size_t count,
                 uint64_t section_bytes, MusaDwg1032MapEntry *out) {
    MusaDwg1032Cursor cursor;
    if (!musa_dwg1032_cursor_init(&cursor,bytes,byte_count)) return 0;
    uint64_t handle=0, offset=0;
    for (size_t i=0;i<count;++i) {
        uint64_t step=0;
        int64_t delta=0;
        if (!musa_dwg1032_modular_unsigned(&cursor,&step) || !step ||
            step>UINT64_MAX-handle ||
            !musa_dwg1032_modular_signed(&cursor,&delta)) return 0;
        handle+=step;
        if (delta<0) {
            const uint64_t back=(uint64_t)(-delta);
            if (back>offset) return 0;
            offset-=back;
        } else {
            const uint64_t forward=(uint64_t)delta;
            if (forward>UINT64_MAX-offset) return 0;
            offset+=forward;
        }
        if (offset>=section_bytes) return 0;
        if (out) out[i]=(MusaDwg1032MapEntry){handle,offset};
    }
    return cursor.bit_position/8u==byte_count;
}
int musa_dwg1032_object_map_decode(const uint8_t *bytes, size_t byte_count,
                                   size_t count, uint64_t section_bytes,
                                   MusaDwg1032MapEntry *out, size_t capacity) {
    if (byte_count>PTRDIFF_MAX/8u || (byte_count && !bytes) ||
        count>capacity || count>SIZE_MAX/sizeof(*out) ||
        (count && (!out || !section_bytes)) ||
        overlaps(bytes,byte_count,out,count*sizeof(*out))) return 0;
    if (!parse(bytes,byte_count,count,section_bytes,NULL)) return 0;
    return parse(bytes,byte_count,count,section_bytes,out);
}
