/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_SECTION_ASSEMBLE_H
#define MUSACAD_DWG_AC1018_SECTION_ASSEMBLE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* The caller MUST independently validate file header, section/page map,
 * page type, mapped compression length, checksum and masked data-page
 * header. Merely setting header_verified is a caller assertion, not
 * independent authentication performed by this function.
 * packed_offset points to COMPRESSED PAYLOAD, excluding the 32-byte
 * R2004 page header. logical_offset is into the uncompressed section.
 */
typedef struct MusaDwgAc1018PageInput {
    size_t packed_offset;
    size_t packed_length;
    size_t logical_offset;
    uint8_t header_verified;
} MusaDwgAc1018PageInput;
/* Reconstruct a section from independently verified data pages. Logical
 * block size = 29696 bytes; final logical page is clipped. This function
 * guarantees no logical gaps/overlaps, exact output length, bounded source
 * and destination, decompression success and atomic output on failure.
 * Inputs remain unchanged. Not a complete DWG file reader or verifier.
 * Pages must be supplied in ascending logical_offset order.
 */
int musa_dwg_ac1018_assemble_section(const uint8_t *file,size_t file_bytes,
    const MusaDwgAc1018PageInput *pages,size_t pages_count,
    uint8_t *output,size_t logical_bytes);
#ifdef __cplusplus
}
#endif
#endif
