/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1015_OBJECT_H
#define MUSACAD_DWG_AC1015_OBJECT_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_ac1015_handles.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Checked read-only R2000 object RECORD envelope.
 * Handle ID and address must come from a validated object map; a
 * candidate record alone does not establish map membership or identity.
 * TYPE (BS) is a raw DWG type code, not proof of geometry decoding.
 */
typedef struct MusaDwgAc1015ObjectRecord {
    uint64_t indexed_handle;
    uint64_t absolute_offset;
    const uint8_t *data;
    size_t data_bytes;
    size_t size_field_bytes;
    size_t total_record_bytes;
    uint16_t type_code;
    uint32_t pre_handle_bits;
    int record_crc_verified;
    int geometry_decoded; /* Always 0: no entity data or handles decoded. */
} MusaDwgAc1015ObjectRecord;
/* Strict AC1015 first-party MS length / BS type / R2000 RL size / CRC.
 * Requires a validated indexed_entry supplied by the caller, original
 * buffer unchanged and CRC-16 including all size-field bytes.
 * On failure keeps *out byte-for-byte unchanged, never writes file data.
 */
int musa_dwg_ac1015_read_object_record(
    const uint8_t *file,size_t file_bytes,
    const MusaDwgAc1015HandleEntry *indexed_entry,
    MusaDwgAc1015ObjectRecord *out);
#ifdef __cplusplus
}
#endif
#endif
