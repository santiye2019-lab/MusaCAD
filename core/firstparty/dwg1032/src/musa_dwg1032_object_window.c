/* SPDX-License-Identifier: MIT
 * Independent C11 lazy object-window bridge over verified raw page ranges.
 */
#include "musa_dwg1032_object_window.h"
#include <stdint.h>
#include <stddef.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}

int musa_dwg1032_object_candidate_read_window(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    const MusaDwg1032ObjectIndex *index,
    uint64_t handle, uint64_t candidate_offset,
    uint8_t *destination, size_t byte_count, uint64_t *candidate_length) {
    if (!index || !candidate_length || !handle ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        page_count>SIZE_MAX/sizeof(*pages) ||
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
        overlaps(destination,byte_count,index->entries,entries_bytes))
        return 0;

    size_t position=0;
    if (!musa_dwg1032_object_index_find(index,handle,&position))
        return 0;
    const MusaDwg1032ObjectLocation entry=index->entries[position];
    if (!entry.length || entry.offset>index->section_bytes ||
        entry.length>index->section_bytes-entry.offset ||
        candidate_offset>entry.length ||
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
