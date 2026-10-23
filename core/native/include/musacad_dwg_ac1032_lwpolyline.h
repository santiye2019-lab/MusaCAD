/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_LWPOLYLINE_H
#define MUSACAD_DWG_AC1032_LWPOLYLINE_H
#include "musacad_dwg_ac1032_entity_prefix.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Native original AC1032 LWPOLYLINE; vertices in entity OCS.
 * One first-party common C11 core for Android, Windows, and iOS.
 * ARC bulge belongs to the segment beginning at each vertex.
 */
typedef struct MusaDwgAc1032LWVertex {
    double x,y,bulge,start_width,end_width;
} MusaDwgAc1032LWVertex;
typedef struct MusaDwgAc1032NativeLWPolyline {
    MusaDwgAc1032EntityCommon common;
    uint64_t indexed_handle;
    uint64_t geometry_end_bit;
    uint32_t vertex_count,bulge_count,width_count;
    uint16_t flags;
    double constant_width,elevation,thickness,extrusion[3];
    uint8_t closed,geometry_decoded;
} MusaDwgAc1032NativeLWPolyline;
/* Bounded authenticated original DATA fields from separately validated
 * source-derived ENTITY common start bit, not a scanned offset.
 * Outputs and caller source remain entirely unchanged on any failure.
 * No truncated emission, unsupported flag acceptance, or hidden allocation.
 */
int musa_dwg_ac1032_lwpolyline_fields_at(
    const uint8_t *payload,size_t payload_bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geometry_start_bit,
    MusaDwgAc1032LWVertex *vertices,size_t capacity,
    MusaDwgAc1032NativeLWPolyline *out);
/* Complete original DWG + indexed H + physical CRC and record proof.
 * Requires enough capacity for every vertex; fail-atomic on mismatch.
 * A drawing-level scene or multi-object handle graph is NOT claimed.
 */
int musa_dwg_ac1032_read_lwpolyline_file(
    const uint8_t *file,size_t file_bytes,uint64_t indexed_handle,
    MusaDwgAc1032LWVertex *vertices,size_t capacity,
    MusaDwgAc1032NativeLWPolyline *out);
#ifdef __cplusplus
}
#endif
#endif
