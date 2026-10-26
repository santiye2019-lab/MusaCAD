/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_BLOCK_REVERSE_OWNER_SOURCE_AUDIT_H
#define MUSACAD_DWG_AC1032_BLOCK_REVERSE_OWNER_SOURCE_AUDIT_H
#include "musacad_dwg_ac1032_block_child_text_source_inventory.h"
#include <stdint.h>

/* Compare the ORIGINAL OT49 candidate H list against only independently
 * authenticated child -> OT49 reverse H4 owner and H5 layer edges.
 * Geometry: strict mode=0 H4/H5-only native MC1032 reader.
 * TEXT/MTEXT: exact original source text graph, OT49 owner and OT51 layer.
 * Unreadable profiles, other object types, and missing owners are UNKNOWN,
 * never silently counted as owned. No whole-block ownership, paint or
 * model-space acceptance, even if all supported candidate edges agree. */
typedef struct MusaDwgAc1032BlockReverseOwnerSourceRow {
    uint64_t child_handle;
    uint64_t verified_source_owner_handle;
    uint64_t verified_source_layer_handle;
    uint16_t raw_object_type;
    uint8_t strict_reverse_edge_source_verified;
    uint8_t reverse_owner_matches_block;
    uint8_t reverse_owner_conflicts_block;
    uint8_t child_visible_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockReverseOwnerSourceRow;
typedef struct MusaDwgAc1032BlockReverseOwnerSourceAudit {
    uint64_t block_header_handle;
    uint32_t child_count,verified_reverse_source_count;
    uint32_t matching_reverse_owner_count,foreign_owner_count;
    uint32_t unresolved_owner_count;
    MusaDwgAc1032BlockReverseOwnerSourceRow rows[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t block_child_h_crc_types_verified;
    uint8_t full_block_reverse_ownership_accepted; /* ALWAYS ZERO */
    uint8_t layout_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockReverseOwnerSourceAudit;

static inline int musa_dwg_ac1032_block_reverse_owner_source_audit(
    const MusaDwgAc1032SceneIndex *scene,uint64_t block_handle,
    MusaDwgAc1032BlockReverseOwnerSourceAudit *out){
    if(!scene||!out||!block_handle)return 0;
    MusaDwgAc1032BlockChildLayerInventory layers={0};
    MusaDwgAc1032BlockChildTextSourceInventory texts={0};
    if(!musa_dwg_ac1032_block_child_layer_evidence(
           scene,block_handle,&layers)||
       !musa_dwg_ac1032_block_child_text_source_inventory(
           scene,block_handle,&texts)||
       !layers.marker_and_child_source_crc_verified||
       !texts.original_child_source_graph_verified||
       layers.reverse_child_owner_links_verified||
       texts.complete_reverse_ownership_verified||
       layers.layer_visibility_color_and_paint_accepted||
       texts.font_layout_and_paint_accepted||
       layers.block_header_handle!=block_handle||
       texts.block_header_handle!=block_handle||
       layers.child_count!=texts.child_count||
       layers.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    MusaDwgAc1032BlockReverseOwnerSourceAudit v={0};
    v.block_header_handle=block_handle;
    v.child_count=layers.child_count;
    for(uint32_t i=0u;i<v.child_count;i++){
        const MusaDwgAc1032BlockChildLayerRow *l=&layers.children[i];
        const MusaDwgAc1032BlockTextChildRow *t=&texts.children[i];
        MusaDwgAc1032BlockReverseOwnerSourceRow *r=&v.rows[i];
        if(!l->child_handle||!l->raw_object_type||
           l->child_handle!=t->child_handle||
           l->raw_object_type!=t->raw_object_type||
           t->text_and_display_accepted)return 0;
        r->child_handle=l->child_handle;
        r->raw_object_type=l->raw_object_type;
        uint64_t owner=0u,layer=0u;
        int verified=0;
        switch(r->raw_object_type){
        case 17u:case 18u:case 19u:case 27u:case 35u:case 77u:{
            MusaDwgAc1032EntityBlockOwnerLayerSource edge={0};
            if(musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
                 scene,r->child_handle,&edge)&&
               edge.child_handle==r->child_handle&&
               edge.raw_entity_type==r->raw_object_type&&
               edge.original_child_record_crc_verified&&
               edge.original_owner_and_layer_records_crc_verified&&
               edge.exact_h4_owner_and_h5_layer_source_verified&&
               !edge.visible_scene_and_paint_accepted){
                owner=edge.owner_block_header_handle;
                layer=edge.layer_handle;
                verified=1;
            }
            break;
        }
        case 1u:case 44u:{
            if(t->source_h_graph_verified&&
               t->source_owner_handle&&t->source_layer_handle&&
               t->source_style_handle){
                uint16_t owner_type=0u,layer_type=0u;
                if(musa_dwg_ac1032_scene_index_object_type(
                      scene,t->source_owner_handle,&owner_type)&&
                   owner_type==49u&&
                   musa_dwg_ac1032_scene_index_object_type(
                      scene,t->source_layer_handle,&layer_type)&&
                   layer_type==51u){
                    owner=t->source_owner_handle;
                    layer=t->source_layer_handle;
                    verified=1;
                }
            }
            break;
        }
        default:break;
        }
        if(verified){
            if(!owner||!layer||owner==r->child_handle||
               layer==r->child_handle||owner==layer)return 0;
            /* A strict source H5 layer must agree with independent OT51
             * provenance if that exact child's profile was admitted. */
            if(l->layer_evidence_status==
                 MUSACAD_AC1032_BLOCK_LAYER_VERIFIED&&
               l->layer_handle!=layer)return 0;
            r->verified_source_owner_handle=owner;
            r->verified_source_layer_handle=layer;
            r->strict_reverse_edge_source_verified=1u;
            v.verified_reverse_source_count++;
            if(owner==block_handle){
                r->reverse_owner_matches_block=1u;
                v.matching_reverse_owner_count++;
            }else{
                r->reverse_owner_conflicts_block=1u;
                v.foreign_owner_count++;
            }
        }else v.unresolved_owner_count++;
    }
    if(v.verified_reverse_source_count+v.unresolved_owner_count!=v.child_count||
       v.matching_reverse_owner_count+v.foreign_owner_count!=
          v.verified_reverse_source_count)return 0;
    v.block_child_h_crc_types_verified=1u;
    *out=v;
    return 1;
}
#endif
