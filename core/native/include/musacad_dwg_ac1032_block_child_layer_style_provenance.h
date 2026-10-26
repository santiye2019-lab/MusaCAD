/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_BLOCK_CHILD_LAYER_STYLE_PROVENANCE_H
#define MUSACAD_DWG_AC1032_BLOCK_CHILD_LAYER_STYLE_PROVENANCE_H
#include "musacad_dwg_ac1032_block_child_color_evidence.h"
#include <stdint.h>

/* An ORIGINAL child's encoded color semantics are NOT one inheritance mode:
 * DWG index 0 is BYBLOCK, index 256 is BYLAYER. The source layer's R2018
 * 70/290/370 numeric flags may not independently prove viewport visibility.
 * An authenticated default-profile layer CMC ACI is SOURCE evidence only.
 * Never synthesize ACI7, effective RGB, owner, plot or final paint. */
#define MUSACAD_AC1032_SOURCE_STYLE_UNRESOLVED 0u
#define MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_ACI 1u
#define MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_RGB 2u
#define MUSACAD_AC1032_SOURCE_STYLE_BYLAYER 3u
#define MUSACAD_AC1032_SOURCE_STYLE_BYBLOCK 4u
#define MUSACAD_AC1032_SOURCE_STYLE_HIDDEN 5u

typedef struct MusaDwgAc1032BlockChildLayerStyleRow {
    uint64_t child_handle,layer_handle;
    uint16_t raw_object_type,raw_entity_color_index;
    uint16_t source_layer_raw_flags;
    uint16_t source_layer_xref_index_plus_one;
    uint8_t source_layer_xref_dependent;
    uint8_t source_layer_entry_64_flag;
    uint8_t original_entity_common_verified;
    uint8_t original_layer_flags_verified;
    uint8_t original_layer_cmc_aci_verified;
    uint8_t original_layer_cmc_aci;
    uint8_t source_style_class;
    uint8_t effective_color_and_visibility_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockChildLayerStyleRow;

typedef struct MusaDwgAc1032BlockChildLayerStyleProvenance {
    uint64_t block_header_handle;
    uint32_t child_count,source_layer_count;
    uint32_t layer_flags_source_count,layer_cmc_source_count;
    uint32_t explicit_aci_count,explicit_rgb_count;
    uint32_t bylayer_count,byblock_count,hidden_count,unresolved_count;
    MusaDwgAc1032BlockChildLayerStyleRow rows[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t original_children_joined;
    uint8_t reverse_ownership_accepted; /* ALWAYS ZERO */
    uint8_t layer_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockChildLayerStyleProvenance;

/* Interpret ONLY a separately source-verified child common profile;
 * inherited BYBLOCK/BYLAYER remain distinctly unresolved effective colors.
 * A hidden/transparent/complex source is never promoted to direct paint. */
static inline uint8_t musa_dwg_ac1032_child_source_style_class(
    const MusaDwgAc1032BlockChildColorRow *color){
    if(!color||!color->source_common_verified||!color->source_layer_handle||
       color->final_paint_accepted)
        return MUSACAD_AC1032_SOURCE_STYLE_UNRESOLVED;
    switch(color->source_color_status){
    case MUSACAD_AC1032_CHILD_COLOR_DIRECT_ACI:
        return MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_ACI;
    case MUSACAD_AC1032_CHILD_COLOR_DIRECT_RGB:
        return MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_RGB;
    case MUSACAD_AC1032_CHILD_COLOR_HIDDEN:
        return MUSACAD_AC1032_SOURCE_STYLE_HIDDEN;
    case MUSACAD_AC1032_CHILD_COLOR_INHERITED:
        if(color->raw_color_index==256u)
            return MUSACAD_AC1032_SOURCE_STYLE_BYLAYER;
        if(color->raw_color_index==0u)
            return MUSACAD_AC1032_SOURCE_STYLE_BYBLOCK;
        return MUSACAD_AC1032_SOURCE_STYLE_UNRESOLVED;
    default:
        return MUSACAD_AC1032_SOURCE_STYLE_UNRESOLVED;
    }
}

/* Read-only source inventory; every original child H/OT joins BOTH the
 * verified source layer list and independently decoded entity color.
 * Original OT51 flags and the narrow anchored CMC are optional strict
 * decoders: a rejected profile stays unknown instead of becoming visible,
 * white or Layer0. This does not prove owner/model-space/viewport/XREF. */
static inline int musa_dwg_ac1032_block_child_layer_style_provenance(
    const MusaDwgAc1032SceneIndex *scene,uint64_t block_header_handle,
    MusaDwgAc1032BlockChildLayerStyleProvenance *out){
    if(!scene||!out||!block_header_handle)return 0;
    MusaDwgAc1032BlockChildLayerInventory layers={0};
    MusaDwgAc1032BlockChildColorEvidence colors={0};
    if(!musa_dwg_ac1032_block_child_layer_evidence(
           scene,block_header_handle,&layers)||
       !musa_dwg_ac1032_block_child_color_evidence(
           scene,block_header_handle,&colors)||
       !layers.marker_and_child_source_crc_verified||
       !colors.original_child_and_layer_evidence_verified||
       layers.reverse_child_owner_links_verified||
       layers.layer_visibility_color_and_paint_accepted||
       colors.reverse_ownership_verified||
       colors.visibility_and_paint_accepted||
       layers.block_header_handle!=block_header_handle||
       colors.block_header_handle!=block_header_handle||
       layers.child_count!=colors.child_count||
       layers.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    MusaDwgAc1032BlockChildLayerStyleProvenance v={0};
    v.block_header_handle=block_header_handle;
    v.child_count=layers.child_count;
    for(uint32_t i=0u;i<v.child_count;i++){
        const MusaDwgAc1032BlockChildLayerRow *l=&layers.children[i];
        const MusaDwgAc1032BlockChildColorRow *c=&colors.rows[i];
        MusaDwgAc1032BlockChildLayerStyleRow *r=&v.rows[i];
        if(!l->child_handle||!l->raw_object_type||
           l->child_handle!=c->child_handle||
           l->raw_object_type!=c->raw_object_type||
           c->final_paint_accepted||
           (l->layer_evidence_status==MUSACAD_AC1032_BLOCK_LAYER_VERIFIED&&
              (!l->layer_handle||c->source_layer_handle!=l->layer_handle))||
           (l->layer_evidence_status!=MUSACAD_AC1032_BLOCK_LAYER_VERIFIED&&
              (l->layer_handle||c->source_layer_handle)))
            return 0;
        r->child_handle=l->child_handle;
        r->raw_object_type=l->raw_object_type;
        r->layer_handle=l->layer_handle;
        r->original_entity_common_verified=c->source_common_verified;
        r->raw_entity_color_index=c->raw_color_index;
        r->source_style_class=musa_dwg_ac1032_child_source_style_class(c);
        if(r->layer_handle){
            v.source_layer_count++;
            MusaDwgAc1032LayerFlagsEvidence flags={0};
            if(musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
                   scene,r->layer_handle,&flags)&&
               flags.source_crc_verified&&
               flags.indexed_handle==r->layer_handle){
                r->original_layer_flags_verified=1u;
                r->source_layer_raw_flags=flags.raw_values_70_290_370;
                r->source_layer_xref_index_plus_one=flags.xref_index_plus_one;
                r->source_layer_xref_dependent=flags.xref_dependent;
                r->source_layer_entry_64_flag=flags.entry_64_flag;
                v.layer_flags_source_count++;
            }
            MusaDwgAc1032LayerCmcEvidence cmc={0};
            if(musa_dwg_ac1032_scene_index_layer_anchored_cmc_evidence(
                   scene,r->layer_handle,&cmc)&&
               cmc.original_record_crc_verified&&
               cmc.indexed_handle==r->layer_handle&&
               cmc.aci_index!=0u){
                r->original_layer_cmc_aci_verified=1u;
                r->original_layer_cmc_aci=cmc.aci_index;
                v.layer_cmc_source_count++;
            }
        }
        switch(r->source_style_class){
        case MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_ACI:
            v.explicit_aci_count++;break;
        case MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_RGB:
            v.explicit_rgb_count++;break;
        case MUSACAD_AC1032_SOURCE_STYLE_BYLAYER:
            v.bylayer_count++;break;
        case MUSACAD_AC1032_SOURCE_STYLE_BYBLOCK:
            v.byblock_count++;break;
        case MUSACAD_AC1032_SOURCE_STYLE_HIDDEN:
            v.hidden_count++;break;
        default:v.unresolved_count++;break;
        }
    }
    if(v.source_layer_count>v.child_count||
       v.layer_flags_source_count>v.source_layer_count||
       v.layer_cmc_source_count>v.source_layer_count||
       v.explicit_aci_count+v.explicit_rgb_count+
       v.bylayer_count+v.byblock_count+v.hidden_count+
       v.unresolved_count!=v.child_count)return 0;
    v.original_children_joined=1u;
    *out=v;
    return 1;
}
#endif
