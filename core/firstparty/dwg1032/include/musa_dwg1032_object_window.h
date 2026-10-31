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

/* Extract a bounded window from an MS-length-prefixed candidate record,
 * without allocating or assembling the entire verified RAW section.
 * Reads only the 2- or 4-byte prefix and the requested payload bytes,
 * including across physical page boundaries. The immutable directory and
 * index MUST already pass the validations required by candidate_read_window.
 * max_payload_bytes is a hard limit, independent of the candidate length.
 * The caller must keep all source data immutable during the operation.
 * On failure neither destination nor *payload_length is modified.
 * A zero-byte read at the payload end is permitted. O(log n + touched
 * pages + byte_count), constant auxiliary memory. The MS envelope is an
 * assumed candidate format, NOT proof of AC1032 object semantics; this
 * function does not discover, decompress or authenticate DWG pages. */
int musa_dwg1032_object_ms_payload_read_window(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *validated_pages, size_t page_count,
    const MusaDwg1032ObjectIndex *validated_index,
    uint64_t handle, uint64_t payload_offset, size_t max_payload_bytes,
    uint8_t *destination, size_t byte_count, uint64_t *payload_length);

#ifdef __cplusplus
}
#endif
#endif
