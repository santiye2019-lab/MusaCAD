/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_BLOCK_CHILD_TEXT_SOURCE_INVENTORY_H
#define MUSACAD_DWG_AC1032_BLOCK_CHILD_TEXT_SOURCE_INVENTORY_H
#include "musacad_dwg_ac1032_block_child_layer_evidence.h"
#include "musacad_dwg_ac1032_source_utf8.h"
#include <stdint.h>

/* CRC/H-authenticated OT49 child candidates are classified individually.
 * TEXT (OT1) and MTEXT (OT44) use the already existing strict first-party
 * source Unicode, numeric placement and H-graph readers independently.
 * A rejected profile is reported, never replaced with placeholder text.
 * The original UTF8 is available from the per-entity reader, not copied
 * into every inventory row. No fonts, glyphs, viewport or final paint. */
typedef struct MusaDwgAc1032BlockTextChildRow {
    uint64_t child_handle,source_owner_handle;
    uint64_t source_layer_handle,source_style_handle;
    uint32_t source_utf8_bytes;
    uint16_t raw_object_type;
    uint8_t utf8_source_verified;
    uint8_t numeric_placement_verified;
    uint8_t source_h_graph_verified;
    uint8_t reverse_owner_edge_matches_block;
    uint8_t source_layer_edge_matches;
    uint8_t text_and_display_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockTextChildRow;

typedef struct MusaDwgAc1032BlockChildTextSourceInventory {
    uint64_t block_header_handle;
    uint32_t child_count,text_family_count;
    uint32_t utf8_source_count,numeric_placement_count,h_graph_count;
    uint32_t matching_owner_edge_count,matching_layer_edge_count;
    uint32_t profile_unresolved_count;
    MusaDwgAc1032BlockTextChildRow children[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t original_child_source_graph_verified;
    uint8_t complete_reverse_ownership_verified; /* ALWAYS ZERO */
    uint8_t font_layout_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockChildTextSourceInventory;

/* Fail atomic on corrupt original block/child/layer H/OT evidence.
 * Optional text profiles may fail independently and are counted; a raw
 * OT1/OT44 classification alone is never interpreted as decoded text.
 * A matching child->BLOCK_HEADER H4 source edge is reported only if that
 * exact first-party graph decoder accepted it, and is NOT permission to
 * expand/render the block or a proof for any other child family. */
static inline int musa_dwg_ac1032_block_child_text_source_inventory(
    const MusaDwgAc1032SceneIndex *scene,uint64_t block_header_handle,
    MusaDwgAc1032BlockChildTextSourceInventory *out){
    if(!scene||!out||!block_header_handle)return 0;
    MusaDwgAc1032BlockChildLayerInventory layer={0};
    if(!musa_dwg_ac1032_block_child_layer_evidence(
           scene,block_header_handle,&layer)||
       !layer.marker_and_child_source_crc_verified||
       layer.reverse_child_owner_links_verified||
       layer.layer_visibility_color_and_paint_accepted||
       layer.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT||
       layer.block_header_handle!=block_header_handle)return 0;
    MusaDwgAc1032BlockChildTextSourceInventory v={0};
    v.block_header_handle=block_header_handle;
    v.child_count=layer.child_count;
    for(uint32_t i=0u;i<v.child_count;i++){
        const MusaDwgAc1032BlockChildLayerRow *source=&layer.children[i];
        MusaDwgAc1032BlockTextChildRow *row=&v.children[i];
        const uint64_t h=source->child_handle;
        const uint16_t t=source->raw_object_type;
        uint16_t authentic_type=0u;
        if(!h||!t||!musa_dwg_ac1032_scene_index_object_type(
             scene,h,&authentic_type)||authentic_type!=t)return 0;
        row->child_handle=h;row->raw_object_type=t;
        if(t!=1u&&t!=44u)continue;
        v.text_family_count++;
        uint8_t string_ok=0u,placement_ok=0u,graph_ok=0u;
        uint64_t owner=0u,graph_layer=0u,style=0u;
        if(t==1u){
            MusaDwgAc1032TextSource text={0};
            MusaDwgAc1032TextPlacement placement={0};
            MusaDwgAc1032TextGraph graph={0};
            string_ok=(uint8_t)(
                musa_dwg_ac1032_scene_index_read_text_source(
                  scene,h,&text)&&text.original_record_crc_verified&&
                text.indexed_handle==h&&!text.geometry_font_and_display_accepted&&
                musa_dwg_ac1032_source_utf8_valid(
                    text.utf8,text.utf8_bytes,sizeof(text.utf8),1));
            if(string_ok)row->source_utf8_bytes=text.utf8_bytes;
            placement_ok=(uint8_t)(
                musa_dwg_ac1032_scene_index_read_text_placement(
                  scene,h,&placement)&&placement.original_record_crc_verified&&
                placement.numeric_fields_decoded&&placement.indexed_handle==h&&
                !placement.wcs_font_and_display_accepted);
            graph_ok=(uint8_t)(
                musa_dwg_ac1032_scene_index_text_graph(
                  scene,h,&graph)&&graph.graph_edges_verified&&
                graph.original_entity_crc_verified&&
                graph.original_layer_crc_verified&&
                graph.original_style_crc_verified&&graph.indexed_handle==h&&
                graph.layer_object_type==51u&&graph.style_object_type==53u&&
                !graph.font_and_display_accepted);
            if(graph_ok){
                owner=graph.owner_handle;graph_layer=graph.layer_handle;
                style=graph.style_handle;
            }
        }else{
            MusaDwgAc1032MTextSource text={0};
            MusaDwgAc1032MTextPlacement placement={0};
            MusaDwgAc1032MTextGraph graph={0};
            string_ok=(uint8_t)(
                musa_dwg_ac1032_scene_index_read_mtext_source(
                  scene,h,&text)&&text.original_record_crc_verified&&
                text.indexed_handle==h&&!text.geometry_style_and_glyphs_accepted&&
                musa_dwg_ac1032_source_utf8_valid(
                    text.utf8,text.utf8_bytes,sizeof(text.utf8),1));
            if(string_ok)row->source_utf8_bytes=text.utf8_bytes;
            placement_ok=(uint8_t)(
                musa_dwg_ac1032_scene_index_read_mtext_placement(
                  scene,h,&placement)&&placement.original_record_crc_verified&&
                placement.geometry_fields_decoded&&placement.indexed_handle==h&&
                !placement.glyph_layout_and_display_accepted);
            graph_ok=(uint8_t)(
                musa_dwg_ac1032_scene_index_mtext_graph(
                  scene,h,&graph)&&graph.graph_edges_verified&&
                graph.original_entity_crc_verified&&
                graph.original_layer_crc_verified&&
                graph.original_style_crc_verified&&graph.indexed_handle==h&&
                graph.layer_object_type==51u&&
                !graph.font_and_display_accepted);
            if(graph_ok){
                owner=graph.owner_handle;graph_layer=graph.layer_handle;
                style=graph.style_handle;
            }
        }
        row->utf8_source_verified=string_ok;
        row->numeric_placement_verified=placement_ok;
        row->source_h_graph_verified=graph_ok;
        if(string_ok)v.utf8_source_count++;
        if(placement_ok)v.numeric_placement_count++;
        if(graph_ok){
            v.h_graph_count++;
            row->source_owner_handle=owner;
            row->source_layer_handle=graph_layer;
            row->source_style_handle=style;
            if(owner&&owner==block_header_handle){
                row->reverse_owner_edge_matches_block=1u;
                v.matching_owner_edge_count++;
            }
            if(source->layer_evidence_status==
                  MUSACAD_AC1032_BLOCK_LAYER_VERIFIED&&
               source->layer_handle&&graph_layer==source->layer_handle){
                row->source_layer_edge_matches=1u;
                v.matching_layer_edge_count++;
            }
        }
        if(!string_ok||!placement_ok||!graph_ok)
            v.profile_unresolved_count++;
    }
    if(v.utf8_source_count>v.text_family_count||
       v.numeric_placement_count>v.text_family_count||
       v.h_graph_count>v.text_family_count||
       v.matching_owner_edge_count>v.h_graph_count||
       v.matching_layer_edge_count>v.h_graph_count||
       v.profile_unresolved_count>v.text_family_count)return 0;
    v.original_child_source_graph_verified=1u;
    *out=v;return 1;
}
#endif
