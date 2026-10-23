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
/* Extended planar original-CAD subset: also ARC/CIRCLE, with exact
 * source center, radius, +Z normal, and CCW angular sweep. V6 CURVE:
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
#ifdef __cplusplus
}
#endif
#endif
