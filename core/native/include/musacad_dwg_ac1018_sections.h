/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_SECTIONS_H
#define MUSACAD_DWG_AC1018_SECTIONS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Strict, read-only R2004 AcDbObjects/AcDb:Handles section parsing.
 * Validates protected file and physical page maps, section descriptors,
 * masked per-page envelopes and data/header checksums.
 * This is not DWG semantics, rendering, graph resolution, or writing. */
typedef enum MusaDwgAc1018SectionKind {
    MUSA_DWG_AC1018_OBJECTS=1,
    MUSA_DWG_AC1018_HANDLES=2
} MusaDwgAc1018SectionKind;
typedef struct MusaDwgAc1018SectionDescriptor {
    uint32_t section_id, compression, encryption, page_count;
    uint64_t logical_bytes;
    uint8_t map_checksum_verified, data_page_checksums_verified;
} MusaDwgAc1018SectionDescriptor;
typedef struct MusaDwgAc1018SectionPage {
    uint32_t page_id, compressed_bytes, physical_span_bytes;
    uint32_t masked_header_decompressed_bytes;
    uint64_t physical_offset, payload_offset, logical_offset;
    uint8_t data_checksum_verified, header_checksum_verified;
} MusaDwgAc1018SectionPage;
/* pages=NULL,capacity=0: fully validated count query. Otherwise caller
 * capacity must hold the entire page list. Atomic outputs on failure. */
int musa_dwg_ac1018_section_pages(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1018SectionKind kind,MusaDwgAc1018SectionPage *pages,
    size_t capacity,MusaDwgAc1018SectionDescriptor *desc,
    size_t *out_page_count);
/* Native end-to-end section extraction from raw AC1018 DWG bytes.
 * Output must have capacity >= logical section length and MUST NOT
 * overlap the source buffer. All source bytes remain untouched.
 * No partial result is published on error. */
int musa_dwg_ac1018_extract_section(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1018SectionKind kind,uint8_t *output,size_t capacity,
    size_t *out_bytes);
#ifdef __cplusplus
}
#endif
#endif
