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

/* After CRC-verified self H and zero-length EED, the published common
 * format has a single graphic-present flag. This narrow witness ONLY
 * accepts flag=0 and returns the next exact DATA bit position. A flag=1
 * means unsupported proxy graphics and is never silently skipped.
 * This does not decode the remaining common entity header, object
 * geometry or any owner/layer handle reference. */
typedef struct MusaDwgAc1024NoProxyPrefix {
    uint64_t indexed_object_offset;
    uint64_t embedded_absolute_handle;
    size_t payload_start_byte;
    size_t data_after_no_proxy_flag_bit;
    size_t handle_stream_start_bit;
    size_t handle_stream_end_bit;
    uint16_t raw_ot;
    uint8_t frame_crc16_verified;
    uint8_t zero_eed_verified;
    uint8_t no_proxy_graphic_verified;
} MusaDwgAc1024NoProxyPrefix;

int musa_dwg_ac1024_record_no_proxy_prefix(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024NoProxyPrefix *out);

/* R2010 common ENTITY preamble witness following authenticated
 * OT + self-H + empty-EED + absent-proxy graphic:
 *   BB entity-mode, BL reactor count, B XDictionary-missing flag.
 * This deliberately covers ONLY fixed built-in types INSERT(7),
 * ARC(17), CIRCLE(18), LINE(19). Other OT values MUST be
 * class-resolved independently before any entity interpretation.
 *
 * Values are raw R2010 DATA evidence, never owner, layer or geometry
 * identity. A missing XDictionary flag does not authenticate any
 * handle reference. The DATA reader never enters the separate H stream.
 * No rendering, normal-open, writer or commercial licensing claim. */
typedef struct MusaDwgAc1024EntityMetaPrefix {
    uint64_t indexed_object_offset;
    uint64_t embedded_absolute_handle;
    size_t payload_start_byte;
    size_t data_after_entity_mode_bit;
    size_t data_after_reactors_bit;
    size_t data_after_xdic_bit;
    size_t handle_stream_start_bit;
    size_t handle_stream_end_bit;
    uint32_t reactor_count;
    uint16_t raw_ot;
    uint8_t entity_mode;
    uint8_t xdictionary_missing;
    uint8_t original_crc16_verified;
} MusaDwgAc1024EntityMetaPrefix;

/* Atomic, source-immutable and fail-closed. Reject the reserved mode 3,
 * more than 1024 reactors, malformed BL, any stream boundary crossing,
 * unsupported raw OT, source alias and malformed CRC. Success exposes
 * only a partial common DATA field boundary, never owner references. */
int musa_dwg_ac1024_record_entity_meta_prefix(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024EntityMetaPrefix *out);

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

/* Read-only O(log n) lookup in an ALREADY VERIFIED result from
 * musa_dwg_ac1024_recover_self_index, sorted by canonical self-H.
 * Does not repeat record CRCs or authenticate caller-made arrays.
 * Performs local identity/neighbor/order checks at the found slot.
 * Returns one immutable ID/offset witness, NEVER owner/layer/geometry.
 * On no match, malformed local evidence or aliasing, leaves *out alone. */
int musa_dwg_ac1024_self_index_find(
    const MusaDwgAc1024SelfIdentity *verified_index, size_t record_count,
    uint64_t canonical_absolute_handle, MusaDwgAc1024SelfIdentity *out);



/* AC1024 common owner/layer H evidence for the narrow verified subset:
 * fixed INSERT/ARC/CIRCLE/LINE, entity mode 0, zero reactors, no XDict,
 * zero EED and no proxy graphic. Caller supplies the immutable output of
 * recover_self_index. Actual owner BLOCK_HEADER (OT49) and LAYER (OT51)
 * source records are re-CRC-checked, not inferred from raw handle values.
 *
 * Only two common H references are consumed; subsequent appearance,
 * linetype, material and geometry data remain UNRESOLVED. Never use this
 * witness to render or save a drawing. All outputs are atomic. */
typedef struct MusaDwgAc1024OwnerLayerEvidence {
    uint64_t entity_handle;
    uint64_t entity_record_offset;
    uint64_t owner_handle;
    uint64_t owner_record_offset;
    uint64_t layer_handle;
    uint64_t layer_record_offset;
    size_t handle_tail_unparsed_bits;
    uint16_t entity_raw_ot;
    uint8_t owner_raw_h_code;
    uint8_t layer_raw_h_code;
    uint8_t source_crc_verified;
    uint8_t target_records_verified;
} MusaDwgAc1024OwnerLayerEvidence;

int musa_dwg_ac1024_entity_owner_layer_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t entity_handle, MusaDwgAc1024OwnerLayerEvidence *out);


#ifdef __cplusplus
}
#endif
#endif
