/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_CIRCLE_BYLAYER_CROSSCHECK_H
#define MUSACAD_DWG_AC1032_INSERT_CIRCLE_BYLAYER_CROSSCHECK_H
#include "musacad_dwg_ac1032_insert_circle_layer_cmc_chain.h"
#include "musacad_dwg_ac1032_color_provenance.h"
#include <stdint.h>
/* Independently join original R2018 CIRCLE ENC BYLAYER to its original
 * CRC/H verified OT51 layer CMC and INSERT WCS axes. This is a source
 * consistency proof, NOT an effective display-color resolver. Raw layer
 * status is preserved as opaque evidence: on/frozen/new viewport/XREF,
 * nested INSERT, model/paper space and per-viewport overrides remain
 * unverified, so this API NEVER authorizes paint.
 * Fail-atomic: unsupported source leaves caller memory untouched. */
typedef struct MusaDwgAc1032InsertCircleByLayerCrosscheck {
    MusaDwgAc1032InsertCircleLayerCmcChain chain;
    MusaAc1032ColorProvenance circle_color;
    MusaDwgAc1032LayerFlagsEvidence original_layer_flags;
    uint8_t source_entity_bylayer_and_layer_cmc_agree;
    uint8_t layer_state_and_viewport_verified; /* ALWAYS ZERO */
    uint8_t final_visible_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertCircleByLayerCrosscheck;
static inline int musa_dwg_ac1032_insert_circle_bylayer_crosscheck(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    uint64_t circle_handle,MusaDwgAc1032InsertCircleByLayerCrosscheck *out){
    if(!scene||!insert_handle||!circle_handle||!out)return 0;
    MusaDwgAc1032InsertCircleLayerCmcChain chain={0};
    MusaAc1032ColorProvenance color={0};
    MusaDwgAc1032CircleLayerProvenance layer={0};
    if(!musa_dwg_ac1032_insert_circle_layer_cmc_chain(
           scene,insert_handle,circle_handle,&chain)||
       !musa_dwg_ac1032_scene_circle_owned_color_provenance(
           scene,circle_handle,&color)||
       !musa_dwg_ac1032_scene_index_circle_layer_provenance(
           scene,circle_handle,&layer)||
       !chain.source_chain_and_wcs_verified||
       chain.final_color_visibility_and_paint_accepted||
       !color.entity_and_layer_crc_verified||
       color.rendering_accepted||
       color.entity_handle!=circle_handle||
       color.layer_handle!=chain.layer_handle||
       color.raw_entity_type!=18u||
       color.entity_aci!=256u||
       !color.entity_by_layer||color.entity_by_block||
       color.entity_has_direct_aci||color.entity_has_direct_rgb24||
       color.entity_complex_color_or_transparency||
       color.source_entity_invisible||
       (color.source_layer_aci_supported&&
        color.source_layer_aci!=chain.original_layer_aci)||
       !layer.source_geometry_and_layer_verified||
       layer.final_visibility_color_and_paint_accepted||
       layer.edge.circle_handle!=circle_handle||
       layer.edge.owner_block_header_handle!=chain.block_header_handle||
       layer.edge.layer_handle!=chain.layer_handle||
       !layer.raw_layer_flags.source_crc_verified||
       layer.raw_layer_flags.indexed_handle!=chain.layer_handle||
       !chain.original_layer_aci)return 0;
    MusaDwgAc1032InsertCircleByLayerCrosscheck result={0};
    result.chain=chain;
    result.circle_color=color;
    result.original_layer_flags=layer.raw_layer_flags;
    result.source_entity_bylayer_and_layer_cmc_agree=1u;
    *out=result;
    return 1;
}
#endif
