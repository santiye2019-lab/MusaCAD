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
#ifdef __cplusplus
}
#endif
#endif
