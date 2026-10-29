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
