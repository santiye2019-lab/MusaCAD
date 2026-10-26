/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_ALL_INSERT_LINE_V6_DIAGNOSTIC_H
#define MUSACAD_DWG_AC1032_ALL_INSERT_LINE_V6_DIAGNOSTIC_H
#include "musacad_dwg_ac1032_insert_line_v6_diagnostic.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* First-party, shared portable-C11, source-only multi-INSERT diagnostic.
 * Scan actual CRC/H indexed OT7 INSERTs; each INSTANCE gets its own
 * independently authenticated H4/H5 transformed source LINE coordinates.
 * A repeated BLOCK_HEADER is NOT one display instance. Do not fall back
 * to unverified children, guessed Layer 0, ACI7 or the origin.
 * Output follows original indexed INSERT H order, not a guessed draw order.
 *
 * It is an explicitly PARTIAL v6 diagnostic, NOT a complete model-space
 * scene. Every source still lacks proven model/paper, viewport, layer
 * freeze/off, nested INSERT ancestry, XREF and BYLAYER/BYBLOCK paint.
 */
#define MUSACAD_AC1032_ALL_INSERT_LINE_MAX_INSERTS 4096u
#define MUSACAD_AC1032_ALL_INSERT_LINE_MAX_FLOAT_WORDS 1048576u

typedef struct MusaDwgAc1032AllInsertLineV6Diagnostic {
    size_t total_indexed_records;
    size_t candidate_insert_records,verified_insert_records;
    size_t unsupported_insert_records,verified_source_line_count;
    size_t admitted_line_count,omitted_verified_line_count;
    size_t required_float_words;
    uint8_t source_bounds_present;
    uint8_t source_packet_is_partial; /* ALWAYS ONE */
    uint8_t final_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032AllInsertLineV6Diagnostic;

static inline int musa_dwg_ac1032_all_insert_line_v6_walk(
    const MusaDwgAc1032SceneIndex *scene,float *dst,size_t cap,
    MusaDwgAc1032AllInsertLineV6Diagnostic *out){
    if(!scene||!out)return 0;
    const size_t count=musa_dwg_ac1032_scene_index_count(scene);
    if(!count||count>1000000u)return 0;
    MusaDwgAc1032AllInsertLineV6Diagnostic r={0};
    r.total_indexed_records=count;
    r.required_float_words=7u;
    r.source_packet_is_partial=1u;
    float packet[7u+6u*MUSACAD_AC1032_OT49_CHILD_LIMIT]={0};
    float extrema[4]={0};
    int present=0;
    uint64_t previous=0u;
    for(size_t i=0u;i<count;i++){
        MusaDwgAc1032HandleEntry entry={0};
        uint16_t type=0u;
        if(!musa_dwg_ac1032_scene_index_entry(scene,i,&entry)||
           !entry.handle||entry.handle<=previous||
           !musa_dwg_ac1032_scene_index_object_type(
               scene,entry.handle,&type))return 0;
        previous=entry.handle;
        if(type!=7u)continue;
        if(r.candidate_insert_records>=
           MUSACAD_AC1032_ALL_INSERT_LINE_MAX_INSERTS)return 0;
        r.candidate_insert_records++;
        MusaDwgAc1032InsertLineV6Diagnostic part={0};
        if(!musa_dwg_ac1032_insert_line_v6_diagnostic(
               scene,entry.handle,packet,
               sizeof(packet)/sizeof(packet[0]),&part)){
            r.unsupported_insert_records++;
            continue; /* source not independently authenticated: omit */
        }
        if(!part.original_h4_h5_wcs_source_verified||
           !part.packet_partial_by_design||
           part.final_visibility_and_paint_accepted||
           part.insert_handle!=entry.handle||
           part.original_source_line_count>
               MUSACAD_AC1032_OT49_CHILD_LIMIT||
           part.admitted_line_count>
               part.original_source_line_count||
           part.omitted_source_line_count!=
               part.original_source_line_count-part.admitted_line_count||
           part.required_float_words<7u||
           part.required_float_words>sizeof(packet)/sizeof(packet[0])||
           packet[0]!=6.0f||packet[6]!=1.0f||
           packet[1]!=(float)part.admitted_line_count)return 0;
        if(r.verified_source_line_count>
           SIZE_MAX-part.original_source_line_count||
           r.admitted_line_count>SIZE_MAX-part.admitted_line_count||
           r.omitted_verified_line_count>
               SIZE_MAX-part.omitted_source_line_count)return 0;
        r.verified_insert_records++;
        r.verified_source_line_count+=part.original_source_line_count;
        r.admitted_line_count+=part.admitted_line_count;
        r.omitted_verified_line_count+=part.omitted_source_line_count;
        const size_t extra=part.required_float_words-7u;
        if(extra>MUSACAD_AC1032_ALL_INSERT_LINE_MAX_FLOAT_WORDS-
                  r.required_float_words)return 0;
        if(extra){
            if(!part.admitted_line_count)return 0;
            for(unsigned k=0u;k<4u;k++)
                if(!isfinite(packet[2u+k]))return 0;
            if(packet[2]>packet[4]||packet[3]>packet[5])return 0;
            if(!present){
                memcpy(extrema,packet+2u,4u*sizeof(float));
            }else{
                if(packet[2]<extrema[0])extrema[0]=packet[2];
                if(packet[3]<extrema[1])extrema[1]=packet[3];
                if(packet[4]>extrema[2])extrema[2]=packet[4];
                if(packet[5]>extrema[3])extrema[3]=packet[5];
            }
            present=1;
            if(dst){
                if(r.required_float_words>cap||
                   extra>cap-r.required_float_words)return 0;
                memcpy(dst+r.required_float_words,packet+7u,
                       extra*sizeof(float));
            }
        }
        r.required_float_words+=extra;
    }
    if(r.candidate_insert_records!=
       r.verified_insert_records+r.unsupported_insert_records||
       r.verified_source_line_count!=
       r.admitted_line_count+r.omitted_verified_line_count||
       r.admitted_line_count>UINT32_C(16777215)||
       r.required_float_words>
           MUSACAD_AC1032_ALL_INSERT_LINE_MAX_FLOAT_WORDS||
       (r.admitted_line_count>0u)!=(present!=0))return 0;
    if(dst){
        if(cap<r.required_float_words)return 0;
        dst[0]=6.0f;
        dst[1]=(float)r.admitted_line_count;
        for(unsigned k=0u;k<4u;k++)
            dst[k+2u]=present?extrema[k]:0.0f;
        dst[6]=1.0f;
    }
    r.source_bounds_present=(uint8_t)present;
    *out=r;
    return 1;
}

/* Two independent source passes and private scratch: a failed capacity,
 * corrupted original graph or changed scene NEVER alters caller output.
 * A zero-emission source does NOT create any synthetic primitive. */
static inline int musa_dwg_ac1032_all_insert_line_v6_diagnostic(
    const MusaDwgAc1032SceneIndex *scene,float *words,size_t capacity,
    MusaDwgAc1032AllInsertLineV6Diagnostic *out){
    if(!scene||!out||(!words&&capacity)||
       capacity>SIZE_MAX/sizeof(float))return 0;
    if(words){
        const uintptr_t a=(uintptr_t)words,b=(uintptr_t)out;
        const size_t bytes=capacity*sizeof(float),meta=sizeof(*out);
        if((a<=b&&b-a<bytes)||(b<a&&a-b<meta))return 0;
    }
    MusaDwgAc1032AllInsertLineV6Diagnostic before={0},after={0};
    if(!musa_dwg_ac1032_all_insert_line_v6_walk(
           scene,NULL,0u,&before))return 0;
    if(!words){
        *out=before;
        return 1;
    }
    if(capacity<before.required_float_words)return 0;
    float *scratch=(float*)malloc(
        before.required_float_words*sizeof(float));
    if(!scratch)return 0;
    const int ok=musa_dwg_ac1032_all_insert_line_v6_walk(
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
