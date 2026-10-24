/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_color_provenance.h"
#include <stddef.h>
#include <stdint.h>
/* Exact original source object is re-read here, so no caller-provided
 * "color_index" can be mistaken for an independently verified DWG value. */
int musa_dwg_ac1032_scene_line_point_color_provenance(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaAc1032ColorProvenance *out){
    if(!s||!h||!out||(const void*)s==(const void*)out)return 0;
    uint16_t type=0u;
    if(!musa_dwg_ac1032_scene_index_object_type(s,h,&type)||
       (type!=19u&&type!=27u))return 0;
    MusaDwgAc1032EntityCommon common={0};
    if(type==19u){
        MusaDwgAc1032NativeLine line={0};
        if(!musa_dwg_ac1032_scene_index_read_line(s,h,&line))return 0;
        common=line.common;
    }else{
        MusaDwgAc1032NativePoint point={0};
        if(!musa_dwg_ac1032_scene_index_read_point(s,h,&point))return 0;
        common=point.common;
    }
    if(!common.common_decoded||!common.prefix.prefix_decoded||
       common.prefix.indexed_handle!=h||
       common.prefix.raw_object_type!=type)return 0;
    MusaDwgAc1032EntityLayerName link={0};
    if(!musa_dwg_ac1032_scene_index_entity_layer_name(s,h,&link)||
       !link.source_graph_and_name_verified||
       link.edge.entity_handle!=h||
       link.edge.raw_entity_type!=type||
       !link.edge.source_handle_ref_verified||
       !link.edge.original_entity_crc_verified||
       !link.edge.original_layer_crc_verified||
       link.layer.indexed_handle!=link.edge.layer_handle||
       !link.layer.original_record_crc_verified)return 0;
    MusaAc1032ColorProvenance result={0};
    result.entity_handle=h;
    result.layer_handle=link.edge.layer_handle;
    result.raw_entity_type=type;
    result.entity_aci=common.color_index;
    result.entity_and_layer_crc_verified=1u;
    result.entity_complex_color_or_transparency=(uint8_t)
        (common.has_rgb||common.has_color_reference||
         common.has_transparency||common.invisibility!=0u);
    if(!result.entity_complex_color_or_transparency){
        result.entity_has_direct_aci=(uint8_t)(
            common.color_index>=1u&&common.color_index<=255u);
        result.entity_by_layer=(uint8_t)(common.color_index==256u);
        result.entity_by_block=(uint8_t)(common.color_index==0u);
    }
    /* Supported only for exact independently proven default R2018 OT51
     * CMC profile. Nonzero/unknown names, source-state/override semantics
     * and other colors are NOT guessed as white. */
    MusaDwgAc1032LayerCmcEvidence color={0};
    if(musa_dwg_ac1032_scene_index_layer_anchored_cmc_evidence(
          s,link.edge.layer_handle,&color)&&
       color.indexed_handle==link.edge.layer_handle&&
       color.original_record_crc_verified&&
       color.aci_index>=1u&&color.aci_index<=255u&&
       color.packed_color_method==
             (UINT32_C(0xC3000000)|(uint32_t)color.aci_index)){
        result.source_layer_aci_supported=1u;
        result.source_layer_aci=color.aci_index;
    }
    result.rendering_accepted=0u; /* not source-certified visible yet */
    *out=result;
    return 1;
}
