/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_COMPLETE_BLOCK_LINE_REBASED_H
#define MUSACAD_DWG_AC1032_COMPLETE_BLOCK_LINE_REBASED_H
#include "musacad_dwg_ac1032_complete_block_line_bounds.h"
#include <float.h>
#include <math.h>
#include <stdint.h>

/* Source-only double-origin + relative float geometry transport.
 * Original R2018 OT49/H reverse ownership and OT51 layer provenance
 * must be independently verified before any conversion. This packet
 * NEVER grants layer/viewport visibility, effective color or paint. */
typedef struct MusaDwgAc1032CompleteBlockLineRebased {
    uint64_t insert_handle,block_header_handle;
    double origin_xy[2],max_absolute_roundtrip_error;
    uint64_t child_handles[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    float relative_xy[MUSACAD_AC1032_OT49_CHILD_LIMIT][4];
    uint32_t line_count;
    uint8_t original_complete_source_rebased_verified;
    uint8_t final_visibility_color_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032CompleteBlockLineRebased;

/* Pure conversion, also usable for hostile-source unit tests.
 * No output changes unless every endpoint, owner, layer and bounding
 * rectangle agrees. Caller supplies the maximum WCS rounding error.
 * A nonzero source segment must not disappear in float transport. */
static inline int musa_dwg_ac1032_complete_block_line_rebase_checked(
    const MusaDwgAc1032CompleteBlockLineBatch *batch,
    const MusaDwgAc1032CompleteBlockLineBounds *bounds,
    double maximum_error,MusaDwgAc1032CompleteBlockLineRebased *out){
    if(!batch||!bounds||!out||
       !isfinite(maximum_error)||maximum_error<0.0||
       !batch->original_complete_source_batch_verified||
       batch->final_visibility_color_and_paint_accepted||
       !bounds->original_complete_source_bounds_verified||
       bounds->final_visibility_color_and_paint_accepted||
       !batch->insert_handle||!batch->block_header_handle||
       batch->insert_handle!=bounds->insert_handle||
       batch->block_header_handle!=bounds->block_header_handle||
       !batch->line_count||batch->line_count!=bounds->line_count||
       batch->line_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    for(unsigned axis=0u;axis<2u;axis++)
        if(!isfinite(bounds->min_xy[axis])||
           !isfinite(bounds->max_xy[axis])||
           bounds->min_xy[axis]>bounds->max_xy[axis])return 0;
    MusaDwgAc1032CompleteBlockLineRebased local={0};
    local.insert_handle=batch->insert_handle;
    local.block_header_handle=batch->block_header_handle;
    local.origin_xy[0]=bounds->min_xy[0];
    local.origin_xy[1]=bounds->min_xy[1];
    double actual_min[2]={0.0,0.0},actual_max[2]={0.0,0.0};
    for(uint32_t i=0u;i<batch->line_count;i++){
        const MusaDwgAc1032CompleteBlockLineLayerSource *row=&batch->lines[i];
        const uint64_t handle=row->source.child_handle;
        if(!handle||!row->complete_block_reverse_owners_verified||
           row->final_display_accepted||
           !row->source.source_line_layer_and_owner_verified||
           row->source.final_visibility_color_and_paint_accepted||
           row->source.insert_handle!=batch->insert_handle||
           row->source.block_header_handle!=batch->block_header_handle||
           !row->source.layer_handle||
           row->source.layer_handle!=row->source.layer_name.indexed_handle||
           row->source.layer_handle!=row->source.raw_layer_flags.indexed_handle||
           !row->source.layer_name.original_record_crc_verified||
           !row->source.raw_layer_flags.source_crc_verified)return 0;
        for(uint32_t j=0u;j<i;j++)
            if(local.child_handles[j]==handle)return 0;
        local.child_handles[i]=handle;
        const double xy[4]={
            row->source.world_start[0],row->source.world_start[1],
            row->source.world_end[0],row->source.world_end[1]};
        for(unsigned k=0u;k<4u;k++){
            const unsigned axis=k%2u;
            const double world=xy[k];
            if(!isfinite(world))return 0;
            if(i==0u&&k<2u){
                actual_min[axis]=world;actual_max[axis]=world;
            }else{
                if(world<actual_min[axis])actual_min[axis]=world;
                if(world>actual_max[axis])actual_max[axis]=world;
            }
            const double relative=world-local.origin_xy[axis];
            if(!isfinite(relative)||relative<0.0||
               relative>(double)FLT_MAX)return 0;
            const float transported=(float)relative;
            if(!isfinite(transported)||
               (relative!=0.0&&transported==0.0f))return 0;
            const double error=fabs((double)transported-relative);
            if(!isfinite(error)||error>maximum_error)return 0;
            if(error>local.max_absolute_roundtrip_error)
                local.max_absolute_roundtrip_error=error;
            local.relative_xy[i][k]=transported;
        }
        if((xy[0]!=xy[2]||xy[1]!=xy[3])&&
           local.relative_xy[i][0]==local.relative_xy[i][2]&&
           local.relative_xy[i][1]==local.relative_xy[i][3])return 0;
        local.line_count++;
    }
    for(unsigned axis=0u;axis<2u;axis++)
        if(actual_min[axis]!=bounds->min_xy[axis]||
           actual_max[axis]!=bounds->max_xy[axis])return 0;
    local.original_complete_source_rebased_verified=1u;
    *out=local;
    return 1;
}

/* Authenticate original DWG again for the two independent source
 * packets; reject partial owners and unsupported child records. */
static inline int musa_dwg_ac1032_complete_block_line_rebased(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    double maximum_error,MusaDwgAc1032CompleteBlockLineRebased *out){
    if(!scene||!insert_handle||!out||
       !isfinite(maximum_error)||maximum_error<0.0)return 0;
    MusaDwgAc1032CompleteBlockLineBatch batch={0};
    MusaDwgAc1032CompleteBlockLineBounds bounds={0};
    if(!musa_dwg_ac1032_complete_block_line_batch(
           scene,insert_handle,&batch)||
       !musa_dwg_ac1032_complete_block_line_bounds(
           scene,insert_handle,&bounds))return 0;
    return musa_dwg_ac1032_complete_block_line_rebase_checked(
        &batch,&bounds,maximum_error,out);
}
#endif
