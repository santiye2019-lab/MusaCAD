/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_CIRCLE_SOURCE_ADAPTIVE_POLYLINE_H
#define MUSACAD_DWG_AC1032_CIRCLE_SOURCE_ADAPTIVE_POLYLINE_H
#include "musacad_dwg_ac1032_circle_source_layer_spatial_index.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>

/* First-party C11 adaptive XY geometry for one CRC/H-authenticated original
 * R2018 INSERT/CIRCLE source instance. Source ACI is PROVENANCE ONLY:
 * owner space, layer visibility, viewport, XREF and nested INSERT remain
 * unresolved. No returned vertex grants final-paint permission. */
#define MUSA_AC1032_CIRCLE_ADAPTIVE_MAX_SEGMENTS 1024u
typedef struct MusaDwgAc1032CircleAdaptiveSourcePolyline {
    uint64_t insert_handle, circle_handle, layer_handle;
    double xy[MUSA_AC1032_CIRCLE_ADAPTIVE_MAX_SEGMENTS+1u][2];
    double min_xy[2], max_xy[2];
    double max_requested_xy_chord_error;
    double conservative_xy_chord_error_bound;
    size_t segment_count;
    uint32_t unresolved_original_dwg_requirements;
    uint16_t original_layer_packed_values;
    uint8_t original_layer_aci;
    uint8_t source_only;
    uint8_t final_visible_paint_accepted;
} MusaDwgAc1032CircleAdaptiveSourcePolyline;

/* Failure is atomic, including impossible precision at the 1024 cap.
 * Segment count is doubled until an affine-invariant upper bound on the
 * curve-to-chord deviation fits the requested WORLD-XY tolerance.
 * For affine axes U,V, ||U||+||V|| bounds ||U cos(t)+V sin(t)||.
 * Thus R*(1-cos(pi/N)) bounds the chord error of every segment.
 * This is an intentionally conservative bound, not pixel-space accuracy. */
static inline int musa_dwg_ac1032_circle_adaptive_source_polyline(
    const MusaDwgAc1032CircleByLayerInventory *inventory,
    size_t index, double max_xy_chord_error,
    MusaDwgAc1032CircleAdaptiveSourcePolyline *out) {
    if (!inventory || !out || !isfinite(max_xy_chord_error) ||
        max_xy_chord_error <= 0.0) return 0;
    MusaDwgAc1032CircleSourceEnvelope e={0};
    if (!musa_dwg_ac1032_circle_source_envelope(inventory,index,&e) ||
        !e.source_crc_wcs_and_layer_aci_verified ||
        e.final_visible_paint_accepted) return 0;
    const MusaDwgAc1032CircleByLayerInventoryRow *row=&inventory->rows[index];
    const double u=hypot(row->world_axis_u[0],row->world_axis_u[1]);
    const double v=hypot(row->world_axis_v[0],row->world_axis_v[1]);
    const double r=u+v;
    if (!isfinite(u)||!isfinite(v)||!isfinite(r)||r<=0.0) return 0;
    const double pi=3.14159265358979323846264338327950288;
    size_t n=8u;
    double bound=0.0;
    for (;;) {
        bound=r*(1.0-cos(pi/(double)n));
        if (!isfinite(bound)) return 0;
        /* Outward rounding for the computed conservative estimate. */
        bound=nextafter(bound,INFINITY);
        if (bound<=max_xy_chord_error) break;
        if (n>=MUSA_AC1032_CIRCLE_ADAPTIVE_MAX_SEGMENTS) return 0;
        n*=2u;
    }
    MusaDwgAc1032CircleAdaptiveSourcePolyline result={0};
    result.insert_handle=e.insert_handle;
    result.circle_handle=e.circle_handle;
    result.layer_handle=e.layer_handle;
    result.original_layer_aci=e.original_layer_aci;
    result.original_layer_packed_values=e.original_layer_packed_values;
    result.unresolved_original_dwg_requirements=
        e.unresolved_original_dwg_requirements;
    result.segment_count=n;
    result.max_requested_xy_chord_error=max_xy_chord_error;
    result.conservative_xy_chord_error_bound=bound;
    for (unsigned k=0u;k<2u;k++) {
        result.min_xy[k]=e.min_xy[k];
        result.max_xy[k]=e.max_xy[k];
    }
    for (size_t i=0u;i<n;i++) {
        const double a=2.0*pi*(double)i/(double)n;
        const double c=cos(a),s=sin(a);
        for (unsigned k=0u;k<2u;k++) {
            const double p=row->world_center[k]+
                row->world_axis_u[k]*c+row->world_axis_v[k]*s;
            if (!isfinite(p)||p<e.min_xy[k]||p>e.max_xy[k])return 0;
            result.xy[i][k]=p;
        }
    }
    /* Exact bitwise closure; do not introduce a trigonometric seam. */
    result.xy[n][0]=result.xy[0][0];
    result.xy[n][1]=result.xy[0][1];
    result.source_only=1u;
    *out=result;
    return 1;
}
#endif
