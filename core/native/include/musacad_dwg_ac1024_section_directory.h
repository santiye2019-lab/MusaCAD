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
    /* Eligible pages: encrypted=0, compression=2, verified checksums,
     * bounded declared output page bytes, exact logical page order.
     * This is read-only diagnostic; neither count implies CAD semantics. */
    uint32_t strict_lz_eligible_pages;
    uint32_t strict_lz_verified;
    /* Only aggregate counters for source-named Handles pages. */
    uint32_t handles_page_references;
    uint32_t handles_strict_lz_eligible;
    uint32_t handles_strict_lz_verified;
} MusaDwgAc1024PageChecksumEvidence;
/* The source must have already passed original AC1024 physical map and
 * section-map Adler32/strict LZ; caller supplies original authenticated
 * physical locations. On invalid argument or map, output is untouched. */
int musa_dwg_ac1024_directory_page_checksums(
    const uint8_t *file,size_t file_bytes,
    const uint8_t *decoded,size_t decoded_bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    MusaDwgAc1024PageChecksumEvidence *out);
/* Diagnostic-only logical section join for original unencrypted R2010
 * DATA pages. 'directory' and 'locations' MUST already come from the
 * independently authenticated original AC1024 SYSTEM/physical map.
 * This helper independently checks all source DATA page headers and dual
 * Adler32, exact logical offsets/counts, then decodes each bounded LZ page
 * into private scratch and copies the contiguous logical bytes atomically.
 * It does not interpret CAD entities, ownership, layers or colors.
 * Strictly allowed names: AcDb:Handles and AcDb:AcDbObjects.
 * Output and out_bytes stay unchanged on any refusal; never modify file. */
int musa_dwg_ac1024_directory_join_section(
    const uint8_t *file,size_t file_bytes,
    const uint8_t *directory,size_t directory_bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    const char *section_name,
    uint8_t *output,size_t capacity,size_t *out_bytes);
/* Diagnostic entry point: identical fail-atomic function, but returns a
 * bounded refusal stage in reason (0 on success; 1..12 on refusal).
 * No handles, raw bytes, coordinates or source names are reported.
 * reason must not alias any source/input/output range. */
int musa_dwg_ac1024_directory_join_section_diagnose(
    const uint8_t *file,size_t file_bytes,
    const uint8_t *directory,size_t directory_bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    const char *section_name,
    uint8_t *output,size_t capacity,size_t *out_bytes,
    uint32_t *reason);
#ifdef __cplusplus
}
#endif
#endif
