/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_color_provenance.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#define MUSA_LWPROV_INLINE_VERTICES 128u
#define MUSA_LWPROV_MAX_VERTICES 100000u
/* Exact original source object is re-read here, so no caller-provided
 * "color_index" can be mistaken for an independently verified DWG value. */
int musa_dwg_ac1032_scene_planar_color_provenance(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaAc1032ColorProvenance *out){
    if(!s||!h||!out||(const void*)s==(const void*)out)return 0;
    uint16_t type=0u;
    if(!musa_dwg_ac1032_scene_index_object_type(s,h,&type)||
       (type!=1u&&type!=17u&&type!=18u&&type!=19u&&type!=27u&&type!=35u&&
        type!=44u&&type!=77u))
        return 0;
    MusaDwgAc1032EntityCommon common={0};
    if(type==1u){
        /* Real OT1 TEXT must authenticate its OWN TU Unicode, OCS numeric
         * fields and strict source H layer/style graph. Never reuse MTEXT
         * decoding or claim WCS/font/viewport visibility. */
        MusaDwgAc1032TextSource content={0};
        MusaDwgAc1032TextPlacement place={0};
        if(!musa_dwg_ac1032_scene_index_read_text_source(s,h,&content)||
           !musa_dwg_ac1032_scene_index_read_text_placement(s,h,&place)||
           !content.original_record_crc_verified||
           !place.original_record_crc_verified||
           !place.numeric_fields_decoded||
           place.wcs_font_and_display_accepted||
           content.indexed_handle!=h||place.indexed_handle!=h||
           !musa_dwg_ac1032_scene_index_read_text_common(s,h,&common))
            return 0;
    }else if(type==19u){
        MusaDwgAc1032NativeLine line={0};
        if(!musa_dwg_ac1032_scene_index_read_line(s,h,&line))return 0;
        common=line.common;
    }else if(type==27u){
        MusaDwgAc1032NativePoint point={0};
        if(!musa_dwg_ac1032_scene_index_read_point(s,h,&point))return 0;
        common=point.common;
    }else if(type==17u||type==18u){
        MusaDwgAc1032NativeCurve curve={0};
        if(!musa_dwg_ac1032_scene_index_read_curve(s,h,&curve)||
           !curve.geometry_decoded||curve.object_type!=type||
           curve.indexed_handle!=h)return 0;
        common=curve.common;
    }else if(type==44u){
        /* Do not mistake mere type ID for authenticated MTEXT content.
         * Require both original source Unicode TU and numeric placement
         * from the same verified H before admitting style provenance. */
        MusaDwgAc1032MTextSource content={0};
        MusaDwgAc1032MTextPlacement place={0};
        if(!musa_dwg_ac1032_scene_index_read_mtext_source(
               s,h,&content)||
           !musa_dwg_ac1032_scene_index_read_mtext_placement(
               s,h,&place)||
           !content.original_record_crc_verified||
           !place.original_record_crc_verified||
           !place.geometry_fields_decoded||
           content.indexed_handle!=h||place.indexed_handle!=h||
           !musa_dwg_ac1032_scene_index_read_mtext_common(
               s,h,&common))return 0;
    }else if(type==35u){
        /* Same original, CRC/H authenticated numeric ellipse as preview.
         * Do not fabricate its ENC, layer handle or display visibility.
         * Refuse corrupt/unsupported ellipses before exposing color. */
        MusaDwgAc1032NativeEllipse ellipse={0};
        if(!musa_dwg_ac1032_scene_index_read_ellipse(s,h,&ellipse)||
           !ellipse.geometry_decoded||ellipse.indexed_handle!=h)
            return 0;
        common=ellipse.common;
    }else{ /* OT77, source indexed REAL LWPOLYLINE geometry and ENC. */
        MusaDwgAc1032NativeLWPolyline poly={0};
        MusaDwgAc1032LWVertex local[MUSA_LWPROV_INLINE_VERTICES];
        int decoded=musa_dwg_ac1032_scene_index_read_lwpolyline(
            s,h,local,MUSA_LWPROV_INLINE_VERTICES,&poly);
        if(!decoded){
            /* A verified original polyline may have more than 128
             * vertices. Retry up to the same strictly bounded scene
             * geometry ceiling; malformed data still fails closed. */
            MusaDwgAc1032LWVertex *large=(MusaDwgAc1032LWVertex*)
                malloc(MUSA_LWPROV_MAX_VERTICES*sizeof(*large));
            if(!large)return 0;
            decoded=musa_dwg_ac1032_scene_index_read_lwpolyline(
                s,h,large,MUSA_LWPROV_MAX_VERTICES,&poly);
            free(large);
        }
        if(!decoded||!poly.geometry_decoded||poly.indexed_handle!=h||
           poly.vertex_count<2u||
           poly.vertex_count>MUSA_LWPROV_MAX_VERTICES)return 0;
        common=poly.common;
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
    /* DWG common entity field (not the unproved LAYER OT51 flags).
     * Never conclude 'visible' from zero: owner, layer, layout and
     * viewport decisions still require their own authenticated graph. */
    result.source_entity_invisible=(uint8_t)(common.invisibility!=0u);
    result.entity_complex_color_or_transparency=(uint8_t)
        (common.has_rgb||common.has_color_reference||
         common.has_transparency||common.invisibility!=0u);
    /* Source-authenticated direct ENC RGB (0..0xFFFFFF): this is the
     * same narrow no-DBCOLOR/no-alpha/no-invisibility acceptance predicate
     * as the shared-C11 v6 preview source_style(). Do not confuse with
     * final resolved BYLAYER, BYBLOCK or viewport rendering. */
    if(common.has_rgb&&!common.has_color_reference&&
       !common.has_transparency&&common.invisibility==0u&&
       common.rgb<=UINT32_C(0x00ffffff)){
        result.entity_has_direct_rgb24=1u;
        result.entity_source_rgb24=common.rgb;
    }
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


/* The genuine R2018 H1BB CIRCLE uses relative owner H(C), not the
 * simple no-owner entity H4 path. Never relax the generic planar reader
 * for every entity: authenticate this narrower reverse BLOCK membership
 * and layer graph, then report source ENC without claiming visible paint. */
int musa_dwg_ac1032_scene_circle_owned_color_provenance(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaAc1032ColorProvenance *out){
    if(!s||!h||!out||(const void*)s==(const void*)out)return 0;
    MusaDwgAc1032CircleLayerProvenance source={0};
    if(!musa_dwg_ac1032_scene_index_circle_layer_provenance(
            s,h,&source)||
       !source.source_geometry_and_layer_verified||
       source.final_visibility_color_and_paint_accepted||
       source.edge.circle_handle!=h||
       source.edge.raw_circle_object_type!=18u||
       !source.edge.exact_relative_owner_and_layer_source_verified||
       !source.edge.original_forward_block_child_membership_verified||
       source.edge.model_space_viewport_and_paint_accepted||
       !source.edge.layer_handle||
       source.layer_name.indexed_handle!=source.edge.layer_handle||
       !source.layer_name.original_record_crc_verified||
       source.raw_layer_flags.indexed_handle!=source.edge.layer_handle||
       !source.raw_layer_flags.source_crc_verified||
       source.circle.indexed_handle!=h||
       source.circle.object_type!=18u||
       !source.circle.geometry_decoded)return 0;
    const MusaDwgAc1032EntityCommon *common=&source.circle.common;
    if(!common->common_decoded||!common->prefix.prefix_decoded||
       common->prefix.indexed_handle!=h||
       common->prefix.raw_object_type!=18u)return 0;
    MusaAc1032ColorProvenance result={0};
    result.entity_handle=h;
    result.layer_handle=source.edge.layer_handle;
    result.raw_entity_type=18u;
    result.entity_aci=common->color_index;
    result.entity_and_layer_crc_verified=1u;
    result.source_entity_invisible=(uint8_t)(common->invisibility!=0u);
    result.entity_complex_color_or_transparency=(uint8_t)(
        common->has_rgb||common->has_color_reference||
        common->has_transparency||common->invisibility!=0u);
    if(common->has_rgb&&!common->has_color_reference&&
       !common->has_transparency&&common->invisibility==0u&&
       common->rgb<=UINT32_C(0x00ffffff)){
        result.entity_has_direct_rgb24=1u;
        result.entity_source_rgb24=common->rgb;
    }
    if(!result.entity_complex_color_or_transparency){
        result.entity_has_direct_aci=(uint8_t)(
            common->color_index>=1u&&common->color_index<=255u);
        result.entity_by_layer=(uint8_t)(common->color_index==256u);
        result.entity_by_block=(uint8_t)(common->color_index==0u);
    }
    /* A verified LAYER record is not proof of effective BYLAYER color.
     * Only the existing exact C3 CMC source profile may be exposed. */
    MusaDwgAc1032LayerCmcEvidence cmc={0};
    if(musa_dwg_ac1032_scene_index_layer_anchored_cmc_evidence(
          s,source.edge.layer_handle,&cmc)&&
       cmc.indexed_handle==source.edge.layer_handle&&
       cmc.original_record_crc_verified&&
       cmc.aci_index>=1u&&cmc.aci_index<=255u&&
       cmc.packed_color_method==
          (UINT32_C(0xC3000000)|(uint32_t)cmc.aci_index)){
        result.source_layer_aci_supported=1u;
        result.source_layer_aci=cmc.aci_index;
    }
    result.rendering_accepted=0u;
    *out=result;
    return 1;
}

/* Compatibility entry point: preserves the original LINE/POINT-only
 * public contract, including atomic refusal for ARC/CIRCLE handles. */
int musa_dwg_ac1032_scene_line_point_color_provenance(
    const MusaDwgAc1032SceneIndex *s,uint64_t h,
    MusaAc1032ColorProvenance *out){
    if(!s||!out||!h)return 0;
    uint16_t type=0u;
    if(!musa_dwg_ac1032_scene_index_object_type(s,h,&type)||
       (type!=19u&&type!=27u))return 0;
    return musa_dwg_ac1032_scene_planar_color_provenance(s,h,out);
}
