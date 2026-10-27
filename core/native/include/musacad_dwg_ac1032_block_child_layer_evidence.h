/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_BLOCK_CHILD_LAYER_EVIDENCE_H
#define MUSACAD_DWG_AC1032_BLOCK_CHILD_LAYER_EVIDENCE_H
#include "musacad_dwg_ac1032_block_marker_commons.h"
#include <stdint.h>

/* These classifications describe ORIGINAL source evidence, not paint.
 * 1: the child's source H graph, original layer OT51 and Unicode name agree.
 * 2: its raw type has a known layer reader but its exact H profile rejects.
 * 3: no supported source layer reader exists for this raw object type.
 * Never synthesize layer 0 or ACI 7 for either unresolved classification. */
#define MUSACAD_AC1032_BLOCK_LAYER_VERIFIED 1u
#define MUSACAD_AC1032_BLOCK_LAYER_PROFILE_UNRESOLVED 2u
#define MUSACAD_AC1032_BLOCK_LAYER_TYPE_UNSUPPORTED 3u

typedef struct MusaDwgAc1032BlockChildLayerRow {
    uint64_t child_handle;
    uint64_t layer_handle; /* ZERO unless independent source is verified */
    uint16_t raw_object_type;
    uint8_t layer_evidence_status;
} MusaDwgAc1032BlockChildLayerRow;

typedef struct MusaDwgAc1032BlockChildLayerInventory {
    uint64_t block_header_handle;
    uint32_t child_count;
    uint32_t verified_layer_count;
    uint32_t unresolved_profile_count;
    uint32_t unsupported_type_count;
    MusaDwgAc1032BlockChildLayerRow children[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t marker_and_child_source_crc_verified;
    uint8_t reverse_child_owner_links_verified; /* ALWAYS ZERO */
    uint8_t layer_visibility_color_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockChildLayerInventory;

/* A local block's bounded, ordered candidate children -> their own
 * independently CRC/H-authenticated layer references and Unicode names.
 * OT49 forward H-prefix is NOT proof of reverse child ownership.
 * Each unsupported/rejected layer is explicitly recorded as unresolved;
 * layer visibility, BYLAYER/BYBLOCK colors, XREF, viewport, block expansion
 * and final paint are never accepted. Fail atomic even on wrong-type H.
 * Read-only portable C11: Android, iOS and Windows, no external CAD codec. */
static inline int musa_dwg_ac1032_block_child_layer_evidence(
    const MusaDwgAc1032SceneIndex *scene,uint64_t block_header_handle,
    MusaDwgAc1032BlockChildLayerInventory *out){
    if(!scene||!out||!block_header_handle)return 0;
    MusaDwgAc1032BlockMarkerCommons markers={0};
    MusaDwgAc1032BlockHeaderTypedChildren typed={0};
    if(!musa_dwg_ac1032_block_marker_commons(
           scene,block_header_handle,&markers)||
       !musa_dwg_ac1032_scene_index_block_header_typed_children(
           scene,block_header_handle,&typed)||
       !markers.marker_records_crc_verified||
       !markers.unique_prefix_handles_verified||
       markers.reverse_owner_links_verified||
       markers.final_display_accepted||
       !typed.all_child_record_types_crc_verified||
       typed.reverse_owner_links_verified||
       typed.final_display_accepted||
       typed.prefix.indexed_handle!=block_header_handle||
       markers.block_header_handle!=block_header_handle||
       typed.prefix.child_count!=markers.child_count||
       typed.prefix.block_begin_handle!=markers.block_begin_handle||
       typed.prefix.endblk_handle!=markers.endblk_handle||
       typed.prefix.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)
        return 0;

    MusaDwgAc1032BlockChildLayerInventory result={0};
    result.block_header_handle=block_header_handle;
    result.child_count=typed.prefix.child_count;
    for(uint32_t i=0u;i<result.child_count;i++){
        MusaDwgAc1032BlockChildLayerRow *row=&result.children[i];
        const uint16_t type=typed.child_object_types[i];
        const uint64_t h=typed.prefix.child_handles[i];
        if(!h||!type||type==4u||type==5u)return 0;
        row->child_handle=h;
        row->raw_object_type=type;
        /* Source entity-layer graph does NOT support child owner profiles
         * indiscriminately. A failed exact-profile read is not a color. */
        if(type==1u||type==17u||type==18u||type==19u||
           type==27u||type==35u||type==44u||type==77u){
            MusaDwgAc1032EntityLayerName link={0};
            if(musa_dwg_ac1032_scene_index_entity_layer_name(scene,h,&link)&&
               link.source_graph_and_name_verified&&
               link.edge.entity_handle==h&&
               link.edge.raw_entity_type==type&&
               link.edge.raw_layer_type==51u&&
               link.edge.source_handle_ref_verified&&
               link.edge.original_entity_crc_verified&&
               link.edge.original_layer_crc_verified&&
               link.layer.original_record_crc_verified&&
               link.layer.indexed_handle==link.edge.layer_handle&&
               link.edge.layer_handle!=h){
                row->layer_handle=link.edge.layer_handle;
                row->layer_evidence_status=MUSACAD_AC1032_BLOCK_LAYER_VERIFIED;
                result.verified_layer_count++;
            }else if(type==19u){
                /* A block-owned LINE has an explicit owner H4 (or a
                 * canonical relative H12) before its H5 layer. The
                 * no-owner layer reader must reject that valid grammar.
                 * Recover only from the independently CRC/type-checked
                 * owner+layer source, and require this exact OT49 parent
                 * plus the original OT51 Unicode name. Never infer
                 * layer 0, BYLAYER color or drawing visibility. */
                MusaDwgAc1032EntityBlockOwnerLayerSource edge={0};
                MusaDwgAc1032LayerName layer={0};
                if(musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
                       scene,h,&edge)&&
                   edge.child_handle==h&&edge.raw_entity_type==19u&&
                   edge.owner_block_header_handle==block_header_handle&&
                   edge.layer_handle&&edge.layer_handle!=h&&
                   edge.original_child_record_crc_verified&&
                   edge.original_owner_and_layer_records_crc_verified&&
                   edge.exact_h4_owner_and_h5_layer_source_verified&&
                   !edge.visible_scene_and_paint_accepted&&
                   musa_dwg_ac1032_scene_index_read_layer_name(
                       scene,edge.layer_handle,&layer)&&
                   layer.indexed_handle==edge.layer_handle&&
                   layer.original_record_crc_verified&&
                   layer.utf8_bytes>0u&&
                   layer.utf8_bytes<sizeof(layer.utf8)&&
                   layer.utf8[layer.utf8_bytes]=='\\0'){
                    row->layer_handle=edge.layer_handle;
                    row->layer_evidence_status=
                        MUSACAD_AC1032_BLOCK_LAYER_VERIFIED;
                    result.verified_layer_count++;
                }else{
                    row->layer_evidence_status=
                        MUSACAD_AC1032_BLOCK_LAYER_PROFILE_UNRESOLVED;
                    result.unresolved_profile_count++;
                }
            }else{
                row->layer_evidence_status=
                    MUSACAD_AC1032_BLOCK_LAYER_PROFILE_UNRESOLVED;
                result.unresolved_profile_count++;
            }
        }else{
            row->layer_evidence_status=
                MUSACAD_AC1032_BLOCK_LAYER_TYPE_UNSUPPORTED;
            result.unsupported_type_count++;
        }
    }
    if(result.verified_layer_count+result.unresolved_profile_count+
       result.unsupported_type_count!=result.child_count)return 0;
    result.marker_and_child_source_crc_verified=1u;
    *out=result;
    return 1;
}
#endif
