/* SPDX-License-Identifier: MIT */
#include "musa_dwg1032_section_window_scratch.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}

int musa_dwg1032_section_read_window_scratch(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *validated_pages, size_t page_count,
    uint64_t section_bytes, uint64_t logical_offset,
    uint8_t *destination, size_t destination_size,
    uint8_t *scratch, size_t scratch_capacity) {
    if (file_size>PTRDIFF_MAX || section_bytes>(uint64_t)PTRDIFF_MAX ||
        destination_size>PTRDIFF_MAX || scratch_capacity>PTRDIFF_MAX ||
        page_count>SIZE_MAX/sizeof(*validated_pages) ||
        (file_size && !file_bytes) || (page_count && !validated_pages) ||
        (destination_size && (!destination || !scratch)) ||
        (scratch_capacity && !scratch) ||
        destination_size>scratch_capacity ||
        logical_offset>section_bytes ||
        (uint64_t)destination_size>section_bytes-logical_offset ||
        overlaps(file_bytes,file_size,destination,destination_size) ||
        overlaps(validated_pages,page_count*sizeof(*validated_pages),
                 destination,destination_size) ||
        overlaps(file_bytes,file_size,scratch,scratch_capacity) ||
        overlaps(validated_pages,page_count*sizeof(*validated_pages),
                 scratch,scratch_capacity) ||
        overlaps(destination,destination_size,scratch,scratch_capacity))
        return 0;
    if (!destination_size)
        return musa_dwg1032_section_read_window(
            file_bytes,file_size,validated_pages,page_count,section_bytes,
            logical_offset,NULL,0u);
    if (!page_count) return 0;

    /* Find the last logical start <= the requested offset. */
    size_t low=0, high=page_count;
    while (low<high) {
        const size_t mid=low+(high-low)/2u;
        if (validated_pages[mid].logical_offset<=logical_offset)
            low=mid+1u;
        else high=mid;
    }
    if (!low) return 0;
    const size_t first=low-1u;
    if (first) {
        const MusaDwg1032FilePage previous=validated_pages[first-1u];
        const MusaDwg1032FilePage current=validated_pages[first];
        if (!previous.byte_count ||
            previous.logical_offset>section_bytes ||
            (uint64_t)previous.byte_count>
                section_bytes-previous.logical_offset ||
            previous.logical_offset+(uint64_t)previous.byte_count!=
                current.logical_offset ||
            previous.file_offset>(uint64_t)file_size ||
            (uint64_t)previous.byte_count>
                (uint64_t)file_size-previous.file_offset) return 0;
    }
    /* Scratch may be partially modified by a late corrupt page. Public
     * destination is committed only when the entire window is validated. */
    size_t remaining=destination_size, written=0;
    uint64_t position=logical_offset;
    for (size_t i=first;remaining;) {
        if (i>=page_count) return 0;
        const MusaDwg1032FilePage p=validated_pages[i];
        if (!p.byte_count || p.logical_offset>section_bytes ||
            (uint64_t)p.byte_count>section_bytes-p.logical_offset ||
            (i==first ? (p.logical_offset>position ||
                position-p.logical_offset>=(uint64_t)p.byte_count)
                      : p.logical_offset!=position) ||
            p.file_offset>(uint64_t)file_size ||
            (uint64_t)p.byte_count>(uint64_t)file_size-p.file_offset)
            return 0;
        if (i+1u<page_count) {
            const MusaDwg1032FilePage next=validated_pages[i+1u];
            if (!next.byte_count || next.logical_offset>section_bytes ||
                (uint64_t)next.byte_count>
                    section_bytes-next.logical_offset ||
                p.logical_offset+(uint64_t)p.byte_count!=
                    next.logical_offset ||
                next.file_offset>(uint64_t)file_size ||
                (uint64_t)next.byte_count>
                    (uint64_t)file_size-next.file_offset) return 0;
        }
        const size_t within=(size_t)(position-p.logical_offset);
        const size_t available=p.byte_count-within;
        const size_t take=remaining<available?remaining:available;
        memcpy(scratch+written,file_bytes+(size_t)p.file_offset+within,take);
        written+=take;
        remaining-=take;
        position+=(uint64_t)take;
        ++i;
    }
    memcpy(destination,scratch,destination_size);
    return 1;
}
