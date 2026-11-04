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
