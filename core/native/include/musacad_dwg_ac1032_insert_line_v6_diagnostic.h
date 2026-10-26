/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_LINE_V6_DIAGNOSTIC_H
#define MUSACAD_DWG_AC1032_INSERT_LINE_V6_DIAGNOSTIC_H
#include "musacad_dwg_ac1032_insert_line_source_bounds.h"
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* A first-party C11 bridge from ORIGINAL R2018 INSERT->LINE H4/H5
 * source evidence to the existing native scene-v6 float transport.
 * This is explicitly a DIAGNOSTIC for a SINGLE INSERT, NOT a visible
 * model-space scene or a JNI first-paint entry point. Unknown layer/space/
 * viewport/parent/nested INSERT semantics forbid final render acceptance.
 *
 * v6 header: [6, lineCount, minX, minY, maxX, maxY, PARTIAL=1].
 * Direct ACI LINE: [256+ACI, x1, y1, x2, y2].
 * Direct RGB LINE: [-258, RGB24-as-exact-float, x1, y1, x2, y2].
 * Never invent ACI7 for inherited/hidden/unknown source styles.
 * Float-collapsed segments are omitted rather than rendered as a point.
 */
typedef struct MusaDwgAc1032InsertLineV6Diagnostic {
    uint64_t insert_handle,block_header_handle;
    size_t original_source_line_count,admitted_line_count;
    size_t omitted_source_line_count,required_float_words;
    uint8_t original_h4_h5_wcs_source_verified;
    uint8_t packet_partial_by_design; /* ALWAYS ONE */
    uint8_t final_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertLineV6Diagnostic;

static inline int musa_dwg_ac1032_insert_line_v6_diagnostic(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    float *words,size_t capacity,
    MusaDwgAc1032InsertLineV6Diagnostic *out){
    if(!scene||!insert_handle||!out||(!words&&capacity)||
       capacity>SIZE_MAX/sizeof(float))return 0;
    if(words){
        const uintptr_t a=(uintptr_t)words,b=(uintptr_t)out;
        const size_t bytes=capacity*sizeof(float),meta=sizeof(*out);
        if((a<=b&&b-a<bytes)||(b<a&&a-b<meta))return 0;
    }
    MusaDwgAc1032InsertLineSourceSegments segments={0};
    MusaDwgAc1032InsertLineStyleRiskEvidence risk={0};
    MusaDwgAc1032InsertLineSourceBounds bounds={0};
    if(!musa_dwg_ac1032_insert_line_source_segments(
           scene,insert_handle,&segments)||
       !musa_dwg_ac1032_insert_line_style_risk_evidence(
           scene,insert_handle,&risk)||
       !musa_dwg_ac1032_insert_line_source_bounds(
           scene,insert_handle,&bounds)||
       !segments.original_source_line_packet_verified||
       !risk.original_source_style_join_verified||
       !bounds.original_line_bounds_source_verified||
       segments.model_viewport_layer_visibility_and_paint_accepted||
       risk.final_visibility_and_paint_accepted||
       bounds.final_visibility_and_paint_accepted||
       !segments.block_header_handle||
       segments.block_header_handle!=risk.block_header_handle||
       segments.block_header_handle!=bounds.block_header_handle||
       segments.verified_planar_line_count!=risk.source_line_count||
       segments.verified_planar_line_count!=bounds.source_line_count||
       segments.verified_planar_line_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)
        return 0;

    float packet[7u+6u*MUSACAD_AC1032_OT49_CHILD_LIMIT]={0};
    packet[0]=6.0f;
    packet[6]=1.0f; /* neither correct final paint nor complete geometry */
    size_t offset=7u,admitted=0u,omitted=0u;
    double min_x=0.0,min_y=0.0,max_x=0.0,max_y=0.0;
    int have=0;
    const uint32_t mandatory_risk=
        MUSACAD_AC1032_LINE_RISK_LAYER_VISIBILITY|
        MUSACAD_AC1032_LINE_RISK_SPACE_VIEWPORT;

    for(uint32_t i=0u;i<segments.verified_planar_line_count;i++){
        const MusaDwgAc1032InsertLineSourceRow *s=&segments.rows[i];
        const MusaDwgAc1032InsertLineStyleRiskRow *r=&risk.rows[i];
        const MusaDwgAc1032InsertLineSourceBoundsRow *b=&bounds.rows[i];
        if(!s->child_handle||s->child_handle!=r->child_handle||
           s->child_handle!=b->child_handle||!s->source_layer_handle||
           s->source_layer_handle!=r->source_layer_handle||
           s->source_layer_handle!=b->source_layer_handle||
           s->source_style_class!=r->source_style_class||
           s->source_color_index!=r->source_aci||
           s->source_rgb24!=r->source_rgb24||
           r->risk_mask!=b->style_risk_mask||
           !s->original_line_h4_h5_and_world_xy_verified||
           !b->original_source_bounds_verified||
           s->final_visibility_and_paint_accepted||
           r->final_visibility_and_paint_accepted||
           b->final_paint_accepted||
           (r->risk_mask&mandatory_risk)!=mandatory_risk)return 0;

        /* Other risks are explicit exclusion criteria. Mandatory unknown
         * viewport/space/layer visibility risks NEVER become accepted. */
        if((r->risk_mask&~mandatory_risk)||
           !r->direct_encoded_color_source_only||
           !b->float_endpoints_distinguishable){
            omitted++;continue;
        }
        float code=0.0f,rgb=0.0f;
        unsigned extra=0u;
        if(r->source_style_class==MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_ACI&&
           r->source_color_status==MUSACAD_AC1032_CHILD_COLOR_DIRECT_ACI&&
           r->source_aci>=1u&&r->source_aci<=255u)
            code=256.0f+(float)r->source_aci;
        else if(r->source_style_class==
                    MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_RGB&&
                r->source_color_status==
                    MUSACAD_AC1032_CHILD_COLOR_DIRECT_RGB&&
                r->source_rgb24<=UINT32_C(0xffffff)){
            code=-258.0f;
            rgb=(float)r->source_rgb24;
            extra=1u;
        }else return 0; /* never fall back to white */

        float xy[4]={
            (float)s->world_start[0],(float)s->world_start[1],
            (float)s->world_end[0],(float)s->world_end[1]
        };
        if(!isfinite(xy[0])||!isfinite(xy[1])||
           !isfinite(xy[2])||!isfinite(xy[3])||
           (xy[0]==xy[2]&&xy[1]==xy[3]))return 0;
        if(offset+5u+extra>sizeof(packet)/sizeof(packet[0]))return 0;
        packet[offset++]=code;
        if(extra)packet[offset++]=rgb;
        for(unsigned k=0;k<4u;k++)packet[offset++]=xy[k];
        for(unsigned k=0;k<2u;k++){
            const double low=b->min_xy[k],high=b->max_xy[k];
            if(!isfinite(low)||!isfinite(high)||low>high)return 0;
            if(!have){
                if(k==0u){min_x=low;max_x=high;}
                else{min_y=low;max_y=high;}
            }else if(k==0u){
                if(low<min_x)min_x=low;
                if(high>max_x)max_x=high;
            }else{
                if(low<min_y)min_y=low;
                if(high>max_y)max_y=high;
            }
        }
        have=1;
        admitted++;
    }
    if(admitted+omitted!=segments.verified_planar_line_count||
       admitted>UINT32_C(16777215))return 0;
    packet[1]=(float)admitted;
    if(have){
        const double val[4]={min_x,min_y,max_x,max_y};
        for(unsigned k=0u;k<4u;k++){
            const int lower=k<2u;
            float f=(float)val[k];
            if(lower?(double)f>val[k]:(double)f<val[k])
                f=nextafterf(f,lower?-INFINITY:INFINITY);
            if(!isfinite(f))return 0;
            packet[2u+k]=f;
        }
    }
    MusaDwgAc1032InsertLineV6Diagnostic value={0};
    value.insert_handle=insert_handle;
    value.block_header_handle=segments.block_header_handle;
    value.original_source_line_count=segments.verified_planar_line_count;
    value.admitted_line_count=admitted;
    value.omitted_source_line_count=omitted;
    value.required_float_words=offset;
    value.original_h4_h5_wcs_source_verified=1u;
    value.packet_partial_by_design=1u;
    if(words){
        if(capacity<offset)return 0;
        memcpy(words,packet,offset*sizeof(float));
    }
    *out=value;
    return 1;
}
#endif
