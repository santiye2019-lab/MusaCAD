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

/* Heap ordering by physical offsets, without mutating the handle index. */
static int offset_before(const MusaDwg1032MapEntry *entries,
                         size_t a, size_t b) {
    return entries[a].offset<entries[b].offset ||
        (entries[a].offset==entries[b].offset &&
         entries[a].handle<entries[b].handle);
}
static void offset_sift(const MusaDwg1032MapEntry *entries, size_t *order,
                        size_t start, size_t end) {
    size_t root=start;
    while (root<end/2u) {
        size_t child=root*2u+1u;
        if (child>=end) break;
        if (child+1u<end &&
            offset_before(entries,order[child],order[child+1u])) ++child;
        if (!offset_before(entries,order[root],order[child])) break;
        const size_t swap=order[root];
        order[root]=order[child];
        order[child]=swap;
        root=child;
    }
}
int musa_dwg1032_object_map_candidate_spans(
    const MusaDwg1032MapEntry *entries, size_t count, uint64_t section_bytes,
    MusaDwg1032ObjectLocation *out, size_t capacity,
    size_t *order, size_t order_capacity) {
    if (count>capacity || count>order_capacity ||
        count>SIZE_MAX/sizeof(*entries) ||
        count>SIZE_MAX/sizeof(*out) ||
        count>SIZE_MAX/sizeof(*order) ||
        (count && (!entries || !out || !order || !section_bytes)))
        return 0;
    const size_t input_bytes=count*sizeof(*entries);
    const size_t output_bytes=count*sizeof(*out);
    const size_t order_bytes=count*sizeof(*order);
    if (overlaps(entries,input_bytes,out,output_bytes) ||
        overlaps(entries,input_bytes,order,order_bytes) ||
        overlaps(out,output_bytes,order,order_bytes))
        return 0;
    uint64_t previous_handle=0;
    for (size_t i=0;i<count;++i) {
        if (!entries[i].handle || entries[i].handle<=previous_handle ||
            entries[i].offset>=section_bytes) return 0;
        previous_handle=entries[i].handle;
    }
    for (size_t i=0;i<count;++i) order[i]=i;
    for (size_t i=count/2u;i>0u;--i)
        offset_sift(entries,order,i-1u,count);
    for (size_t end=count;end>1u;--end) {
        const size_t swap=order[0];
        order[0]=order[end-1u];
        order[end-1u]=swap;
        offset_sift(entries,order,0u,end-1u);
    }
    for (size_t i=1;i<count;++i)
        if (entries[order[i-1u]].offset==entries[order[i]].offset)
            return 0;
    /* Validation complete: publish only after every possible failure. */
    for (size_t i=0;i<count;++i) {
        const size_t at=order[i];
        const uint64_t begin=entries[at].offset;
        const uint64_t end=(i+1u<count)
            ? entries[order[i+1u]].offset : section_bytes;
        out[at]=(MusaDwg1032ObjectLocation){
            entries[at].handle,begin,end-begin};
    }
    return 1;
}
