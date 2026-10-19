/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_NONENTITY_H
#define MUSACAD_DWG_AC1018_NONENTITY_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* R2004 AC1018 non-entity common header, read-only first-party C11.
 * This is only an identity PREFIX proof, not an AcDb class/owner graph.
 * Type is the record's numerical BS declaration; it is NOT CLASSES proof.
 * Callers must separately validate layer/owner/XDictionary associations.
 */
typedef struct MusaDwgAc1018NonEntityIdentity {
    uint64_t indexed_handle;
    size_t record_bytes;
    uint32_t pre_handle_bits;
    uint32_t reactor_count;
    uint32_t eed_skipped_bytes;
    uint16_t declared_type;
    uint8_t xdictionary_missing;
    uint8_t record_crc_verified;
    uint8_t embedded_self_verified;
} MusaDwgAc1018NonEntityIdentity;
/* Validate one MS-framed record at the indexed object offset: CRC16/ARC,
 * BS type, RL pre-handle boundary, absolute H code 0 matching indexed ID,
 * bounded EED, reactor BL and R2004 xdictionary-missing bit.
 * Only fixed nonentity types 42/49/51 are currently supported. They are
 * DICTIONARY/BLOCK_HEADER/LAYER numerical claims, NOT verified classes.
 * Reject unsupported types, malformed payload, CRC mismatch, wrong self,
 * aliasing, or out of range. Preserve *out unchanged on every error.
 * A valid common prefix is NOT a complete semantic object decoder.
 */
int musa_dwg_ac1018_nonentity_identity_record(
    const uint8_t *objects,size_t object_bytes,size_t object_offset,
    uint64_t indexed_handle,MusaDwgAc1018NonEntityIdentity *out);
#ifdef __cplusplus
}
#endif
#endif
