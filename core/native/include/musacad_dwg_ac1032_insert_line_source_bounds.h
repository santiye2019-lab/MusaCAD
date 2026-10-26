/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_LINE_SOURCE_BOUNDS_H
#define MUSACAD_DWG_AC1032_INSERT_LINE_SOURCE_BOUNDS_H
#include "musacad_dwg_ac1032_insert_line_style_risk_evidence.h"
#include <float.h>
#include <math.h>
#include <stdint.h>

/* Original R2018 INSERT LINE WCS envelope evidence only. A finite
 * double-precision segment can still disappear when rounded to float;
 * record that loss instead of issuing a zero-length render primitive.
 * Visibility/viewport, lineweight, model-space and paint are NOT proved. */
typedef struct MusaDwgAc1032InsertLineSourceBoundsRow {
    uint64_t child_handle,source_layer_handle;
    double min_xy[2],max_xy[2];
    float float_min_xy[2],float_max_xy[2];
    uint32_t style_risk_mask;
    uint8_t original_source_bounds_verified;
    uint8_t float_endpoints_distinguishable;
    uint8_t final_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertLineSourceBoundsRow;
typedef struct MusaDwgAc1032InsertLineSourceBounds {
    uint64_t insert_handle,block_header_handle;
    uint32_t source_line_count,float_distinguishable_count;
    uint32_t float_degenerate_or_rounded_count;
    double source_min_xy[2],source_max_xy[2];
    MusaDwgAc1032InsertLineSourceBoundsRow
        rows[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t source_bounds_present;
    uint8_t original_line_bounds_source_verified;
    uint8_t final_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertLineSourceBounds;

/* Fail atomic; exact H/layer/order/style match between independent
 * original source packets. Empty source sets have NO synthetic origin
 * bounds. Only source XY bounds and float-representability are reported. */
static inline int musa_dwg_ac1032_insert_line_source_bounds(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    MusaDwgAc1032InsertLineSourceBounds *out){
    if(!scene||!insert_handle||!out)return 0;
    MusaDwgAc1032InsertLineSourceSegments lines={0};
    MusaDwgAc1032InsertLineStyleRiskEvidence risks={0};
    if(!musa_dwg_ac1032_insert_line_source_segments(
           scene,insert_handle,&lines)||
       !musa_dwg_ac1032_insert_line_style_risk_evidence(
           scene,insert_handle,&risks)||
       !lines.original_source_line_packet_verified||
       !risks.original_source_style_join_verified||
       lines.model_viewport_layer_visibility_and_paint_accepted||
       risks.final_visibility_and_paint_accepted||
       lines.block_header_handle!=risks.block_header_handle||
       lines.verified_planar_line_count!=risks.source_line_count||
       lines.verified_planar_line_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)
        return 0;
    MusaDwgAc1032InsertLineSourceBounds v={0};
    v.insert_handle=insert_handle;
    v.block_header_handle=lines.block_header_handle;
    v.source_line_count=lines.verified_planar_line_count;
    for(uint32_t i=0u;i<v.source_line_count;i++){
        const MusaDwgAc1032InsertLineSourceRow *s=&lines.rows[i];
        const MusaDwgAc1032InsertLineStyleRiskRow *r=&risks.rows[i];
        if(!s->original_line_h4_h5_and_world_xy_verified||
           s->final_visibility_and_paint_accepted||
           r->final_visibility_and_paint_accepted||
           !s->child_handle||s->child_handle!=r->child_handle||
           !s->source_layer_handle||
           s->source_layer_handle!=r->source_layer_handle||
           s->source_style_class!=r->source_style_class||
           s->source_color_index!=r->source_aci||
           s->source_rgb24!=r->source_rgb24)return 0;
        MusaDwgAc1032InsertLineSourceBoundsRow *dst=&v.rows[i];
        dst->child_handle=s->child_handle;
        dst->source_layer_handle=s->source_layer_handle;
        dst->style_risk_mask=r->risk_mask;
        int distinct=0,distinct_float=0;
        for(unsigned k=0u;k<2u;k++){
            const double a=s->world_start[k],b=s->world_end[k];
            if(!isfinite(a)||!isfinite(b)||
               fabs(a)>FLT_MAX||fabs(b)>FLT_MAX)return 0;
            dst->min_xy[k]=a<b?a:b;
            dst->max_xy[k]=a>b?a:b;
            dst->float_min_xy[k]=(float)dst->min_xy[k];
            dst->float_max_xy[k]=(float)dst->max_xy[k];
            if(!isfinite(dst->float_min_xy[k])||
               !isfinite(dst->float_max_xy[k]))return 0;
            if(a!=b)distinct=1;
            if(dst->float_min_xy[k]!=dst->float_max_xy[k])
                distinct_float=1;
            if(!v.source_bounds_present){
                v.source_min_xy[k]=dst->min_xy[k];
                v.source_max_xy[k]=dst->max_xy[k];
            }else{
                if(dst->min_xy[k]<v.source_min_xy[k])
                    v.source_min_xy[k]=dst->min_xy[k];
                if(dst->max_xy[k]>v.source_max_xy[k])
                    v.source_max_xy[k]=dst->max_xy[k];
            }
        }
        v.source_bounds_present=1u;
        dst->original_source_bounds_verified=1u;
        if(distinct&&distinct_float){
            dst->float_endpoints_distinguishable=1u;
            v.float_distinguishable_count++;
        }else v.float_degenerate_or_rounded_count++;
    }
    if(v.float_distinguishable_count+
       v.float_degenerate_or_rounded_count!=v.source_line_count)return 0;
    v.original_line_bounds_source_verified=1u;
    *out=v;return 1;
}
#endif
