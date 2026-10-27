/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_SCENE_INDEX_H
#define MUSACAD_DWG_AC1032_SCENE_INDEX_H
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_line_file.h"
#include "musacad_dwg_ac1032_curve.h"
#include "musacad_dwg_ac1032_point.h"
#include "musacad_dwg_ac1032_text.h"
#include "musacad_dwg_ac1032_insert.h"
#include "musacad_dwg_ac1032_block_header_identity.h"
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
/* CRC/H authenticated OT49 identity only; base-point remains undecoded. */
int musa_dwg_ac1032_scene_index_read_block_header_prefix_evidence(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032BlockHeaderPrefixEvidence *out);
/* Extract verified OT49 DATA bits after the identity prefix, packed MSB-first.
 * This is opaque source evidence, not BLOCK_HEADER base-point decoding.
 * Unsupported handles/ranges leave caller bytes unchanged. */
int musa_dwg_ac1032_scene_index_copy_block_header_field_bits(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    size_t start_bit,size_t bit_count,uint8_t *out,size_t out_bytes);
/* Original CRC/H authenticated OT49 base XYZ and owned count. These
 * numeric fields are NOT proof of child H ownership, model space or paint. */
/* CRC/H-verified R2018 local BLOCK_HEADER H prefix only: BLOCK marker,
 * bounded candidate owned entity handles and ENDBLK. Full tail, owner
 * reverse links, visibility and paint are NOT accepted. */
int musa_dwg_ac1032_scene_index_block_header_child_prefix(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032BlockHeaderChildPrefix *out);
/* Typed original child H inventory, for future entity-specific decoders.
 * Each OT is read from its own CRC-authenticated indexed DWG record.
 * This does NOT prove reverse ownership, model-space visibility, XREF
 * overrides, nested INSERT transforms, or permission to paint. */
typedef struct MusaDwgAc1032BlockHeaderTypedChildren {
    MusaDwgAc1032BlockHeaderChildPrefix prefix;
    uint16_t child_object_types[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t all_child_record_types_crc_verified;
    uint8_t reverse_owner_links_verified; /* ALWAYS ZERO */
    uint8_t final_display_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockHeaderTypedChildren;
int musa_dwg_ac1032_scene_index_block_header_typed_children(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032BlockHeaderTypedChildren *out);
int musa_dwg_ac1032_scene_index_read_block_header_base(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032BlockHeaderBase *out);
int musa_dwg_ac1032_scene_index_read_block_header_field_span(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032BlockHeaderFieldSpan *out);
int musa_dwg_ac1032_scene_index_read_block_header_identity(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032BlockHeaderIdentity *out);
int musa_dwg_ac1032_scene_index_object_type(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    uint16_t *out_type);
/* CRC/H authenticated original R2018 OT7 INSERT common entity prefix, ENC
 * color and raw invisibility only. Numeric placement, BLOCK_HEADER H graph,
 * owner/model-space, viewport and final display remain UNVERIFIED. */
/* Authenticated original R2018 OT2 ATTRIB entity-common only. This is NOT
 * ATTRIB text, attachment to INSERT, BLOCK_HEADER ownership, geometry,
 * viewport visibility or rendering. Fail-atomic on unsupported source. */
/* Authenticated original R2018 OT3 ATTDEF entity-common only. No tag,
 * text value, default attribute, INSERT binding, owner or final paint. */
/* Authenticated original R2018 OT4 BLOCK begin entity-common only.
 * BLOCK_HEADER owner, child sequence, names, nested INSERT and display
 * remain UNVERIFIED; BLOCK marker itself is never painted. */
/* Authenticated original R2018 OT5 ENDBLK entity-common only.
 * ENDBLK is a nonpaintable marker; BLOCK_HEADER ownership, sequence and
 * final visibility remain UNVERIFIED. */
int musa_dwg_ac1032_scene_index_read_endblk_common(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032EntityCommon *out);
int musa_dwg_ac1032_scene_index_read_block_common(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032EntityCommon *out);
int musa_dwg_ac1032_scene_index_read_attdef_common(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032EntityCommon *out);
int musa_dwg_ac1032_scene_index_read_attrib_common(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032EntityCommon *out);
/* Original R2018 OT7 INSERT OCS numeric placement; no block expansion,
 * owner/model-space or final rendering acceptance. */
/* Authenticated original INSERT H edges only. No block expansion or paint. */
int musa_dwg_ac1032_scene_index_insert_source_graph(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032InsertSourceGraph *out);
/* Read-only OT7 INSERT -> OT49 BLOCK_HEADER child type inventory.
 * The INSERT H graph and OT49 child H prefix are independently CRC/H
 * authenticated; matching target handles does NOT prove reverse owner
 * links, nested transforms, XREF visibility or permission to render.
 * A failed source edge leaves the caller's output unchanged. */
typedef struct MusaDwgAc1032InsertBlockChildInventory {
    MusaDwgAc1032InsertSourceGraph insert;
    MusaDwgAc1032BlockHeaderTypedChildren block;
    uint8_t insert_to_block_source_edge_verified;
    uint8_t reverse_owner_and_nested_transforms_verified; /* ALWAYS 0 */
    uint8_t final_display_accepted; /* ALWAYS 0 */
} MusaDwgAc1032InsertBlockChildInventory;
static inline int musa_dwg_ac1032_scene_index_insert_block_child_inventory(
    const MusaDwgAc1032SceneIndex *index,uint64_t insert_handle,
    MusaDwgAc1032InsertBlockChildInventory *out){
    if(!index||!insert_handle||!out)return 0;
    MusaDwgAc1032InsertBlockChildInventory local={0};
    if(!musa_dwg_ac1032_scene_index_insert_source_graph(
            index,insert_handle,&local.insert)||
       !local.insert.original_entity_and_targets_crc_verified||
       !local.insert.source_edges_verified||
       local.insert.indexed_handle!=insert_handle||
       !local.insert.block_header_handle||
       local.insert.block_expansion_and_display_accepted||
       !musa_dwg_ac1032_scene_index_block_header_typed_children(
            index,local.insert.block_header_handle,&local.block)||
       local.block.prefix.indexed_handle!=local.insert.block_header_handle||
       !local.block.prefix.original_record_crc_verified||
       !local.block.prefix.h_prefix_decoded||
       !local.block.all_child_record_types_crc_verified||
       local.block.reverse_owner_links_verified||
       local.block.final_display_accepted)return 0;
    local.insert_to_block_source_edge_verified=1u;
    *out=local;
    return 1;
}

/* Genuine R2018 OT7 INSERT -> authenticated LAYER OT51 source evidence.
 * The INSERT H graph, Unicode layer name, raw numeric layer status and
 * entity common ENC are all read from the same immutable CRC/H scene.
 * Source evidence only: NOT block expansion, owner/model-space,
 * viewport/XREF overrides, final visibility, BYLAYER/BYBLOCK or paint.
 * Unsupported/missing evidence rejects without touching caller output. */
typedef struct MusaDwgAc1032InsertLayerProvenance {
    MusaDwgAc1032InsertSourceGraph graph;
    MusaDwgAc1032LayerName layer_name;
    MusaDwgAc1032LayerFlagsEvidence raw_layer_flags;
    MusaDwgAc1032EntityCommon insert_common;
    uint8_t source_layer_and_entity_verified;
    uint8_t final_visibility_color_and_paint_accepted; /* ALWAYS 0 */
} MusaDwgAc1032InsertLayerProvenance;
int musa_dwg_ac1032_scene_index_insert_layer_provenance(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032InsertLayerProvenance *out);
/* Narrow ORIGINAL R2018 INSERT -> CRC/H-authenticated LAYER OT51 CMC
 * evidence. Only the independently proven default-profile C3 ACI is
 * accepted. This is source ACI, NOT resolved BYLAYER/BYBLOCK, RGB, layer
 * visibility, block child paint or viewport/XREF override acceptance.
 * Failure is atomic; unsupported colors must never silently become white. */
typedef struct MusaDwgAc1032InsertLayerCmcSource {
    MusaDwgAc1032InsertLayerProvenance source;
    MusaDwgAc1032LayerCmcEvidence cmc;
    uint8_t source_aci_verified;
    uint8_t resolved_color_and_paint_accepted; /* ALWAYS 0 */
} MusaDwgAc1032InsertLayerCmcSource;
int musa_dwg_ac1032_scene_index_insert_layer_cmc_source(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032InsertLayerCmcSource *out);
int musa_dwg_ac1032_scene_index_read_insert_placement(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032InsertPlacement *out);
/* Join independently authenticated original OT7 placement, OT7 H
 * target, and OT49 base before producing a numeric 2D affine. This is
 * source geometry only: reverse owner, model-space, nested blocks,
 * layer visibility and final painting remain unverified. Fail-atomic. */
static inline int musa_dwg_ac1032_scene_index_original_insert_block_affine(
    const MusaDwgAc1032SceneIndex *index,uint64_t insert_handle,
    double out_six[6]){
    if(!index||!insert_handle||!out_six)return 0;
    MusaDwgAc1032InsertSourceGraph graph={0};
    MusaDwgAc1032InsertPlacement placement={0};
    MusaDwgAc1032BlockHeaderBase base={0};
    double local[6];
    if(!musa_dwg_ac1032_scene_index_insert_source_graph(
           index,insert_handle,&graph)||
       !graph.original_entity_and_targets_crc_verified||
       !graph.source_edges_verified||
       graph.indexed_handle!=insert_handle||
       !graph.block_header_handle||
       graph.block_expansion_and_display_accepted||
       !musa_dwg_ac1032_scene_index_read_insert_placement(
           index,insert_handle,&placement)||
       placement.indexed_handle!=insert_handle||
       !musa_dwg_ac1032_scene_index_read_block_header_base(
           index,graph.block_header_handle,&base)||
       base.indexed_handle!=graph.block_header_handle||
       !base.original_record_crc_verified||!base.base_point_decoded||
       base.block_children_and_display_accepted||
       base.is_xref||base.xref_overlaid||base.xref_dependent||
       base.xref_index_plus_one||
       !musa_dwg_ac1032_insert_planar_affine(
           &placement,base.base_xyz,local))return 0;
    for(unsigned i=0;i<6u;i++)out_six[i]=local[i];
    return 1;
}
int musa_dwg_ac1032_scene_index_read_insert_common(
    const MusaDwgAc1032SceneIndex *index,uint64_t indexed_handle,
    MusaDwgAc1032EntityCommon *out);
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

/* Narrow original R2018 geometry ENTITY H4 owner -> OT49 BLOCK_HEADER
 * and H5 layer -> OT51 LAYER proof. Unlike the no-owner entity-layer
 * reader, this admits ONLY entity_mode=0 and exactly those TWO source H
 * references plus <=7 zero padding bits. Optional handles, other modes
 * and unknown layouts fail closed. Every target is independently indexed
 * and CRC/type checked. A verified edge is a single-child source claim,
 * not complete BLOCK ownership, space/viewport or visible geometry. */
typedef struct MusaDwgAc1032EntityBlockOwnerLayerSource {
    uint64_t child_handle,owner_block_header_handle,layer_handle;
    uint16_t raw_entity_type;
    uint8_t original_child_record_crc_verified;
    uint8_t original_owner_and_layer_records_crc_verified;
    /* Historical name: verified exact owner + layer graph role. The
     * canonical original handle stream may encode OT18's owner as absolute
     * H4 or the independently verified H12 base-relative reference.
     * Explicitly retain its raw wire code instead of claiming both H4. */
    uint8_t exact_h4_owner_and_h5_layer_source_verified;
    uint8_t original_owner_wire_code; /* original 4 or OT18-only 12 */
    uint8_t original_relative_h12_owner_verified;
    uint8_t visible_scene_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032EntityBlockOwnerLayerSource;
int musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
    const MusaDwgAc1032SceneIndex *scene,uint64_t child_handle,
    MusaDwgAc1032EntityBlockOwnerLayerSource *out);
/* Extract transformed LINE endpoints from an authenticated INSERT's
 * OT49 child H list. SOURCE-ONLY: reverse owner and display are NOT
 * verified. Callers MUST NOT treat these as paint-authorized geometry.
 * Fail-atomic and bounded to the original OT49 64-child inventory. */
typedef struct MusaDwgAc1032InsertBlockLineSources {
    uint64_t insert_handle,block_header_handle;
    uint64_t child_handles[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    double endpoints[MUSACAD_AC1032_OT49_CHILD_LIMIT][4];
    uint32_t line_count;
    uint8_t original_source_geometry_verified;
    uint8_t reverse_owner_links_verified; /* ALWAYS ZERO */
    uint8_t final_display_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertBlockLineSources;
static inline int musa_dwg_ac1032_scene_index_insert_block_line_sources(
    const MusaDwgAc1032SceneIndex *index,uint64_t insert_handle,
    MusaDwgAc1032InsertBlockLineSources *out){
    if(!index||!insert_handle||!out)return 0;
    MusaDwgAc1032InsertBlockChildInventory inventory={0};
    double m[6]={0};
    if(!musa_dwg_ac1032_scene_index_insert_block_child_inventory(
            index,insert_handle,&inventory)||
       !musa_dwg_ac1032_scene_index_original_insert_block_affine(
            index,insert_handle,m)||
       inventory.block.prefix.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT||
       !inventory.insert_to_block_source_edge_verified||
       !inventory.block.all_child_record_types_crc_verified||
       inventory.reverse_owner_and_nested_transforms_verified||
       inventory.final_display_accepted)
        return 0;
    MusaDwgAc1032InsertBlockLineSources local={0};
    local.insert_handle=insert_handle;
    local.block_header_handle=inventory.insert.block_header_handle;
    for(uint32_t i=0;i<inventory.block.prefix.child_count;i++){
        if(inventory.block.child_object_types[i]!=19u)continue;
        MusaDwgAc1032NativeLine line={0};
        const uint64_t handle=inventory.block.prefix.child_handles[i];
        MusaDwgAc1032EntityBlockOwnerLayerSource owner={0};
        if(!handle||
           !musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
                index,handle,&owner)||
           owner.child_handle!=handle||
           owner.owner_block_header_handle!=local.block_header_handle||
           owner.raw_entity_type!=19u||
           !owner.original_child_record_crc_verified||
           !owner.original_owner_and_layer_records_crc_verified||
           !owner.exact_h4_owner_and_h5_layer_source_verified||
           owner.visible_scene_and_paint_accepted||
           !musa_dwg_ac1032_scene_index_read_line(
                index,handle,&line)||
           line.common.invisibility!=0u)return 0;
        for(unsigned j=0;j<3u;j++){
            if(!isfinite(line.geometry.start[j])||
               !isfinite(line.geometry.end[j]))return 0;
        }
        if(line.geometry.start[2]!=0.0||
           line.geometry.end[2]!=0.0||
           line.geometry.extrusion[0]!=0.0||
           line.geometry.extrusion[1]!=0.0||
           line.geometry.extrusion[2]!=1.0||
           !isfinite(line.geometry.thickness)||
           line.geometry.thickness!=0.0)return 0;
        const double x=line.geometry.start[0],y=line.geometry.start[1];
        const double u=line.geometry.end[0],v=line.geometry.end[1];
        double xy[4]={m[0]*x+m[1]*y+m[2],
                      m[3]*x+m[4]*y+m[5],
                      m[0]*u+m[1]*v+m[2],
                      m[3]*u+m[4]*v+m[5]};
        for(unsigned k=0;k<4u;k++)if(!isfinite(xy[k]))return 0;
        const uint32_t at=local.line_count++;
        local.child_handles[at]=handle;
        for(unsigned k=0;k<4u;k++)local.endpoints[at][k]=xy[k];
    }
    local.original_source_geometry_verified=1u;
    *out=local;
    return 1;
}



/* R2018 OT18 original CRC/H forensic probe. first_raw_target and
 * second_raw_target are untrusted H references, NOT certified owner/layer.
 * MSB prefix is copied from the authenticated original HANDLE bitstream.
 * No diagnostic status (even EXACT_EDGE) certifies model-space, XREF,
 * frozen/off/viewport state, BYLAYER/BYBLOCK or final drawing. */
#define MUSACAD_AC1032_CIRCLE_H_COMMON_UNSUPPORTED 1u
#define MUSACAD_AC1032_CIRCLE_H_FIRST_UNREADABLE 2u
#define MUSACAD_AC1032_CIRCLE_H_FIRST_NOT_H4 3u
#define MUSACAD_AC1032_CIRCLE_H_SECOND_UNREADABLE 4u
#define MUSACAD_AC1032_CIRCLE_H_SECOND_NOT_H5 5u
#define MUSACAD_AC1032_CIRCLE_H_TAIL_NONCANONICAL 6u
#define MUSACAD_AC1032_CIRCLE_H_TARGET_TYPES_REJECTED 7u
#define MUSACAD_AC1032_CIRCLE_H_GRAPH_REJECTED 8u
#define MUSACAD_AC1032_CIRCLE_H_EXACT_EDGE 9u
#define MUSACAD_AC1032_CIRCLE_H_EXACT_RELATIVE_H12_EDGE 10u
typedef struct MusaDwgAc1032CircleHTailProfile {
    uint64_t indexed_handle,handle_start_bit,handle_end_bit;
    uint64_t first_raw_target,second_raw_target;
    uint64_t residual_bits_after_two_refs;
    uint32_t reactor_count;
    uint16_t raw_object_type;
    uint8_t prefix_msb[8],prefix_bit_count;
    uint8_t entity_mode,xdictionary_missing,has_color_reference;
    uint8_t linetype_flags,plotstyle_flags,material_flags,visualstyle_flags;
    uint8_t first_ref_decoded,first_ref_code,first_ref_count;
    uint8_t second_ref_decoded,second_ref_code,second_ref_count;
    uint8_t profile_status,original_record_crc_verified;
    uint8_t exact_h4_h5_source_verified;
    uint8_t verified_relative_h12_owner_h5_layer_source;
    uint8_t final_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032CircleHTailProfile;
int musa_dwg_ac1032_scene_index_circle_h_tail_profile(
    const MusaDwgAc1032SceneIndex *scene,uint64_t circle_handle,
    MusaDwgAc1032CircleHTailProfile *out);


/* Narrow source-authenticated R2018 OT18 H(C relative owner) + H5 layer.
 * Both referenced original OT49/OT51 records are independently CRC/OT
 * checked and the OT49 forward child list must contain the same OT18 H.
 * This does NOT prove complete block ownership, scene/viewport visibility,
 * layer flags, resolved BYLAYER/BYBLOCK color or any final drawing. */
typedef struct MusaDwgAc1032CircleRelativeOwnerLayerSource {
    uint64_t circle_handle,owner_block_header_handle,layer_handle;
    uint16_t raw_circle_object_type;
    uint8_t first_source_handle_code,second_source_handle_code;
    uint8_t original_circle_and_targets_crc_verified;
    uint8_t original_forward_block_child_membership_verified;
    uint8_t exact_relative_owner_and_layer_source_verified;
    uint8_t model_space_viewport_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032CircleRelativeOwnerLayerSource;
int musa_dwg_ac1032_scene_index_circle_relative_owner_layer_source(
    const MusaDwgAc1032SceneIndex *scene,uint64_t circle_handle,
    MusaDwgAc1032CircleRelativeOwnerLayerSource *out);

/* Original OT18 CIRCLE -> authenticated relative owner H(C) + OT51 H5
 * LAYER -> Unicode layer name, raw layer numeric flags and source ENC.
 * Geometry is verified from the SAME immutable CRC/H-indexed R2018 source.
 * Raw flags, BYLAYER/BYBLOCK, off/frozen, viewport, XREF, model-space
 * ownership and final color/visibility are NOT resolved by this evidence.
 * Fail atomic: no caller output is modified for an unsupported source. */
typedef struct MusaDwgAc1032CircleLayerProvenance {
    MusaDwgAc1032CircleRelativeOwnerLayerSource edge;
    MusaDwgAc1032LayerName layer_name;
    MusaDwgAc1032LayerFlagsEvidence raw_layer_flags;
    MusaDwgAc1032NativeCurve circle;
    uint8_t source_geometry_and_layer_verified;
    uint8_t final_visibility_color_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032CircleLayerProvenance;
int musa_dwg_ac1032_scene_index_circle_layer_provenance(
    const MusaDwgAc1032SceneIndex *scene,uint64_t circle_handle,
    MusaDwgAc1032CircleLayerProvenance *out);


/* Original R2018 OT18 CIRCLE H(C relative owner) -> H5 OT51 Unicode
 * LAYER -> strictly anchored numeric CMC source ACI. A successful
 * source ACI is NOT a resolved BYLAYER display color: off/frozen, block
 * ancestry, model/paper, viewport/XREF and overrides remain unknown.
 * Missing or unsupported original source rejects atomically. */
typedef struct MusaDwgAc1032CircleLayerCmcSource {
    MusaDwgAc1032CircleLayerProvenance source;
    MusaDwgAc1032LayerCmcEvidence layer_cmc;
    uint8_t original_layer_cmc_aci_verified;
    uint8_t final_visible_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032CircleLayerCmcSource;
int musa_dwg_ac1032_scene_index_circle_layer_cmc_source(
    const MusaDwgAc1032SceneIndex *scene,uint64_t circle_handle,
    MusaDwgAc1032CircleLayerCmcSource *out);

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
