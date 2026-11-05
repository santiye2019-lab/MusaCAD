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

/* Bounded original R2010 extended entity data (EED) frame proof:
 * BS application-data byte length, H5 application ID, opaque raw bytes,
 * repeated until an explicit zero-length BS terminator. The application
 * bytes are NOT interpreted, and no app semantics, geometry, owner,
 * layer, proxy graphic or drawing is certified.
 *
 * This separate read-only gate does not change the original zero-EED
 * APIs or admit EED-bearing entities to existing LINE/scene readers.
 * Returns an exact payload-relative bit cursor after the terminator,
 * all-or-nothing and never writes to the source or caller on failure.
 */
typedef struct MusaDwgAc1024EedChainEvidence {
    uint64_t indexed_object_offset;
    uint64_t embedded_absolute_handle;
    size_t payload_start_byte;
    size_t data_after_eed_bit;
    size_t handle_stream_start_bit;
    size_t handle_stream_end_bit;
    size_t application_count;
    size_t application_data_bytes;
    uint16_t raw_ot;
    uint8_t original_crc16_verified;
    uint8_t eed_boundaries_verified;
} MusaDwgAc1024EedChainEvidence;

int musa_dwg_ac1024_record_eed_chain_probe(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024EedChainEvidence *out);

/* Original R2010 EED APPID target proof (fixed OT19 LINE only).
 * For each BS-length/H5/application-data frame, verify that the H5
 * resolves to a unique source-indexed, original CRC-backed APPID (OT67)
 * record with matching canonical self-H. Application bytes remain opaque.
 * Repeated use of the same APPID is counted without fabricating a new
 * target. No application semantics, geometry/scene, writer, visibility
 * or external reference permission is inferred. Fail closed and atomic. */
typedef struct MusaDwgAc1024EedAppidEvidence {
    MusaDwgAc1024EedChainEvidence chain;
    size_t appid_reference_count;
    size_t distinct_appid_count;
    uint64_t first_appid_handle;
    uint64_t first_appid_record_offset;
    uint8_t original_appid_records_crc_verified;
} MusaDwgAc1024EedAppidEvidence;

int musa_dwg_ac1024_line_eed_appid_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_line_handle, MusaDwgAc1024EedAppidEvidence *out);

/* A distinct OT51 LAYER EED H5 APPID target witness, using the exact
 * first-party CRC/OT67/self-H checks already required for OT19 LINE.
 * The APPID target's bytes are not interpreted, and neither visibility
 * nor layer CMC/colour follows from this evidence. */
int musa_dwg_ac1024_layer_eed_appid_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle, MusaDwgAc1024EedAppidEvidence *out);

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

/* Deliberately separate from the zero-EED owner/layer/scene gates:
 * after an authentic nonzero, fully terminated EED application chain,
 * bound the no-proxy / entity-mode / reactor / XDictionary DATA header.
 * Fixed OT19 only. No EED interpretation, original handle graph, scene
 * or normal DWG-open permission follows from this evidence. */
int musa_dwg_ac1024_record_line_eed_meta_prefix(
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



/* Source-internal R2010 LINE layer-only evidence for implicit paper/model
 * space entity mode (1/2). NO owner H is serialized, so NO owner, BLOCK
 * relationship, layout identity, inherited style or scene readiness is
 * inferred. The decoded layer H must point at an independently verified
 * original CRC/OT51/self-H record, not merely at a numeric handle. */
typedef struct MusaDwgAc1024ImplicitLayerEvidence {
    uint64_t entity_handle;
    uint64_t entity_record_offset;
    uint64_t layer_handle;
    uint64_t layer_record_offset;
    size_t handle_tail_unparsed_bits;
    uint16_t entity_raw_ot;
    uint8_t entity_mode;
    uint8_t layer_raw_h_code;
    uint8_t source_crc_verified;
    uint8_t layer_crc_verified;
} MusaDwgAc1024ImplicitLayerEvidence;

/* Read-only, atomic on failure and no alias with original objects or
 * independently authenticated recovered handle index. Fixed OT19 only.
 * Authentication is scoped to the original indexed AcDbObjects section.
 * This is NOT permission to render the provisional LINE geometry. */
int musa_dwg_ac1024_implicit_layer_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t entity_handle, MusaDwgAc1024ImplicitLayerEvidence *out);


/* Original R2010 LINE mode 0/1/2 with XDictionary-present flag.
 * Verifies H order: [mode-0 owner], original dictionary H (hard owner)
 * and original layer H (hard pointer). Targets must be independently
 * recovered CRC-backed OT49 / OT42 / OT51 records respectively.
 * For implicit mode 1/2 no owner is serialized and NO owner is fabricated.
 * This is bounded handle-graph evidence only: does not prove geometry,
 * dictionary contents, layer visibility, inherited style or scene admission.
 */
typedef struct MusaDwgAc1024LineXDictionaryGraph {
    uint64_t entity_handle;
    uint64_t entity_record_offset;
    uint64_t owner_handle;
    uint64_t owner_record_offset;
    uint64_t xdictionary_handle;
    uint64_t xdictionary_record_offset;
    uint64_t layer_handle;
    uint64_t layer_record_offset;
    size_t handle_tail_unparsed_bits;
    uint16_t entity_raw_ot;
    uint8_t entity_mode;
    uint8_t owner_raw_h_code;
    uint8_t xdictionary_raw_h_code;
    uint8_t layer_raw_h_code;
    uint8_t original_records_crc_verified;
} MusaDwgAc1024LineXDictionaryGraph;

/* All-or-nothing and source immutable. Recovered index must originate in
 * the independently authenticated AcDbHandles mapping. */
int musa_dwg_ac1024_line_xdictionary_graph_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t entity_handle, MusaDwgAc1024LineXDictionaryGraph *out);


/* Exact original H-stream evidence for EED-bearing LINE (fixed OT19).
 * After a CRC-bounded, NONEMPTY EED application chain, this verifies:
 * - original OT51 LAYER reference on mode 0/1/2,
 * - original OT49 BLOCK_HEADER when mode 0 serializes an owner H,
 * - original OT42 DICTIONARY when XDictionary-present is flagged.
 * Modes 1/2 do NOT serialize an owner H; owner remains exactly zero.
 * EED APPID H values are bounded, NOT separately object-verified here.
 * This witness never implies layout membership, block transform, layer
 * visibility, or render/scene permission. All output is atomic. */
typedef struct MusaDwgAc1024LineEedHandleGraph {
    MusaDwgAc1024LineXDictionaryGraph handles;
    MusaDwgAc1024EedChainEvidence eed;
} MusaDwgAc1024LineEedHandleGraph;

int musa_dwg_ac1024_line_eed_handle_graph_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t entity_handle, MusaDwgAc1024LineEedHandleGraph *out);


/* R2010 source-LITERAL LAYER OT51 numeric FLAGS preflight:
 * after CRC-proven absolute self-H and zero EED, the original object
 * common BL reactors (must be zero), B XDictionary flag, then LAYER's
 * B 64-bit-entity flag, BS xref-index+1, B xref-dependent, and BS raw
 * combined 70/290/370 values. No TU name, CMC color, viewport override,
 * XDictionary target or complete H graph is inferred by this function.
 * Low 10 raw bits are reported independently, NOT rendered as visibility.
 * A nonzero unmapped mask is preserved; display_accepted ALWAYS 0.
 * Caller supplies source-H/CRC recovered index; *out is fail-atomic. */
typedef struct MusaDwgAc1024LayerRawFlagsEvidence {
    uint64_t layer_handle;
    uint64_t layer_record_offset;
    size_t raw_flags_start_bit;
    size_t raw_flags_end_bit;
    uint16_t xref_index_plus_one;
    uint16_t raw_values_70_290_370;
    uint16_t unmapped_bits;
    uint8_t entry_64_flag;
    uint8_t xref_dependent;
    uint8_t xdictionary_missing;
    uint8_t frozen_bit;
    uint8_t on_bit;
    uint8_t frozen_new_viewports_bit;
    uint8_t locked_bit;
    uint8_t plotting_bit;
    uint8_t lineweight_field;
    uint8_t original_record_crc_verified;
    uint8_t display_accepted;
} MusaDwgAc1024LayerRawFlagsEvidence;

int musa_dwg_ac1024_layer_raw_flags_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t layer_handle, MusaDwgAc1024LayerRawFlagsEvidence *out);

/* Separate NONZERO-EED original OT51 numerical preflight; verified H5
 * application chain is bounded and CRC backed, but individual APPID
 * target records/application bytes are not semantically interpreted.
 * Output cannot be used to hide, recolor, or promote CAD geometry. */
int musa_dwg_ac1024_layer_eed_raw_flags_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t layer_handle, MusaDwgAc1024LayerRawFlagsEvidence *out);

/* Original R2010 LAYER OT51 H stream, EED-bearing and XDictionary
 * present ONLY. A CRC-backed layer and its OT67 APPID targets are
 * mandatory. The first H4 is an OT50 LAYER_CONTROL soft-pointer; the
 * next H3 is an OT42 DICTIONARY hard-owner reference. Relative H6/H8/
 * HA/HC codes are admitted only after exact target OT/CRC/self-H proof.
 * Later linetype, plotstyle, material and XREF H refs are UNPARSED.
 * This does not resolve CMC, VP visibility, draw/paint or block context.
 * Outputs are atomic and never authorize scene rendering. */
typedef struct MusaDwgAc1024LayerEedXDictionaryGraph {
    uint64_t layer_handle;
    uint64_t layer_record_offset;
    uint64_t layer_control_handle;
    uint64_t layer_control_record_offset;
    uint64_t xdictionary_handle;
    uint64_t xdictionary_record_offset;
    size_t remaining_handle_tail_bits;
    size_t eed_appid_reference_count;
    uint8_t layer_control_raw_h_code;
    uint8_t xdictionary_raw_h_code;
    uint8_t original_records_crc_verified;
    uint8_t display_accepted;
} MusaDwgAc1024LayerEedXDictionaryGraph;

int musa_dwg_ac1024_layer_eed_xdictionary_graph_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle,
    MusaDwgAc1024LayerEedXDictionaryGraph *out);


/* Diagnostic ONLY: inspect the next raw H header after independently
 * authenticated OT50 and OT42 references of an EED-bearing OT51.
 * This bounds the declared H payload but does NOT resolve its target,
 * assume linetype/plotstyle/material/XREF roles, or enable rendering.
 * A missing/malformed header is refused; output is fail-atomic. */
typedef struct MusaDwgAc1024LayerEedTailRawHeader {
    uint64_t layer_handle;
    uint64_t layer_record_offset;
    size_t original_tail_start_bit;
    size_t remaining_handle_tail_bits;
    uint8_t raw_h_code;
    uint8_t raw_h_size;
    uint8_t original_records_crc_verified;
    uint8_t display_accepted;
} MusaDwgAc1024LayerEedTailRawHeader;

int musa_dwg_ac1024_layer_eed_tail_raw_header_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle,
    MusaDwgAc1024LayerEedTailRawHeader *out);


/* Experimental shape-only scan of ALL H tokens remaining after the
 * CRC/authenticated OT51 -> OT50 + OT42 graph. The reader enforces
 * a 64-token budget, byte counts and exact bit termination; H codes
 * are NOT mapped to target types, color/linetype, or XREF visibility.
 * H5 with zero payload is counted, never treated as proof of any role.
 * No source mutations, no implicit scene permission, fail-atomic. */
typedef struct MusaDwgAc1024LayerEedTailShape {
    uint64_t layer_handle;
    uint64_t layer_record_offset;
    size_t tail_start_bit;
    size_t tail_bit_count;
    size_t h_token_count;
    size_t raw_h5_zero_length_count;
    size_t other_raw_h_count;
    size_t raw_payload_octets;
    size_t raw_h5_nonzero_length_count;
    size_t raw_non_h5_zero_length_count;
    size_t raw_non_h5_nonzero_length_count;
    uint8_t original_records_crc_verified;
    uint8_t display_accepted;
} MusaDwgAc1024LayerEedTailShape;

int musa_dwg_ac1024_layer_eed_tail_shape_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle, MusaDwgAc1024LayerEedTailShape *out);


/* First refusal from exactly the same strict tail-shape parser.
 * Diagnosis records only a stage, never source bytes or H values. */
enum {
    MUSA_DWG_LAYER_TAIL_SHAPE_OK = 0,
    MUSA_DWG_LAYER_TAIL_SHAPE_ARGUMENT = 1,
    MUSA_DWG_LAYER_TAIL_SHAPE_PRECONDITION = 2,
    MUSA_DWG_LAYER_TAIL_SHAPE_STREAM = 3,
    MUSA_DWG_LAYER_TAIL_SHAPE_PREFIX = 4,
    MUSA_DWG_LAYER_TAIL_SHAPE_TOKEN_LIMIT = 5,
    MUSA_DWG_LAYER_TAIL_SHAPE_SHORT_HEADER = 6,
    MUSA_DWG_LAYER_TAIL_SHAPE_HEADER_READ = 7,
    MUSA_DWG_LAYER_TAIL_SHAPE_LENGTH = 8,
    MUSA_DWG_LAYER_TAIL_SHAPE_LENGTH_OVERRUN = 9,
    MUSA_DWG_LAYER_TAIL_SHAPE_PAYLOAD = 10,
    MUSA_DWG_LAYER_TAIL_SHAPE_FINAL = 11
};
uint8_t musa_dwg_ac1024_layer_eed_tail_shape_failure_stage(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle);

/* Read-only original OT51 EED first H header, for source-format
 * diagnosis only. Returns the exact 4-bit H code/size without resolving
 * target roles. The record, EED APPIDs and XDictionary-present flag
 * must already pass independent original CRC/H checks. This DOES NOT
 * authenticate any H target, layer visibility or scene/draw permission.
 * The output is fail-atomic and contains no file path or coordinates. */
typedef struct MusaDwgAc1024LayerEedRawHandleHeader {
    uint64_t layer_handle;
    uint64_t layer_record_offset;
    size_t original_handle_start_bit;
    size_t handle_tail_bits;
    uint8_t first_h_code;
    uint8_t first_h_size;
    uint8_t record_crc_verified;
    uint8_t display_accepted;
} MusaDwgAc1024LayerEedRawHandleHeader;

int musa_dwg_ac1024_layer_eed_first_h_header_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle,
    MusaDwgAc1024LayerEedRawHandleHeader *out);

/* Exact same reader, with its first refusal stage. This is a read-only
 * diagnosis and never a fallback into render/scene acceptance. */
enum {
    MUSA_DWG_LAYER_XDIC_GRAPH_OK = 0,
    MUSA_DWG_LAYER_XDIC_GRAPH_ARGUMENT = 1,
    MUSA_DWG_LAYER_XDIC_GRAPH_IDENTITY = 2,
    MUSA_DWG_LAYER_XDIC_GRAPH_EED = 3,
    MUSA_DWG_LAYER_XDIC_GRAPH_FLAGS = 4,
    MUSA_DWG_LAYER_XDIC_GRAPH_APPID = 5,
    MUSA_DWG_LAYER_XDIC_GRAPH_TAIL = 6,
    MUSA_DWG_LAYER_XDIC_GRAPH_CONTROL_H = 7,
    MUSA_DWG_LAYER_XDIC_GRAPH_DICTIONARY_H = 8,
    MUSA_DWG_LAYER_XDIC_GRAPH_TARGET_INDEX = 9,
    MUSA_DWG_LAYER_XDIC_GRAPH_TARGET_CLASS = 10,
    MUSA_DWG_LAYER_XDIC_GRAPH_TARGET_CRC = 11
};
uint8_t musa_dwg_ac1024_layer_eed_xdictionary_graph_failure_stage(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_layer_handle);


/* Diagnostics use the EXACT strict OT51 proof; no alternative parsing,
 * fallback flags, or scene/display acceptance. */
enum {
    MUSA_DWG_LAYER_FLAGS_OK = 0,
    MUSA_DWG_LAYER_FLAGS_ARGUMENT = 1,
    MUSA_DWG_LAYER_FLAGS_INDEX = 2,
    MUSA_DWG_LAYER_FLAGS_ZERO_EED = 3,
    MUSA_DWG_LAYER_FLAGS_RECORD_BOUNDS = 4,
    MUSA_DWG_LAYER_FLAGS_REACTORS = 5,
    MUSA_DWG_LAYER_FLAGS_XDICTIONARY_BIT = 6,
    MUSA_DWG_LAYER_FLAGS_NUMERIC = 7
};
uint8_t musa_dwg_ac1024_layer_raw_flags_failure_stage(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t layer_handle);

#ifdef __cplusplus
}
#endif
#endif
