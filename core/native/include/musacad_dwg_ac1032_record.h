/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_RECORD_H
#define MUSACAD_DWG_AC1032_RECORD_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* First-party AC1032 read-only object record envelope.
 * The source handle MUST be discovered from a validated original
 * AcDb:Handles map; its offset is relative to decompressed Objects.
 * MS is the modular-short payload size; MC is R2010+ handle-stream
 * bit length, before the object type field. CRC-16 covers MS+payload.
 * The type, owner graph and geometry are deliberately NOT decoded.
 */
typedef struct MusaDwgAc1032RecordEnvelope {
    uint64_t indexed_handle;
    uint64_t object_section_offset;
    uint32_t payload_bytes;
    uint32_t handle_stream_bits;
    uint32_t total_record_bytes;
    uint8_t ms_field_bytes;
    uint8_t mc_field_bytes;
    uint8_t record_crc_verified;
} MusaDwgAc1032RecordEnvelope;

/* Byte-level envelope probe for independently verified logical section
 * bytes and source-indexed offset. Does not establish file/map authenticity
 * by itself. Outputs untouched on error. */
int musa_dwg_ac1032_record_envelope_bytes(
    const uint8_t *logical_objects,size_t logical_bytes,
    uint64_t indexed_handle,uint64_t section_offset,
    MusaDwgAc1032RecordEnvelope *out);

/* Full original AC1032 DWG entry point: authenticates physical directory,
 * original handle index and original object section before parsing.
 * Returns one verified immutable record envelope for an indexed handle.
 * Fail-atomic, source-safe, no third-party CAD SDK or runtime.
 */
int musa_dwg_ac1032_record_envelope_file(
    const uint8_t *file,size_t file_bytes,uint64_t handle,
    MusaDwgAc1032RecordEnvelope *out);
#ifdef __cplusplus
}
#endif
#endif
