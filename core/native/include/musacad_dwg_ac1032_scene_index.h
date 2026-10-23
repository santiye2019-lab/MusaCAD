/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_SCENE_INDEX_H
#define MUSACAD_DWG_AC1032_SCENE_INDEX_H
#include "musacad_dwg_ac1032_handles.h"
#include "musacad_dwg_ac1032_line_file.h"
#include "musacad_dwg_ac1032_curve.h"
#include "musacad_dwg_ac1032_point.h"
#include "musacad_dwg_ac1032_lwpolyline.h"
#include "musacad_dwg_ac1032_polyline3d.h"
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
 * subset includes LINE/ARC/CIRCLE/POINT/LWPOLYLINE/ordered POLYLINE3D.
 * A supported-type payload rejected by its decoder is reported explicitly
 * as GEOMETRY_REJECTED, not treated as complete. Unknown types are opaque.
 * rows=NULL/capacity=0 performs a complete count-only scan.
 * All outputs fail-atomic on any fatal corrupt indexed record, alias, or
 * insufficient destination. Exact source file bytes need not remain live. */
int musa_dwg_ac1032_scene_index_scan_geometry(
    const MusaDwgAc1032SceneIndex *index,
    MusaDwgAc1032SceneGeometryRecord *rows,size_t capacity,
    MusaDwgAc1032SceneGeometryReport *out);
#ifdef __cplusplus
}
#endif
#endif
