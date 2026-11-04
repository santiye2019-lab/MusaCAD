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
    uint32_t rgb;
    uint32_t transparency;
    double linetype_scale;
    uint8_t linetype_flags;
    uint8_t plotstyle_flags;
    uint8_t material_flags;
    uint8_t shadow_flags;
    uint8_t visualstyle_flags;
    uint8_t lineweight;
    uint8_t has_rgb;
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

#ifdef __cplusplus
}
#endif
#endif
