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
int musa_dwg1032_object_map_iterator_init(
    MusaDwg1032MapIterator *iterator, const uint8_t *bytes,
    size_t byte_count, uint64_t section_bytes) {
    if (!iterator ||
        overlaps(iterator,sizeof(*iterator),bytes,byte_count)) return 0;
    MusaDwg1032MapIterator trial;
    if (!musa_dwg1032_cursor_init(&trial.cursor,bytes,byte_count)) return 0;
    trial.handle=0;
    trial.offset=0;
    trial.section_bytes=section_bytes;
    trial.entries_read=0u;
    *iterator=trial;
    return 1;
}
int musa_dwg1032_object_map_iterator_next(
    MusaDwg1032MapIterator *iterator, MusaDwg1032MapEntry *entry) {
    if (!iterator || !entry ||
        overlaps(iterator,sizeof(*iterator),entry,sizeof(*entry)) ||
        overlaps(iterator->cursor.bytes,iterator->cursor.byte_count,
                 entry,sizeof(*entry))) return 0;
    MusaDwg1032MapIterator trial=*iterator;
    if (trial.entries_read==SIZE_MAX) return 0;
    uint64_t step=0;
    int64_t delta=0;
    if (!musa_dwg1032_modular_unsigned(&trial.cursor,&step) || !step ||
        step>UINT64_MAX-trial.handle ||
        !musa_dwg1032_modular_signed(&trial.cursor,&delta)) return 0;
    trial.handle+=step;
    if (delta<0) {
        const uint64_t back=(uint64_t)(-delta);
        if (back>trial.offset) return 0;
        trial.offset-=back;
    } else {
        const uint64_t forward=(uint64_t)delta;
        if (forward>UINT64_MAX-trial.offset) return 0;
        trial.offset+=forward;
    }
    if (trial.offset>=trial.section_bytes) return 0;
    const MusaDwg1032MapEntry candidate={trial.handle,trial.offset};
    ++trial.entries_read;
    *entry=candidate;
    *iterator=trial;
    return 1;
}
int musa_dwg1032_object_map_iterator_finished(
    const MusaDwg1032MapIterator *iterator, size_t expected_count) {
    if (!iterator ||
        iterator->cursor.byte_count>SIZE_MAX/8u ||
        (iterator->cursor.byte_count && !iterator->cursor.bytes))
        return 0;
    return iterator->entries_read==expected_count &&
           iterator->cursor.bit_position==
               iterator->cursor.byte_count*8u;
}
int musa_dwg1032_object_map_iterator_page(
    MusaDwg1032MapIterator *iterator, MusaDwg1032MapEntry *out,
    size_t count, size_t capacity) {
    if (!iterator || count>capacity ||
        count>SIZE_MAX/sizeof(*out) ||
        (count && !out) ||
        overlaps(iterator,sizeof(*iterator),out,count*sizeof(*out)) ||
        overlaps(iterator->cursor.bytes,iterator->cursor.byte_count,
                 out,count*sizeof(*out))) return 0;
    /* Validate the entire requested page before publishing any entry. */
    MusaDwg1032MapIterator trial=*iterator;
    for (size_t i=0;i<count;++i) {
        MusaDwg1032MapEntry entry;
        if (!musa_dwg1032_object_map_iterator_next(&trial,&entry))
            return 0;
    }
    /* The verified source is immutable; replaying the same bytes is
     * deterministic and requires no temporary page-sized allocation. */
    MusaDwg1032MapIterator publish=*iterator;
    for (size_t i=0;i<count;++i) {
        MusaDwg1032MapEntry entry;
        if (!musa_dwg1032_object_map_iterator_next(&publish,&entry))
            return 0;
        out[i]=entry;
    }
    *iterator=publish;
    return 1;
}
static int parse(const uint8_t *bytes, size_t byte_count, size_t count,
                 uint64_t section_bytes, MusaDwg1032MapEntry *out) {
    MusaDwg1032MapIterator iterator;
    if (!musa_dwg1032_object_map_iterator_init(
            &iterator,bytes,byte_count,section_bytes)) return 0;
    for (size_t i=0;i<count;++i) {
        MusaDwg1032MapEntry entry;
        if (!musa_dwg1032_object_map_iterator_next(&iterator,&entry))
            return 0;
        if (out) out[i]=entry;
    }
    return musa_dwg1032_object_map_iterator_finished(&iterator,count);
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

/* Complete delta-map to candidate-span bridge; no partial publication. */
int musa_dwg1032_object_map_decode_locations(
    const uint8_t *bytes, size_t byte_count, size_t count,
    uint64_t section_bytes, MusaDwg1032ObjectLocation *out,
    size_t capacity, MusaDwg1032MapEntry *scratch,
    size_t scratch_capacity, size_t *order, size_t order_capacity) {
    if (count>capacity || count>scratch_capacity ||
        count>order_capacity ||
        count>SIZE_MAX/sizeof(*out) ||
        count>SIZE_MAX/sizeof(*scratch) ||
        count>SIZE_MAX/sizeof(*order) ||
        (count && (!out || !scratch || !order)) ||
        overlaps(bytes,byte_count,out,count*sizeof(*out)) ||
        overlaps(bytes,byte_count,scratch,count*sizeof(*scratch)) ||
        overlaps(bytes,byte_count,order,count*sizeof(*order)) ||
        overlaps(out,count*sizeof(*out),scratch,count*sizeof(*scratch)) ||
        overlaps(out,count*sizeof(*out),order,count*sizeof(*order)) ||
        overlaps(scratch,count*sizeof(*scratch),order,count*sizeof(*order)))
        return 0;
    if (!musa_dwg1032_object_map_decode(
            bytes,byte_count,count,section_bytes,scratch,scratch_capacity))
        return 0;
    return musa_dwg1032_object_map_candidate_spans(
        scratch,count,section_bytes,out,capacity,order,order_capacity);
}
