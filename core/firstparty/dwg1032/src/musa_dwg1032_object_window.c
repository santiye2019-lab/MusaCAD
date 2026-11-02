/* SPDX-License-Identifier: MIT
 * Independent C11 lazy object-window bridge over verified raw page ranges.
 */
#include "musa_dwg1032_object_window.h"
#include "musa_dwg1032_modular.h"
#include "musa_dwg1032_cursor.h"
#include "musa_dwg1032_section_window_scratch.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}

/* Both ordinal and handle-based reads use the same constant-time local
 * borrowed-index guard. This does not replace full index immutability. */
int musa_dwg1032_object_candidate_read_window_at_position(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    const MusaDwg1032ObjectIndex *index,
    size_t position, uint64_t candidate_offset,
    uint8_t *destination, size_t byte_count, uint64_t *candidate_length) {
    if (!index || !candidate_length || position>=index->count ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        page_count>SIZE_MAX/sizeof(*pages) ||
        file_size>PTRDIFF_MAX ||
        (file_size && !file_bytes) ||
        (index->count && !index->entries) ||
        (page_count && !pages) ||
        (byte_count && !destination))
        return 0;
    const size_t entries_bytes=index->count*sizeof(*index->entries);
    const size_t pages_bytes=page_count*sizeof(*pages);
    if (overlaps(candidate_length,sizeof(*candidate_length),
                 destination,byte_count) ||
        overlaps(candidate_length,sizeof(*candidate_length),
                 index,sizeof(*index)) ||
        overlaps(candidate_length,sizeof(*candidate_length),
                 index->entries,entries_bytes) ||
        overlaps(candidate_length,sizeof(*candidate_length),
                 pages,pages_bytes) ||
        overlaps(candidate_length,sizeof(*candidate_length),
                 file_bytes,file_size) ||
        overlaps(destination,byte_count,index,sizeof(*index)) ||
        overlaps(destination,byte_count,index->entries,entries_bytes) ||
        overlaps(destination,byte_count,pages,pages_bytes) ||
        overlaps(destination,byte_count,file_bytes,file_size))
        return 0;

    if (!musa_dwg1032_object_index_neighborhood_valid(index,position)) return 0;
    const MusaDwg1032ObjectLocation entry=index->entries[position];
    if (candidate_offset>entry.length ||
        (uint64_t)byte_count>entry.length-candidate_offset)
        return 0;

    /* The page reader preflights every touched source page before copying.
     * It requires the same immutable, previously validated directory. */
    if (!musa_dwg1032_section_read_window(
            file_bytes,file_size,pages,page_count,index->section_bytes,
            entry.offset+candidate_offset,destination,byte_count))
        return 0;
    *candidate_length=entry.length;
    return 1;
}

/* A viewport can request a few bytes from an already located candidate
 * without mapping or assembling the full logical section. The short prefix
 * is parsed by the existing independent, canonical MS scalar reader. */
/* Reuse one validated handle lookup for the prefix and payload. This
 * avoids three independent index searches per visible object while keeping
 * the page reader's two-pass, no-partial-copy guarantee. */
int musa_dwg1032_object_candidate_read_window(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    const MusaDwg1032ObjectIndex *index,
    uint64_t handle, uint64_t candidate_offset,
    uint8_t *destination, size_t byte_count, uint64_t *candidate_length) {
    size_t position=0;
    if (!musa_dwg1032_object_index_find(index,handle,&position))
        return 0;
    return musa_dwg1032_object_candidate_read_window_at_position(
        file_bytes,file_size,pages,page_count,index,position,candidate_offset,
        destination,byte_count,candidate_length);
}

int musa_dwg1032_object_ms_payload_read_window_at_position(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    const MusaDwg1032ObjectIndex *index,
    size_t position, uint64_t payload_offset, size_t max_payload_bytes,
    uint8_t *destination, size_t byte_count, uint64_t *payload_length) {
    if (!index || !payload_length || position>=index->count || !max_payload_bytes ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        page_count>SIZE_MAX/sizeof(*pages) ||
        file_size>PTRDIFF_MAX ||
        (file_size && !file_bytes) ||
        (index->count && !index->entries) ||
        (page_count && !pages) ||
        (byte_count && !destination))
        return 0;
    const size_t entries_bytes=index->count*sizeof(*index->entries);
    const size_t pages_bytes=page_count*sizeof(*pages);
    if (overlaps(payload_length,sizeof(*payload_length),
                 destination,byte_count) ||
        overlaps(payload_length,sizeof(*payload_length),
                 index,sizeof(*index)) ||
        overlaps(payload_length,sizeof(*payload_length),
                 index->entries,entries_bytes) ||
        overlaps(payload_length,sizeof(*payload_length),
                 pages,pages_bytes) ||
        overlaps(payload_length,sizeof(*payload_length),
                 file_bytes,file_size) ||
        overlaps(destination,byte_count,index,sizeof(*index)) ||
        overlaps(destination,byte_count,index->entries,entries_bytes) ||
        overlaps(destination,byte_count,pages,pages_bytes) ||
        overlaps(destination,byte_count,file_bytes,file_size))
        return 0;

    if (!musa_dwg1032_object_index_neighborhood_valid(index,position)) return 0;
    const MusaDwg1032ObjectLocation entry=index->entries[position];
    if (entry.length<2u) return 0;

    uint8_t prefix[4]={0};
    if (!musa_dwg1032_section_read_window(
            file_bytes,file_size,pages,page_count,index->section_bytes,
            entry.offset,prefix,2u))
        return 0;
    const size_t prefix_size=(prefix[1]&0x80u)?4u:2u;
    if ((uint64_t)prefix_size>entry.length)
        return 0;
    if (prefix_size==4u &&
        !musa_dwg1032_section_read_window(
            file_bytes,file_size,pages,page_count,index->section_bytes,
            entry.offset+2u,prefix+2u,2u))
        return 0;

    MusaDwg1032Cursor cursor;
    uint32_t length=0;
    if (!musa_dwg1032_cursor_init(&cursor,prefix,prefix_size) ||
        !musa_dwg1032_modular_short(&cursor,&length) ||
        !length || (size_t)length>max_payload_bytes ||
        (uint64_t)length>entry.length-(uint64_t)prefix_size ||
        payload_offset>(uint64_t)length ||
        (uint64_t)byte_count>(uint64_t)length-payload_offset)
        return 0;

    if (!musa_dwg1032_section_read_window(
            file_bytes,file_size,pages,page_count,index->section_bytes,
            entry.offset+(uint64_t)prefix_size+payload_offset,
            destination,byte_count))
        return 0;
    *payload_length=(uint64_t)length;
    return 1;
}
int musa_dwg1032_object_ms_payload_read_window(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    const MusaDwg1032ObjectIndex *index,
    uint64_t handle, uint64_t payload_offset, size_t max_payload_bytes,
    uint8_t *destination, size_t byte_count, uint64_t *payload_length) {
    size_t position=0;
    if (!musa_dwg1032_object_index_find(index,handle,&position))
        return 0;
    return musa_dwg1032_object_ms_payload_read_window_at_position(
        file_bytes,file_size,pages,page_count,index,position,payload_offset,
        max_payload_bytes,destination,byte_count,payload_length);
}

/* Reuse the immutable ordinal index without a second page-copy validation
 * pass. The scratch reader commits destination only after every touched page
 * has passed its physical and logical boundary checks. */
int musa_dwg1032_object_candidate_read_window_scratch_at_position(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    const MusaDwg1032ObjectIndex *index,
    size_t position, uint64_t candidate_offset,
    uint8_t *destination, size_t byte_count, uint64_t *candidate_length,
    uint8_t *scratch, size_t scratch_capacity) {
    if (!index || !candidate_length || position>=index->count ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        page_count>SIZE_MAX/sizeof(*pages) ||
        file_size>PTRDIFF_MAX || byte_count>PTRDIFF_MAX ||
        scratch_capacity>PTRDIFF_MAX ||
        (file_size && !file_bytes) ||
        (index->count && !index->entries) ||
        (page_count && !pages) ||
        (byte_count && (!destination || !scratch)) ||
        (scratch_capacity && !scratch) ||
        byte_count>scratch_capacity)
        return 0;
    const size_t entries_bytes=index->count*sizeof(*index->entries);
    const size_t pages_bytes=page_count*sizeof(*pages);
    if (overlaps(candidate_length,sizeof(*candidate_length),
                 destination,byte_count) ||
        overlaps(candidate_length,sizeof(*candidate_length),
                 scratch,scratch_capacity) ||
        overlaps(candidate_length,sizeof(*candidate_length),
                 index,sizeof(*index)) ||
        overlaps(candidate_length,sizeof(*candidate_length),
                 index->entries,entries_bytes) ||
        overlaps(candidate_length,sizeof(*candidate_length),
                 pages,pages_bytes) ||
        overlaps(candidate_length,sizeof(*candidate_length),
                 file_bytes,file_size) ||
        overlaps(destination,byte_count,index,sizeof(*index)) ||
        overlaps(destination,byte_count,index->entries,entries_bytes) ||
        overlaps(destination,byte_count,pages,pages_bytes) ||
        overlaps(destination,byte_count,file_bytes,file_size) ||
        overlaps(scratch,scratch_capacity,index,sizeof(*index)) ||
        overlaps(scratch,scratch_capacity,index->entries,entries_bytes))
        return 0;

    if (!musa_dwg1032_object_index_neighborhood_valid(index,position))
        return 0;
    const MusaDwg1032ObjectLocation entry=index->entries[position];
    if (candidate_offset>entry.length ||
        (uint64_t)byte_count>entry.length-candidate_offset)
        return 0;
    if (!musa_dwg1032_section_read_window_scratch(
            file_bytes,file_size,pages,page_count,index->section_bytes,
            entry.offset+candidate_offset,destination,byte_count,
            scratch,scratch_capacity))
        return 0;
    *candidate_length=entry.length;
    return 1;
}

int musa_dwg1032_object_candidate_read_window_scratch(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    const MusaDwg1032ObjectIndex *index,
    uint64_t handle, uint64_t candidate_offset,
    uint8_t *destination, size_t byte_count, uint64_t *candidate_length,
    uint8_t *scratch, size_t scratch_capacity) {
    size_t position=0;
    if (!musa_dwg1032_object_index_find(index,handle,&position))
        return 0;
    return musa_dwg1032_object_candidate_read_window_scratch_at_position(
        file_bytes,file_size,pages,page_count,index,position,
        candidate_offset,destination,byte_count,candidate_length,
        scratch,scratch_capacity);
}

/* Scratch-backed counterpart to the MS candidate payload window reader.
 * The prefix is parsed from a bounded, immutable RAW candidate before any
 * public output is written. The payload copy uses the transactional scratch
 * page reader; the scratch region is caller-owned and disposable. */
int musa_dwg1032_object_ms_payload_read_window_scratch_at_position(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    const MusaDwg1032ObjectIndex *index,
    size_t position, uint64_t payload_offset, size_t max_payload_bytes,
    uint8_t *destination, size_t byte_count, uint64_t *payload_length,
    uint8_t *scratch, size_t scratch_capacity) {
    if (!index || !payload_length || !max_payload_bytes ||
        position>=index->count)
        return 0;
    uint8_t prefix[4]={0};
    uint64_t candidate_length=0;
    if (!musa_dwg1032_object_candidate_read_window_at_position(
            file_bytes,file_size,pages,page_count,index,position,0,
            prefix,2u,&candidate_length))
        return 0;
    const size_t prefix_size=(prefix[1]&0x80u)?4u:2u;
    if ((uint64_t)prefix_size>candidate_length) return 0;
    if (prefix_size==4u &&
        !musa_dwg1032_object_candidate_read_window_at_position(
            file_bytes,file_size,pages,page_count,index,position,2u,
            prefix+2u,2u,&candidate_length))
        return 0;

    MusaDwg1032Cursor cursor;
    uint32_t length=0;
    if (!musa_dwg1032_cursor_init(&cursor,prefix,prefix_size) ||
        !musa_dwg1032_modular_short(&cursor,&length) ||
        !length || (size_t)length>max_payload_bytes ||
        (uint64_t)length>candidate_length-(uint64_t)prefix_size ||
        payload_offset>(uint64_t)length ||
        (uint64_t)byte_count>(uint64_t)length-payload_offset)
        return 0;

    uint64_t checked_length=0;
    if (!musa_dwg1032_object_candidate_read_window_scratch_at_position(
            file_bytes,file_size,pages,page_count,index,position,
            (uint64_t)prefix_size+payload_offset,
            destination,byte_count,&checked_length,scratch,scratch_capacity))
        return 0;
    *payload_length=(uint64_t)length;
    return 1;
}

int musa_dwg1032_object_ms_payload_read_window_scratch(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    const MusaDwg1032ObjectIndex *index,
    uint64_t handle, uint64_t payload_offset, size_t max_payload_bytes,
    uint8_t *destination, size_t byte_count, uint64_t *payload_length,
    uint8_t *scratch, size_t scratch_capacity) {
    size_t position=0;
    if (!musa_dwg1032_object_index_find(index,handle,&position))
        return 0;
    return musa_dwg1032_object_ms_payload_read_window_scratch_at_position(
        file_bytes,file_size,pages,page_count,index,position,
        payload_offset,max_payload_bytes,destination,byte_count,
        payload_length,scratch,scratch_capacity);
}
