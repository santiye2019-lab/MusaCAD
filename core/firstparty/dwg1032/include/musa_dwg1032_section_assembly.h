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

#ifdef __cplusplus
}
#endif
#endif
