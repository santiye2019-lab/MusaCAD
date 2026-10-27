/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_CIRCLE_BYLAYER_INVENTORY_H
#define MUSACAD_DWG_AC1032_CIRCLE_BYLAYER_INVENTORY_H
#include "musacad_dwg_ac1032_circle_layer_visibility_veto.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
/* Read-only, FIRST-PARTY C11 inventory of authenticated R2018 INSERT
 * instances with OT18 CIRCLE child source BYLAYER and OT51 CMC ACI.
 * Each INSERT is a separate instance; no deduplication by BLOCK_HEADER.
 * This is NOT a renderer. In particular, model/paper-space, nested
 * INSERTs, DWG packed layer OFF/frozen, viewport/XREF, and BYBLOCK are
 * not certified. No row can be promoted to final paint.
 * A bounded, fail-atomic output is used rather than silently truncating. */
#define MUSACAD_AC1032_CIRCLE_BYLAYER_INVENTORY_MAX 256u
typedef struct MusaDwgAc1032CircleByLayerInventoryRow {
    uint64_t insert_handle,block_header_handle,circle_handle,layer_handle;
    double world_center[3],world_axis_u[3],world_axis_v[3];
    uint16_t original_layer_packed_values;
    uint8_t original_layer_aci;
    uint8_t original_source_crc_and_wcs_verified;
    uint8_t final_visible_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032CircleByLayerInventoryRow;
typedef struct MusaDwgAc1032CircleByLayerInventory {
    size_t indexed_record_count;
    size_t insert_count,source_indexed_insert_count;
    size_t unresolved_insert_count,undecodable_indexed_count;
    size_t circle_candidate_count,unresolved_circle_count;
    size_t verified_source_color_count;
    MusaDwgAc1032CircleByLayerInventoryRow
        rows[MUSACAD_AC1032_CIRCLE_BYLAYER_INVENTORY_MAX];
    uint8_t original_source_inventory_only;
    uint8_t final_visible_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032CircleByLayerInventory;
static inline int musa_dwg_ac1032_circle_bylayer_inventory(
    const MusaDwgAc1032SceneIndex *scene,
    MusaDwgAc1032CircleByLayerInventory *out){
    if(!scene||!out)return 0;
    const size_t count=musa_dwg_ac1032_scene_index_count(scene);
    if(!count||count>1000000u)return 0;
    MusaDwgAc1032CircleByLayerInventory v={0};
    v.indexed_record_count=count;
    for(size_t i=0u;i<count;i++){
        MusaDwgAc1032HandleEntry entry={0};
        uint16_t object_type=0u;
        if(!musa_dwg_ac1032_scene_index_entry(scene,i,&entry)||
           !entry.handle)return 0;
        if(!musa_dwg_ac1032_scene_index_object_type(
                scene,entry.handle,&object_type)){
            v.undecodable_indexed_count++;
            continue;
        }
        if(object_type!=7u)continue; /* original OT7 INSERT only */
        v.insert_count++;
        MusaDwgAc1032InsertCircleSourceAxes axes={0};
        if(!musa_dwg_ac1032_insert_circle_source_axes(
                scene,entry.handle,&axes)){
            v.unresolved_insert_count++;
            continue;
        }
        if(!axes.original_numeric_source_circle_axes_verified||
           axes.final_visibility_and_paint_accepted||
           axes.insert_handle!=entry.handle||
           axes.verified_circle_count>MUSACAD_AC1032_OT49_CHILD_LIMIT||
           axes.verified_circle_count+axes.unverified_circle_count!=
             axes.circle_candidate_count)return 0;
        v.source_indexed_insert_count++;
        v.circle_candidate_count+=axes.circle_candidate_count;
        v.unresolved_circle_count+=axes.unverified_circle_count;
        if(v.circle_candidate_count>1000000u)return 0;
        for(uint32_t j=0u;j<axes.verified_circle_count;j++){
            const MusaDwgAc1032InsertCircleSourceAxesRow *row=&axes.rows[j];
            MusaDwgAc1032CircleLayerStatusPreflight evidence={0};
            if(!row->original_numeric_affine_axes_verified||
               row->final_visibility_and_paint_accepted||
               !musa_dwg_ac1032_circle_layer_status_preflight(
                   scene,entry.handle,row->child_handle,&evidence)){
                v.unresolved_circle_count++;
                continue;
            }
            const MusaDwgAc1032InsertCircleLayerCmcChain *chain=
                &evidence.source.chain;
            if(!evidence.original_layer_status_bits_crc_verified||
               !evidence.source.source_entity_bylayer_and_layer_cmc_agree||
               evidence.effective_color_and_visible_paint_accepted||
               evidence.candidate_bits.display_accepted||
               chain->insert_handle!=entry.handle||
               chain->circle_handle!=row->child_handle||
               chain->block_header_handle!=axes.block_header_handle||
               !chain->source_chain_and_wcs_verified||
               chain->final_color_visibility_and_paint_accepted||
               !chain->layer_handle||!chain->original_layer_aci){
                v.unresolved_circle_count++;
                continue;
            }
            int exact=1;
            for(unsigned k=0u;k<3u;k++){
                if(!isfinite(chain->world_center[k])||
                   !isfinite(chain->world_axis_u[k])||
                   !isfinite(chain->world_axis_v[k])||
                   chain->world_center[k]!=row->world_center[k]||
                   chain->world_axis_u[k]!=row->world_axis_u[k]||
                   chain->world_axis_v[k]!=row->world_axis_v[k])
                    exact=0;
            }
            if(!exact){
                v.unresolved_circle_count++;
                continue;
            }
            if(v.verified_source_color_count>=
                MUSACAD_AC1032_CIRCLE_BYLAYER_INVENTORY_MAX)return 0;
            MusaDwgAc1032CircleByLayerInventoryRow *dst=
                &v.rows[v.verified_source_color_count++];
            dst->insert_handle=entry.handle;
            dst->block_header_handle=chain->block_header_handle;
            dst->circle_handle=row->child_handle;
            dst->layer_handle=chain->layer_handle;
            dst->original_layer_aci=chain->original_layer_aci;
            dst->original_layer_packed_values=
                evidence.candidate_bits.raw_values;
            for(unsigned k=0u;k<3u;k++){
                dst->world_center[k]=chain->world_center[k];
                dst->world_axis_u[k]=chain->world_axis_u[k];
                dst->world_axis_v[k]=chain->world_axis_v[k];
            }
            dst->original_source_crc_and_wcs_verified=1u;
        }
    }
    if(v.source_indexed_insert_count+v.unresolved_insert_count!=
         v.insert_count||
       v.verified_source_color_count+v.unresolved_circle_count!=
         v.circle_candidate_count)return 0;
    v.original_source_inventory_only=1u;
    *out=v;
    return 1;
}
#endif
