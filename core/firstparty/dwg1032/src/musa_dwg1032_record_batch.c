/* SPDX-License-Identifier: MIT
 * Independently authored transactional, allocation-free frame extraction.
 */
#include "musa_dwg1032_record_batch.h"
#include "musa_dwg1032_object_spans.h"
#include "musa_dwg1032_record_frame.h"
#include "musa_dwg1032_object_map.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

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
        /* Sorting scratch is writable: it must never overwrite the
         * immutable section descriptor or its backing object bytes. */
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

/* Parse each already-located record exactly once into disjoint caller
 * scratch, then publish the complete array only after the last succeeds.
 * Unlike the two-pass API, this trades O(count) scratch for fewer reads. */
int musa_dwg1032_record_batch_extract_scratch(
    const MusaDwg1032ObjectIndex *index,
    const MusaDwg1032Cursor *section,
    size_t max_payload_bytes,
    MusaDwg1032ObjectLocation *payloads, size_t capacity,
    MusaDwg1032ObjectLocation *scratch, size_t scratch_capacity,
    size_t *order, size_t order_capacity) {
    if (!index || !section || index->count>capacity ||
        index->count>scratch_capacity || index->count>order_capacity ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        index->count>SIZE_MAX/sizeof(*payloads) ||
        index->count>SIZE_MAX/sizeof(*order) ||
        section->byte_count>SIZE_MAX/8u ||
        index->section_bytes!=(uint64_t)section->byte_count ||
        section->bit_position!=0u ||
        (section->byte_count && !section->bytes) ||
        (index->count && (!index->entries || !payloads || !scratch ||
                          !order || !max_payload_bytes)))
        return 0;
    const size_t count=index->count;
    const size_t result_bytes=count*sizeof(*payloads);
    const size_t order_bytes=count*sizeof(*order);
    const size_t entries_bytes=count*sizeof(*index->entries);
    const void *sources[]={index,section,index->entries,section->bytes};
    const size_t source_bytes[]={sizeof(*index),sizeof(*section),
                                 entries_bytes,section->byte_count};
    const void *destinations[]={payloads,scratch,order};
    const size_t destination_bytes[]={result_bytes,result_bytes,order_bytes};
    for (size_t i=0;i<3u;++i) {
        for (size_t j=i+1u;j<3u;++j)
            if (overlaps(destinations[i],destination_bytes[i],
                         destinations[j],destination_bytes[j])) return 0;
        for (size_t j=0;j<4u;++j)
            if (overlaps(destinations[i],destination_bytes[i],
                         sources[j],source_bytes[j])) return 0;
    }
    if (!musa_dwg1032_object_spans_prepare(index,order,order_capacity))
        return 0;
    for (size_t i=0;i<count;++i)
        if (!extract_one(index,section,i,max_payload_bytes,&scratch[i]))
            return 0;
    if (count) {
        /* One contiguous publication after all records have passed.
         * The two buffers are checked disjoint before the first read. */
        memcpy(payloads,scratch,result_bytes);
    }
    return 1;
}

/* A fast page reader for an immutable, previously span-validated index.
 * Per-page preflight preserves output even when a later frame is malformed.
 * Global span validation remains the caller's explicit prerequisite. */
int musa_dwg1032_record_batch_extract_page(
    const MusaDwg1032ObjectIndex *index,
    const MusaDwg1032Cursor *section,
    size_t start, size_t max_payload_bytes,
    MusaDwg1032ObjectLocation *payloads, size_t capacity,
    size_t *written, size_t *total) {
    if (!index || !section || !written || !total ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        section->byte_count>SIZE_MAX/8u ||
        index->section_bytes!=(uint64_t)section->byte_count ||
        section->bit_position!=0u ||
        (index->count && !index->entries) ||
        (section->byte_count && !section->bytes))
        return 0;
    const size_t remaining=start<index->count ? index->count-start : 0u;
    const size_t page_count=remaining<capacity ? remaining : capacity;
    if (page_count>SIZE_MAX/sizeof(*payloads) ||
        (page_count && (!payloads || !max_payload_bytes)))
        return 0;
    const size_t entries_bytes=index->count*sizeof(*index->entries);
    const size_t output_bytes=page_count*sizeof(*payloads);
    const void *inputs[]={index,section,index->entries,section->bytes};
    const size_t sizes[]={sizeof(*index),sizeof(*section),
                          entries_bytes,section->byte_count};
    for (size_t i=0;i<4u;++i) {
        if (overlaps(payloads,output_bytes,inputs[i],sizes[i]) ||
            overlaps(written,sizeof(*written),inputs[i],sizes[i]) ||
            overlaps(total,sizeof(*total),inputs[i],sizes[i]))
            return 0;
    }
    if (overlaps(payloads,output_bytes,written,sizeof(*written)) ||
        overlaps(payloads,output_bytes,total,sizeof(*total)) ||
        overlaps(written,sizeof(*written),total,sizeof(*total)))
        return 0;
    for (size_t i=0;i<page_count;++i) {
        MusaDwg1032ObjectLocation candidate;
        if (!extract_one(index,section,start+i,max_payload_bytes,&candidate))
            return 0;
    }
    for (size_t i=0;i<page_count;++i) {
        MusaDwg1032ObjectLocation candidate;
        /* The caller must keep the validated inputs immutable. */
        if (!extract_one(index,section,start+i,max_payload_bytes,&candidate))
            return 0;
        payloads[i]=candidate;
    }
    *written=page_count;
    *total=index->count;
    return 1;
}

/* Random-access viewport lookup in an immutable, span-validated map.
 * The handle index remains in logical order; no full-map re-sort is needed.
 * The caller owns the lifetime and validation of the decoded section. */
int musa_dwg1032_record_batch_extract_handle(
    const MusaDwg1032ObjectIndex *index,
    const MusaDwg1032Cursor *section, uint64_t handle,
    size_t max_payload_bytes, MusaDwg1032ObjectLocation *payload) {
    if (!index || !section || !payload || !handle || !max_payload_bytes ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        section->byte_count>SIZE_MAX/8u ||
        index->section_bytes!=(uint64_t)section->byte_count ||
        section->bit_position!=0u ||
        (index->count && !index->entries) ||
        (section->byte_count && !section->bytes) ||
        overlaps(payload,sizeof(*payload),index,sizeof(*index)) ||
        overlaps(payload,sizeof(*payload),section,sizeof(*section)) ||
        overlaps(payload,sizeof(*payload),index->entries,
                 index->count*sizeof(*index->entries)) ||
        overlaps(payload,sizeof(*payload),section->bytes,section->byte_count))
        return 0;
    size_t position=0;
    if (!musa_dwg1032_object_index_find(index,handle,&position))
        return 0;
    MusaDwg1032ObjectLocation result;
    if (!extract_one(index,section,position,max_payload_bytes,&result))
        return 0;
    *payload=result;
    return 1;
}

/* Integrate already-verified, decompressed map and object section buffers.
 * Scratch is caller-owned; the payload result is published only after every
 * candidate frame has passed the bounded, transactional batch preflight.
 * This does NOT discover or authenticate DWG pages or decode entities. */
int musa_dwg1032_record_batch_from_verified_map(
    const uint8_t *map_bytes, size_t map_byte_count, size_t object_count,
    const MusaDwg1032Cursor *section, size_t max_payload_bytes,
    MusaDwg1032MapEntry *map_entries, size_t map_capacity,
    MusaDwg1032ObjectLocation *candidates, size_t candidate_capacity,
    MusaDwg1032ObjectLocation *payloads, size_t payload_capacity,
    size_t *order, size_t order_capacity) {
    if (!section || object_count>map_capacity ||
        object_count>candidate_capacity || object_count>payload_capacity ||
        object_count>order_capacity ||
        object_count>SIZE_MAX/sizeof(*map_entries) ||
        object_count>SIZE_MAX/sizeof(*candidates) ||
        object_count>SIZE_MAX/sizeof(*payloads) ||
        object_count>SIZE_MAX/sizeof(*order) ||
        section->byte_count>SIZE_MAX/8u ||
        section->bit_position!=0u ||
        (section->byte_count && !section->bytes) ||
        (map_byte_count && !map_bytes) ||
        (object_count && (!map_entries || !candidates || !payloads ||
                          !order || !max_payload_bytes)))
        return 0;
    const void *scratch[]={map_entries,candidates,payloads,order};
    const size_t lengths[]={
        object_count*sizeof(*map_entries),
        object_count*sizeof(*candidates),
        object_count*sizeof(*payloads),
        object_count*sizeof(*order)
    };
    const void *inputs[]={map_bytes,section,section->bytes};
    const size_t input_lengths[]={
        map_byte_count,sizeof(*section),section->byte_count
    };
    for (size_t i=0;i<4u;++i) {
        for (size_t j=i+1u;j<4u;++j)
            if (overlaps(scratch[i],lengths[i],scratch[j],lengths[j]))
                return 0;
        for (size_t j=0;j<3u;++j)
            if (overlaps(scratch[i],lengths[i],inputs[j],input_lengths[j]))
                return 0;
    }
    if (!musa_dwg1032_object_map_decode(
            map_bytes,map_byte_count,object_count,
            (uint64_t)section->byte_count,map_entries,map_capacity) ||
        !musa_dwg1032_object_map_candidate_spans(
            map_entries,object_count,(uint64_t)section->byte_count,
            candidates,candidate_capacity,order,order_capacity))
        return 0;
    const MusaDwg1032ObjectIndex index={
        candidates,object_count,(uint64_t)section->byte_count
    };
    return musa_dwg1032_record_batch_extract(
        &index,section,max_payload_bytes,payloads,payload_capacity,
        order,order_capacity);
}
