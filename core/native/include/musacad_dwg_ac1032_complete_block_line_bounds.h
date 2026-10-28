/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_COMPLETE_BLOCK_LINE_BOUNDS_H
#define MUSACAD_DWG_AC1032_COMPLETE_BLOCK_LINE_BOUNDS_H
#include "musacad_dwg_ac1032_complete_block_line_batch.h"
#include <float.h>
#include <math.h>
#include <stdint.h>

/* Conservative source geometry bounds for one ORIGINAL R2018 INSERT.
 * Strict complete reverse ownership and independent original OT51 layer
 * evidence are mandatory. No viewport, BYLAYER/BYBLOCK, visibility,
 * XREF, nested INSERT or final paint is inferred from these bounds. */
typedef struct MusaDwgAc1032CompleteBlockLineBounds {
    uint64_t insert_handle,block_header_handle;
    double min_xy[2],max_xy[2];
    uint32_t line_count;
    uint8_t original_complete_source_bounds_verified;
    uint8_t final_visibility_color_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032CompleteBlockLineBounds;

/* Fail atomic: no caller writes until every original child LINE endpoint
 * has passed full owner, layer, numeric and finite-float checks. Bounds
 * remain double precision, avoiding premature viewport float clipping. */
static inline int musa_dwg_ac1032_complete_block_line_bounds(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    MusaDwgAc1032CompleteBlockLineBounds *out){
    if(!scene||!insert_handle||!out)return 0;
    MusaDwgAc1032CompleteBlockLineBatch batch={0};
    if(!musa_dwg_ac1032_complete_block_line_batch(
           scene,insert_handle,&batch)||
       !batch.original_complete_source_batch_verified||
       batch.final_visibility_color_and_paint_accepted||
       batch.insert_handle!=insert_handle||!batch.block_header_handle||
       !batch.line_count||
       batch.line_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    MusaDwgAc1032CompleteBlockLineBounds local={0};
    local.insert_handle=insert_handle;
    local.block_header_handle=batch.block_header_handle;
    for(uint32_t i=0u;i<batch.line_count;i++){
        const MusaDwgAc1032CompleteBlockLineLayerSource *row=
            &batch.lines[i];
        if(!row->complete_block_reverse_owners_verified||
           row->final_display_accepted||
           !row->source.source_line_layer_and_owner_verified||
           row->source.final_visibility_color_and_paint_accepted||
           row->source.insert_handle!=insert_handle||
           row->source.block_header_handle!=batch.block_header_handle||
           !row->source.child_handle||!row->source.layer_handle||
           !row->source.layer_name.original_record_crc_verified||
           !row->source.raw_layer_flags.source_crc_verified)return 0;
        for(uint32_t j=0u;j<i;j++)
            if(batch.lines[j].source.child_handle==row->source.child_handle)
                return 0;
        for(unsigned axis=0u;axis<2u;axis++){
            const double a=row->source.world_start[axis];
            const double b=row->source.world_end[axis];
            if(!isfinite(a)||!isfinite(b)||
               fabs(a)>FLT_MAX||fabs(b)>FLT_MAX)return 0;
            const double lo=a<b?a:b,hi=a>b?a:b;
            if(i==0u){
                local.min_xy[axis]=lo;
                local.max_xy[axis]=hi;
            }else{
                if(lo<local.min_xy[axis])local.min_xy[axis]=lo;
                if(hi>local.max_xy[axis])local.max_xy[axis]=hi;
            }
        }
        local.line_count++;
    }
    if(local.line_count!=batch.line_count)return 0;
    local.original_complete_source_bounds_verified=1u;
    *out=local;
    return 1;
}
#endif
