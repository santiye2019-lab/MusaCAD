/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1018_LWPOLYLINE_H
#define MUSACAD_DWG_AC1018_LWPOLYLINE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* R2004 LWPOLYLINE coordinates are OCS X/Y (NOT WCS). 3D placement also
 * requires drawing's elevation and extrusion. No entity owner graph here. */
typedef struct MusaDwgAc1018LWVertex {
    double x, y;              /* OCS */
    double bulge;             /* Segment starting at this vertex */
    double start_width, end_width;
} MusaDwgAc1018LWVertex;
typedef struct MusaDwgAc1018LWPolyline {
    uint64_t handle;
    uint32_t vertex_count;
    uint32_t bulge_count;
    uint32_t width_count;
    uint32_t eed_raw_bytes_skipped; /* opaque, not interpreted or preserved */
    uint16_t flags;
    uint16_t color;
    double constant_width;
    double elevation;
    double thickness;
    double extrusion[3];
    double linetype_scale;
    uint16_t invisibility;
    uint8_t entity_mode;
    uint8_t xdictionary_missing;
    uint8_t linetype_flags;
    uint8_t plotstyle_flags;
    uint8_t lineweight;
    uint8_t closed;
} MusaDwgAc1018LWPolyline;
/* Caller gives separately verified uncompressed AcDbObjects section and
 * CRC-backed indexed handle/offset. Bounded 1..100000 vertices. On failure
 * leaves all outputs unchanged. Rejects reactors and unknown fields; never
 * silently emits a geometry-only result as lossless DWG scene or metadata.
 * If capacity < decoded count, returns 0 with outputs unchanged.
 * Source data is NEVER modified. No runtime external CAD decoder. */
int musa_dwg_ac1018_read_lwpolyline_record(const uint8_t *object_section,
    size_t section_bytes, size_t object_offset, uint64_t indexed_handle,
    MusaDwgAc1018LWVertex *out_vertices, size_t capacity,
    MusaDwgAc1018LWPolyline *out);
#ifdef __cplusplus
}
#endif
#endif
