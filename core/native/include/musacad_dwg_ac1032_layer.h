/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_LAYER_H
#define MUSACAD_DWG_AC1032_LAYER_H
#include "musacad_dwg_ac1032_object_streams.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original R2018 OT51 LAYER identity, NOT yet its style definition.
 * Source must already be independently H-indexed, checksum-authenticated
 * and partitioned into bounded DATA and HANDLES.  No presumed color or
 * layer visibility is ever supplied by this structure.
 * Shared first-party C11 across Windows, iOS and Android. */
typedef struct MusaDwgAc1032LayerIdentity {
    uint64_t indexed_handle;
    size_t data_fields_start_bit;
    uint16_t object_type;
    uint8_t original_record_crc_verified;
    uint8_t embedded_self_handle_verified;
    uint8_t empty_eed_verified;
    uint8_t no_reactors_verified;
    uint8_t xdictionary_missing;
} MusaDwgAc1032LayerIdentity;
/* Strict no-EED/no-reactors/missing-XDictionary subset. All outputs
 * fail-atomic and source unchanged, even if bitstream is corrupt. */
int musa_dwg_ac1032_layer_ot51_identity_at(
    const uint8_t *payload,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LayerIdentity *out);
/* R2018 LAYER name: UTF-16LE TU in the separately length-framed,
 * bit-aligned object string stream immediately preceding H records.
 * Validated source record and strict LAYER identity are required.
 * Unicode BMP and valid surrogate pairs become UTF-8; unsupported
 * names/resource limits reject without changing caller-owned output.
 * No BYLAYER color, freeze, off, or lock claim. */
typedef struct MusaDwgAc1032LayerName {
    uint64_t indexed_handle;
    size_t original_string_start_bit;
    uint32_t utf16_code_units;
    uint32_t utf8_bytes;
    uint8_t original_record_crc_verified;
    char utf8[512];
} MusaDwgAc1032LayerName;
int musa_dwg_ac1032_layer_ot51_name_at(
    const uint8_t *payload,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LayerName *out);
/* SOURCE-LITERAL R2018 LAYER numeric flag region (after proven
 * self-H/EED/reactor/XDict identity, before the independent TU NAME).
 * The R2000+ LAYER grammar carries 64-bit, xref-index+1, xref-dependent,
 * then one BS values group containing 70/290/370 information.
 *
 * IMPORTANT: An original record may express viewport-specific overrides,
 * xrefs and other style semantics. This structure deliberately exposes
 * the SOURCE bitfield with bit positions, NOT an independently certified
 * visible/frozen/locked state or a BYLAYER color. No rendering code may
 * use it to hide or recolor geometry until semantic/override tests pass.
 * All caller outputs are fail-atomic, all source data stays unchanged. */
typedef struct MusaDwgAc1032LayerFlagsEvidence {
    uint64_t indexed_handle;
    size_t numeric_flags_start_bit;
    size_t numeric_flags_end_bit;
    uint16_t xref_index_plus_one;
    uint16_t raw_values_70_290_370;
    uint8_t entry_64_flag;
    uint8_t xref_dependent;
    uint8_t source_crc_verified;
} MusaDwgAc1032LayerFlagsEvidence;
int musa_dwg_ac1032_layer_ot51_flags_evidence_at(
    const uint8_t *payload,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LayerFlagsEvidence *out);

/* DWG R2000+ numeric VALUES(70,290,370) bit decomposition per ODA
 * published on-disk format. Separate FROM DXF LAYER group 70 flags:
 * bit 0 frozen, 1 on, 2 frozen in new viewports, 3 locked, 4 plot,
 * bits [9:5] lineweight. This reads source bits, NOT actual viewport
 * or full-layer visibility. R2018 original Layer0 evidence yields 0xE0
 * (on bit clear); without independent rendering-state cross-check, using
 * that bit to hide objects would be unsafe. display_accepted is ALWAYS 0.
 * Unknown high bits are reported rather than masked away. */
typedef struct MusaDwgAc1032LayerBaseBitPreflight {
    uint64_t layer_handle;
    uint16_t raw_values;
    uint16_t unmapped_bits;
    uint8_t frozen_bit;
    uint8_t on_bit;
    uint8_t frozen_in_new_viewports_bit;
    uint8_t locked_bit;
    uint8_t plotting_bit;
    uint8_t lineweight_field;
    uint8_t source_crc_verified;
    uint8_t display_accepted;
} MusaDwgAc1032LayerBaseBitPreflight;
/* From a plain numeric value; useful for independent bit-order tests,
 * produces no CRC/source claim. */
int musa_dwg_ac1032_layer_base_bits_decode(
    uint16_t raw_values,MusaDwgAc1032LayerBaseBitPreflight *out);
/* Original strict OT51 H/CRC record with field/bit bounds. Fail atomic. */
int musa_dwg_ac1032_layer_ot51_base_bits_at(
    const uint8_t *payload,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LayerBaseBitPreflight *out);


/* Narrow source-evidence profile for ORIGINAL R2018 LAYER CMC:
 * authenticated H/CRC layer identity+name+numeric status, exactly 7 opaque
 * bits between status and the CMC, then one complete numeric CMC ending
 * exactly at the independently authenticated STRING stream start.
 *
 * The 7 bits are NOT yet independently assigned DWG semantics. We only
 * accept the observed default-layer profile (value=3) with packed CMC
 * method 0xC3 / ACI 1..255. All other profiles fail CLOSED, not
 * fallback to white. No truecolor, per-viewport override, BYBLOCK or
 * final visibility/palette conversion claim. This is evidence, not a
 * general solution to BYLAYER rendering. */
typedef struct MusaDwgAc1032LayerCmcEvidence {
    uint64_t indexed_handle;
    uint32_t packed_color_method;
    size_t opaque_field_start_bit;
    size_t cmc_start_bit;
    size_t cmc_end_bit;
    uint8_t opaque_field_bits;
    uint8_t aci_index;
    uint8_t original_record_crc_verified;
} MusaDwgAc1032LayerCmcEvidence;
int musa_dwg_ac1032_layer_ot51_anchored_cmc_evidence_at(
    const uint8_t *payload,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LayerCmcEvidence *out);

#ifdef __cplusplus
}
#endif
#endif
