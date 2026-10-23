/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_POLYLINE3D_H
#define MUSACAD_DWG_AC1032_POLYLINE3D_H
#include "musacad_dwg_ac1032_entity_prefix.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* The original R2018 3D POLYLINE (fixed OT=16) has its own header, NOT
 * LWPOLYLINE inline XY vertices. R2004+ owner references to independent
 * VERTEX3D objects (OT=11) are in the HANDLES stream. These types do NOT
 * claim resolved ownership, vertex order, or a complete scene.
 */
typedef struct MusaDwgAc1032Polyline3DHeader {
    MusaDwgAc1032EntityCommon common;
    uint64_t indexed_handle,geometry_end_bit;
    uint32_t owned_object_count;
    uint8_t spline_flags,polyline_flags,closed,header_decoded;
} MusaDwgAc1032Polyline3DHeader;
typedef struct MusaDwgAc1032Vertex3D {
    MusaDwgAc1032EntityCommon common;
    uint64_t indexed_handle,geometry_end_bit;
    double point[3];
    uint8_t flags,coordinates_decoded;
} MusaDwgAc1032Vertex3D;
/* These strict DATA-only field primitives need separately authenticated
 * common ENTITY offsets, reject aliases and keep output fail-atomic. */
int musa_dwg_ac1032_polyline3d_header_fields_at(
    const uint8_t *payload,size_t payload_bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geometry_start_bit,
    MusaDwgAc1032Polyline3DHeader *out);
int musa_dwg_ac1032_vertex3d_fields_at(
    const uint8_t *payload,size_t payload_bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geometry_start_bit,
    MusaDwgAc1032Vertex3D *out);
/* Genuine original file + index / page / original record CRC; no guessed
 * bytes and no LibreDWG/ODA runtime. Same portable C11 core on all OSes. */
int musa_dwg_ac1032_read_polyline3d_header_file(
    const uint8_t *file,size_t file_bytes,uint64_t indexed_handle,
    MusaDwgAc1032Polyline3DHeader *out);
int musa_dwg_ac1032_read_vertex3d_file(
    const uint8_t *file,size_t file_bytes,uint64_t indexed_handle,
    MusaDwgAc1032Vertex3D *out);
/* The actual R2004+ graph is encoded in an ordered HARD-OWNER H list
 * AFTER common ENTITY handle attributes, followed by a SEQEND owner.
 * Never infer child order by numerical H, file offset, or XYZ distance.
 * Each returned child is checked against its reverse owner reference.
 */
typedef struct MusaDwgAc1032Polyline3DOrderedVertex {
    uint64_t indexed_handle;
    double point[3];
    uint8_t flags;
} MusaDwgAc1032Polyline3DOrderedVertex;
typedef struct MusaDwgAc1032Polyline3DOrdered {
    MusaDwgAc1032Polyline3DHeader header;
    uint64_t layer_handle;
    uint64_t seqend_handle;
    uint32_t vertex_count;
    uint8_t hard_owner_links_verified;
    uint8_t reverse_vertex_owners_verified;
    uint8_t seqend_type_verified;
} MusaDwgAc1032Polyline3DOrdered;
/* Decodes only the supported plain common R2018 entity handle-tail.
 * A source-verification-backed layout is required; all references are
 * bounded to HANDLES, never DATA. Returned original order is authoritative.
 * This primitive cannot alone verify the child objects or reverse owners.
 */
int musa_dwg_ac1032_polyline3d_handle_fields_at(
    const uint8_t *payload,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    const MusaDwgAc1032Polyline3DHeader *header,
    uint64_t *ordered_handles,size_t capacity,
    uint64_t *layer_handle,uint64_t *seqend_handle);
/* Full original indexed AC1032 file: return ordered real XYZ of every
 * referenced OT11 VERTEX, verify each reverse OT11->OT16 owner link
 * and exact indexed OT6 SEQEND. Source and outputs unchanged on failure;
 * no speculative or partial 3D polyline emission. */
int musa_dwg_ac1032_read_polyline3d_vertices_file(
    const uint8_t *file,size_t file_bytes,uint64_t polyline_handle,
    MusaDwgAc1032Polyline3DOrderedVertex *vertices,size_t capacity,
    MusaDwgAc1032Polyline3DOrdered *out);

#ifdef __cplusplus
}
#endif
#endif
