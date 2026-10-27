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
    uint32_t utf8_bytes,layer_utf8_bytes;
    double insertion[3],text_height;
    /* Preserve the complete independently CRC-backed numeric source records.
     * OT1 is OCS; OT44 is original layout axes, neither is glyph paint. */
    MusaDwgAc1032TextPlacement text_ocs_placement;
    MusaDwgAc1032MTextPlacement mtext_source_placement;
    uint8_t source_numeric_placement_preserved;
    char utf8[2048];
    char layer_utf8[512]; /* Authenticated OT51 name, not visibility/color. */
    uint8_t original_unicode_numeric_h_graph_verified;
    uint8_t original_layer_name_verified;
    uint8_t font_layout_visibility_color_and_paint_accepted; /* ALWAYS 0 */
} MusaDwgAc1032TextSourceBundle;

/* Numeric source must not expose NaN/Inf to any future 2B geometry stage.
 * This is a pure validator, deliberately separate from font/paint approval. */
static inline int musa_dwg_ac1032_text_ocs_numeric_valid(
    const MusaDwgAc1032TextPlacement *p){
    if(!p||!p->original_record_crc_verified||!p->numeric_fields_decoded||
       p->wcs_font_and_display_accepted||!isfinite(p->height)||
       p->height<=0.0||!isfinite(p->width_factor)||
       !isfinite(p->rotation)||!isfinite(p->oblique)||
       !isfinite(p->thickness))return 0;
    for(unsigned k=0u;k<3u;k++)
        if(!isfinite(p->insertion[k])||!isfinite(p->alignment[k])||
           !isfinite(p->extrusion[k]))return 0;
    /* Zero extrusion cannot define an OCS normal, even if every
     * component is finite. Do not publish it to a future 2D layout. */
    return p->extrusion[0]!=0.0||p->extrusion[1]!=0.0||
           p->extrusion[2]!=0.0;
}
static inline int musa_dwg_ac1032_mtext_numeric_valid(
    const MusaDwgAc1032MTextPlacement *p){
    if(!p||!p->original_record_crc_verified||!p->geometry_fields_decoded||
       p->glyph_layout_and_display_accepted||!isfinite(p->text_height)||
       p->text_height<=0.0||!isfinite(p->rect_width)||
       !isfinite(p->rect_height))return 0;
    for(unsigned k=0u;k<3u;k++)
        if(!isfinite(p->insertion[k])||!isfinite(p->extrusion[k])||
           !isfinite(p->x_axis[k]))return 0;
    /* Both directions are required to build an MTEXT source frame.
     * This does not validate orthogonality or approve glyph painting. */
    return (p->extrusion[0]!=0.0||p->extrusion[1]!=0.0||
            p->extrusion[2]!=0.0)&&
           (p->x_axis[0]!=0.0||p->x_axis[1]!=0.0||
            p->x_axis[2]!=0.0);
}

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
           !musa_dwg_ac1032_text_ocs_numeric_valid(&placement)||
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
        result.text_ocs_placement=placement;
        result.source_numeric_placement_preserved=1u;
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
           placement.indexed_handle!=handle||
           graph.indexed_handle!=handle||
           !source.original_record_crc_verified||
           !placement.original_record_crc_verified||
           !placement.geometry_fields_decoded||
           !musa_dwg_ac1032_mtext_numeric_valid(&placement)||
           !graph.graph_edges_verified||
           !graph.original_entity_crc_verified||
           !graph.original_layer_crc_verified||
           !graph.original_style_crc_verified||
           graph.layer_object_type!=51u||
           graph.style_object_type!=53u||
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
        result.mtext_source_placement=placement;
        result.source_numeric_placement_preserved=1u;
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
           layer.utf8,layer.utf8_bytes,sizeof(layer.utf8),0)||
       layer.utf8_bytes>=sizeof(result.layer_utf8))return 0;
    result.layer_utf8_bytes=layer.utf8_bytes;
    memcpy(result.layer_utf8,layer.utf8,layer.utf8_bytes+1u);
    result.original_unicode_numeric_h_graph_verified=1u;
    result.original_layer_name_verified=1u;
    *out=result;
    return 1;
}
#endif
