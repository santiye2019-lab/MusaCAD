/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_COMPLETE_BLOCK_LINE_BATCH_H
#define MUSACAD_DWG_AC1032_COMPLETE_BLOCK_LINE_BATCH_H
#include "musacad_dwg_ac1032_insert_line_source_segments.h"
#include <math.h>
#include <stdint.h>

/* One bounded source-only batch of ORIGINAL R2018 INSERT child LINEs.
 * Both the complete OT49 reverse-owner inventory and the independent
 * original H4/H5 + OT51 Unicode layer / raw color join must agree.
 * This is not a paint list: effective BYLAYER/BYBLOCK, layer visibility,
 * nested INSERT, viewport, XREF and font layout are NOT accepted. */
typedef struct MusaDwgAc1032CompleteBlockLineBatch {
    uint64_t insert_handle,block_header_handle;
    uint32_t line_count;
    MusaDwgAc1032CompleteBlockLineLayerSource
        lines[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t original_complete_source_batch_verified;
    uint8_t final_visibility_color_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032CompleteBlockLineBatch;

/* All-or-nothing: a partial BLOCK or one inconsistent child rejects the
 * entire batch without modifying caller output. Zero-LINE batches are
 * rejected rather than masquerading as a successful geometry result. */
static inline int musa_dwg_ac1032_complete_block_line_batch(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    MusaDwgAc1032CompleteBlockLineBatch *out){
    if(!scene||!insert_handle||!out)return 0;
    MusaDwgAc1032InsertBlockOwnerCoverage coverage={0};
    MusaDwgAc1032InsertBlockLineSources geometry={0};
    if(!musa_dwg_ac1032_scene_index_insert_block_owner_coverage(
           scene,insert_handle,&coverage)||
       !musa_dwg_ac1032_block_owner_coverage_complete(&coverage)||
       !musa_dwg_ac1032_scene_index_complete_block_line_sources(
           scene,insert_handle,&geometry)||
       !geometry.original_source_geometry_verified||
       geometry.final_display_accepted||
       geometry.insert_handle!=insert_handle||
       geometry.block_header_handle!=coverage.block_header_handle||
       !geometry.line_count||
       geometry.line_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    uint32_t expected=0u;
    for(uint32_t i=0u;i<coverage.child_count;i++)
        if(coverage.child_types[i]==19u)expected++;
    if(expected!=geometry.line_count)return 0;
    MusaDwgAc1032CompleteBlockLineBatch local={0};
    local.insert_handle=insert_handle;
    local.block_header_handle=coverage.block_header_handle;
    for(uint32_t i=0u;i<geometry.line_count;i++){
        const uint64_t child=geometry.child_handles[i];
        if(!child)return 0;
        for(uint32_t j=0u;j<i;j++)
            if(geometry.child_handles[j]==child)return 0;
        MusaDwgAc1032CompleteBlockLineLayerSource joined={0};
        if(!musa_dwg_ac1032_complete_block_line_layer_source(
               scene,insert_handle,child,&joined)||
           !joined.complete_block_reverse_owners_verified||
           joined.final_display_accepted||
           !joined.source.source_line_layer_and_owner_verified||
           joined.source.final_visibility_color_and_paint_accepted||
           joined.source.insert_handle!=insert_handle||
           joined.source.block_header_handle!=coverage.block_header_handle||
           joined.source.child_handle!=child||
           !joined.source.layer_handle||
           joined.source.layer_handle!=joined.source.layer_name.indexed_handle||
           joined.source.layer_handle!=joined.source.raw_layer_flags.indexed_handle||
           !joined.source.layer_name.original_record_crc_verified||
           !joined.source.raw_layer_flags.source_crc_verified)return 0;
        const double xy[4]={
            joined.source.world_start[0],joined.source.world_start[1],
            joined.source.world_end[0],joined.source.world_end[1]};
        for(unsigned k=0u;k<4u;k++)
            if(!isfinite(xy[k])||!isfinite(geometry.endpoints[i][k])||
               fabs(xy[k]-geometry.endpoints[i][k])>1e-5)return 0;
        local.lines[i]=joined;
        local.line_count++;
    }
    if(local.line_count!=expected)return 0;
    local.original_complete_source_batch_verified=1u;
    *out=local;
    return 1;
}
#endif
