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
#ifdef __cplusplus
}
#endif
#endif
