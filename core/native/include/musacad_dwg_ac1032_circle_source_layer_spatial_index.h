/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_CIRCLE_SOURCE_LAYER_SPATIAL_INDEX_H
#define MUSACAD_DWG_AC1032_CIRCLE_SOURCE_LAYER_SPATIAL_INDEX_H
#include "musacad_dwg_ac1032_circle_source_envelope.h"
#include <stddef.h>
#include <stdint.h>
/* Read-only, portable C11 per-source-LAYER analytic XY union for genuine
 * original R2018 INSERT/CIRCLE instances. One layer can own many instances.
 * These are evidence bounds for future culling, NOT visible drawables.
 * Unknown model/paper space, viewport, XREF, nested INSERT and packed
 * layer visibility forbid all final painting. */
typedef struct MusaDwgAc1032CircleSourceLayerSpatialRow {
    uint64_t layer_handle;
    double min_xy[2],max_xy[2];
    size_t verified_instance_count;
    uint16_t original_layer_packed_values;
    uint8_t original_layer_aci;
} MusaDwgAc1032CircleSourceLayerSpatialRow;
typedef struct MusaDwgAc1032CircleSourceLayerSpatialIndex {
    MusaDwgAc1032CircleSourceLayerSpatialRow
        rows[MUSACAD_AC1032_CIRCLE_BYLAYER_INVENTORY_MAX];
    size_t source_layer_count,verified_instance_count;
    size_t unresolved_circle_count,unresolved_insert_count;
    uint32_t unresolved_original_dwg_requirements;
    uint8_t source_only;
    uint8_t final_visible_paint_accepted;
} MusaDwgAc1032CircleSourceLayerSpatialIndex;
/* Fail-atomic: no output changes on malformed or contradictory source.
 * The source inventory itself is never modified. */
static inline int musa_dwg_ac1032_circle_source_layer_spatial_index(
    const MusaDwgAc1032CircleByLayerInventory *inventory,
    MusaDwgAc1032CircleSourceLayerSpatialIndex *out){
    if(!inventory||!out||
       !inventory->original_source_inventory_only||
       inventory->final_visible_paint_accepted||
       inventory->verified_source_color_count>
         MUSACAD_AC1032_CIRCLE_BYLAYER_INVENTORY_MAX||
       inventory->verified_source_color_count>
         inventory->circle_candidate_count||
       inventory->unresolved_circle_count!=
         inventory->circle_candidate_count-
           inventory->verified_source_color_count||
       inventory->source_indexed_insert_count>inventory->insert_count||
       inventory->unresolved_insert_count!=
         inventory->insert_count-inventory->source_indexed_insert_count)
        return 0;
    MusaDwgAc1032CircleSourceLayerSpatialIndex v={0};
    v.unresolved_circle_count=inventory->unresolved_circle_count;
    v.unresolved_insert_count=inventory->unresolved_insert_count;
    v.unresolved_original_dwg_requirements=
        MUSA_CIRCLE_DWG_UNRESOLVED_LAYER_BITS|
        MUSA_CIRCLE_DWG_UNRESOLVED_OWNER_SPACE|
        MUSA_CIRCLE_DWG_UNRESOLVED_NESTED_INSERT|
        MUSA_CIRCLE_DWG_UNRESOLVED_VIEWPORT|
        MUSA_CIRCLE_DWG_UNRESOLVED_XREF;
    for(size_t i=0;i<inventory->verified_source_color_count;i++){
        MusaDwgAc1032CircleSourceEnvelope e={0};
        if(!musa_dwg_ac1032_circle_source_envelope(inventory,i,&e)||
           !e.source_crc_wcs_and_layer_aci_verified||
           e.final_visible_paint_accepted||
           e.unresolved_original_dwg_requirements!=
             v.unresolved_original_dwg_requirements)
            return 0;
        size_t j=0u;
        for(;j<v.source_layer_count;j++)
            if(v.rows[j].layer_handle==e.layer_handle)break;
        if(j==v.source_layer_count){
            if(j>=MUSACAD_AC1032_CIRCLE_BYLAYER_INVENTORY_MAX)
                return 0;
            MusaDwgAc1032CircleSourceLayerSpatialRow *r=&v.rows[j];
            r->layer_handle=e.layer_handle;
            r->original_layer_aci=e.original_layer_aci;
            r->original_layer_packed_values=e.original_layer_packed_values;
            for(unsigned k=0;k<2u;k++){
                r->min_xy[k]=e.min_xy[k];
                r->max_xy[k]=e.max_xy[k];
            }
            v.source_layer_count++;
        }else{
            MusaDwgAc1032CircleSourceLayerSpatialRow *r=&v.rows[j];
            if(r->original_layer_aci!=e.original_layer_aci||
               r->original_layer_packed_values!=
                 e.original_layer_packed_values)return 0;
            for(unsigned k=0;k<2u;k++){
                if(e.min_xy[k]<r->min_xy[k])r->min_xy[k]=e.min_xy[k];
                if(e.max_xy[k]>r->max_xy[k])r->max_xy[k]=e.max_xy[k];
            }
        }
        v.rows[j].verified_instance_count++;
        v.verified_instance_count++;
    }
    if(v.verified_instance_count!=inventory->verified_source_color_count)
        return 0;
    v.source_only=1u;
    *out=v;
    return 1;
}
#endif
