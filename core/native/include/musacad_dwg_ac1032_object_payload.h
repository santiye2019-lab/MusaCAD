/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_OBJECT_PAYLOAD_H
#define MUSACAD_DWG_AC1032_OBJECT_PAYLOAD_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Authenticated ORIGINAL R2018 record payload, not decoded geometry.
 * The copied bytes exclude MS/MC record-prefix and trailing CRC, but
 * INCLUDE the bit-coded OT prefix and the rest of the unmodified payload.
 * No guesses about where object/owner/handle bit substreams begin.
 * Same C11 ABI on Android, Windows and iOS; read-only and independent.
 */
typedef struct MusaDwgAc1032PayloadInfo {
    uint64_t indexed_handle;
    uint64_t object_section_offset;
    uint32_t payload_bytes;
    uint32_t handle_stream_bits;
    uint16_t raw_object_type;
    uint8_t object_type_field_bits;
    uint8_t original_record_crc_verified;
} MusaDwgAc1032PayloadInfo;

/* For independently authenticated original logical Objects bytes +
 * independently indexed handle location. Rechecks actual CRC itself.
 * Destination NULL + capacity 0 is a validated size-only request.
 * On ANY failure source/destination/required/info remain unchanged;
 * aliased inputs and outputs are rejected. */
int musa_dwg_ac1032_object_payload_bytes(
    const uint8_t *objects,size_t objects_size,
    uint64_t indexed_handle,uint64_t object_section_offset,
    uint8_t *destination,size_t destination_capacity,
    size_t *required,MusaDwgAc1032PayloadInfo *info);

/* End-to-end full original DWG verification: physical directory/CRC,
 * indexed handle, original Objects section, record CRC and OT code.
 * Returns a copy for future first-party bitfield/geometry decoders.
 * This is NOT yet LINE, ARC, ownership graph, edit or DWG write. */
int musa_dwg_ac1032_object_payload_file(
    const uint8_t *file,size_t file_bytes,uint64_t indexed_handle,
    uint8_t *destination,size_t destination_capacity,
    size_t *required,MusaDwgAc1032PayloadInfo *info);
#ifdef __cplusplus
}
#endif
#endif
