/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_CIRCLE_ADAPTIVE_DIAGNOSTIC_V6_H
#define MUSACAD_DWG_AC1032_CIRCLE_ADAPTIVE_DIAGNOSTIC_V6_H
#include "musacad_dwg_ac1032_circle_source_adaptive_polyline.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* One source-verified R2018 INSERT/CIRCLE in a read-only partial v6 scene.
 * ACI6 in the packet is DIAGNOSTIC, never original or resolved layer color.
 * Genuine source ACI is retained only in the separate evidence structure.
 * No model-space, visibility, viewport, XREF or final paint claim. */
#define MUSA_AC1032_CIRCLE_ADAPTIVE_V6_MAX_WORDS \
    (7u+2u+2u*(MUSA_AC1032_CIRCLE_ADAPTIVE_MAX_SEGMENTS+1u))
#define MUSA_AC1032_CIRCLE_ADAPTIVE_DIAGNOSTIC_ACI 6u
typedef struct MusaDwgAc1032CircleAdaptiveV6Evidence {
    uint64_t insert_handle,circle_handle,layer_handle;
    size_t segment_count,float_word_count;
    double requested_world_xy_error,conservative_world_xy_error_bound;
    double max_float32_vertex_displacement,combined_world_xy_error_bound;
    uint32_t unresolved_original_dwg_requirements;
    uint16_t original_layer_packed_values;
    uint8_t original_layer_aci;
    uint8_t diagnostic_packet_aci;
    uint8_t source_only;
    uint8_t original_color_or_final_paint_accepted;
} MusaDwgAc1032CircleAdaptiveV6Evidence;

/* Fail-atomic output for all bad capacities, non-finite/overflowed float
 * coordinates, unsupported precision or unverified source geometry. */
static inline int musa_dwg_ac1032_circle_adaptive_diagnostic_v6(
    const MusaDwgAc1032CircleByLayerInventory *inventory,
    size_t index,double max_world_xy_error,
    float *out,size_t capacity,
    MusaDwgAc1032CircleAdaptiveV6Evidence *evidence){
    if(!inventory||!out||!evidence||
       capacity>MUSA_AC1032_CIRCLE_ADAPTIVE_V6_MAX_WORDS)return 0;
    const uintptr_t a=(uintptr_t)out,b=(uintptr_t)evidence;
    const size_t bytes=capacity*sizeof(float),meta=sizeof(*evidence);
    if((a<=b&&b-a<bytes)||(b<a&&a-b<meta))return 0;
    MusaDwgAc1032CircleAdaptiveSourcePolyline poly={0};
    if(!musa_dwg_ac1032_circle_adaptive_source_polyline(
           inventory,index,max_world_xy_error,&poly)||
       !poly.source_only||poly.final_visible_paint_accepted||
       poly.segment_count>MUSA_AC1032_CIRCLE_ADAPTIVE_MAX_SEGMENTS||
       !poly.segment_count)return 0;
    const size_t need=7u+2u+2u*(poly.segment_count+1u);
    if(need>capacity)return 0;
    float v[MUSA_AC1032_CIRCLE_ADAPTIVE_V6_MAX_WORDS]={0};
    v[0]=6.0f;v[1]=1.0f;v[6]=1.0f;
    for(unsigned k=0u;k<2u;k++){
        const double lo=poly.min_xy[k],hi=poly.max_xy[k];
        float flo=(float)lo,fhi=(float)hi;
        if(!isfinite(flo)||!isfinite(fhi))return 0;
        if((double)flo>lo)flo=nextafterf(flo,-INFINITY);
        if((double)fhi<hi)fhi=nextafterf(fhi,INFINITY);
        if(!isfinite(flo)||!isfinite(fhi)||flo>fhi)return 0;
        v[2u+k]=flo;v[4u+k]=fhi;
    }
    v[7]=(float)(512u+MUSA_AC1032_CIRCLE_ADAPTIVE_DIAGNOSTIC_ACI);
    v[8]=(float)(poly.segment_count+1u);
    double max_float_displacement=0.0;
    for(size_t i=0u;i<=poly.segment_count;i++){
        for(unsigned k=0u;k<2u;k++){
            const float f=(float)poly.xy[i][k];
            if(!isfinite(f))return 0;
            v[9u+2u*i+k]=f;
        }
        /* The final vertex is transported as an exact float32 seam.
         * Apply that rounding BEFORE computing the error budget, so the
         * reported bound describes the actual emitted packet. */
        if(i==poly.segment_count){
            v[9u+2u*i]=v[9u];
            v[10u+2u*i]=v[10u];
        }
        const double dx=(double)v[9u+2u*i]-poly.xy[i][0];
        const double dy=(double)v[10u+2u*i]-poly.xy[i][1];
        const double displacement=hypot(dx,dy);
        if(!isfinite(displacement))return 0;
        if(displacement>max_float_displacement)
            max_float_displacement=displacement;
    }
    /* The float32 transport adds vertex displacement to the original
     * source curve-to-chord error. Reject if the sum exceeds the caller's
     * WORLD-XY budget; successful source tessellation alone is insufficient. */
    const double float_bound=nextafter(max_float_displacement,INFINITY);
    const double combined=nextafter(
        poly.conservative_xy_chord_error_bound+float_bound,INFINITY);
    if(!isfinite(combined)||combined>max_world_xy_error)return 0;
    /* Reject float32 quantization that erases the entire source curve.
     * A double-precision source circle can be nondegenerate yet collapse
     * into one float32 point at very large WCS offsets. Such a packet
     * must not masquerade as a successfully transported geometry. */
    int distinct_float_vertex=0;
    for(size_t i=1u;i<poly.segment_count;i++){
        if(v[9u+2u*i]!=v[9u]||v[10u+2u*i]!=v[10u]){
            distinct_float_vertex=1;
            break;
        }
    }
    if(!distinct_float_vertex)return 0;
    /* The diagnostic packet stores a closed polyline. Reject an invalid
     * vertex count before writing anything to caller-owned memory. */
    if(v[8]!=(float)(poly.segment_count+1u)||
       v[9u+2u*poly.segment_count]!=v[9u]||
       v[10u+2u*poly.segment_count]!=v[10u])return 0;
    /* The packet's bounds must enclose its ACTUAL transported float32
     * vertices, not merely the double-precision source envelope. A
     * rounded vertex outside the envelope must not be silently culled. */
    for(size_t i=0u;i<=poly.segment_count;i++){
        const float x=v[9u+2u*i],y=v[10u+2u*i];
        if(x<v[2]||x>v[4]||y<v[3]||y>v[5])return 0;
    }
    /* Exact seam was established before the error-budget computation. */
    MusaDwgAc1032CircleAdaptiveV6Evidence proof={0};
    proof.insert_handle=poly.insert_handle;
    proof.circle_handle=poly.circle_handle;
    proof.layer_handle=poly.layer_handle;
    proof.segment_count=poly.segment_count;
    proof.float_word_count=need;
    proof.requested_world_xy_error=poly.max_requested_xy_chord_error;
    proof.conservative_world_xy_error_bound=
        poly.conservative_xy_chord_error_bound;
    proof.max_float32_vertex_displacement=float_bound;
    proof.combined_world_xy_error_bound=combined;
    proof.unresolved_original_dwg_requirements=
        poly.unresolved_original_dwg_requirements;
    proof.original_layer_packed_values=poly.original_layer_packed_values;
    proof.original_layer_aci=poly.original_layer_aci;
    proof.diagnostic_packet_aci=
        MUSA_AC1032_CIRCLE_ADAPTIVE_DIAGNOSTIC_ACI;
    proof.source_only=1u;
    memcpy(out,v,need*sizeof(float));
    *evidence=proof;
    return 1;
}
#endif
