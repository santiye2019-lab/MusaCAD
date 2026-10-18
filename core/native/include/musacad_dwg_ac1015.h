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
 * record identities and interval boundaries. The recorded CRC is returned
 * for diagnostics but NOT checked; no object decode / DWG save is certified.
 * Do NOT pass these unverified regions to an editable CAD object database.
 * Modern AC1018..AC1032 have different file organizations and are refused.
 */
#define MUSA_DWG_AC1015_MAX_LOCATORS 5u
typedef struct MusaDwgAc1015Locators {
    uint16_t codepage;
    uint16_t stored_header_crc;
    uint32_t image_seeker_unverified;
    uint32_t region_count;
    uint32_t header_end;
    int crc_verified; /* Always 0 until independent CRC validation exists. */
    MusaDwgRegion regions[MUSA_DWG_AC1015_MAX_LOCATORS];
} MusaDwgAc1015Locators;

/* Return 1 only for a structurally safe AC1015 section table. Valid
 * output is NOT enough to accept a DWG file as complete or editable.
 * On any error, original file and output record remain unchanged.
 */
int musa_dwg_ac1015_read_locators(const uint8_t *bytes,size_t byte_count,
                                  MusaDwgAc1015Locators *out);
#ifdef __cplusplus
}
#endif
#endif
