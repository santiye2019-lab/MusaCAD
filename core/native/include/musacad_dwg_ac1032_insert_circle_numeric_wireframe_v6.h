/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_AC1032_INSERT_CIRCLE_WIREFRAME_V6_H
#define MUSACAD_AC1032_INSERT_CIRCLE_WIREFRAME_V6_H
#include "musacad_dwg_ac1032_all_insert_circle_envelope.h"
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Explicitly NON-ORIGINAL-color, geometry-only R2018 WCS CIRCLE evidence.
 * An original circle transformed by a nonuniform INSERT is an ellipse.
 * Its actual parametric locus is center + U*cos(t) + V*sin(t).
 * Draw 64 sampled chords as one open v6 polyline with coincident endpoints.
 * v6 ACI6 here is a bright DIAGNOSTIC WIRE COLOR ONLY; it is NEVER the
 * original layer, source ACI or effective BYBLOCK/BYLAYER paint.
 *
 * This is a standalone inspector, NOT a DWG scene/owner/viewport solution:
 * UNKNOWN H4/H5 source ownership stays UNKNOWN, and UNKNOWN layer/space/
 * paper/model/viewport state is NOT certified or visually hidden.
 */
#define MUSACAD_AC1032_CIRCLE_WIRE_SAMPLES 64u
#define MUSACAD_AC1032_CIRCLE_WIRE_LIMIT 1024u
#define MUSACAD_AC1032_CIRCLE_WIRE_WORDS (7u+MUSACAD_AC1032_CIRCLE_WIRE_LIMIT*(2u+2u*(MUSACAD_AC1032_CIRCLE_WIRE_SAMPLES+1u)))
#define MUSACAD_AC1032_CIRCLE_DIAGNOSTIC_ACI 6u

typedef struct MusaDwgAc1032CircleWireframeEvidence {
    size_t indexed_records,insert_candidates,verified_inserts;
    size_t unsupported_inserts,circle_candidates,source_numeric_circles;
    size_t skipped_unverified_circles,unresolved_h4_h5_circles;
    size_t emitted_numeric_wireframes,required_float_words;
    uint8_t source_only_partial; /* ALWAYS 1 */
    uint8_t paint_is_diagnostic_not_source; /* ALWAYS 1 */
    uint8_t final_color_visibility_and_ownership_accepted; /* ALWAYS 0 */
} MusaDwgAc1032CircleWireframeEvidence;

static inline int musa_dwg_ac1032_circle_wireframe_v6_walk(
    const MusaDwgAc1032SceneIndex *scene,float *words,size_t capacity,
    MusaDwgAc1032CircleWireframeEvidence *out){
    if(!scene||!out)return 0;
    size_t n=musa_dwg_ac1032_scene_index_count(scene);
    if(n<1u||n>1000000u)return 0;
    MusaDwgAc1032CircleWireframeEvidence v={0};
    v.indexed_records=n;
    v.required_float_words=7u;
    v.source_only_partial=1u;
    v.paint_is_diagnostic_not_source=1u;
    double ext[4]={0.0,0.0,0.0,0.0};
    int has_ext=0;
    uint64_t previous=0u;
    for(size_t i=0u;i<n;i++){
        MusaDwgAc1032HandleEntry entry={0};
        uint16_t type=0u;
        if(!musa_dwg_ac1032_scene_index_entry(scene,i,&entry)||
           !entry.handle||entry.handle<=previous||
           !musa_dwg_ac1032_scene_index_object_type(
               scene,entry.handle,&type))return 0;
        previous=entry.handle;
        if(type!=7u)continue;
        if(v.insert_candidates>=4096u)return 0;
        v.insert_candidates++;
        MusaDwgAc1032InsertCircleSourceAxes circles={0};
        if(!musa_dwg_ac1032_insert_circle_source_axes(
               scene,entry.handle,&circles)){
            v.unsupported_inserts++;
            continue;
        }
        if(!circles.original_numeric_source_circle_axes_verified||
           circles.insert_handle!=entry.handle||
           circles.final_visibility_and_paint_accepted||
           circles.circle_candidate_count!=
             circles.verified_circle_count+circles.unverified_circle_count||
           circles.h4_h5_source_verified_count+
             circles.h4_h5_unresolved_count!=circles.verified_circle_count)
            return 0;
        v.verified_inserts++;
        v.circle_candidates+=circles.circle_candidate_count;
        v.source_numeric_circles+=circles.verified_circle_count;
        v.skipped_unverified_circles+=circles.unverified_circle_count;
        v.unresolved_h4_h5_circles+=circles.h4_h5_unresolved_count;
        for(uint32_t c=0u;c<circles.verified_circle_count;c++){
            const MusaDwgAc1032InsertCircleSourceAxesRow *row=
                &circles.rows[c];
            if(!row->child_handle||
               !row->original_numeric_affine_axes_verified||
               row->final_visibility_and_paint_accepted||
               (!row->original_h4_h5_source_verified&&
                 (row->source_layer_handle||
                  row->verified_block_owner_handle||
                  row->source_aci||row->source_rgb24)))
                return 0;
            if(v.emitted_numeric_wireframes>=
               MUSACAD_AC1032_CIRCLE_WIRE_LIMIT)return 0;
            const size_t need=2u+2u*(MUSACAD_AC1032_CIRCLE_WIRE_SAMPLES+1u);
            if(v.required_float_words>
               MUSACAD_AC1032_CIRCLE_WIRE_WORDS-need)return 0;
            const double cx=row->world_center[0],cy=row->world_center[1];
            const double ux=row->world_axis_u[0],uy=row->world_axis_u[1];
            const double vx=row->world_axis_v[0],vy=row->world_axis_v[1];
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
            if(words){
                if(v.required_float_words>capacity||
                   need>capacity-v.required_float_words)return 0;
                const size_t start=v.required_float_words;
                /* ACI6 is a conspicuous *diagnostic* hue, not source color. */
                words[start]=(float)(512u+MUSACAD_AC1032_CIRCLE_DIAGNOSTIC_ACI);
                words[start+1u]=(float)(MUSACAD_AC1032_CIRCLE_WIRE_SAMPLES+1u);
                for(unsigned k=0u;k<=MUSACAD_AC1032_CIRCLE_WIRE_SAMPLES;k++){
                    const double t=(6.283185307179586476925286766559/64.0)*
                                     (double)k;
                    const double x=cx+ux*cos(t)+vx*sin(t);
                    const double y=cy+uy*cos(t)+vy*sin(t);
                    const float fx=(float)x,fy=(float)y;
                    if(!isfinite(x)||!isfinite(y)||!isfinite(fx)||
                       !isfinite(fy))return 0;
                    if(k==0u){first_x=fx;first_y=fy;}
                    /* Close the sampled ellipse at the SAME original
                     * world-space point; no float32 end gap. */
                    words[start+2u+2u*k]=
                        k==MUSACAD_AC1032_CIRCLE_WIRE_SAMPLES?first_x:fx;
                    words[start+3u+2u*k]=
                        k==MUSACAD_AC1032_CIRCLE_WIRE_SAMPLES?first_y:fy;
                }
            }
            v.required_float_words+=need;
            v.emitted_numeric_wireframes++;
        }
    }
    if(v.insert_candidates!=v.verified_inserts+v.unsupported_inserts||
       v.circle_candidates!=
          v.source_numeric_circles+v.skipped_unverified_circles||
       v.emitted_numeric_wireframes!=v.source_numeric_circles||
       (v.emitted_numeric_wireframes>0u)!=(has_ext!=0))
        return 0;
    if(words){
        if(capacity<v.required_float_words)return 0;
        words[0]=6.0f;words[1]=(float)v.emitted_numeric_wireframes;
        for(unsigned i=0u;i<4u;i++){
            const double value=has_ext?ext[i]:0.0;
            float f=(float)value;
            if(!isfinite(f))return 0;
            if(i<2u?(double)f>value:(double)f<value)
                f=nextafterf(f,i<2u?-INFINITY:INFINITY);
            if(!isfinite(f))return 0;
            words[2u+i]=f;
        }
        words[6]=1.0f;
    }
    *out=v;
    return 1;
}

static inline int musa_dwg_ac1032_circle_wireframe_v6_diagnostic(
    const MusaDwgAc1032SceneIndex *scene,float *words,size_t capacity,
    MusaDwgAc1032CircleWireframeEvidence *out){
    if(!scene||!out||(!words&&capacity)||
       capacity>MUSACAD_AC1032_CIRCLE_WIRE_WORDS)return 0;
    if(words){
        const uintptr_t a=(uintptr_t)words,b=(uintptr_t)out;
        const size_t bytes=capacity*sizeof(float),meta=sizeof(*out);
        if((a<=b&&b-a<bytes)||(b<a&&a-b<meta))return 0;
    }
    MusaDwgAc1032CircleWireframeEvidence before={0},after={0};
    if(!musa_dwg_ac1032_circle_wireframe_v6_walk(
           scene,NULL,0u,&before))return 0;
    if(!words){*out=before;return 1;}
    if(capacity<before.required_float_words)return 0;
    float *scratch=(float*)malloc(before.required_float_words*sizeof(float));
    if(!scratch)return 0;
    int ok=musa_dwg_ac1032_circle_wireframe_v6_walk(
               scene,scratch,before.required_float_words,&after)&&
           !memcmp(&before,&after,sizeof(before));
    if(ok){
        memcpy(words,scratch,before.required_float_words*sizeof(float));
        *out=after;
    }
    free(scratch);
    return ok;
}
#endif
