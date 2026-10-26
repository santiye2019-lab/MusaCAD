/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_BINDING_H
#define MUSACAD_DWG_AC1032_INSERT_BINDING_H
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_insert_transform.h"
/* Authenticated source binding only. BLOCK_HEADER base-point, ownership,
 * child enumeration, nested INSERT and final paint are NOT inferred. */
typedef struct MusaDwgAc1032InsertBinding {
    MusaDwgAc1032InsertSourceGraph graph;
    MusaDwgAc1032InsertPlacement placement;
    uint8_t source_binding_verified;
    uint8_t block_expansion_accepted; /* ALWAYS zero */
} MusaDwgAc1032InsertBinding;
static inline int musa_dwg_ac1032_insert_bind_source(
    const MusaDwgAc1032SceneIndex *scene,uint64_t handle,
    MusaDwgAc1032InsertBinding *out){
    if(!scene||!handle||!out)return 0;
    MusaDwgAc1032InsertBinding v={0};
    if(!musa_dwg_ac1032_scene_index_insert_source_graph(
           scene,handle,&v.graph)||
       !musa_dwg_ac1032_scene_index_read_insert_placement(
           scene,handle,&v.placement)||
       v.graph.indexed_handle!=handle||
       v.placement.indexed_handle!=handle||
       !v.graph.original_entity_and_targets_crc_verified||
       !v.graph.source_edges_verified||
       !v.placement.original_record_crc_verified||
       !v.placement.numeric_fields_decoded||
       !v.graph.block_header_handle||
       !v.graph.layer_handle||
       v.graph.owned_attrib_count!=v.placement.owned_attrib_count||
       v.placement.has_attribs!=
           (uint8_t)(v.graph.owned_attrib_count!=0u)||
       v.graph.block_expansion_and_display_accepted||
       v.placement.block_graph_and_display_accepted)return 0;
    v.source_binding_verified=1u;
    *out=v;
    return 1;
}
#endif
