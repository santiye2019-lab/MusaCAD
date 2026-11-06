/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1024_SECTION_ENVELOPE_H
#define MUSACAD_DWG_AC1024_SECTION_ENVELOPE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Diagnostic-only proof of the original R2010 SYSTEM/section-map envelope.
 * Physical map, page locator and compressed page-body checksum must pass.
 * Logical section descriptors, entities, handle graphs, geometry, visual
 * correctness and DWG writer support are NOT inferred or accepted here. */
typedef struct MusaDwgAc1024SectionEnvelopeEvidence {
    uint32_t section_map_page_id;
    uint32_t compressed_bytes;
    uint32_t declared_expanded_bytes;
    uint32_t declared_section_count_candidate;
    uint64_t original_physical_offset;
    uint8_t adler_verified;
    uint8_t strict_lz_completed;
    /* Source-authenticated LZ output may match the bounded R2004-family
     * directory shape; this remains provisional, NOT full CAD admission. */
    uint8_t directory_shape_candidate;
    uint32_t directory_sections_candidate;
    uint32_t directory_referenced_pages_candidate;
    uint8_t directory_physical_links_candidate;
    uint32_t directory_matched_physical_refs_candidate;
    uint32_t source_data_masked_headers_verified;
    uint32_t source_data_payload_adler_verified;
    uint32_t source_data_header_adler_verified;
    uint32_t source_data_lz_eligible_pages;
    uint32_t source_data_lz_verified;
    /* Source-authenticated, private-buffer logical Handles section join.
     * This is NOT a handle index, owner graph or scene-open acceptance. */
    uint8_t source_handles_logical_join_verified;
    uint32_t source_handles_logical_bytes;
    /* Aggregate reason-only metadata; never expose handle values or names. */
    uint32_t source_handles_name_candidates;
    uint32_t source_handles_candidate_pages;
    uint32_t source_handles_candidate_logical_bytes;
    uint32_t source_handles_candidate_page_bytes;
    uint32_t source_handles_candidate_compression;
    uint32_t source_handles_candidate_encryption;
    uint8_t source_directory_duplicate_names;
    uint32_t source_handles_logical_offset_mismatch_pages;
    uint32_t source_handles_lz_eligible_pages;
    uint32_t source_handles_lz_verified_pages;
    /* Exact bounded reason from the private first-party join helper. */
    uint32_t source_handles_join_inner_failure_stage;
} MusaDwgAc1024SectionEnvelopeEvidence;
/* Source immutable, caller output atomic on all failures. Return 1 only
 * after exact protected header+page CRC proof; LZ failure is separately
 * reported as strict_lz_completed=0, not silently upgraded. */
int musa_dwg_ac1024_section_envelope_probe(const uint8_t *file,size_t n,
    MusaDwgAc1024SectionEnvelopeEvidence *out);
#ifdef __cplusplus
}
#endif
#endif
