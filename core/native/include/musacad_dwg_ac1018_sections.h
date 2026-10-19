/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_SECTIONS_H
#define MUSACAD_DWG_AC1018_SECTIONS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Independent read-only AC1018 section map and masked 32-byte data-page
 * parser. Page map, section map, data + decoded-header checksums, logical
 * spans, compressed bounds and section ownership are validated. */
typedef enum MusaDwgAc1018SectionKind {
    MUSA_DWG_AC1018_OBJECTS = 1,
    MUSA_DWG_AC1018_HANDLES = 2
} MusaDwgAc1018SectionKind;
typedef struct MusaDwgAc1018SectionDescriptor {
    uint32_t section_id;
    uint32_t compression;
    uint32_t encryption;
    uint32_t page_count;
    uint64_t logical_bytes;
    uint8_t map_checksum_verified;
    uint8_t data_page_checksums_verified;
} MusaDwgAc1018SectionDescriptor;
typedef struct MusaDwgAc1018SectionPage {
    uint32_t page_id;
    uint32_t compressed_bytes;
    uint32_t physical_span_bytes;
    uint32_t masked_header_decompressed_bytes;
    uint64_t physical_offset;
    uint64_t payload_offset;
    uint64_t logical_offset;
    uint8_t data_checksum_verified;
    uint8_t header_checksum_verified;
} MusaDwgAc1018SectionPage;
/* pages=NULL/capacity=0 validates and counts; otherwise capacity >= pages.
 * Failure preserves entire caller output and out_page_count. */
int musa_dwg_ac1018_section_pages(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1018SectionKind kind,MusaDwgAc1018SectionPage *pages,
    size_t capacity,MusaDwgAc1018SectionDescriptor *desc,
    size_t *out_page_count);
/* Read the selected full section from RAW DWG bytes with verified map and
 * page checksums and LZ decompression. Never edits the source or exposes
 * partially decoded output; returns 0 without changing output/out_bytes.
 * This validates binary sections, NOT complete layer/owner semantics or
 * DWG rendering/editing/save or independent AutoCAD parity. */
int musa_dwg_ac1018_extract_section(const uint8_t *file,size_t file_bytes,
    MusaDwgAc1018SectionKind kind,uint8_t *output,size_t capacity,
    size_t *out_bytes);
#ifdef __cplusplus
}
#endif
#endif
