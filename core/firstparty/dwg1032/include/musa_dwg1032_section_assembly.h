/* SPDX-License-Identifier: MIT
 * First-party assembly of caller-verified, decompressed logical page fragments.
 * Does not locate, authenticate, or decompress AC1032 pages.
 */
#ifndef MUSA_DWG1032_SECTION_ASSEMBLY_H
#define MUSA_DWG1032_SECTION_ASSEMBLY_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MusaDwg1032PageFragment {
    uint64_t logical_offset;
    const uint8_t *data;
    size_t byte_count;
} MusaDwg1032PageFragment;

/* Fragments must be in increasing logical order, with no gaps or overlaps,
 * and exactly cover destination_size bytes. Empty sections are supported.
 * All input data and fragment metadata must be disjoint from destination.
 * Validation is completed before the first destination write; failure
 * leaves destination untouched. O(page_count + destination_size) time,
 * O(1) auxiliary memory, with no allocations. */
int musa_dwg1032_section_assemble(const MusaDwg1032PageFragment *fragments,
                                  size_t fragment_count,
                                  uint8_t *destination,
                                  size_t destination_size);
/* File-backed page descriptor: physical file offsets may be out of order,
 * but logical offsets must be consecutive and cover the destination.
 * This copies caller-verified RAW / ALREADY DECOMPRESSED pages only. It
 * neither discovers nor authenticates AC1032 section maps, nor inflates
 * compressed DWG pages. */
typedef struct MusaDwg1032FilePage {
    uint64_t logical_offset;
    uint64_t file_offset;
    size_t byte_count;
} MusaDwg1032FilePage;

/* Copies logical pages from a bounded file mapping without allocating.
 * Rejects missing/truncated/overlapping logical ranges, source/destination
 * aliasing and metadata/destination aliasing before the first write.
 * Failure leaves destination unchanged. O(page_count + output_size). */
int musa_dwg1032_section_assemble_file_pages(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    uint8_t *destination, size_t destination_size);


/* Optional corruption gate for a caller-verified physical page directory.
 * Sorts physical file ranges in caller-provided scratch space and rejects
 * duplicate/overlapping source ranges, empty pages and out-of-file spans.
 * Does not infer AC1032 page headers, compression or authentication.
 * O(page_count log page_count), no allocation. On failure order may change;
 * pages are never modified. This is deliberately separate from assembly:
 * some callers may legitimately reuse physical byte ranges. */
int musa_dwg1032_file_pages_validate_physical(
    const MusaDwg1032FilePage *pages, size_t page_count,
    size_t file_size, size_t *order, size_t order_capacity);


/* Strict one-shot physical directory validation plus logical assembly.
 * Requires a caller-owned order array with at least page_count elements.
 * Rejects overlapping physical pages (unlike the permissive assembly API),
 * and rejects scratch overlap with source, destination or page metadata.
 * Validates both physical and logical ranges before writing destination.
 * Failure leaves destination unchanged; scratch may change. */
int musa_dwg1032_section_assemble_checked_file_pages(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *pages, size_t page_count,
    uint8_t *destination, size_t destination_size,
    size_t *order, size_t order_capacity);


/* One-time validation for random-access reads of a caller-supplied RAW,
 * already decompressed section directory. Pages must be in consecutive
 * logical order and exactly cover section_bytes; physical ranges must be
 * disjoint and inside the file. order is caller-owned sorting scratch.
 * No DWG page discovery, decompression, CRC or authenticity is implied. */
int musa_dwg1032_file_pages_validate_window_directory(
    const MusaDwg1032FilePage *pages, size_t page_count,
    size_t file_size, uint64_t section_bytes,
    size_t *order, size_t order_capacity);

/* Read a bounded logical window without allocating or assembling the entire
 * section. REQUIRES a previously validated, immutable directory from the
 * function above. O(log(page_count) + touched_pages + window_bytes).
 * Source file, page metadata and destination must not overlap. All touched
 * pages are checked before writing, so failure preserves destination.
 * Does not decode DWG pages, records or entities. */
int musa_dwg1032_section_read_window(
    const uint8_t *file_bytes, size_t file_size,
    const MusaDwg1032FilePage *validated_pages, size_t page_count,
    uint64_t section_bytes, uint64_t logical_offset,
    uint8_t *destination, size_t destination_size);

#ifdef __cplusplus
}
#endif
#endif
