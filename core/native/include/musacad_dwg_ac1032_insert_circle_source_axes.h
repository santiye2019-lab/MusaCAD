/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_CIRCLE_SOURCE_AXES_H
#define MUSACAD_DWG_AC1032_INSERT_CIRCLE_SOURCE_AXES_H
#include "musacad_dwg_ac1032_insert_reverse_owner_source_join.h"
#include <float.h>
#include <math.h>
#include <stdint.h>

/* ORIGINAL R2018 CIRCLE (OT18) inside a verified INSERT/BLOCK source.
 * Two mapped radius basis vectors preserve an anisotropically scaled or
 * reflected circle as an affine ellipse; do not emit a fake circle radius.
 * H4 reverse owner and H5 OT51 layer are separately CRC authenticated.
 * This is source geometry ONLY: no layer on/off/frozen, viewport, model/
 * paper-space, XREF, inherited color, nested INSERT or final paint proof. */
typedef struct MusaDwgAc1032InsertCircleSourceAxesRow {
    uint64_t child_handle,verified_block_owner_handle,source_layer_handle;
    double world_center[3],world_axis_u[3],world_axis_v[3],source_radius;
    uint32_t source_rgb24;
    uint16_t source_aci;
    uint8_t source_style_class,source_color_status;
    uint8_t original_circle_h4_h5_affine_axes_verified;
    uint8_t final_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertCircleSourceAxesRow;
typedef struct MusaDwgAc1032InsertCircleSourceAxes {
    uint64_t insert_handle,block_header_handle;
    uint32_t child_count,circle_candidate_count,verified_circle_count;
    uint32_t unverified_circle_count,non_circle_child_count;
    MusaDwgAc1032InsertCircleSourceAxesRow
        rows[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t original_source_circle_axes_verified;
    uint8_t final_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertCircleSourceAxes;

/* Fail atomic. A rejected candidate contributes to the audit counts,
 * never to drawable geometry. Local XY basis points are mapped through
 * the actual original BLOCK_HEADER base and original INSERT placement. */
static inline int musa_dwg_ac1032_insert_circle_source_axes(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    MusaDwgAc1032InsertCircleSourceAxes *out){
    if(!scene||!insert_handle||!out)return 0;
    MusaDwgAc1032InsertChildSourceJoin sources={0};
    MusaDwgAc1032InsertReverseOwnerJoin owners={0};
    if(!musa_dwg_ac1032_insert_child_source_join(
           scene,insert_handle,&sources)||
       !musa_dwg_ac1032_insert_reverse_owner_source_join(
           scene,insert_handle,&owners)||
       !sources.original_insert_block_child_source_join_verified||
       !owners.original_insert_and_reverse_source_join_verified||
       sources.reverse_ownership_verified||sources.space_and_viewport_verified||
       sources.final_paint_accepted||owners.complete_block_ownership_verified||
       owners.space_visibility_and_paint_accepted||
       !sources.block_header_handle||
       sources.block_header_handle!=owners.block_header_handle||
       sources.child_count!=owners.child_count||
       sources.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    MusaDwgAc1032InsertCircleSourceAxes v={0};
    v.insert_handle=insert_handle;
    v.block_header_handle=sources.block_header_handle;
    v.child_count=sources.child_count;
    for(uint32_t i=0u;i<v.child_count;i++){
        const MusaDwgAc1032InsertChildSourceRow *s=&sources.rows[i];
        const MusaDwgAc1032InsertReverseOwnerRow *o=&owners.rows[i];
        if(!s->child_handle||s->child_handle!=o->child_handle||
           !s->raw_object_type||s->raw_object_type!=o->raw_object_type||
           s->source_layer_handle!=o->source_layer_handle||
           s->coordinate_mapped!=o->source_coordinate_mapped||
           s->source_style_class!=o->source_style_class||
           s->reverse_owner_verified||s->visibility_and_paint_accepted||
           o->final_paint_accepted)return 0;
        if(s->raw_object_type!=18u){
            v.non_circle_child_count++;
            continue;
        }
        v.circle_candidate_count++;
        if(!s->coordinate_mapped||!s->original_common_verified||
           !s->source_layer_handle||!o->original_reverse_source_edge_verified||
           o->reverse_owner_status!=MUSACAD_AC1032_INSERT_OWNER_MATCH||
           o->verified_owner_handle!=v.block_header_handle){
            v.unverified_circle_count++;
            continue;
        }
        MusaDwgAc1032NativeCurve curve={0};
        if(!musa_dwg_ac1032_scene_index_read_curve(
               scene,s->child_handle,&curve)||
           !curve.geometry_decoded||curve.indexed_handle!=s->child_handle||
           curve.object_type!=18u||!isfinite(curve.radius)||
           curve.radius<=0.0||!isfinite(curve.thickness)||
           fabs(curve.thickness)>1e-9||
           !isfinite(curve.extrusion[0])||
           !isfinite(curve.extrusion[1])||
           !isfinite(curve.extrusion[2])||
           fabs(curve.extrusion[0])>1e-12||
           fabs(curve.extrusion[1])>1e-12||
           fabs(curve.extrusion[2]-1.0)>1e-12){
            v.unverified_circle_count++;
            continue;
        }
        double u[3],w[3],center[3]={0.0,0.0,0.0};
        double world_u[3]={0.0,0.0,0.0};
        double world_v[3]={0.0,0.0,0.0};
        int valid=1;
        for(unsigned k=0u;k<3u;k++){
            if(!isfinite(curve.center[k]))valid=0;
            u[k]=curve.center[k];w[k]=curve.center[k];
        }
        if(!valid){
            v.unverified_circle_count++;
            continue;
        }
        u[0]+=curve.radius;
        w[1]+=curve.radius;
        if(!isfinite(u[0])||!isfinite(w[1])||
           u[0]==curve.center[0]||w[1]==curve.center[1]||
           !musa_dwg_ac1032_insert_map_original_block_base(
               scene,insert_handle,curve.center,center)||
           !musa_dwg_ac1032_insert_map_original_block_base(
               scene,insert_handle,u,world_u)||
           !musa_dwg_ac1032_insert_map_original_block_base(
               scene,insert_handle,w,world_v)){
            v.unverified_circle_count++;
            continue;
        }
        double axis_u[3],axis_v[3];
        for(unsigned k=0u;k<3u;k++){
            axis_u[k]=world_u[k]-center[k];
            axis_v[k]=world_v[k]-center[k];
            if(!isfinite(center[k])||!isfinite(axis_u[k])||
               !isfinite(axis_v[k])||
               center[k]!=s->world_anchor[k])valid=0;
            if(k<2u&&(fabs(center[k])>FLT_MAX||
                       fabs(axis_u[k])>FLT_MAX||
                       fabs(axis_v[k])>FLT_MAX))valid=0;
        }
        const double determinant=
            axis_u[0]*axis_v[1]-axis_u[1]*axis_v[0];
        if(!isfinite(determinant)||determinant==0.0||
           fabs(center[2])>1e-9||fabs(axis_u[2])>1e-9||
           fabs(axis_v[2])>1e-9)valid=0;
        if(!valid){
            v.unverified_circle_count++;
            continue;
        }
        MusaDwgAc1032InsertCircleSourceAxesRow *row=
            &v.rows[v.verified_circle_count++];
        row->child_handle=s->child_handle;
        row->verified_block_owner_handle=o->verified_owner_handle;
        row->source_layer_handle=s->source_layer_handle;
        row->source_radius=curve.radius;
        row->source_aci=s->raw_color_index;
        row->source_rgb24=s->raw_rgb;
        row->source_style_class=s->source_style_class;
        row->source_color_status=s->source_color_status;
        for(unsigned k=0u;k<3u;k++){
            row->world_center[k]=center[k];
            row->world_axis_u[k]=axis_u[k];
            row->world_axis_v[k]=axis_v[k];
        }
        row->original_circle_h4_h5_affine_axes_verified=1u;
    }
    if(v.circle_candidate_count+v.non_circle_child_count!=v.child_count||
       v.verified_circle_count+v.unverified_circle_count!=
           v.circle_candidate_count)return 0;
    v.original_source_circle_axes_verified=1u;
    *out=v;return 1;
}
#endif
