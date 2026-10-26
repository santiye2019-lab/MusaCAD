/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_PREVIEW_V6_H
#define MUSACAD_DWG_AC1032_PREVIEW_V6_H
#include "musacad_dwg_ac1032_scene_index.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* A FIRST real source-derived bridge to existing Android-compatible scene v6
 * float words. NOT connected to first-paint JNI yet, and NOT a complete DWG
 * scene. One independently cached original binary AC1032 H index only.
 *
 * v6 float words: [6, primitiveCount, minX,minY,maxX,maxY,truncated]
 * LINE: [263, x1,y1,x2,y2]    POINT: [775, x,y]
 * (ACI=7 is an explicit provisional style, NOT original layer/ByBlock color).
 *
 * Only accepts source-authenticated planar WCS LINE/POINT with finite,
 * representable float XY, zero Z/thickness, +Z extrusion. Other indexed
 * records are OMITTED AND COUNTED, never silently promoted to a complete
 * drawing: v6 truncated=1 whenever any record is not rendered.
 * Duplicate geometry is never inferred from numeric H offsets.
 */
typedef struct MusaAc1032PreviewV6Stats {
    size_t total_indexed_records;
    size_t rendered_primitives;
    size_t omitted_records;
    size_t required_float_words;
    uint8_t partial_geometry;
    uint8_t provisional_aci_style;
} MusaAc1032PreviewV6Stats;
/* Count-only: words=NULL and capacity=0. Output is fail-atomic for invalid
 * buffer, insufficient capacity, aliased output/metadata or corrupt H/OT/CRC.
 * No original file bytes required after scene_index_open().
 * Shared native C11 ABI: Android, Windows, iOS. */
int musa_dwg_ac1032_preview_v6_line_point(
    const MusaDwgAc1032SceneIndex *scene,
    float *words,size_t capacity,MusaAc1032PreviewV6Stats *out);
/* Extended planar original-CAD subset: also ARC/CIRCLE/ELLIPSE, with exact
 * source center, circular radius or ellipse WCS axes, +Z normal, and\n * CCW angular sweep. V6 CURVE:
 * [1031,cx,cy,rx,0,0,ry,startRadians,sweepRadians].
 * Retains incomplete indicator for unsupported/proxy/layer classes.
 * Does NOT claim full model-space visibility or correct layer colors. */
int musa_dwg_ac1032_preview_v6_planar_curves(
    const MusaDwgAc1032SceneIndex *scene,
    float *words,size_t capacity,MusaAc1032PreviewV6Stats *out);
/* Source-authenticated planar original R2018 LWPOLYLINE preview:
 * exact straight segments and signed bulge circular arcs, including final
 * closed edge only when source CLOSED. Also admits genuine OT16 3D POLYLINE
 * ONLY with verified ordered/reverse source owner graph, SEQEND and strictly
 * planar Z=0 nonspline OT11 vertices. Never flatten real nonplanar/mesh paths.
 * Reject all widths, thickness, elevation
 * and unsupported OCS planes rather than drawing a visibly wrong line.
 * Version 6 POLY for all-straight paths, or LINE/CURVE segments if any bulge.
 * Source-indexed original H/CRC, always explicitly partial while unsupported
 * object/layer classes remain; NOT connected to Android JNI first paint. */
int musa_dwg_ac1032_preview_v6_planar_polylines(
    const MusaDwgAc1032SceneIndex *scene,
    float *words,size_t capacity,MusaAc1032PreviewV6Stats *out);
/* Opt-in source-assigned ACI / direct RGB path on original H/CRC.
 * ACI 1..255 uses v6 code (kind<<8)|ACI.
 * Source DIRECT 24-bit RGB (no color-handle/reference or transparency)
 * uses negative v6 code -(kind*256+2), followed by one exact float word
 * holding RGB 0..0xFFFFFF. 32-bit ARGB MUST NOT be stored in float.
 * BYLAYER/BYBLOCK, indexed color references, alpha/transparency
 * and unsupported styles remain PROVISIONAL ACI7 and mark partial.
 * Invisible entities are not emitted in this mode, preserving caution.
 * Does NOT resolve layer table visibility/style, block insert ancestry
 * or actual Android full drawing appearance. Original APIs unchanged. */
int musa_dwg_ac1032_preview_v6_planar_source_aci(
    const MusaDwgAc1032SceneIndex *scene,float *words,
    size_t capacity,MusaAc1032PreviewV6Stats *out);
/* Conservative first-paint MTEXT formatting admission. Accepts only a
 * valid single-line Unicode UTF-8 string without MTEXT formatting syntax,
 * braces, backslash escapes, raw controls or paragraph separators. This
 * does NOT decode MTEXT paragraph/font/rich-text syntax; formatted source
 * is omitted rather than displayed as misleading literal escape codes.
 * Pure C11, no external DWG decoder. The input must have bytes+1 readable
 * bytes, with NUL exactly at utf8[bytes]. */
int musa_dwg_ac1032_mtext_v6_plain_source_preflight(
    const char *utf8,size_t bytes);
/* Experimental source-authenticated R2018 MTEXT (OT44) optional visual
 * bridge: original UTF-8 content and planar position/axes/text size are
 * transported as native v6 type 6 glyph proxy. Android uses generic font
 * and a single line: paragraphs, font metrics, line wrapping, layer
 * visibility, owner graph and final colors are NOT equivalent to AutoCAD.
 * Always explicitly partial; no guarantee of exact glyph placement.
 * Source-only exact direct ACI/RGB24 may be transported when both original
 * entity common ENC and its OT51 layer H graph authenticate. Source-invisible
 * MTEXT is omitted. BYLAYER/BYBLOCK/alpha/color-reference remain provisional
 * ACI7, NEVER inferred from LAYER0 or owner. Font and viewport remain unknown.
 * Legacy preview functions remain unaffected. */
int musa_dwg_ac1032_preview_v6_planar_source_with_mtext(
    const MusaDwgAc1032SceneIndex *scene,float *words,
    size_t capacity,MusaAc1032PreviewV6Stats *out);

/* Verified AC1032 source-record accounting for the actual v6 native
 * read-only preview admission path, NOT a percentage of full AutoCAD 2D
 * equivalence and NOT final visible-entity/viewport/model-space coverage.
 * A bulged LWPOLYLINE can emit MANY primitives but remains ONE source
 * record. Other indexed records include tables, dependent objects,
 * and unknown/unsupported types, NOT necessarily missing 2D geometry.
 *
 * Counts originate from the SAME CRC/H-validated source record scan as
 * the preview, without assuming that a matching object type is displayable.
 * All caller-owned output remains unchanged on error. */
typedef struct MusaAc1032PreviewV6SourceCensus {
    size_t total_indexed_records;
    size_t candidate_source_records;
    size_t packet_admitted_source_records;
    size_t candidate_not_admitted_records;
    size_t other_indexed_records;
    size_t emitted_v6_primitives;
    size_t omitted_v6_records;
    uint8_t complete_dwg_display_accepted; /* ALWAYS 0 */
} MusaAc1032PreviewV6SourceCensus;
int musa_dwg_ac1032_preview_v6_source_census(
    const MusaDwgAc1032SceneIndex *scene,
    MusaAc1032PreviewV6SourceCensus *out);
/* Same exact optional MTEXT-admitting source census, preserving the
 * existing source_census API's proven no-MTEXT contract. */
int musa_dwg_ac1032_preview_v6_source_census_with_mtext(
    const MusaDwgAc1032SceneIndex *scene,
    MusaAc1032PreviewV6SourceCensus *out);


/* Exact arc and signed-bulge XY bounds, with source-only provenance.
 * No layer/style/viewport semantic acceptance. Invalid inputs fail-atomic. */
int musa_dwg_ac1032_preview_v6_arc_bounds(
    double cx,double cy,double radius,double start,double sweep,
    double out_xyxy[4]);
/* Portable pure selector for an already independently source-authenticated
 * common entity; returns exact=1 for direct ACI, exact=0 for provisional
 * fallback. Returns failure without touching outputs for invalid requests.
 * 1 LINE, 2 POLYLINE, 3 POINT, 4 CURVE v6 kinds only. */
int musa_dwg_ac1032_preview_v6_entity_aci_code(
    const MusaDwgAc1032EntityCommon *source,
    uint8_t v6_kind,float *code,uint8_t *exact);
/* A pure portable C11 source-style selector. Assumes caller supplied
 * independently authenticated common entity bytes (including H/CRC).
 * extra=1 means negative kind code and one extra exact 24-bit RGB float;
 * extra=0 means positive ACI (direct or provisional 7). 'exact' is exact
 * source-assigned color only, NOT final visibility/color inheritance.
 * Supports v6 kinds 1 LINE, 2 POLY, 3 POINT, 4 CURVE and 6 TEXT.
 * This is a source-style selector, not proof of entity visibility or a
 * genuine direct-RGB MTEXT fixture; owner/layer/viewport remain unresolved.
 * All four outputs fail-atomic on invalid/overlapping arguments. */
int musa_dwg_ac1032_preview_v6_entity_source_style_code(
    const MusaDwgAc1032EntityCommon *source,uint8_t v6_kind,
    float *code,float *rgb,uint8_t *extra,uint8_t *exact);

/* New opt-in source-authenticated OT1 TEXT + OT44 MTEXT v6 diagnostic.
 * OT1 is a distinct TU decoder; unknown owner/viewport/font remain partial.
 * Older no-TEXT and MTEXT-only APIs retain their original semantics. */
int musa_dwg_ac1032_preview_v6_planar_source_with_text(
    const MusaDwgAc1032SceneIndex *scene,float *words,
    size_t capacity,MusaAc1032PreviewV6Stats *out);
int musa_dwg_ac1032_preview_v6_source_census_with_text(
    const MusaDwgAc1032SceneIndex *scene,
    MusaAc1032PreviewV6SourceCensus *out);
#ifdef __cplusplus
}
#endif
#endif
