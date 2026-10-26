/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_CIRCLE_SOURCE_ENVELOPE_H
#define MUSACAD_DWG_AC1032_INSERT_CIRCLE_SOURCE_ENVELOPE_H
#include "musacad_dwg_ac1032_insert_circle_source_axes.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

/* Genuine R2018 INSERT CIRCLE numeric envelope, from independently
 * decoded affine WCS axes.  For an ellipse P(t)=C+U*cos(t)+V*sin(t),
 * the exact XY half-extent is hypot(U[k],V[k]), NOT radius*scaleX/Y.
 *
 * Numeric source evidence ONLY.  Unresolved H4/H5, layer visibility,
 * viewport, nested INSERT, XREF and BYLAYER/BYBLOCK never become paint.
 * The outward-rounded float rectangle is for diagnostic framing only,
 * not an authorization to emit a v6 primitive. */
typedef struct MusaDwgAc1032InsertCircleEnvelopeRow {
    uint64_t child_handle;
    double min_xy[2],max_xy[2];
    float outward_float_xy[4]; /* minX,minY,maxX,maxY */
    uint8_t original_h4_h5_source_verified;
    uint8_t final_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertCircleEnvelopeRow;

typedef struct MusaDwgAc1032InsertCircleEnvelope {
    uint64_t insert_handle,block_header_handle;
    uint32_t circle_candidates,verified_circles,unverified_circles;
    uint32_t h4_h5_unresolved_circles;
    uint8_t has_numeric_bounds;
    uint8_t original_numeric_source_bounds_verified;
    uint8_t final_visibility_and_paint_accepted; /* ALWAYS ZERO */
    double combined_min_xy[2],combined_max_xy[2];
    MusaDwgAc1032InsertCircleEnvelopeRow
        rows[MUSACAD_AC1032_OT49_CHILD_LIMIT];
} MusaDwgAc1032InsertCircleEnvelope;

static inline int musa_dwg_ac1032_insert_circle_source_envelope(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    MusaDwgAc1032InsertCircleEnvelope *out){
    if(!scene||!insert_handle||!out)return 0;
    MusaDwgAc1032InsertCircleSourceAxes axes={0};
    if(!musa_dwg_ac1032_insert_circle_source_axes(
           scene,insert_handle,&axes)||
       !axes.original_numeric_source_circle_axes_verified||
       axes.final_visibility_and_paint_accepted||
       axes.verified_circle_count>MUSACAD_AC1032_OT49_CHILD_LIMIT||
       axes.verified_circle_count+axes.unverified_circle_count!=
           axes.circle_candidate_count||
       axes.h4_h5_source_verified_count+axes.h4_h5_unresolved_count!=
           axes.verified_circle_count)return 0;
    MusaDwgAc1032InsertCircleEnvelope v={0};
    v.insert_handle=axes.insert_handle;
    v.block_header_handle=axes.block_header_handle;
    v.circle_candidates=axes.circle_candidate_count;
    v.verified_circles=axes.verified_circle_count;
    v.unverified_circles=axes.unverified_circle_count;
    v.h4_h5_unresolved_circles=axes.h4_h5_unresolved_count;
    for(uint32_t i=0u;i<v.verified_circles;i++){
        const MusaDwgAc1032InsertCircleSourceAxesRow *src=&axes.rows[i];
        MusaDwgAc1032InsertCircleEnvelopeRow *dst=&v.rows[i];
        if(!src->child_handle||
           !src->original_numeric_affine_axes_verified||
           src->final_visibility_and_paint_accepted||
           (!src->original_h4_h5_source_verified&&
             (src->source_layer_handle||src->verified_block_owner_handle||
              src->source_aci||src->source_rgb24)))return 0;
        dst->child_handle=src->child_handle;
        dst->original_h4_h5_source_verified=
            src->original_h4_h5_source_verified;
        for(unsigned k=0u;k<2u;k++){
            const double c=src->world_center[k];
            const double radius=hypot(src->world_axis_u[k],
                                      src->world_axis_v[k]);
            const double lo=c-radius,hi=c+radius;
            if(!isfinite(c)||!isfinite(radius)||radius<=0.0||
               !isfinite(lo)||!isfinite(hi)||lo>hi)return 0;
            float flo=(float)lo,fhi=(float)hi;
            if(!isfinite(flo)||!isfinite(fhi))return 0;
            if((double)flo>lo)flo=nextafterf(flo,-INFINITY);
            if((double)fhi<hi)fhi=nextafterf(fhi,INFINITY);
            if(!isfinite(flo)||!isfinite(fhi)||
               (double)flo>lo||(double)fhi<hi)return 0;
            dst->min_xy[k]=lo;
            dst->max_xy[k]=hi;
            dst->outward_float_xy[k]=flo;
            dst->outward_float_xy[k+2u]=fhi;
            if(!v.has_numeric_bounds){
                v.combined_min_xy[k]=lo;
                v.combined_max_xy[k]=hi;
            }else{
                if(lo<v.combined_min_xy[k])v.combined_min_xy[k]=lo;
                if(hi>v.combined_max_xy[k])v.combined_max_xy[k]=hi;
            }
        }
        v.has_numeric_bounds=1u;
    }
    if((v.verified_circles>0u)!=(v.has_numeric_bounds!=0u))return 0;
    v.original_numeric_source_bounds_verified=1u;
    *out=v;
    return 1;
}
#endif
