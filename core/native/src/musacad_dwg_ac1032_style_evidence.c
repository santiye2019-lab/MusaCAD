/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_style_evidence.h"
#include <stdint.h>
/* Composes source-authenticated data already decoded in the ONE portable
 * MusaCAD C11 engine; never fabricates color or visibility from raw bits. */
int musa_dwg_ac1032_scene_planar_style_evidence(
    const MusaDwgAc1032SceneIndex *scene,
    uint64_t h,MusaAc1032PlanarStyleEvidence *out){
    if(!scene||!h||!out||(const void*)scene==(const void*)out)return 0;
    MusaAc1032ColorProvenance color={0};
    MusaDwgAc1032EntityLayerName join={0};
    MusaDwgAc1032LayerFlagsEvidence flags={0};
    if(!musa_dwg_ac1032_scene_planar_color_provenance(
            scene,h,&color)||
       !musa_dwg_ac1032_scene_index_entity_layer_name(
            scene,h,&join)||
       !join.source_graph_and_name_verified||
       !join.edge.original_entity_crc_verified||
       !join.edge.original_layer_crc_verified||
       !join.edge.source_handle_ref_verified||
       join.edge.entity_handle!=h||
       join.edge.raw_entity_type!=color.raw_entity_type||
       join.edge.layer_handle!=color.layer_handle||
       join.layer.indexed_handle!=color.layer_handle||
       !join.layer.original_record_crc_verified||
       !join.layer.utf8_bytes||
       !musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
            scene,color.layer_handle,&flags)||
       !flags.source_crc_verified||
       flags.indexed_handle!=color.layer_handle||
       !flags.numeric_flags_end_bit||
       flags.numeric_flags_end_bit<=flags.numeric_flags_start_bit||
       !color.entity_and_layer_crc_verified||
       color.rendering_accepted)return 0;
    MusaAc1032PlanarStyleEvidence result={0};
    result.color=color;
    result.raw_layer_status=flags;
    result.original_layer_name=join.layer;
    result.source_layer_join_verified=1u;
    /* Raw numeric flags must NOT become guessed on/off/locked state. */
    result.visibility_resolved=0u;
    result.display_style_resolved=0u;
    *out=result;
    return 1;
}
