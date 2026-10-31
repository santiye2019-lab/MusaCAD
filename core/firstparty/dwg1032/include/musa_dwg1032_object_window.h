/* SPDX-License-Identifier: MIT
 * Independent bounded reads of caller-verified RAW object candidate pages.
 * This does not locate or decompress AC1032 pages or decode DWG entities.
 */
#ifndef MUSA_DWG1032_OBJECT_WINDOW_H
#define MUSA_DWG1032_OBJECT_WINDOW_H
#include "musa_dwg1032_object_index.h"
#include "musa_dwg1032_section_assembly.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Read a small window from a candidate object without assembling its whole
 * section. Before calling, the immutable page directory MUST pass
 * musa_dwg1032_file_pages_validate_window_directory(), and the immutable
 * index MUST pass musa_dwg1032_object_spans_prepare(). Candidate spans are
 * upper bounds, not authenticated AC1032 record lengths.
 *
 * The handle index and page directory must refer to the same logical section.
 * The requested window must fit inside the selected candidate. A zero-byte
 * read at the candidate end is valid. On success *candidate_length is the
 * entire candidate's length; on failure destination and *candidate_length
 * are unchanged. Writable output must not alias source or metadata.
 * O(log(object_count) + log(page_count) + touched_pages + byte_count).
 * No allocation, no page decompression, no production JNI integration. */
int musa_dwg1032_object_candidate_read_window(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *validated_pages, size_t page_count,
    const MusaDwg1032ObjectIndex *validated_index,
    uint64_t handle, uint64_t candidate_offset,
    uint8_t *destination, size_t byte_count, uint64_t *candidate_length);

#ifdef __cplusplus
}
#endif
#endif
