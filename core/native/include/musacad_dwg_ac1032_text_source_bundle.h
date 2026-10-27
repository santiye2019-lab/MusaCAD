/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_TEXT_SOURCE_BUNDLE_H
#define MUSACAD_DWG_AC1032_TEXT_SOURCE_BUNDLE_H
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_source_utf8.h"
#include <math.h>
#include <string.h>
#include <stdint.h>

/* Read-only join of ORIGINAL R2018 TEXT (OT1) or MTEXT (OT44):
 * authenticated source Unicode, numeric insertion, H layer/style graph,
 * and independently authenticated OT51 layer name. Never substitute
 * fonts, infer owner/model-space, apply viewport or paint any glyphs.
 * This is a source packet, not a display-ready text object. */
typedef struct MusaDwgAc1032TextSourceBundle {
    uint64_t indexed_handle,owner_handle,layer_handle,style_handle;
    uint16_t raw_object_type;
    uint32_t utf8_bytes;
    double insertion[3],text_height;
    char utf8[2048];
    uint8_t original_unicode_numeric_h_graph_verified;
    uint8_t original_layer_name_verified;
    uint8_t font_layout_visibility_color_and_paint_accepted; /* ALWAYS 0 */
} MusaDwgAc1032TextSourceBundle;

/* Fail-atomic for invalid handle/type, unsupported source profiles,
 * malformed Unicode, nonfinite insertion, or mismatched H/CRC layer.
 * expected_type must be 1 (TEXT) or 44 (MTEXT); no type guessing. */
static inline int musa_dwg_ac1032_text_source_bundle(
    const MusaDwgAc1032SceneIndex *scene,uint64_t handle,
    uint16_t expected_type,MusaDwgAc1032TextSourceBundle *out){
    if(!scene||!handle||!out||
       (expected_type!=1u&&expected_type!=44u))return 0;
    uint16_t actual_type=0u;
    if(!musa_dwg_ac1032_scene_index_object_type(
           scene,handle,&actual_type)||actual_type!=expected_type)return 0;
    MusaDwgAc1032TextSourceBundle result={0};
    result.indexed_handle=handle;
    result.raw_object_type=expected_type;
    if(expected_type==1u){
        MusaDwgAc1032TextSource source={0};
        MusaDwgAc1032TextPlacement placement={0};
        MusaDwgAc1032TextGraph graph={0};
        if(!musa_dwg_ac1032_scene_index_read_text_source(
               scene,handle,&source)||
           !musa_dwg_ac1032_scene_index_read_text_placement(
               scene,handle,&placement)||
           !musa_dwg_ac1032_scene_index_text_graph(
               scene,handle,&graph)||
           source.indexed_handle!=handle||
           placement.indexed_handle!=handle||
           graph.indexed_handle!=handle||
           !source.original_record_crc_verified||
           !placement.original_record_crc_verified||
           !placement.numeric_fields_decoded||
           !graph.graph_edges_verified||
           !graph.original_entity_crc_verified||
           !graph.original_layer_crc_verified||
           !graph.original_style_crc_verified||
           graph.layer_object_type!=51u||
           graph.style_object_type!=53u||
           source.geometry_font_and_display_accepted||
           placement.wcs_font_and_display_accepted||
           graph.font_and_display_accepted||
           !musa_dwg_ac1032_source_utf8_valid(
               source.utf8,source.utf8_bytes,sizeof(source.utf8),1))
            return 0;
        result.owner_handle=graph.owner_handle;
        result.layer_handle=graph.layer_handle;
        result.style_handle=graph.style_handle;
        result.utf8_bytes=source.utf8_bytes;
        memcpy(result.utf8,source.utf8,source.utf8_bytes+1u);
        result.text_height=placement.height;
        for(unsigned k=0u;k<3u;k++)result.insertion[k]=placement.insertion[k];
    }else{
        MusaDwgAc1032MTextSource source={0};
        MusaDwgAc1032MTextPlacement placement={0};
        MusaDwgAc1032MTextGraph graph={0};
        if(!musa_dwg_ac1032_scene_index_read_mtext_source(
               scene,handle,&source)||
           !musa_dwg_ac1032_scene_index_read_mtext_placement(
               scene,handle,&placement)||
           !musa_dwg_ac1032_scene_index_mtext_graph(
               scene,handle,&graph)||
           source.indexed_handle!=handle||
           graph.indexed_handle!=handle||
           !source.original_record_crc_verified||
           !placement.original_record_crc_verified||
           !placement.geometry_fields_decoded||
           !graph.graph_edges_verified||
           !graph.original_entity_crc_verified||
           !graph.original_layer_crc_verified||
           !graph.original_style_crc_verified||
           graph.layer_object_type!=51u||
           source.geometry_style_and_glyphs_accepted||
           placement.glyph_layout_and_display_accepted||
           graph.font_and_display_accepted||
           !musa_dwg_ac1032_source_utf8_valid(
               source.utf8,source.utf8_bytes,sizeof(source.utf8),1))
            return 0;
        result.owner_handle=graph.owner_handle;
        result.layer_handle=graph.layer_handle;
        result.style_handle=graph.style_handle;
        result.utf8_bytes=source.utf8_bytes;
        memcpy(result.utf8,source.utf8,source.utf8_bytes+1u);
        result.text_height=placement.text_height;
        for(unsigned k=0u;k<3u;k++)result.insertion[k]=placement.insertion[k];
    }
    if(!result.layer_handle||!result.style_handle||
       !isfinite(result.text_height)||result.text_height<=0.0)return 0;
    for(unsigned k=0u;k<3u;k++)
        if(!isfinite(result.insertion[k]))return 0;
    MusaDwgAc1032LayerName layer={0};
    if(!musa_dwg_ac1032_scene_index_read_layer_name(
           scene,result.layer_handle,&layer)||
       !layer.original_record_crc_verified||
       layer.indexed_handle!=result.layer_handle||
       !musa_dwg_ac1032_source_utf8_valid(
           layer.utf8,layer.utf8_bytes,sizeof(layer.utf8),0))return 0;
    result.original_unicode_numeric_h_graph_verified=1u;
    result.original_layer_name_verified=1u;
    *out=result;
    return 1;
}
#endif
