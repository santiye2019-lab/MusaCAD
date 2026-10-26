/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_text_bundle.h"
#include <string.h>
int musa_dwg_ac1032_scene_text_bundle(
    const MusaDwgAc1032SceneIndex *scene,
    uint64_t indexed_handle,MusaDwgAc1032TextBundle *out){
    if(!scene||!out||!indexed_handle||
       (const void*)scene==(const void*)out)return 0;
    MusaDwgAc1032TextBundle v={0};
    MusaDwgAc1032TextPlacement placement={0};
    MusaDwgAc1032EntityCommon common={0};
    if(!musa_dwg_ac1032_scene_index_read_text_source(
           scene,indexed_handle,&v.unicode)||
       !musa_dwg_ac1032_scene_index_read_text_placement(
           scene,indexed_handle,&placement)||
       !musa_dwg_ac1032_text_placement_world(
           &placement,&v.world)||
       !musa_dwg_ac1032_scene_index_read_text_common(
           scene,indexed_handle,&common)||
       !musa_dwg_ac1032_scene_index_text_graph(
           scene,indexed_handle,&v.graph)||
       !musa_dwg_ac1032_scene_index_read_layer_name(
           scene,v.graph.layer_handle,&v.layer)||
       !musa_dwg_ac1032_scene_planar_color_provenance(
           scene,indexed_handle,&v.color))return 0;
    const unsigned classes=(unsigned)v.color.entity_has_direct_aci+
        (unsigned)v.color.entity_by_layer+
        (unsigned)v.color.entity_by_block+
        (unsigned)v.color.entity_complex_color_or_transparency;
    if(v.unicode.indexed_handle!=indexed_handle||
       !v.unicode.original_record_crc_verified||
       v.unicode.geometry_font_and_display_accepted||
       !v.unicode.utf8_bytes||
       v.unicode.utf8_bytes>=sizeof(v.unicode.utf8)||
       v.unicode.utf8[v.unicode.utf8_bytes]!='\0'||
       strlen(v.unicode.utf8)!=v.unicode.utf8_bytes||
       v.world.indexed_handle!=indexed_handle||
       !v.world.original_record_crc_verified||
       !v.world.wcs_math_decoded||v.world.font_and_display_accepted||
       v.graph.indexed_handle!=indexed_handle||
       !v.graph.graph_edges_verified||
       !v.graph.original_entity_crc_verified||
       !v.graph.original_layer_crc_verified||
       !v.graph.original_style_crc_verified||
       v.graph.layer_object_type!=51u||
       v.graph.style_object_type!=53u||
       v.graph.font_and_display_accepted||
       !v.layer.original_record_crc_verified||
       !v.layer.utf8[0]||
       !common.common_decoded||
       common.prefix.indexed_handle!=indexed_handle||
       common.prefix.raw_object_type!=1u||
       !v.color.entity_and_layer_crc_verified||
       v.color.entity_handle!=indexed_handle||
       v.color.raw_entity_type!=1u||
       v.color.layer_handle!=v.graph.layer_handle||
       v.color.entity_aci!=common.color_index||
       v.color.source_entity_invisible!=
           (uint8_t)(common.invisibility!=0u)||
       v.color.rendering_accepted||
       classes!=1u||
       (v.color.entity_has_direct_rgb24&&
        v.color.entity_source_rgb24>UINT32_C(0x00ffffff)))return 0;
    v.source_join_verified=1u;
    /* Fail-closed: even a source-direct RGB is NOT final display color.
     * No viewport/owner/font acceptance is inferred from source joins. */
    *out=v;
    return 1;
}
