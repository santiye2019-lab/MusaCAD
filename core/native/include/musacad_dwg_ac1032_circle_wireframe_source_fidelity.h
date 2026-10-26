/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_AC1032_CIRCLE_WIREFRAME_SOURCE_FIDELITY_H
#define MUSACAD_AC1032_CIRCLE_WIREFRAME_SOURCE_FIDELITY_H
#include "musacad_dwg_ac1032_insert_circle_numeric_wireframe_v6.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Independent read-only verification of the v6 diagnostic against actual
 * CRC-indexed R2018 INSERT/CIRCLE records. Never accepts source paint:
 * ACI6 is a WARNING, not DWG color, and unknown H4/H5 remains unknown.
 * Output is fail-atomic; packet bytes are never modified. */
typedef struct MusaDwgAc1032CircleWireframeFidelity {
    size_t inspected_insert_records,source_numeric_circles;
    size_t unresolved_h4_h5_circles,verified_wireframes;
    uint8_t geometry_only_verified; /* 1 on success */
    uint8_t source_color_or_visibility_accepted; /* ALWAYS 0 */
} MusaDwgAc1032CircleWireframeFidelity;

static inline int musa_dwg_ac1032_circle_wireframe_source_fidelity(
    const MusaDwgAc1032SceneIndex *scene,
    const float *packet,size_t words,
    MusaDwgAc1032CircleWireframeFidelity *out){
    if(!scene||!packet||!out||words<7u||
       words>MUSACAD_AC1032_CIRCLE_WIRE_WORDS||
       packet[0]!=6.0f||packet[6]!=1.0f||
       !isfinite(packet[1])||packet[1]<0.0f||
       packet[1]>(float)MUSACAD_AC1032_CIRCLE_WIRE_LIMIT)return 0;
    const size_t count=musa_dwg_ac1032_scene_index_count(scene);
    if(count<1u||count>1000000u)return 0;
    MusaDwgAc1032CircleWireframeFidelity v={0};
    size_t at=7u;
    uint64_t previous=0u;
    double ext[4]={0.0,0.0,0.0,0.0};
    int has_ext=0;
    for(size_t i=0u;i<count;i++){
        MusaDwgAc1032HandleEntry entry={0};
        uint16_t type=0u;
        if(!musa_dwg_ac1032_scene_index_entry(scene,i,&entry)||
           !entry.handle||entry.handle<=previous||
           !musa_dwg_ac1032_scene_index_object_type(
               scene,entry.handle,&type))return 0;
        previous=entry.handle;
        if(type!=7u)continue;
        if(v.inspected_insert_records>=4096u)return 0;
        v.inspected_insert_records++;
        MusaDwgAc1032InsertCircleSourceAxes src={0};
        if(!musa_dwg_ac1032_insert_circle_source_axes(
               scene,entry.handle,&src))continue;
        if(!src.original_numeric_source_circle_axes_verified||
           src.final_visibility_and_paint_accepted||
           src.insert_handle!=entry.handle||
           src.verified_circle_count+src.unverified_circle_count!=
               src.circle_candidate_count||
           src.h4_h5_source_verified_count+src.h4_h5_unresolved_count!=
               src.verified_circle_count)return 0;
        v.unresolved_h4_h5_circles+=src.h4_h5_unresolved_count;
        for(uint32_t j=0u;j<src.verified_circle_count;j++){
            const MusaDwgAc1032InsertCircleSourceAxesRow *r=&src.rows[j];
            const size_t segment=2u+2u*(MUSACAD_AC1032_CIRCLE_WIRE_SAMPLES+1u);
            if(!r->child_handle||!r->original_numeric_affine_axes_verified||
               r->final_visibility_and_paint_accepted||
               (!r->original_h4_h5_source_verified&&
                 (r->source_layer_handle||r->verified_block_owner_handle||
                  r->source_aci||r->source_rgb24))||
               v.verified_wireframes>=MUSACAD_AC1032_CIRCLE_WIRE_LIMIT||
               at>words||segment>words-at)return 0;
            if(packet[at]!=(float)(512u+MUSACAD_AC1032_CIRCLE_DIAGNOSTIC_ACI)||
               packet[at+1u]!=(float)(MUSACAD_AC1032_CIRCLE_WIRE_SAMPLES+1u))
                return 0;
            const double cx=r->world_center[0],cy=r->world_center[1];
            const double ux=r->world_axis_u[0],uy=r->world_axis_u[1];
            const double vx=r->world_axis_v[0],vy=r->world_axis_v[1];
            const double rx=hypot(ux,vx),ry=hypot(uy,vy);
            if(!isfinite(cx)||!isfinite(cy)||!isfinite(rx)||
               !isfinite(ry)||rx<=0.0||ry<=0.0||
               !isfinite(cx-rx)||!isfinite(cx+rx)||
               !isfinite(cy-ry)||!isfinite(cy+ry))return 0;
            const double lo[2]={cx-rx,cy-ry},hi[2]={cx+rx,cy+ry};
            if(!has_ext){
                ext[0]=lo[0];ext[1]=lo[1];ext[2]=hi[0];ext[3]=hi[1];
                has_ext=1;
            }else{
                if(lo[0]<ext[0])ext[0]=lo[0];
                if(lo[1]<ext[1])ext[1]=lo[1];
                if(hi[0]>ext[2])ext[2]=hi[0];
                if(hi[1]>ext[3])ext[3]=hi[1];
            }
            float first_x=0.0f,first_y=0.0f;
            for(unsigned k=0u;k<=MUSACAD_AC1032_CIRCLE_WIRE_SAMPLES;k++){
                const double angle=(6.283185307179586476925286766559/64.0)*
                                    (double)k;
                const double x=cx+ux*cos(angle)+vx*sin(angle);
                const double y=cy+uy*cos(angle)+vy*sin(angle);
                const float fx=(float)x,fy=(float)y;
                if(!isfinite(x)||!isfinite(y)||
                   !isfinite(fx)||!isfinite(fy))return 0;
                if(k==0u){first_x=fx;first_y=fy;}
                const float expect_x=
                    k==MUSACAD_AC1032_CIRCLE_WIRE_SAMPLES?first_x:fx;
                const float expect_y=
                    k==MUSACAD_AC1032_CIRCLE_WIRE_SAMPLES?first_y:fy;
                if(packet[at+2u+2u*k]!=expect_x||
                   packet[at+3u+2u*k]!=expect_y)return 0;
            }
            at+=segment;
            v.source_numeric_circles++;
            v.verified_wireframes++;
        }
    }
    if(at!=words||packet[1]!=(float)v.verified_wireframes||
       (v.verified_wireframes>0u)!=(has_ext!=0))return 0;
    for(unsigned k=0u;k<4u;k++){
        const double exact=has_ext?ext[k]:0.0;
        float expected=(float)exact;
        if(!isfinite(expected))return 0;
        if(k<2u?(double)expected>exact:(double)expected<exact)
            expected=nextafterf(expected,k<2u?-INFINITY:INFINITY);
        if(!isfinite(expected)||packet[2u+k]!=expected)return 0;
    }
    v.geometry_only_verified=1u;
    *out=v;
    return 1;
}
#endif
