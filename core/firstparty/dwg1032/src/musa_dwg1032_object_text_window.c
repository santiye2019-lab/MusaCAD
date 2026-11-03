/* SPDX-License-Identifier: MIT
 * First-party bounded RAW-page to Unicode display-text bridge.
 */
#include "musa_dwg1032_object_text_window.h"
#include "musa_dwg1032_text_utf16le.h"
#include <stddef.h>
#include <stdint.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}

int musa_dwg1032_object_ms_utf16le_text(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    const MusaDwg1032ObjectIndex *index, uint64_t handle,
    uint64_t field_payload_offset, size_t field_byte_count,
    size_t max_payload_bytes, uint8_t *scratch, size_t scratch_capacity,
    char *utf8, size_t utf8_capacity, size_t *written) {
    if (!index || !utf8 || !written || !utf8_capacity || !handle ||
        !max_payload_bytes || (field_byte_count & 1u) ||
        field_byte_count>scratch_capacity ||
        (scratch_capacity && !scratch) ||
        file_size>PTRDIFF_MAX || scratch_capacity>PTRDIFF_MAX ||
        utf8_capacity>PTRDIFF_MAX ||
        page_count>SIZE_MAX/sizeof(*pages) ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        (file_size && !file_bytes) || (page_count && !pages) ||
        (index->count && !index->entries))
        return 0;
    const void *inputs[]={file_bytes,pages,index,index->entries};
    const size_t sizes[]={file_size,page_count*sizeof(*pages),
                          sizeof(*index),index->count*sizeof(*index->entries)};
    const void *outputs[]={scratch,utf8,written};
    const size_t lengths[]={scratch_capacity,utf8_capacity,sizeof(*written)};
    for (size_t i=0u;i<3u;++i) {
        for (size_t j=i+1u;j<3u;++j)
            if (overlaps(outputs[i],lengths[i],outputs[j],lengths[j]))
                return 0;
        for (size_t j=0u;j<4u;++j)
            if (overlaps(outputs[i],lengths[i],inputs[j],sizes[j]))
                return 0;
    }
    /* This read writes only disposable scratch; public output remains
     * unchanged even if a late page or malformed MS envelope is rejected. */
    uint64_t payload_length=0u;
    if (!musa_dwg1032_object_ms_payload_read_window(
            file_bytes,file_size,pages,page_count,index,handle,
            field_payload_offset,max_payload_bytes,
            scratch,field_byte_count,&payload_length))
        return 0;
    (void)payload_length;
    /* The converter preflights every scalar before its first UTF-8 write.
     * In particular, a late invalid surrogate cannot publish partial text. */
    return musa_dwg1032_text_utf16le_to_utf8(
        scratch,field_byte_count,utf8,utf8_capacity,written);
}

/* Viewport paging already knows the validated object ordinal. Preserve the
 * handle variant for callers without an index position, but do not search
 * the sorted index again for every text field in a visible object. */
int musa_dwg1032_object_ms_utf16le_text_at_position(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    const MusaDwg1032ObjectIndex *index, size_t position,
    uint64_t field_payload_offset, size_t field_byte_count,
    size_t max_payload_bytes, uint8_t *scratch, size_t scratch_capacity,
    char *utf8, size_t utf8_capacity, size_t *written) {
    if (!index || position>=index->count || !utf8 || !written ||
        !utf8_capacity || !max_payload_bytes ||
        (field_byte_count & 1u) ||
        field_byte_count>scratch_capacity ||
        (scratch_capacity && !scratch) ||
        file_size>PTRDIFF_MAX || scratch_capacity>PTRDIFF_MAX ||
        utf8_capacity>PTRDIFF_MAX ||
        page_count>SIZE_MAX/sizeof(*pages) ||
        index->count>SIZE_MAX/sizeof(*index->entries) ||
        (file_size && !file_bytes) || (page_count && !pages) ||
        (index->count && !index->entries))
        return 0;
    const void *inputs[]={file_bytes,pages,index,index->entries};
    const size_t sizes[]={file_size,page_count*sizeof(*pages),
                          sizeof(*index),index->count*sizeof(*index->entries)};
    const void *outputs[]={scratch,utf8,written};
    const size_t lengths[]={scratch_capacity,utf8_capacity,sizeof(*written)};
    for (size_t i=0u;i<3u;++i) {
        for (size_t j=i+1u;j<3u;++j)
            if (overlaps(outputs[i],lengths[i],outputs[j],lengths[j]))
                return 0;
        for (size_t j=0u;j<4u;++j)
            if (overlaps(outputs[i],lengths[i],inputs[j],sizes[j]))
                return 0;
    }
    uint64_t payload_length=0u;
    if (!musa_dwg1032_object_ms_payload_read_window_at_position(
            file_bytes,file_size,pages,page_count,index,position,
            field_payload_offset,max_payload_bytes,
            scratch,field_byte_count,&payload_length))
        return 0;
    (void)payload_length;
    return musa_dwg1032_text_utf16le_to_utf8(
        scratch,field_byte_count,utf8,utf8_capacity,written);
}
