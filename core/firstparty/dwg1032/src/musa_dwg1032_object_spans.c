/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_object_spans.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int before(const MusaDwg1032ObjectLocation *entries,
                  size_t a, size_t b) {
    return entries[a].offset<entries[b].offset ||
           (entries[a].offset==entries[b].offset &&
            entries[a].handle<entries[b].handle);
}
/* Max-heap: the larger offset is closer to the root. */
static void sift(const MusaDwg1032ObjectLocation *entries, size_t *order,
                 size_t start, size_t end) {
    size_t root=start;
    while (root<end/2u) {
        size_t child=root*2u+1u;
        if (child>=end) break;
        if (child+1u<end && before(entries,order[child],order[child+1u]))
            ++child;
        if (!before(entries,order[root],order[child])) break;
        const size_t tmp=order[root]; order[root]=order[child]; order[child]=tmp;
        root=child;
    }
}
int musa_dwg1032_object_spans_prepare(const MusaDwg1032ObjectIndex *index,
                                      size_t *order, size_t capacity) {
    if (!index || index->count>capacity ||
        index->count>SIZE_MAX/sizeof(size_t) ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        (index->count && (!order || !index->entries))) return 0;
    const size_t order_bytes=index->count*sizeof(*order);
    const size_t entry_bytes=index->count*sizeof(*index->entries);
    if (overlaps(order,order_bytes,index,sizeof(*index)) ||
        overlaps(order,order_bytes,index->entries,entry_bytes)) return 0;
    uint64_t previous_handle=0;
    for (size_t i=0;i<index->count;++i) {
        const MusaDwg1032ObjectLocation e=index->entries[i];
        if (!e.handle || e.handle<=previous_handle || !e.length ||
            e.offset>index->section_bytes ||
            e.length>index->section_bytes-e.offset) return 0;
        previous_handle=e.handle;
    }
    for (size_t i=0;i<index->count;++i) order[i]=i;
    /* Build max heap. Using i-1 avoids unsigned underflow. */
    for (size_t i=index->count/2u;i>0u;--i)
        sift(index->entries,order,i-1u,index->count);
    for (size_t end=index->count;end>1u;--end) {
        const size_t tmp=order[0];order[0]=order[end-1u];order[end-1u]=tmp;
        sift(index->entries,order,0u,end-1u);
    }
    for (size_t i=1;i<index->count;++i) {
        const MusaDwg1032ObjectLocation left=index->entries[order[i-1u]];
        const MusaDwg1032ObjectLocation right=index->entries[order[i]];
        if (right.offset-left.offset<left.length) return 0;
    }
    return 1;
}
int musa_dwg1032_object_spans_find(const MusaDwg1032ObjectIndex *index,
                                   const size_t *order, uint64_t byte_offset,
                                   size_t *position) {
    if (!index || !position || !index->count || !index->entries || !order ||
        index->count>SIZE_MAX/sizeof(*order) ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        overlaps(position,sizeof(*position),index,sizeof(*index)) ||
        overlaps(position,sizeof(*position),order,index->count*sizeof(*order)) ||
        overlaps(position,sizeof(*position),index->entries,
                 index->count*sizeof(*index->entries)) ||
        byte_offset>=index->section_bytes) return 0;
    size_t lo=0,hi=index->count;
    while (lo<hi) {
        const size_t mid=lo+(hi-lo)/2u;
        const size_t idx=order[mid];
        if (idx>=index->count) return 0;
        if (index->entries[idx].offset<=byte_offset) lo=mid+1u;
        else hi=mid;
    }
    if (!lo) return 0;
    const size_t idx=order[lo-1u];
    if (idx>=index->count) return 0;
    const MusaDwg1032ObjectLocation e=index->entries[idx];
    if (byte_offset-e.offset>=e.length) return 0;
    *position=idx;
    return 1;
}
int musa_dwg1032_object_span_cursor(const MusaDwg1032ObjectIndex *index,
                                    const MusaDwg1032Cursor *section,
                                    size_t position, MusaDwg1032Cursor *object) {
    if (!index || !section || !object || position>=index->count ||
        !index->entries || index->count>SIZE_MAX/sizeof(*index->entries) ||
        index->section_bytes>SIZE_MAX ||
        section->byte_count!=(size_t)index->section_bytes ||
        section->byte_count>SIZE_MAX/8u || section->bit_position!=0u ||
        (section->byte_count && !section->bytes) ||
        overlaps(object,sizeof(*object),section,sizeof(*section)) ||
        overlaps(object,sizeof(*object),index,sizeof(*index)) ||
        overlaps(object,sizeof(*object),index->entries,
                 index->count*sizeof(*index->entries)) ||
        overlaps(object,sizeof(*object),section->bytes,section->byte_count))
        return 0;
    const MusaDwg1032ObjectLocation e=index->entries[position];
    if (!e.handle || !e.length || e.offset>index->section_bytes ||
        e.length>index->section_bytes-e.offset) return 0;
    const MusaDwg1032Cursor candidate={section->bytes+(size_t)e.offset,
                                        (size_t)e.length,0u};
    *object=candidate;
    return 1;
}
