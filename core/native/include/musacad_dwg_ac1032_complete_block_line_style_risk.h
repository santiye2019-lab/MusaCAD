/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_COMPLETE_BLOCK_LINE_STYLE_RISK_H
#define MUSACAD_DWG_AC1032_COMPLETE_BLOCK_LINE_STYLE_RISK_H
#include "musacad_dwg_ac1032_insert_line_style_risk_evidence.h"
#include <stdint.h>

/* Source-only complete-owner LINE risk census for original R2018 INSERTs.
 * Every returned row cross-checks strict OT49 reverse ownership, WCS
 * endpoints, OT51 layer identity and independent raw ACI/RGB style risk.
 * Mandatory layer/space/viewport unknowns are NEVER cleared, even for
 * a direct source color. This does not authorize drawing or hiding. */
typedef struct MusaDwgAc1032CompleteBlockLineStyleRisk {
    uint64_t insert_handle,block_header_handle;
    uint32_t line_count,source_inherited_or_unresolved_color_count;
    uint32_t source_hidden_count,source_direct_color_count;
    MusaDwgAc1032InsertLineStyleRiskRow
        rows[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t complete_owner_and_style_risk_verified;
    uint8_t final_visibility_color_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032CompleteBlockLineStyleRisk;

static inline int musa_dwg_ac1032_complete_block_line_style_risk(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    MusaDwgAc1032CompleteBlockLineStyleRisk *out){
    if(!scene||!insert_handle||!out)return 0;
    MusaDwgAc1032InsertBlockOwnerCoverage coverage={0};
    MusaDwgAc1032InsertBlockLineSources geometry={0};
    MusaDwgAc1032InsertLineStyleRiskEvidence styles={0};
    if(!musa_dwg_ac1032_scene_index_insert_block_owner_coverage(
           scene,insert_handle,&coverage)||
       !musa_dwg_ac1032_block_owner_coverage_complete(&coverage)||
       !musa_dwg_ac1032_scene_index_complete_block_line_sources(
           scene,insert_handle,&geometry)||
       !musa_dwg_ac1032_insert_line_style_risk_evidence(
           scene,insert_handle,&styles)||
       !geometry.original_source_geometry_verified||
       geometry.final_display_accepted||
       !styles.original_source_style_join_verified||
       styles.final_visibility_and_paint_accepted||
       geometry.insert_handle!=insert_handle||
       styles.insert_handle!=insert_handle||
       geometry.block_header_handle!=coverage.block_header_handle||
       styles.block_header_handle!=coverage.block_header_handle||
       !geometry.line_count||
       geometry.line_count>MUSACAD_AC1032_OT49_CHILD_LIMIT||
       styles.source_line_count!=geometry.line_count)return 0;
    MusaDwgAc1032CompleteBlockLineStyleRisk local={0};
    local.insert_handle=insert_handle;
    local.block_header_handle=coverage.block_header_handle;
    const uint32_t mandatory=
        MUSACAD_AC1032_LINE_RISK_LAYER_VISIBILITY|
        MUSACAD_AC1032_LINE_RISK_SPACE_VIEWPORT;
    for(uint32_t i=0u;i<geometry.line_count;i++){
        const MusaDwgAc1032InsertLineStyleRiskRow *r=&styles.rows[i];
        const uint64_t child=geometry.child_handles[i];
        MusaDwgAc1032CompleteBlockLineLayerSource joined={0};
        if(!child||r->child_handle!=child||
           !musa_dwg_ac1032_complete_block_line_layer_source(
               scene,insert_handle,child,&joined)||
           !joined.complete_block_reverse_owners_verified||
           joined.final_display_accepted||
           !joined.source.source_line_layer_and_owner_verified||
           joined.source.final_visibility_color_and_paint_accepted||
           r->final_visibility_and_paint_accepted||
           r->source_layer_handle!=joined.source.layer_handle||
           r->source_style_class!=joined.source.source_style_class||
           r->source_aci!=joined.source.source_color_index||
           r->source_rgb24!=joined.source.source_rgb24||
           !r->original_layer_flags_verified||
           (r->risk_mask&mandatory)!=mandatory)return 0;
        local.rows[i]=*r;
        local.line_count++;
        if(r->risk_mask&(MUSACAD_AC1032_LINE_RISK_INHERITED_COLOR|
                         MUSACAD_AC1032_LINE_RISK_UNRESOLVED_COLOR))
            local.source_inherited_or_unresolved_color_count++;
        if(r->risk_mask&MUSACAD_AC1032_LINE_RISK_HIDDEN)
            local.source_hidden_count++;
        if(r->direct_encoded_color_source_only)
            local.source_direct_color_count++;
    }
    if(local.line_count!=styles.source_line_count||
       local.source_inherited_or_unresolved_color_count!=
           styles.inherited_or_unresolved_color_count||
       local.source_hidden_count!=styles.hidden_source_count||
       local.source_direct_color_count!=styles.direct_encoded_color_source_count)
        return 0;
    local.complete_owner_and_style_risk_verified=1u;
    *out=local;
    return 1;
}
#endif
