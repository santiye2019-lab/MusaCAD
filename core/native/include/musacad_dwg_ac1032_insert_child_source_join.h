/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_CHILD_SOURCE_JOIN_H
#define MUSACAD_DWG_AC1032_INSERT_CHILD_SOURCE_JOIN_H
#include "musacad_dwg_ac1032_insert_child_coordinate_probe.h"
#include "musacad_dwg_ac1032_block_child_layer_style_provenance.h"
#include <stdint.h>

/* Immutable original R2018 evidence: one INSERT's candidate transformed
 * anchors joined by EXACT child H/type/order to its OT49->OT51 layer and
 * encoded entity color. This is a source inspection result, NOT an entity
 * to draw. Reverse owner, space, frozen/off/viewport, XREF and nested block
 * visibility are not proven. BYLAYER/BYBLOCK never become RGB or ACI7. */
typedef struct MusaDwgAc1032InsertChildSourceRow {
    uint64_t child_handle,source_layer_handle;
    uint16_t raw_object_type,raw_color_index;
    uint32_t raw_rgb;
    double world_anchor[3],world_line_end[3];
    uint8_t coordinate_mapped,line_end_mapped;
    uint8_t source_style_class,source_color_status;
    uint8_t original_common_verified,original_layer_flags_verified;
    uint8_t original_layer_cmc_verified;
    uint8_t raw_has_rgb,raw_invisibility,raw_has_transparency;
    uint8_t raw_has_color_reference;
    uint8_t reverse_owner_verified; /* ALWAYS ZERO */
    uint8_t visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertChildSourceRow;
typedef struct MusaDwgAc1032InsertChildSourceJoin {
    uint64_t insert_handle,block_header_handle;
    uint32_t child_count,mapped_count,unmapped_count;
    uint32_t verified_source_layer_count;
    uint32_t explicit_color_source_count,unresolved_style_count;
    MusaDwgAc1032InsertChildSourceRow rows[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t original_insert_block_child_source_join_verified;
    uint8_t reverse_ownership_verified; /* ALWAYS ZERO */
    uint8_t space_and_viewport_verified; /* ALWAYS ZERO */
    uint8_t final_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertChildSourceJoin;

/* Fail atomic: all three independent original-source readers must agree
 * on the same INSERT, BLOCK_HEADER and every child H/type/order. Never
 * promote a mapped anchor or explicit source color to display permission. */
static inline int musa_dwg_ac1032_insert_child_source_join(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    MusaDwgAc1032InsertChildSourceJoin *out){
    if(!scene||!insert_handle||!out)return 0;
    MusaDwgAc1032InsertChildCoordinateProbe probe={0};
    if(!musa_dwg_ac1032_insert_child_coordinate_probe(
           scene,insert_handle,&probe)||
       !probe.original_source_graph_verified||
       probe.reverse_child_owner_verified||probe.display_accepted||
       probe.insert_handle!=insert_handle||
       !probe.block_header_handle||
       probe.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    MusaDwgAc1032BlockChildLayerStyleProvenance styles={0};
    MusaDwgAc1032BlockChildColorEvidence colors={0};
    if(!musa_dwg_ac1032_block_child_layer_style_provenance(
           scene,probe.block_header_handle,&styles)||
       !musa_dwg_ac1032_block_child_color_evidence(
           scene,probe.block_header_handle,&colors)||
       !styles.original_children_joined||
       !colors.original_child_and_layer_evidence_verified||
       styles.reverse_ownership_accepted||
       styles.layer_visibility_and_paint_accepted||
       colors.reverse_ownership_verified||
       colors.visibility_and_paint_accepted||
       styles.block_header_handle!=probe.block_header_handle||
       colors.block_header_handle!=probe.block_header_handle||
       styles.child_count!=probe.child_count||
       colors.child_count!=probe.child_count)return 0;
    MusaDwgAc1032InsertChildSourceJoin v={0};
    v.insert_handle=insert_handle;
    v.block_header_handle=probe.block_header_handle;
    v.child_count=probe.child_count;
    for(uint32_t i=0u;i<v.child_count;i++){
        const MusaDwgAc1032BlockChildLayerStyleRow *s=&styles.rows[i];
        const MusaDwgAc1032BlockChildColorRow *c=&colors.rows[i];
        MusaDwgAc1032InsertChildSourceRow *r=&v.rows[i];
        const uint64_t h=probe.child_handles[i];
        const uint16_t t=probe.child_types[i];
        if(!h||!t||s->child_handle!=h||c->child_handle!=h||
           s->raw_object_type!=t||c->raw_object_type!=t||
           s->layer_handle!=c->source_layer_handle||
           s->original_entity_common_verified!=c->source_common_verified||
           s->source_style_class!=musa_dwg_ac1032_child_source_style_class(c)||
           s->effective_color_and_visibility_accepted||
           c->final_paint_accepted||
           (probe.line_end_mapped[i]&&
            (!probe.anchor_mapped[i]||t!=19u)))return 0;
        r->child_handle=h;
        r->raw_object_type=t;
        r->source_layer_handle=s->layer_handle;
        r->raw_color_index=c->raw_color_index;
        r->raw_rgb=c->raw_rgb;
        r->source_style_class=s->source_style_class;
        r->source_color_status=c->source_color_status;
        r->original_common_verified=c->source_common_verified;
        r->original_layer_flags_verified=s->original_layer_flags_verified;
        r->original_layer_cmc_verified=s->original_layer_cmc_aci_verified;
        r->raw_has_rgb=c->raw_has_rgb;
        r->raw_invisibility=(uint8_t)(c->raw_invisibility!=0u);
        r->raw_has_transparency=c->raw_has_transparency;
        r->raw_has_color_reference=c->raw_has_color_reference;
        if(r->source_layer_handle)v.verified_source_layer_count++;
        if(r->source_style_class==MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_ACI||
           r->source_style_class==MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_RGB)
            v.explicit_color_source_count++;
        else v.unresolved_style_count++;
        if(probe.anchor_mapped[i]){
            r->coordinate_mapped=1u;
            r->line_end_mapped=probe.line_end_mapped[i];
            for(unsigned k=0u;k<3u;k++){
                r->world_anchor[k]=probe.world_anchor[i][k];
                if(r->line_end_mapped)
                    r->world_line_end[k]=probe.world_line_end[i][k];
            }
            v.mapped_count++;
        }else v.unmapped_count++;
    }
    if(v.mapped_count!=probe.mapped_candidates||
       v.unmapped_count!=probe.unmapped_candidates||
       v.mapped_count+v.unmapped_count!=v.child_count||
       v.explicit_color_source_count+v.unresolved_style_count!=v.child_count||
       v.verified_source_layer_count>v.child_count)return 0;
    v.original_insert_block_child_source_join_verified=1u;
    *out=v;return 1;
}
#endif
