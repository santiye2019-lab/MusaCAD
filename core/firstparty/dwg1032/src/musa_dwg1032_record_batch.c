/* SPDX-License-Identifier: MIT
 * Independently authored transactional, allocation-free frame extraction.
 */
#include "musa_dwg1032_record_batch.h"
#include "musa_dwg1032_object_spans.h"
#include "musa_dwg1032_record_frame.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}

static int extract_one(const MusaDwg1032ObjectIndex *index,
                       const MusaDwg1032Cursor *section, size_t position,
                       size_t limit, MusaDwg1032ObjectLocation *result) {
    MusaDwg1032Cursor candidate, frame;
    if (!musa_dwg1032_object_span_cursor(index,section,position,&candidate) ||
        !musa_dwg1032_record_take_ms_frame(&candidate,limit,&frame))
        return 0;
    const MusaDwg1032ObjectLocation e=index->entries[position];
    const size_t consumed=candidate.bit_position/8u;
    if (consumed<frame.byte_count || consumed>e.length) return 0;
    const uint64_t prefix=(uint64_t)(consumed-frame.byte_count);
    if (prefix>e.length || (uint64_t)frame.byte_count>e.length-prefix)
        return 0;
    *result=(MusaDwg1032ObjectLocation){
        e.handle,e.offset+prefix,(uint64_t)frame.byte_count};
    return 1;
}

int musa_dwg1032_record_batch_extract(
    const MusaDwg1032ObjectIndex *index,
    const MusaDwg1032Cursor *section,
    size_t max_payload_bytes,
    MusaDwg1032ObjectLocation *payloads, size_t capacity,
    size_t *order, size_t order_capacity) {
    if (!index || !section || index->count>capacity ||
        index->count>order_capacity ||
        index->count>SIZE_MAX/sizeof(MusaDwg1032ObjectLocation) ||
        index->count>SIZE_MAX/sizeof(size_t) ||
        section->byte_count>SIZE_MAX/8u ||
        index->section_bytes!=(uint64_t)section->byte_count ||
        section->bit_position!=0u ||
        (section->byte_count && !section->bytes) ||
        (index->count && (!payloads || !order || !max_payload_bytes)))
        return 0;
    const size_t out_bytes=index->count*sizeof(*payloads);
    const size_t order_bytes=index->count*sizeof(*order);
    const size_t entries_bytes=index->count*sizeof(*index->entries);
    if (overlaps(payloads,out_bytes,order,order_bytes) ||
        overlaps(payloads,out_bytes,index,sizeof(*index)) ||
        overlaps(payloads,out_bytes,section,sizeof(*section)) ||
        overlaps(payloads,out_bytes,index->entries,entries_bytes) ||
        overlaps(payloads,out_bytes,section->bytes,section->byte_count) ||
        overlaps(order,order_bytes,section,sizeof(*section)) ||
        overlaps(order,order_bytes,section->bytes,section->byte_count))
        return 0;
    if (!musa_dwg1032_object_spans_prepare(index,order,order_capacity))
        return 0;
    /* Preflight every record before writing even the first output. */
    for (size_t i=0;i<index->count;++i) {
        MusaDwg1032ObjectLocation ignored;
        if (!extract_one(index,section,i,max_payload_bytes,&ignored))
            return 0;
    }
    for (size_t i=0;i<index->count;++i) {
        MusaDwg1032ObjectLocation next;
        /* Inputs must remain immutable throughout this call. */
        if (!extract_one(index,section,i,max_payload_bytes,&next))
            return 0;
        payloads[i]=next;
    }
    return 1;
}
