/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_BLOCK_CHILD_COLOR_EVIDENCE_H
#define MUSACAD_DWG_AC1032_BLOCK_CHILD_COLOR_EVIDENCE_H
#include "musacad_dwg_ac1032_block_child_geometry_audit.h"
#include "musacad_dwg_ac1032_block_child_layer_evidence.h"
#include <stdint.h>

/* This is original R2018 source color evidence, NEVER a paint decision.
 * Unknown geometry/layer, BYLAYER, BYBLOCK, hidden, transparency and
 * complex color references are explicitly withheld from direct color.
 * No owner/model-space, viewport, XREF, frozen/off or nested block proof. */
#define MUSACAD_AC1032_CHILD_COLOR_UNRESOLVED 0u
#define MUSACAD_AC1032_CHILD_COLOR_DIRECT_ACI 1u
#define MUSACAD_AC1032_CHILD_COLOR_DIRECT_RGB 2u
#define MUSACAD_AC1032_CHILD_COLOR_INHERITED 3u
#define MUSACAD_AC1032_CHILD_COLOR_HIDDEN 4u

typedef struct MusaDwgAc1032BlockChildColorRow {
    uint64_t child_handle,source_layer_handle;
    uint16_t raw_object_type,raw_color_index,raw_encoded_color;
    uint32_t raw_rgb,raw_invisibility;
    uint8_t raw_has_rgb,raw_has_transparency,raw_has_color_reference;
    uint8_t source_color_status;
    uint8_t source_common_verified;
    uint8_t final_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockChildColorRow;
typedef struct MusaDwgAc1032BlockChildColorEvidence {
    uint64_t block_header_handle;
    uint32_t child_count,direct_aci_count,direct_rgb_count;
    uint32_t inherited_count,hidden_count,unresolved_count;
    MusaDwgAc1032BlockChildColorRow rows[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t original_child_and_layer_evidence_verified;
    uint8_t reverse_ownership_verified; /* ALWAYS ZERO */
    uint8_t visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockChildColorEvidence;

/* Pure classification; a nonzero direct-color result is meaningful only
 * after the caller has authenticated the original entity H/CRC, geometry,
 * and its exact OT51 layer edge. No palette or style is synthesized. */
static inline uint8_t musa_dwg_ac1032_block_child_direct_color_class(
    const MusaDwgAc1032EntityCommon *c){
    if(!c||!c->common_decoded||!c->prefix.prefix_decoded)return
        MUSACAD_AC1032_CHILD_COLOR_UNRESOLVED;
    if(c->invisibility)return MUSACAD_AC1032_CHILD_COLOR_HIDDEN;
    if(c->has_transparency||c->has_color_reference)return
        MUSACAD_AC1032_CHILD_COLOR_UNRESOLVED;
    if(c->has_rgb)return MUSACAD_AC1032_CHILD_COLOR_DIRECT_RGB;
    if(c->color_index==0u||c->color_index==256u)return
        MUSACAD_AC1032_CHILD_COLOR_INHERITED;
    if(c->color_index>=1u&&c->color_index<=255u)return
        MUSACAD_AC1032_CHILD_COLOR_DIRECT_ACI;
    return MUSACAD_AC1032_CHILD_COLOR_UNRESOLVED;
}

static inline int musa_dwg_ac1032_block_child_color_evidence(
    const MusaDwgAc1032SceneIndex *scene,uint64_t block_handle,
    MusaDwgAc1032BlockChildColorEvidence *out){
    if(!scene||!block_handle||!out)return 0;
    MusaDwgAc1032BlockChildGeometryAudit geo={0};
    MusaDwgAc1032BlockChildLayerInventory layers={0};
    if(!musa_dwg_ac1032_block_child_geometry_audit(
           scene,block_handle,&geo)||
       !musa_dwg_ac1032_block_child_layer_evidence(
           scene,block_handle,&layers)||
       !geo.original_h_crc_type_verified||
       !layers.marker_and_child_source_crc_verified||
       geo.reverse_ownership_verified||
       layers.reverse_child_owner_links_verified||
       geo.visibility_and_paint_accepted||
       layers.layer_visibility_color_and_paint_accepted||
       geo.child_count!=layers.child_count||
       geo.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    MusaDwgAc1032BlockChildColorEvidence v={0};
    v.block_header_handle=block_handle;
    v.child_count=geo.child_count;
    for(uint32_t i=0u;i<v.child_count;i++){
        MusaDwgAc1032BlockChildColorRow *row=&v.rows[i];
        const MusaDwgAc1032BlockChildLayerRow *lr=&layers.children[i];
        const uint64_t h=geo.child_handles[i];
        const uint16_t t=geo.child_types[i];
        if(!h||!t||h!=lr->child_handle||t!=lr->raw_object_type)
            return 0;
        row->child_handle=h;row->raw_object_type=t;
        if(lr->layer_evidence_status==
               MUSACAD_AC1032_BLOCK_LAYER_VERIFIED){
            if(!lr->layer_handle)return 0;
            row->source_layer_handle=lr->layer_handle;
        }else if(lr->layer_handle||
                 (lr->layer_evidence_status!=
                    MUSACAD_AC1032_BLOCK_LAYER_PROFILE_UNRESOLVED&&
                  lr->layer_evidence_status!=
                    MUSACAD_AC1032_BLOCK_LAYER_TYPE_UNSUPPORTED))
            return 0;
        if(!geo.numeric_geometry_ok[i]||!row->source_layer_handle){
            v.unresolved_count++;continue;
        }
        MusaDwgAc1032EntityCommon common={0};
        int decoded=0;
        switch(t){
        case 19u:{
            MusaDwgAc1032NativeLine x={0};
            decoded=musa_dwg_ac1032_scene_index_read_line(scene,h,&x)&&
                    x.geometry.coordinates_decoded&&
                    x.geometry.indexed_handle==h;
            if(decoded)common=x.common;
            break;
        }
        case 17u:case 18u:{
            MusaDwgAc1032NativeCurve x={0};
            decoded=musa_dwg_ac1032_scene_index_read_curve(scene,h,&x)&&
                    x.geometry_decoded&&x.indexed_handle==h&&
                    x.object_type==t;
            if(decoded)common=x.common;
            break;
        }
        case 35u:{
            MusaDwgAc1032NativeEllipse x={0};
            decoded=musa_dwg_ac1032_scene_index_read_ellipse(scene,h,&x)&&
                    x.geometry_decoded&&x.indexed_handle==h;
            if(decoded)common=x.common;
            break;
        }
        case 27u:{
            MusaDwgAc1032NativePoint x={0};
            decoded=musa_dwg_ac1032_scene_index_read_point(scene,h,&x)&&
                    x.geometry_decoded&&x.indexed_handle==h;
            if(decoded)common=x.common;
            break;
        }
        default:break;
        }
        if(!decoded||!common.common_decoded||
           !common.prefix.prefix_decoded||
           common.prefix.indexed_handle!=h||
           common.prefix.raw_object_type!=t){
            v.unresolved_count++;continue;
        }
        row->source_common_verified=1u;
        row->raw_color_index=common.color_index;
        row->raw_encoded_color=common.encoded_color;
        row->raw_rgb=common.rgb;
        row->raw_invisibility=common.invisibility;
        row->raw_has_rgb=common.has_rgb;
        row->raw_has_transparency=common.has_transparency;
        row->raw_has_color_reference=common.has_color_reference;
        row->source_color_status=
            musa_dwg_ac1032_block_child_direct_color_class(&common);
        switch(row->source_color_status){
        case MUSACAD_AC1032_CHILD_COLOR_DIRECT_ACI:v.direct_aci_count++;break;
        case MUSACAD_AC1032_CHILD_COLOR_DIRECT_RGB:v.direct_rgb_count++;break;
        case MUSACAD_AC1032_CHILD_COLOR_INHERITED:v.inherited_count++;break;
        case MUSACAD_AC1032_CHILD_COLOR_HIDDEN:v.hidden_count++;break;
        default:v.unresolved_count++;break;
        }
    }
    if(v.direct_aci_count+v.direct_rgb_count+v.inherited_count+
       v.hidden_count+v.unresolved_count!=v.child_count)return 0;
    v.original_child_and_layer_evidence_verified=1u;
    *out=v;return 1;
}
#endif
