/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_display_readiness.h"
#include <stdint.h>
#define MUSA_DISPLAY_MAX_RECORDS 1000000u

int musa_dwg_ac1032_scene_display_readiness(
    const MusaDwgAc1032SceneIndex *scene,MusaAc1032DisplayReadiness *out){
    if(!scene||!out||(const void*)scene==(const void*)out)return 0;
    const size_t count=musa_dwg_ac1032_scene_index_count(scene);
    if(!count||count>MUSA_DISPLAY_MAX_RECORDS)return 0;
    MusaAc1032DisplayReadiness result={0};
    result.indexed_records=count;
    uint64_t previous=0u;
    for(size_t i=0;i<count;i++){
        MusaDwgAc1032HandleEntry entry={0};
        uint16_t type=0u;
        if(!musa_dwg_ac1032_scene_index_entry(scene,i,&entry)||
           !entry.handle||entry.handle<=previous||
           !musa_dwg_ac1032_scene_index_object_type(
                scene,entry.handle,&type))return 0;
        previous=entry.handle;
        if(type!=17u&&type!=18u&&type!=19u&&type!=27u&&type!=77u){
            result.other_indexed_records++;
            continue;
        }
        result.known_planar_records++;
        MusaAc1032PlanarStyleEvidence evidence={0};
        if(!musa_dwg_ac1032_scene_planar_style_evidence(
               scene,entry.handle,&evidence)||
           !evidence.source_layer_join_verified||
           !evidence.color.entity_and_layer_crc_verified||
           evidence.color.entity_handle!=entry.handle||
           evidence.color.raw_entity_type!=type||
           evidence.raw_layer_status.indexed_handle!=
               evidence.color.layer_handle||
           evidence.original_layer_name.indexed_handle!=
               evidence.color.layer_handle||
           evidence.visibility_resolved||
           evidence.display_style_resolved||
           evidence.color.rendering_accepted){
            result.known_style_rejected++;
            continue;
        }
        const MusaAc1032ColorProvenance *p=&evidence.color;
        const unsigned classes=(unsigned)p->entity_has_direct_aci+
             (unsigned)p->entity_by_layer+
             (unsigned)p->entity_by_block+
             (unsigned)p->entity_complex_color_or_transparency;
        if(classes!=1u){
            /* Unmapped source color form is one unresolved entity, never
             * an excuse to discard the entire authenticated scene. */
            result.known_style_rejected++;
            continue;
        }
        result.source_style_joined++;
        if(p->entity_has_direct_aci)result.source_direct_aci++;
        if(p->entity_by_layer)result.source_bylayer++;
        if(p->entity_by_block)result.source_byblock++;
        if(p->entity_complex_color_or_transparency)
            result.source_complex_or_invisible++;
        if(p->source_entity_invisible){
            /* A source-hidden entity must never be counted as resolved
             * direct RGB or reported as proof of visible layer state. */
            if(!p->entity_complex_color_or_transparency||
               p->entity_has_direct_rgb24||
               p->entity_has_direct_aci||p->entity_by_layer||
               p->entity_by_block)return 0;
            result.source_entity_invisible++;
        }
        if(p->entity_has_direct_rgb24){
            /* Direct source RGB is already in the original preview code;
             * remains one complex category for backwards accounting. */
            if(!p->entity_complex_color_or_transparency||
               p->entity_has_direct_aci||p->entity_by_layer||
               p->entity_by_block||
               p->entity_source_rgb24>UINT32_C(0x00ffffff))
                return 0;
            result.source_direct_rgb24++;
        }else if(p->entity_source_rgb24)return 0;
        if(p->source_layer_aci_supported)
            result.narrow_layer_aci_available++;
    }
    if(result.known_planar_records+
           result.other_indexed_records!=result.indexed_records||
       result.source_style_joined+
           result.known_style_rejected!=result.known_planar_records||
       result.source_direct_aci+result.source_bylayer+
           result.source_byblock+
           result.source_complex_or_invisible!=result.source_style_joined||
       result.narrow_layer_aci_available>result.source_style_joined||
       result.source_direct_rgb24>result.source_complex_or_invisible||
       result.source_entity_invisible>result.source_complex_or_invisible||
       result.source_entity_invisible>
          result.source_complex_or_invisible-result.source_direct_rgb24)
        return 0;
    result.display_accepted=0u; /* Full scene, layer and model-state unmet. */
    *out=result;
    return 1;
}
