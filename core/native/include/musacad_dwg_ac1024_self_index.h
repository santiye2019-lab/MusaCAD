/* SPDX-License-Identifier: MIT
 * Original MusaCAD R2010/AC1024 embedded absolute self-H index evidence.
 * No LibreDWG, CAD SDK, scene, visual geometry or DWG writing.
 */
#ifndef MUSACAD_DWG_AC1024_SELF_INDEX_H
#define MUSACAD_DWG_AC1024_SELF_INDEX_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MusaDwgAc1024SelfIdentity {
    uint64_t embedded_absolute_handle;
    uint64_t record_offset;  /* In independently decoded AcDbObjects section */
    uint16_t raw_ot;         /* Numeric declaration, not resolved class */
    uint8_t frame_crc16_verified;
    uint8_t embedded_handle_code0_verified;
} MusaDwgAc1024SelfIdentity;

/* Verified absolute self-H and exact DATA cursor immediately following it.
 * Every offset is relative to the CRC-backed, independently decompressed
 * AcDbObjects buffer/payload. This is an authenticated prefix boundary,
 * NOT a complete common-entity prefix or proof of owner/layer/geometry.
 * No source modification, and caller output is unchanged on failure. */
typedef struct MusaDwgAc1024SelfPrefix {
    uint64_t indexed_object_offset;
    uint64_t embedded_absolute_handle;
    size_t payload_start_byte;
    size_t data_after_self_bit;
    size_t handle_stream_start_bit;
    size_t handle_stream_end_bit;
    uint16_t raw_ot;
    uint8_t frame_crc16_verified;
    uint8_t embedded_handle_code0_verified;
} MusaDwgAc1024SelfPrefix;

int musa_dwg_ac1024_record_self_prefix(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024SelfPrefix *out);

/* Narrow shared-core common-prefix witness for the NO-EED subset.
 * The published R2010 common object/entity format places a BS extended
 * entity data size after the absolute self-H. ONLY EED size==0 is
 * supported; nonzero EED remains unsupported, not silently skipped.
 * All offsets are payload-relative bit offsets. The remaining common
 * entity fields (proxy graphics, flags, colors, handle references) are
 * NOT parsed, and no renderer or writer may accept this as a scene. */
typedef struct MusaDwgAc1024ZeroEedPrefix {
    uint64_t indexed_object_offset;
    uint64_t embedded_absolute_handle;
    size_t payload_start_byte;
    size_t data_after_self_bit;
    size_t data_after_zero_eed_bit;
    size_t handle_stream_start_bit;
    size_t handle_stream_end_bit;
    uint16_t raw_ot;
    uint8_t record_crc16_verified;
    uint8_t zero_eed_verified;
} MusaDwgAc1024ZeroEedPrefix;

/* Atomic and read-only. Returns 1 ONLY after verifying the entire
 * AC1024 record CRC, canonical self-H and a bounded BS value of zero.
 * Rejects aliasing with source and preserves output on any failure.
 * This is field-boundary evidence, not verified owner/layer semantics. */
int musa_dwg_ac1024_record_zero_eed_prefix(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024ZeroEedPrefix *out);

/* All-or-nothing read-only reconciliation of independently authenticated
 * AcDb:Handles RECORD OFFSETS with CRC-verified R2010 object self-H.
 *
 * offset_in_handle_order MUST come from a separately CRC-checked original
 * handle-map page stream. Numeric IDs from a borrowed R2004 handle map
 * are NOT used. For each offset, verify full MS+MC+OT+CRC16, bounded
 * data/handle stream, and canonical absolute code-0 H immediately after OT.
 * Every embedded H must be nonzero and strictly increasing across entries;
 * every input record offset must be valid. No raw geometry is decoded.
 *
 * No mutations of source, offsets or caller result on ANY failure, including
 * alias, invalid stream, duplicate IDs, non-monotone IDs, capacity/memory
 * failures. A successful reconciliation is a HANDLE-ID/RECORD-OFFSET
 * candidate, NOT owner/layer linkage, per-entity semantics, Android preview
 * acceptance or AutoCAD parity.
 */
int musa_dwg_ac1024_recover_self_index(
    const uint8_t *objects, size_t object_bytes,
    const uint64_t *offset_in_handle_order, size_t record_count,
    MusaDwgAc1024SelfIdentity *out, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif
