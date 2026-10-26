/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_CIRCLE_LAYER_CMC_CHAIN_H
#define MUSACAD_DWG_AC1032_INSERT_CIRCLE_LAYER_CMC_CHAIN_H
#include "musacad_dwg_ac1032_insert_circle_source_axes.h"
#include <math.h>
#include <stdint.h>
/* Read-only original R2018 source chain: CRC/H INSERT -> BLOCK_HEADER
 * -> CIRCLE with relative H(C) reverse owner -> OT51 Unicode LAYER
 * -> original CMC ACI. Existing numeric WCS axes are joined by the
 * original CIRCLE handle, not a geometric proximity heuristic.
 * This deliberately does NOT inherit layer colors across nested INSERTs
 * or certify layer on/frozen, viewport, model-space, XREF or final paint.
 * The legacy H4/H5-only row remains unchanged until independently proven.
 */
typedef struct MusaDwgAc1032InsertCircleLayerCmcChain {
    uint64_t insert_handle,circle_handle,block_header_handle,layer_handle;
    double world_center[3],world_axis_u[3],world_axis_v[3];
    double original_circle_radius;
    uint8_t original_layer_aci;
    uint8_t source_chain_and_wcs_verified;
    uint8_t final_color_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertCircleLayerCmcChain;
static inline int musa_dwg_ac1032_insert_circle_layer_cmc_chain(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    uint64_t circle_handle,MusaDwgAc1032InsertCircleLayerCmcChain *out){
    if(!scene||!insert_handle||!circle_handle||!out||
       insert_handle==circle_handle)return 0;
    MusaDwgAc1032InsertCircleSourceAxes axes={0};
    MusaDwgAc1032CircleLayerCmcSource color={0};
    if(!musa_dwg_ac1032_insert_circle_source_axes(
           scene,insert_handle,&axes)||
       !musa_dwg_ac1032_scene_index_circle_layer_cmc_source(
           scene,circle_handle,&color)||
       !axes.original_numeric_source_circle_axes_verified||
       axes.final_visibility_and_paint_accepted||
       !color.original_layer_cmc_aci_verified||
       color.final_visible_paint_accepted||
       !color.source.source_geometry_and_layer_verified||
       color.source.final_visibility_color_and_paint_accepted||
       !color.source.edge.exact_relative_owner_and_layer_source_verified||
       !color.source.edge.original_forward_block_child_membership_verified||
       color.source.edge.model_space_viewport_and_paint_accepted||
       axes.insert_handle!=insert_handle||
       !axes.block_header_handle||
       axes.block_header_handle!=color.source.edge.owner_block_header_handle||
       color.source.edge.circle_handle!=circle_handle||
       !color.source.edge.layer_handle||
       color.layer_cmc.indexed_handle!=color.source.edge.layer_handle||
       !color.layer_cmc.original_record_crc_verified||
       color.layer_cmc.aci_index<1u||color.layer_cmc.aci_index>255u||
       color.layer_cmc.packed_color_method!=
          (UINT32_C(0xC3000000)|(uint32_t)color.layer_cmc.aci_index)||
       !isfinite(color.source.circle.radius)||
       color.source.circle.radius<=0.0)return 0;
    const MusaDwgAc1032InsertCircleSourceAxesRow *matched=NULL;
    for(uint32_t i=0u;i<axes.verified_circle_count;i++){
        const MusaDwgAc1032InsertCircleSourceAxesRow *row=&axes.rows[i];
        if(row->child_handle!=circle_handle)continue;
        if(matched||!row->original_numeric_affine_axes_verified||
           row->final_visibility_and_paint_accepted||
           !isfinite(row->source_radius)||
           row->source_radius!=color.source.circle.radius)return 0;
        for(unsigned j=0u;j<3u;j++)
            if(!isfinite(row->world_center[j])||
               !isfinite(row->world_axis_u[j])||
               !isfinite(row->world_axis_v[j]))return 0;
        matched=row;
    }
    if(!matched)return 0;
    MusaDwgAc1032InsertCircleLayerCmcChain v={0};
    v.insert_handle=insert_handle;
    v.circle_handle=circle_handle;
    v.block_header_handle=axes.block_header_handle;
    v.layer_handle=color.source.edge.layer_handle;
    v.original_circle_radius=matched->source_radius;
    v.original_layer_aci=color.layer_cmc.aci_index;
    for(unsigned j=0u;j<3u;j++){
        v.world_center[j]=matched->world_center[j];
        v.world_axis_u[j]=matched->world_axis_u[j];
        v.world_axis_v[j]=matched->world_axis_v[j];
    }
    v.source_chain_and_wcs_verified=1u;
    *out=v;
    return 1;
}
#endif
