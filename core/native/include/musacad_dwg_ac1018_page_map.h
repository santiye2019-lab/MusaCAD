/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_PAGE_MAP_H
#define MUSACAD_DWG_AC1018_PAGE_MAP_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Verified fixed R2004 encrypted header and protected page-location map.
 * Does not yet parse AcDb section descriptions or entity ownership. */
typedef struct MusaDwgAc1018FileMapHeader {
    uint64_t page_map_offset;
    uint32_t page_count;
    uint32_t page_map_id;
    uint32_t section_map_id;
    uint32_t map_compressed_bytes;
    uint32_t map_decoded_bytes;
} MusaDwgAc1018FileMapHeader;
typedef struct MusaDwgAc1018PageLocation {
    uint32_t page_id;
    uint32_t file_span_bytes;
    uint64_t file_offset;
} MusaDwgAc1018PageLocation;
/* On failure preserve out bytes. Read-only source. No CAD SDK/runtime. */
int musa_dwg_ac1018_page_map_header(const uint8_t *file,size_t file_size,
                                    MusaDwgAc1018FileMapHeader *out);
/* Entries: page IDs 1..N; physical locations start at 256 bytes.
 * entries=NULL/capacity=0: validated count only. Atomic on all failures.
 * Separate per-data-page authentication remains required. */
int musa_dwg_ac1018_page_locations(const uint8_t *file,size_t file_size,
    MusaDwgAc1018PageLocation *entries,size_t capacity,size_t *out_count);
#ifdef __cplusplus
}
#endif
#endif
