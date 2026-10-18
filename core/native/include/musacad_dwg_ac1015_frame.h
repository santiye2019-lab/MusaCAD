/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1015_FRAME_H
#define MUSACAD_DWG_AC1015_FRAME_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Only R2000/AC1015 HEADER VARIABLES (0) and CLASSES (1).
 * Returned payload is borrowed read-only from unchanged source bytes.
 * Framing validation is NOT payload CRC or entity/class decoding.
 */
typedef struct MusaDwgAc1015Frame {
    const uint8_t *payload;
    size_t payload_bytes;
    uint16_t stored_payload_crc_unverified;
    uint32_t section_kind;
    int envelope_verified;
    int payload_crc_verified; /* Always zero until separately implemented. */
} MusaDwgAc1015Frame;
/* Fail closed on malformed envelope and preserve *out on every error.
 * Requires original file-header CRC, bounded sections, beginning and
 * ending 16-byte sentinels, and exact 32-bit payload byte count.
 * All other section kinds, modern DWG versions and nulls are rejected.
 */
int musa_dwg_ac1015_parse_frame(const uint8_t *file,size_t file_bytes,
                                uint32_t section_kind,MusaDwgAc1015Frame *out);
/* Strict CRC-validated view for original R2000 HEADER and CLASSES.
 * Requires all structural checks from parse_frame() plus the documented
 * seeded (0xC0C1) CRC-16 on 4-byte section length followed by payload.
 * On mismatch, leave *out untouched. Never copy or modify source bytes.
 * CRC verifies accidental corruption only: NOT cryptographic authenticity,
 * parsing of class/header variables, or correctness of real-world DWGs.
 * The caller owns file and must keep it unchanged while borrowing payload.
 */
int musa_dwg_ac1015_parse_crc_verified_frame(const uint8_t *file,
                                               size_t file_bytes,
                                               uint32_t section_kind,
                                               MusaDwgAc1015Frame *out);
#ifdef __cplusplus
}
#endif
#endif
