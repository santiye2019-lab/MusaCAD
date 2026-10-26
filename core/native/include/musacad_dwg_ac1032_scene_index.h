/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_SCENE_INDEX_H
#define MUSACAD_DWG_AC1032_SCENE_INDEX_H
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_line_file.h"
#include "musacad_dwg_ac1032_curve.h"
#include "musacad_dwg_ac1032_point.h"
#include "musacad_dwg_ac1032_text.h"
#include "musacad_dwg_ac1032_insert.h"
#include "musacad_dwg_ac1032_mtext.h"
#include "musacad_dwg_ac1032_lwpolyline.h"
#include "musacad_dwg_ac1032_polyline3d.h"
#include "musacad_dwg_ac1032_layer.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Opaque immutable authenticated original R2018 scene INDEX, NOT an
 * editable/lossless DWG document. Original physical page verification,
 * handles and object-section extraction are performed at OPEN time, not
 * repeatedly for every entity. Owns its extracted Objects and H index;
 * the input DWG byte array may be released after open succeeds.
 * Supports one shared pure C11 ABI on Windows, iOS and Android.
 */
typedef struct MusaDwgAc1032SceneIndex MusaDwgAc1032SceneIndex;
int musa_dwg_ac1032_scene_index_open(
    const uint8_t *file,size_t bytes,MusaDwgAc1032SceneIndex **out);
void musa_dwg_ac1032_scene_index_close(MusaDwgAc1032SceneIndex *index);
/* Both information queries are O(1) on already CRC-validated H index. */
size_t musa_dwg_ac1032_scene_index_count(
    const MusaDwgAc1032SceneIndex *index);
int musa_dwg_ac1032_scene_index_entry(
    const MusaDwgAc1032SceneIndex *index,size_t at,
    MusaDwgAc1032HandleEntry *out);
/* Original record CRC and OT checked on each individual binary-search
 * access; O(log N) index lookup, independent of original file size.
 * Does not claim all indexed objects have decoded geometry.
 */
int musa_dwg_ac1032_scene_index_object_type(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    uint16_t *out_type);
int musa_dwg_ac1032_scene_index_read_line(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032NativeLine *out);
int musa_dwg_ac1032_scene_index_read_curve(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032NativeCurve *out);
int musa_dwg_ac1032_scene_index_read_ellipse(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032NativeEllipse *out);
/* One original OT44 MTEXT UTF-16 TU string from CRC/H-authenticated
 * R2018 source. This is source text, not layout, fonts or glyph rendering.
 * Does NOT parse OT1 TEXT, paragraph formatting or owner visibility. */
/* One original CRC/H verified common source COLOR/visibility field.
 * Fonts/layout, block ownership, viewport and final BYLAYER remain open. */
/* Genuine R2018 OT1 TEXT entity-common source only. Strict OT/H/CRC and
 * ENC/visibility fields, no claim of TEXT TU, WCS, font, owner or glyphs.
 * Deliberately distinct from OT44 MTEXT and its string grammar. */
/* Original OT1 TEXT Unicode TU source; not font or WCS. */
int musa_dwg_ac1032_scene_index_read_text_source(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032TextSource *out);
/* Original OT1 TEXT OCS numeric placement, not font/WCS or glyph paint. */
int musa_dwg_ac1032_scene_index_read_text_placement(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032TextPlacement *out);
int musa_dwg_ac1032_scene_index_read_text_common(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032EntityCommon *out);
int musa_dwg_ac1032_scene_index_read_mtext_common(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032EntityCommon *out);
int musa_dwg_ac1032_scene_index_read_mtext_source(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032MTextSource *out);
/* Authenticated original OT44 MTEXT position/axes/text height and width;
 * NOT a glyph/font renderer or final 2D display acceptance. */
int musa_dwg_ac1032_scene_index_read_mtext_placement(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032MTextPlacement *out);
/* OT1 TEXT source H graph, independent of OT44 MTEXT. No font,
 * block-owner/model-space, viewport or final BYLAYER display claim. */
typedef struct MusaDwgAc1032TextGraph {
    uint64_t indexed_handle,owner_handle,layer_handle,style_handle;
    uint16_t layer_object_type,style_object_type;
    uint8_t original_entity_crc_verified;
    uint8_t original_layer_crc_verified;
    uint8_t original_style_crc_verified;
    uint8_t graph_edges_verified;
    uint8_t font_and_display_accepted; /* ALWAYS 0 */
} MusaDwgAc1032TextGraph;
int musa_dwg_ac1032_scene_index_text_graph(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032TextGraph *out);
/* MTEXT original H-tail edges, not font decoding or final style resolution.
 * Only the strict no-EED/no-reactor/no-XDic profile is admitted. */
typedef struct MusaDwgAc1032MTextGraph {
    uint64_t indexed_handle;
    uint64_t owner_handle,layer_handle,style_handle;
    uint16_t layer_object_type,style_object_type;
    uint8_t original_entity_crc_verified;
    uint8_t original_layer_crc_verified;
    uint8_t original_style_crc_verified;
    uint8_t graph_edges_verified;
    uint8_t font_and_display_accepted; /* ALWAYS 0 */
} MusaDwgAc1032MTextGraph;
int musa_dwg_ac1032_scene_index_mtext_graph(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032MTextGraph *out);
int musa_dwg_ac1032_scene_index_read_point(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032NativePoint *out);
int musa_dwg_ac1032_scene_index_read_lwpolyline(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032LWVertex *vertices,size_t capacity,
    MusaDwgAc1032NativeLWPolyline *out);
int musa_dwg_ac1032_scene_index_read_vertex3d(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032Vertex3D *out);
/* Complete R2018 OT16 3D POLYLINE: original indexed hard-owner sequence
 * -> original OT11 children -> independently verified reverse owner H,
 * same layer H, and indexed OT6 SEQEND. All lookups use cached authenticated
 * Objects/H index, so the original file bytes may already be freed.
 * Never invent geometry from numeric H order or positional proximity.
 * Fail-atomic for both outputs and no partial emission. No nested full-file
 * extraction, no external CAD runtime, no claim of arbitrary DWG scene. */
int musa_dwg_ac1032_scene_index_read_polyline3d_vertices(
    const MusaDwgAc1032SceneIndex *index,uint64_t polyline_handle,
    MusaDwgAc1032Polyline3DOrderedVertex *vertices,size_t capacity,
    MusaDwgAc1032Polyline3DOrdered *out);
/* R2018 original LAYER OT51 strict self-H/EED/reactor/XDic identity,
 * CRC/H-backed cached source. Does not claim layer color, freeze, or off
 * flags; source has already been indexed and verified at scene opening. */
int musa_dwg_ac1032_scene_index_read_layer_identity(
    const MusaDwgAc1032SceneIndex *scene,
    uint64_t layer_handle,MusaDwgAc1032LayerIdentity *out);

/* Extract the original indexed R2018 LAYER name, not a guessed
 * palette/style. TU Unicode is converted into bounded UTF8 directly from
 * the source-authenticated cached object STRING stream. */
int musa_dwg_ac1032_scene_index_read_layer_name(
    const MusaDwgAc1032SceneIndex *scene,uint64_t layer_handle,
    MusaDwgAc1032LayerName *out);

/* Read authenticated LAYER OT51 raw numerical status flags from
 * original source H. Not a resolved layer visibility or BYLAYER ACI:
 * rendering must remain provisional until status/color semantics pass
 * independent original-file and per-viewport/XREF override tests. */
int musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
    const MusaDwgAc1032SceneIndex *scene,uint64_t layer_handle,
    MusaDwgAc1032LayerFlagsEvidence *out);

/* Narrow original R2018 LAYER CMC evidence, CRC/H and exact string
 * boundary validated. Not general BYLAYER rendering: opaque 7-bit
 * source prefix is admitted only for explicitly tested default profile. */
int musa_dwg_ac1032_scene_index_layer_anchored_cmc_evidence(
    const MusaDwgAc1032SceneIndex *scene,uint64_t layer_handle,
    MusaDwgAc1032LayerCmcEvidence *out);

/* First step toward REAL BYLAYER rendering: source-CRC/H-validated
 * common built-in entity -> original DWG LAYER record (OT=51).
 * Only strict simple no-owner/no-reactor/no-extra-H 2018 entities are
 * accepted. The original LAYER record color/visibility has NOT been
 * decoded by this operation; callers must NOT infer a default ACI7
 * or claim fully resolved BYLAYER. This read-only graph evidence is
 * common C11 code (Android, Windows, iOS) and fail-atomic. */
typedef struct MusaDwgAc1032EntityLayerRef {
    uint64_t entity_handle;
    uint64_t layer_handle;
    uint16_t raw_entity_type;
    uint16_t raw_layer_type;
    uint8_t original_entity_crc_verified;
    uint8_t original_layer_crc_verified;
    uint8_t source_handle_ref_verified;
} MusaDwgAc1032EntityLayerRef;
int musa_dwg_ac1032_scene_index_entity_layer_ref(
    const MusaDwgAc1032SceneIndex *index,
    uint64_t entity_handle,MusaDwgAc1032EntityLayerRef *out);

/* Single ORIGINAL indexed DWG graph walk from one geometry entity
 * to its source-CRC-verified LAYER table record and bounded Unicode TU
 * name. Built from separate first-party readers, never by guessing a
 * default layer name/handle or using object H proximity. Colors and
 * visibility are NOT yet read; BYLAYER rendering must not claim exactness
 * based on a successfully decoded NAME alone.
 * Fail-atomic, read-only, portable C11 across Android, iOS and Windows. */
typedef struct MusaDwgAc1032EntityLayerName {
    MusaDwgAc1032EntityLayerRef edge;
    MusaDwgAc1032LayerName layer;
    uint8_t source_graph_and_name_verified;
} MusaDwgAc1032EntityLayerName;
int musa_dwg_ac1032_scene_index_entity_layer_name(
    const MusaDwgAc1032SceneIndex *index,
    uint64_t entity_handle,MusaDwgAc1032EntityLayerName *out);

/* Original indexed R2018 source inventory/geometry evidence. Every
 * indexed H is CRC-checked for type. Native geometry claims require the
 * actual first-party type-specific decoder, not just a matching raw OT.
 * VERTEX3D and SEQEND are DEPENDENT; unknown/proxy/nonentity remains
 * OPAQUE rather than silently disappearing from a drawing. */
#define MUSA_AC1032_SCENE_OPAQUE 0u
#define MUSA_AC1032_SCENE_GEOMETRY_DECODED 1u
#define MUSA_AC1032_SCENE_GEOMETRY_REJECTED 2u
#define MUSA_AC1032_SCENE_DEPENDENT 3u
typedef struct MusaDwgAc1032SceneGeometryRecord {
    uint64_t indexed_handle;
    uint16_t raw_object_type;
    uint8_t category;
    uint8_t original_record_crc_verified;
    uint32_t decoded_vertex_count; /* LWPLINE / 3D POLYLINE only */
} MusaDwgAc1032SceneGeometryRecord;
typedef struct MusaDwgAc1032SceneGeometryReport {
    size_t indexed_records;
    size_t decoded_geometry_objects;
    size_t rejected_supported_geometry;
    size_t dependent_objects;
    size_t opaque_objects;
    uint8_t all_indexed_raw_types_crc_verified;
} MusaDwgAc1032SceneGeometryReport;
/* Full validated read-only cached scene scan: all original numeric H entries,
 * no guessed offsets, no silently skipped unknown object types. This is a
 * diagnostic completeness EVIDENCE report, NOT renderable full drawing,
 * complete owner/layer graph or AutoCAD 2027 equivalence. Decodable entity
 * subset includes LINE/ARC/CIRCLE/ELLIPSE/POINT/LWPOLYLINE/ordered POLYLINE3D.
 * A supported-type payload rejected by its decoder is reported explicitly
 * as GEOMETRY_REJECTED, not treated as complete. Unknown types are opaque.
 * rows=NULL/capacity=0 performs a complete count-only scan.
 * All outputs fail-atomic on any fatal corrupt indexed record, alias, or
 * insufficient destination. Exact source file bytes need not remain live. */
int musa_dwg_ac1032_scene_index_scan_geometry(
    const MusaDwgAc1032SceneIndex *index,
    MusaDwgAc1032SceneGeometryRecord *rows,size_t capacity,
    MusaDwgAc1032SceneGeometryReport *out);
/* Original R2018 indexed raw OT inventory for the *currently UNDECODED*
 * TEXT, MTEXT, ATTRIB, ATTDEF, BLOCK, ENDBLK, INSERT and MINSERT families.
 * Source-only H/CRC type evidence, NOT parsed text/block geometry or an
 * exact visible-count: model-space, block graph, viewport are unproved.
 * Block definition markers are not in themselves drawn objects.
 * 'other_indexed_records' can contain unsupported 2B/3B/proxy classes.
 * Never infer a successful 2B render from a raw type count. */
typedef struct MusaDwgAc1032Unrendered2dInventory {
    size_t indexed_records;
    size_t text_records;          /* OT 1 */
    size_t mtext_records;         /* OT 44 */
    size_t attrib_records;        /* OT 2 */
    size_t attdef_records;        /* OT 3 */
    size_t block_begin_records;   /* OT 4 */
    size_t block_end_records;     /* OT 5 */
    size_t insert_records;        /* OT 7 */
    size_t minsert_records;       /* OT 8 */
    size_t other_indexed_records; /* every other raw type */
    uint8_t original_types_crc_verified;
    uint8_t complete_2d_display_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032Unrendered2dInventory;
/* Read-only, portable C11. One source-authenticated pass with strict
 * index ordering. All outputs remain unchanged if the request fails. */
int musa_dwg_ac1032_scene_index_unrendered_2d_inventory(
    const MusaDwgAc1032SceneIndex *index,
    MusaDwgAc1032Unrendered2dInventory *out);

#ifdef __cplusplus
}
#endif
#endif
