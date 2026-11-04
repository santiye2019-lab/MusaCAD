/* SPDX-License-Identifier: MIT
 * First-party R2010/AC1024 LINE field bridge. No external CAD code.
 */
#ifndef MUSACAD_DWG_AC1024_LINE_H
#define MUSACAD_DWG_AC1024_LINE_H
#include <stddef.h>
#include <stdint.h>
#include "musacad_dwg_ac1024_self_index.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Evidence-only common DATA fields for an original CRC-verified AC1024
 * fixed OT19 LINE: entity mode 0 (explicit owner), 1 (paper space),
 * or 2 (model space), with zero EED/reactors and no proxy/XDictionary.
 * Modes 1/2 lack a serialized owner H; parsing their DATA NEVER proves
 * a block owner, authenticated layer, visibility or drawing readiness.
 * Check the verified metadata prefix for mode; no owner is synthesized.
 * The R2013+ DS bit MUST NOT be consumed from an R2010 record.
 * Bit offsets refer to the record payload, never to the source file.
 * This is not layer/color inheritance, Android paint, or normal-open proof.
 */
typedef struct MusaDwgAc1024LineCommon {
    uint64_t indexed_handle;
    size_t geometry_start_bit;
    size_t data_end_bit;
    uint16_t encoded_color;
    uint16_t color_index;
    uint16_t invisibility;
    uint32_t rgb; /* 24-bit RRGGBB, method byte stripped if present */
    uint32_t rgb_raw; /* original BL, including any color-method byte */
    uint32_t transparency;
    double linetype_scale;
    uint8_t linetype_flags;
    uint8_t plotstyle_flags;
    uint8_t material_flags;
    uint8_t shadow_flags;
    uint8_t visualstyle_flags;
    uint8_t lineweight;
    uint8_t has_rgb;
    uint8_t rgb_method; /* 0=legacy 24-bit, 0xC2=explicit truecolor */
    uint8_t has_color_reference;
    uint8_t has_transparency;
    uint8_t record_crc_verified;
} MusaDwgAc1024LineCommon;

/* Both APIs preserve caller output on failure and never alter source data.
 * Their authentication scope is ONE indexed record in a separately verified
 * AcDbObjects section, not a complete source DWG or entity owner graph.
 */
int musa_dwg_ac1024_line_common_probe(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024LineCommon *out);
/* Diagnostic only, not an admission path. Zero means the strict original
 * zero-EED common LINE DATA was read; other values label the first failed
 * stage. No drawing data, coordinates or source bytes are exposed. */
enum {
    MUSA_DWG_LINE_COMMON_OK = 0,
    MUSA_DWG_LINE_COMMON_META = 1,
    MUSA_DWG_LINE_COMMON_STREAM = 2,
    MUSA_DWG_LINE_COMMON_COLOR = 3,
    MUSA_DWG_LINE_COMMON_RGB = 4,
    MUSA_DWG_LINE_COMMON_TRANSPARENCY = 5,
    MUSA_DWG_LINE_COMMON_LTYPE_SCALE = 6,
    MUSA_DWG_LINE_COMMON_APPEARANCE = 7,
    MUSA_DWG_LINE_COMMON_GEOMETRY_BOUND = 8,
    MUSA_DWG_LINE_COMMON_ARGUMENT = 9
};
uint8_t musa_dwg_ac1024_line_common_failure_stage(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset);


typedef struct MusaDwgAc1024LineFields {
    uint64_t indexed_handle;
    size_t geometry_start_bit;
    size_t geometry_end_bit;
    double start[3];
    double end[3];
    double thickness;
    double extrusion[3];
    uint8_t coordinates_decoded;
} MusaDwgAc1024LineFields;

/* Reuses shared bounded RD/DD/BT/BE decoders, but ONLY after the AC1024
 * common-data boundary is authenticated from the same CRC-checked record.
 * It does not prove display, measurement, owner/layer semantics or parity
 * with the source drawing; those require independent real-DWG tests.
 * When entity mode is 1/2, returned coordinates are provisional only:
 * there is no explicit owner handle and a layer/block/scene join is required
 * before painting anything.
 */
int musa_dwg_ac1024_line_fields_probe(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024LineFields *out);

/* Provisional EED-bearing AC1024 LINE DATA only. An independently bounded
 * and terminated nonzero EED application chain must precede the no-proxy,
 * entity-mode, common color and RD/DD/BT/BE geometry fields. This is
 * deliberately separate from the legacy zero-EED-only entry points.
 * EED application semantics, original owner/layer/dictionary H graph,
 * AutoCAD parity and scene/render acceptance remain UNPROVEN.
 * Source is immutable, output is preserved on every failure. */
int musa_dwg_ac1024_line_eed_fields_probe(
    const uint8_t *objects, size_t object_bytes, uint64_t indexed_offset,
    MusaDwgAc1024LineFields *out);

/* A stronger source-internal LINE witness: CRC-verified coordinates
 * AND original self-H -> independently authenticated owner OT49 and
 * layer OT51. This API admits only mode 0 (explicit owner) and refuses
 * implicit model/paper-space modes until an independent graph algorithm
 * can authenticate their ownership. No scene/visibility/color inheritance,
 * WCS placement, rendering or AutoCAD parity is claimed.
 *
 * Output is atomic on error and MUST NOT overlap the source or index.
 * The recovered index must come from authenticated handle-map offsets.
 */
typedef struct MusaDwgAc1024LineGraphEvidence {
    MusaDwgAc1024LineFields geometry;
    MusaDwgAc1024OwnerLayerEvidence owner_layer;
} MusaDwgAc1024LineGraphEvidence;

int musa_dwg_ac1024_line_graph_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_line_handle, MusaDwgAc1024LineGraphEvidence *out);


/* Stronger than provisional coordinates, weaker than scene admission.
 * Fixed OT19 mode 1/2 LINE DATA and independently CRC-backed LAYER OT51
 * from the same record. There is NO serialized owner H; this API does
 * NOT assert a block owner, layout, inherited color or image readiness.
 */
typedef struct MusaDwgAc1024ImplicitLineLayerEvidence {
    MusaDwgAc1024LineFields geometry;
    MusaDwgAc1024ImplicitLayerEvidence implicit_layer;
} MusaDwgAc1024ImplicitLineLayerEvidence;

int musa_dwg_ac1024_line_implicit_layer_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_line_handle,
    MusaDwgAc1024ImplicitLineLayerEvidence *out);


/* One-record combined witness for EED-bearing OT19 LINE XYZ + original
 * handle graph. It proves the layer OT51 is CRC/authenticated, and an
 * OT49 owner ONLY when explicit mode 0 encodes an owner handle. In modes
 * 1/2 owner remains zero. Optional XDictionary OT42 is authenticated
 * only if its own H exists. EED APPID semantics, inherited appearance,
 * block placement, visibility and scene admission are NOT validated. */
typedef struct MusaDwgAc1024LineEedJoinedEvidence {
    MusaDwgAc1024LineFields geometry;
    MusaDwgAc1024LineEedHandleGraph graph;
} MusaDwgAc1024LineEedJoinedEvidence;

int musa_dwg_ac1024_line_eed_verified_probe(
    const uint8_t *objects, size_t object_bytes,
    const MusaDwgAc1024SelfIdentity *recovered_index, size_t record_count,
    uint64_t canonical_line_handle, MusaDwgAc1024LineEedJoinedEvidence *out);

#ifdef __cplusplus
}
#endif
#endif
