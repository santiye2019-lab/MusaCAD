/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_REGIONS_H
#define MUSACAD_DWG_REGIONS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/*
 * First-party, platform-independent validation of CANDIDATE binary file
 * regions; deliberately no DWG page-directory parsing, compression, CRC,
 * block decoding or writer implementation. Never treat these raw ranges as
 * proven DWG sections until a separate version-specific parser certifies them.
 *
 * A candidate region must be nonempty and wholly within a caller-provided
 * original file length. This API permits adjacent ranges; rejects overlaps.
 * No allocation and no input mutation; callers retain ownership of bytes.
 */
#define MUSA_DWG_REGIONS_MAX 1024u
typedef struct MusaDwgRegion {
    uint64_t offset;
    uint64_t length;
    uint32_t section_kind;
} MusaDwgRegion;

/* 1 if all ranges are in bounds and mutually disjoint, else 0. Zero
 * candidate regions is valid; a single region must not have zero length.
 */
int musa_dwg_regions_validate(uint64_t file_bytes,
                              const MusaDwgRegion *regions,
                              size_t count);

/* Create a read-only bounded slice only from a validated region. On
 * any failure, leave both caller output variables unchanged.
 */
int musa_dwg_region_readonly_span(const uint8_t *source,
                                 size_t source_bytes,
                                 const MusaDwgRegion *region,
                                 const uint8_t **out_start,
                                 size_t *out_length);
#ifdef __cplusplus
}
#endif
#endif
