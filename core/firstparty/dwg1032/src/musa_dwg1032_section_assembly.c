/* SPDX-License-Identifier: MIT
 * Independent C11 checked assembly of pre-validated logical fragments.
 */
#include "musa_dwg1032_section_assembly.h"
#include <stdint.h>
#include <string.h>

static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn) return 0;
    const uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}

int musa_dwg1032_section_assemble(const MusaDwg1032PageFragment *fragments,
                                  size_t fragment_count,
                                  uint8_t *destination,
                                  size_t destination_size) {
    if ((fragment_count && !fragments) ||
        (destination_size && !destination) ||
        fragment_count>SIZE_MAX/sizeof(*fragments) ||
        overlaps(fragments,fragment_count*sizeof(*fragments),
                 destination,destination_size)) return 0;
    size_t filled=0;
    for (size_t i=0;i<fragment_count;++i) {
        const MusaDwg1032PageFragment *f=&fragments[i];
        if (!f->data || !f->byte_count ||
            f->logical_offset!=(uint64_t)filled ||
            f->byte_count>destination_size-filled ||
            overlaps(f->data,f->byte_count,destination,destination_size))
            return 0;
        filled+=f->byte_count;
    }
    if (filled!=destination_size) return 0;
    for (size_t i=0;i<fragment_count;++i) {
        const MusaDwg1032PageFragment *f=&fragments[i];
        memcpy(destination+(size_t)f->logical_offset,f->data,f->byte_count);
    }
    return 1;
}

/* Physical file order need not match logical section order. All validation
 * precedes copying, including a late invalid page: no partial output. */
int musa_dwg1032_section_assemble_file_pages(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    uint8_t *destination, size_t destination_size) {
    if ((file_size && !file_bytes) || (destination_size && !destination) ||
        (page_count && !pages) ||
        file_size>PTRDIFF_MAX || destination_size>PTRDIFF_MAX ||
        page_count>SIZE_MAX/sizeof(*pages) ||
        overlaps(file_bytes,file_size,destination,destination_size) ||
        overlaps(pages,page_count*sizeof(*pages),
                 destination,destination_size)) return 0;
    size_t filled=0;
    for (size_t i=0;i<page_count;++i) {
        const MusaDwg1032FilePage *page=&pages[i];
        if (!page->byte_count ||
            page->logical_offset!=(uint64_t)filled ||
            page->byte_count>destination_size-filled ||
            page->file_offset>(uint64_t)file_size ||
            (uint64_t)page->byte_count>
                (uint64_t)file_size-page->file_offset)
            return 0;
        filled+=page->byte_count;
    }
    if (filled!=destination_size) return 0;
    for (size_t i=0;i<page_count;++i) {
        const MusaDwg1032FilePage *page=&pages[i];
        memcpy(destination+(size_t)page->logical_offset,
               file_bytes+(size_t)page->file_offset,page->byte_count);
    }
    return 1;
}

/* Physical page validation is optional and independent of logical assembly.
 * A max-heap sorts caller-owned indices without allocating or moving pages. */
static int physical_before(const MusaDwg1032FilePage *pages,
                           size_t a, size_t b) {
    return pages[a].file_offset < pages[b].file_offset ||
        (pages[a].file_offset == pages[b].file_offset && a < b);
}

static void physical_sift(const MusaDwg1032FilePage *pages, size_t *order,
                          size_t root, size_t end) {
    while (root < end / 2u) {
        size_t child = root * 2u + 1u;
        if (child >= end) break;
        if (child + 1u < end &&
            physical_before(pages, order[child], order[child + 1u]))
            ++child;
        if (!physical_before(pages, order[root], order[child])) break;
        const size_t swap = order[root];
        order[root] = order[child];
        order[child] = swap;
        root = child;
    }
}

int musa_dwg1032_file_pages_validate_physical(
    const MusaDwg1032FilePage *pages, size_t page_count,
    size_t file_size, size_t *order, size_t order_capacity) {
    if (page_count > order_capacity ||
        page_count > SIZE_MAX / sizeof(*pages) ||
        page_count > SIZE_MAX / sizeof(*order) ||
        (page_count && (!pages || !order)) ||
        overlaps(pages, page_count * sizeof(*pages),
                 order, page_count * sizeof(*order)))
        return 0;
    for (size_t i = 0; i < page_count; ++i) {
        const MusaDwg1032FilePage *page = &pages[i];
        if (!page->byte_count || page->file_offset > (uint64_t)file_size ||
            (uint64_t)page->byte_count >
                (uint64_t)file_size - page->file_offset)
            return 0;
    }
    for (size_t i = 0; i < page_count; ++i) order[i] = i;
    for (size_t i = page_count / 2u; i > 0u; --i)
        physical_sift(pages, order, i - 1u, page_count);
    for (size_t end = page_count; end > 1u; --end) {
        const size_t swap = order[0];
        order[0] = order[end - 1u];
        order[end - 1u] = swap;
        physical_sift(pages, order, 0u, end - 1u);
    }
    for (size_t i = 1; i < page_count; ++i) {
        const MusaDwg1032FilePage *previous = &pages[order[i - 1u]];
        const MusaDwg1032FilePage *next = &pages[order[i]];
        if (next->file_offset - previous->file_offset <
            (uint64_t)previous->byte_count)
            return 0;
    }
    return 1;
}

/* A strict caller opt-in: the existing assembly API intentionally allows
 * reused physical ranges, but untrusted page directories should not. */
int musa_dwg1032_section_assemble_checked_file_pages(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    uint8_t *destination, size_t destination_size,
    size_t *order, size_t order_capacity) {
    if (page_count > order_capacity ||
        page_count > SIZE_MAX / sizeof(*order) ||
        page_count > SIZE_MAX / sizeof(*pages) ||
        (page_count && !order))
        return 0;
    const size_t scratch_bytes = page_count * sizeof(*order);
    if (overlaps(order, scratch_bytes, file_bytes, file_size) ||
        overlaps(order, scratch_bytes, destination, destination_size) ||
        overlaps(order, scratch_bytes, pages,
                 page_count * sizeof(*pages)))
        return 0;
    if (!musa_dwg1032_file_pages_validate_physical(
            pages, page_count, file_size, order, order_capacity))
        return 0;
    return musa_dwg1032_section_assemble_file_pages(
        file_bytes, file_size, pages, page_count,
        destination, destination_size);
}

/* The directory is checked once; repeated viewport windows do not sort or
 * assemble the entire section. Only caller-verified RAW page bytes are used. */
int musa_dwg1032_file_pages_validate_window_directory(
    const MusaDwg1032FilePage *pages, size_t page_count,
    size_t file_size, uint64_t section_bytes,
    size_t *order, size_t order_capacity) {
    if (file_size>PTRDIFF_MAX || section_bytes>(uint64_t)PTRDIFF_MAX ||
        page_count>SIZE_MAX/sizeof(*pages) ||
        (page_count && !pages)) return 0;
    uint64_t filled=0;
    for (size_t i=0;i<page_count;++i) {
        const MusaDwg1032FilePage p=pages[i];
        if (!p.byte_count || p.logical_offset!=filled ||
            p.byte_count>section_bytes-filled) return 0;
        filled+=(uint64_t)p.byte_count;
    }
    if (filled!=section_bytes) return 0;
    return musa_dwg1032_file_pages_validate_physical(
        pages,page_count,file_size,order,order_capacity);
}

int musa_dwg1032_section_read_window(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    uint64_t section_bytes, uint64_t logical_offset,
    uint8_t *destination, size_t destination_size) {
    if (file_size>PTRDIFF_MAX || section_bytes>(uint64_t)PTRDIFF_MAX ||
        page_count>SIZE_MAX/sizeof(*pages) ||
        (file_size && !file_bytes) || (page_count && !pages) ||
        (destination_size && !destination) ||
        logical_offset>section_bytes ||
        (uint64_t)destination_size>section_bytes-logical_offset ||
        overlaps(file_bytes,file_size,destination,destination_size) ||
        overlaps(pages,page_count*sizeof(*pages),
                 destination,destination_size))
        return 0;
    if (!page_count) return !section_bytes && !destination_size;
    if (!destination_size) return 1;
    /* Binary search: first page whose logical start is greater than offset. */
    size_t lo=0,hi=page_count;
    while (lo<hi) {
        const size_t mid=lo+(hi-lo)/2u;
        if (pages[mid].logical_offset<=logical_offset) lo=mid+1u;
        else hi=mid;
    }
    if (!lo) return 0;
    const size_t first=lo-1u;
    /* Validate all touched pages before the first destination write.
     * The caller guarantees directory and file immutability between passes. */
    for (unsigned pass=0;pass<2u;++pass) {
        uint64_t pos=logical_offset;
        size_t remaining=destination_size, written=0;
        for (size_t i=first;remaining;) {
            if (i>=page_count) return 0;
            const MusaDwg1032FilePage p=pages[i];
            if (!p.byte_count || p.logical_offset>pos ||
                pos-p.logical_offset>=(uint64_t)p.byte_count ||
                p.file_offset>(uint64_t)file_size ||
                (uint64_t)p.byte_count>(uint64_t)file_size-p.file_offset)
                return 0;
            const size_t within=(size_t)(pos-p.logical_offset);
            const size_t available=p.byte_count-within;
            const size_t take=remaining<available?remaining:available;
            if (pass)
                memcpy(destination+written,
                       file_bytes+(size_t)p.file_offset+within,take);
            written+=take;
            remaining-=take;
            pos+=(uint64_t)take;
            ++i;
        }
    }
    return 1;
}
