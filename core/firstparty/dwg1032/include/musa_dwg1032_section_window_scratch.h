/* SPDX-License-Identifier: MIT
 * First-party bounded, transactional window reads from verified RAW pages.
 */
#ifndef MUSA_DWG1032_SECTION_WINDOW_SCRATCH_H
#define MUSA_DWG1032_SECTION_WINDOW_SCRATCH_H
#include "musa_dwg1032_section_assembly.h"
#ifdef __cplusplus
extern "C" {
#endif
/* A caller-owned scratch window allows a single pass over touched pages,
 * followed by one publication copy. The directory must have passed
 * musa_dwg1032_file_pages_validate_window_directory and remain immutable.
 * All source, destination, metadata, and the ENTIRE scratch capacity must be
 * disjoint. On failure destination remains unchanged; scratch is disposable.
 * No allocation, DWG authentication, decompression or entity decoding. */
int musa_dwg1032_section_read_window_scratch(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *validated_pages, size_t page_count,
    uint64_t section_bytes, uint64_t logical_offset,
    uint8_t *destination, size_t destination_size,
    uint8_t *scratch, size_t scratch_capacity);
#ifdef __cplusplus
}
#endif
#endif
