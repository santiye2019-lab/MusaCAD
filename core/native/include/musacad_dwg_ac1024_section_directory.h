/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1024_SECTION_DIRECTORY_H
#define MUSACAD_DWG_AC1024_SECTION_DIRECTORY_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_ac1018_page_map.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded *candidate* record layout of a separately decoded R2010 section
 * map. It cannot authenticate the original DWG, section data or geometry
 * by itself. Full-byte coverage and structural consistency are required. */
typedef struct MusaDwgAc1024DirectoryShape {
    uint32_t sections;
    uint32_t referenced_pages;
    uint32_t objects_name_candidates;
    uint32_t handles_name_candidates;
    uint8_t duplicate_section_names;
} MusaDwgAc1024DirectoryShape;
/* Fail atomic. No names, source bytes, handles or geometry leave this
 * function. Caller must independently verify the original source map CRC. */
int musa_dwg_ac1024_directory_shape(
    const uint8_t *decoded,size_t bytes,MusaDwgAc1024DirectoryShape *out);
/* Cross-check tentative R2010 logical page IDs against independently
 * authenticated physical page locations. Still NOT DATA-page CRC proof. */
typedef struct MusaDwgAc1024DirectoryPageLinkEvidence {
    uint32_t matched_page_references;
    uint32_t matched_section_records;
} MusaDwgAc1024DirectoryPageLinkEvidence;
/* The source page array must be strictly sorted by positive ID. Complete
 * only when every bounded candidate page ref matches a physical original
 * page and its packed length fits within the source physical span. */
int musa_dwg_ac1024_directory_physical_links(
    const uint8_t *decoded,size_t bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    MusaDwgAc1024DirectoryPageLinkEvidence *out);
/* Three independent R2004-family R2010 DATA page proof stages:
 * (1) masked 32-byte envelope, (2) payload Adler32, (3) header Adler32.
 * Even complete checksum proof does NOT decode entity geometry or admit
 * rendering; do not infer completeness from any of these counters. */
typedef struct MusaDwgAc1024PageChecksumEvidence {
    uint32_t referenced_pages;
    uint32_t masked_headers_verified;
    uint32_t payload_adler_verified;
    uint32_t header_adler_verified;
} MusaDwgAc1024PageChecksumEvidence;
/* The source must have already passed original AC1024 physical map and
 * section-map Adler32/strict LZ; caller supplies original authenticated
 * physical locations. On invalid argument or map, output is untouched. */
int musa_dwg_ac1024_directory_page_checksums(
    const uint8_t *file,size_t file_bytes,
    const uint8_t *decoded,size_t decoded_bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    MusaDwgAc1024PageChecksumEvidence *out);
#ifdef __cplusplus
}
#endif
#endif
