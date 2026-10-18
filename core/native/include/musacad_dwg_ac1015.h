/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1015_H
#define MUSACAD_DWG_AC1015_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_regions.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * First-party R2000/AC1015 section-locator READER, per publicly documented
 * ODA R13-R15 DWG file-header layout, not copied from another CAD engine.
 * This safely extracts original on-disk offsets and validates the sentinel,
 * record identities, interval boundaries and the R2000 CRC16 checksum.
 * Six locator records are valid. Optional record IDs 3..5 may have zero
 * length (no section). ID 5 is retained as opaque metadata only.
 * No entity/object decoder, editable drawing or DWG save is certified.
 * Do NOT pass these unverified regions to an editable CAD object database.
 * Modern AC1018..AC1032 have different file organizations and are refused.
 */
#define MUSA_DWG_AC1015_MAX_LOCATORS 6u
typedef struct MusaDwgAc1015Locators {
    uint16_t codepage;
    uint16_t stored_header_crc;
    uint32_t image_seeker_unverified;
    uint32_t region_count;
    uint32_t header_end;
    int crc_verified; /* 1 only after first-party R2000 CRC check. */
    MusaDwgRegion regions[MUSA_DWG_AC1015_MAX_LOCATORS];
} MusaDwgAc1015Locators;

/* Return 1 only for a structurally safe AC1015 section table. Valid
 * output is NOT enough to accept a DWG file as complete or editable.
 * On any error, original file and output record remain unchanged.
 */
int musa_dwg_ac1015_read_locators(const uint8_t *bytes,size_t byte_count,
                                  MusaDwgAc1015Locators *out);
/* Obtain a bounded, read-only ORIGINAL file section by documented R2000
 * locator ID 0..4. Revalidate the complete file header, its CRC16 and
 * every section extent on EVERY call: never trust saved/stale locators.
 * The returned span is borrowed from 'bytes', valid only as long as the
 * original allocation and file bytes remain unchanged.
 *
 * IMPORTANT: This checks header integrity ONLY. The section payload has
 * NOT been checksum-verified or decoded and MUST NOT be admitted as a CAD
 * document. Missing/unknown sections and invalid arguments fail closed,
 * leaving *out_start and *out_length unchanged.
 */
int musa_dwg_ac1015_readonly_section(const uint8_t *bytes,size_t byte_count,
                                      uint32_t section_kind,
                                      const uint8_t **out_start,
                                      size_t *out_length);
#ifdef __cplusplus
}
#endif
#endif
