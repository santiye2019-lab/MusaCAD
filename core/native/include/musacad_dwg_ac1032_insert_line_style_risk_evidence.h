/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_LINE_STYLE_RISK_EVIDENCE_H
#define MUSACAD_DWG_AC1032_INSERT_LINE_STYLE_RISK_EVIDENCE_H
#include "musacad_dwg_ac1032_insert_line_source_segments.h"
#include <stdint.h>

/* Read-only ORIGINAL R2018 evidence, never a draw command. A decoded
 * entity's direct ACI/RGB is not sufficient to prove a visible line:
 * BLOCK ownership, model/paper space, layer off/frozen state, XREF,
 * viewport overrides, transparency and nested INSERT remain independent. */
#define MUSACAD_AC1032_LINE_RISK_HIDDEN                UINT32_C(0x0001)
#define MUSACAD_AC1032_LINE_RISK_INHERITED_COLOR       UINT32_C(0x0002)
#define MUSACAD_AC1032_LINE_RISK_UNRESOLVED_COLOR      UINT32_C(0x0004)
#define MUSACAD_AC1032_LINE_RISK_TRANSPARENCY          UINT32_C(0x0008)
#define MUSACAD_AC1032_LINE_RISK_LAYER_FLAGS_UNKNOWN   UINT32_C(0x0010)
#define MUSACAD_AC1032_LINE_RISK_LAYER_VISIBILITY      UINT32_C(0x0020)
#define MUSACAD_AC1032_LINE_RISK_XREF                  UINT32_C(0x0040)
#define MUSACAD_AC1032_LINE_RISK_COLOR_REFERENCE       UINT32_C(0x0080)
#define MUSACAD_AC1032_LINE_RISK_RGB_PROFILE           UINT32_C(0x0100)
#define MUSACAD_AC1032_LINE_RISK_SPACE_VIEWPORT        UINT32_C(0x0200)

typedef struct MusaDwgAc1032InsertLineStyleRiskRow {
    uint64_t child_handle,source_layer_handle;
    uint32_t source_rgb24,risk_mask;
    uint16_t source_aci,raw_layer_flags_70_290_370;
    uint8_t source_style_class,source_color_status;
    uint8_t original_layer_flags_verified,original_layer_cmc_verified;
    uint8_t original_layer_cmc_aci;
    uint8_t source_xref_dependent,source_layer_entry_64_flag;
    uint8_t direct_encoded_color_source_only;
    uint8_t final_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertLineStyleRiskRow;
typedef struct MusaDwgAc1032InsertLineStyleRiskEvidence {
    uint64_t insert_handle,block_header_handle;
    uint32_t source_line_count,direct_encoded_color_source_count;
    uint32_t inherited_or_unresolved_color_count,hidden_source_count;
    MusaDwgAc1032InsertLineStyleRiskRow rows[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t original_source_style_join_verified;
    uint8_t final_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertLineStyleRiskEvidence;

/* Keep risks monotone: the mandatory viewport/layer/space uncertainty
 * bits cannot be cleared by a direct RGB/ACI or layer CMC source value.
 * Layer flags are a RAW 70/290/370 profile, not a visibility boolean. */
static inline uint32_t musa_dwg_ac1032_line_source_risk_mask(
    const MusaDwgAc1032InsertChildSourceRow *s,
    const MusaDwgAc1032BlockChildLayerStyleRow *l){
    if(!s||!l)return MUSACAD_AC1032_LINE_RISK_UNRESOLVED_COLOR|
        MUSACAD_AC1032_LINE_RISK_LAYER_FLAGS_UNKNOWN|
        MUSACAD_AC1032_LINE_RISK_LAYER_VISIBILITY|
        MUSACAD_AC1032_LINE_RISK_SPACE_VIEWPORT;
    uint32_t risk=MUSACAD_AC1032_LINE_RISK_LAYER_VISIBILITY|
                  MUSACAD_AC1032_LINE_RISK_SPACE_VIEWPORT;
    if(!l->original_layer_flags_verified)
        risk|=MUSACAD_AC1032_LINE_RISK_LAYER_FLAGS_UNKNOWN;
    if(l->source_layer_xref_dependent||l->source_layer_xref_index_plus_one)
        risk|=MUSACAD_AC1032_LINE_RISK_XREF;
    if(s->raw_invisibility||
       s->source_style_class==MUSACAD_AC1032_SOURCE_STYLE_HIDDEN)
        risk|=MUSACAD_AC1032_LINE_RISK_HIDDEN;
    if(s->raw_has_transparency)
        risk|=MUSACAD_AC1032_LINE_RISK_TRANSPARENCY;
    if(s->raw_has_color_reference)
        risk|=MUSACAD_AC1032_LINE_RISK_COLOR_REFERENCE;
    if(s->source_style_class==MUSACAD_AC1032_SOURCE_STYLE_BYLAYER||
       s->source_style_class==MUSACAD_AC1032_SOURCE_STYLE_BYBLOCK)
        risk|=MUSACAD_AC1032_LINE_RISK_INHERITED_COLOR;
    else if(s->source_style_class!=MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_ACI&&
            s->source_style_class!=MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_RGB&&
            s->source_style_class!=MUSACAD_AC1032_SOURCE_STYLE_HIDDEN)
        risk|=MUSACAD_AC1032_LINE_RISK_UNRESOLVED_COLOR;
    if(s->source_style_class==MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_RGB&&
       (!s->raw_has_rgb||(s->raw_rgb&UINT32_C(0xff000000))))
        risk|=MUSACAD_AC1032_LINE_RISK_RGB_PROFILE;
    return risk;
}

/* Fail atomic. Join the separately authenticated LINE H4/H5/WCS packet
 * and OT49 source color/layer flags by exact child H, type and order.
 * No fallback color or visibility is inferred from missing evidence. */
static inline int musa_dwg_ac1032_insert_line_style_risk_evidence(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    MusaDwgAc1032InsertLineStyleRiskEvidence *out){
    if(!scene||!out||!insert_handle)return 0;
    MusaDwgAc1032InsertLineSourceSegments lines={0};
    MusaDwgAc1032InsertChildSourceJoin sources={0};
    MusaDwgAc1032BlockChildLayerStyleProvenance styles={0};
    if(!musa_dwg_ac1032_insert_line_source_segments(
           scene,insert_handle,&lines)||
       !musa_dwg_ac1032_insert_child_source_join(
           scene,insert_handle,&sources)||
       !lines.original_source_line_packet_verified||
       !sources.original_insert_block_child_source_join_verified||
       lines.model_viewport_layer_visibility_and_paint_accepted||
       sources.final_paint_accepted||
       !lines.block_header_handle||
       lines.block_header_handle!=sources.block_header_handle||
       lines.child_count!=sources.child_count||
       lines.verified_planar_line_count>MUSACAD_AC1032_OT49_CHILD_LIMIT||
       !musa_dwg_ac1032_block_child_layer_style_provenance(
           scene,lines.block_header_handle,&styles)||
       !styles.original_children_joined||
       styles.layer_visibility_and_paint_accepted||
       styles.child_count!=sources.child_count)return 0;
    MusaDwgAc1032InsertLineStyleRiskEvidence v={0};
    v.insert_handle=insert_handle;
    v.block_header_handle=lines.block_header_handle;
    v.source_line_count=lines.verified_planar_line_count;
    uint32_t line_index=0u;
    for(uint32_t i=0u;i<sources.child_count;i++){
        const MusaDwgAc1032InsertChildSourceRow *s=&sources.rows[i];
        const MusaDwgAc1032BlockChildLayerStyleRow *l=&styles.rows[i];
        if(!s->child_handle||s->child_handle!=l->child_handle||
           s->raw_object_type!=l->raw_object_type||
           s->source_layer_handle!=l->layer_handle||
           s->source_style_class!=l->source_style_class||
           s->original_common_verified!=l->original_entity_common_verified||
           s->original_layer_flags_verified!=l->original_layer_flags_verified||
           s->original_layer_cmc_verified!=l->original_layer_cmc_aci_verified||
           s->visibility_and_paint_accepted||
           l->effective_color_and_visibility_accepted)return 0;
        if(line_index>=v.source_line_count||
           s->child_handle!=lines.rows[line_index].child_handle)continue;
        const MusaDwgAc1032InsertLineSourceRow *line=&lines.rows[line_index];
        if(s->raw_object_type!=19u||!s->original_common_verified||
           !s->source_layer_handle||!line->original_line_h4_h5_and_world_xy_verified||
           line->final_visibility_and_paint_accepted||
           line->source_layer_handle!=s->source_layer_handle||
           line->source_style_class!=s->source_style_class||
           line->source_color_index!=s->raw_color_index||
           line->source_rgb24!=s->raw_rgb)return 0;
        MusaDwgAc1032InsertLineStyleRiskRow *r=&v.rows[line_index++];
        r->child_handle=s->child_handle;
        r->source_layer_handle=s->source_layer_handle;
        r->source_aci=s->raw_color_index;
        r->source_rgb24=s->raw_rgb;
        r->source_style_class=s->source_style_class;
        r->source_color_status=s->source_color_status;
        r->original_layer_flags_verified=l->original_layer_flags_verified;
        r->raw_layer_flags_70_290_370=l->source_layer_raw_flags;
        r->original_layer_cmc_verified=l->original_layer_cmc_aci_verified;
        r->original_layer_cmc_aci=l->original_layer_cmc_aci;
        r->source_xref_dependent=l->source_layer_xref_dependent;
        r->source_layer_entry_64_flag=l->source_layer_entry_64_flag;
        r->risk_mask=musa_dwg_ac1032_line_source_risk_mask(s,l);
        if(r->risk_mask&MUSACAD_AC1032_LINE_RISK_HIDDEN)
            v.hidden_source_count++;
        if(r->risk_mask&(MUSACAD_AC1032_LINE_RISK_INHERITED_COLOR|
                         MUSACAD_AC1032_LINE_RISK_UNRESOLVED_COLOR))
            v.inherited_or_unresolved_color_count++;
        if((s->source_style_class==MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_ACI&&
            s->source_color_status==MUSACAD_AC1032_CHILD_COLOR_DIRECT_ACI&&
            s->raw_color_index>=1u&&s->raw_color_index<=255u)||
           (s->source_style_class==MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_RGB&&
            s->source_color_status==MUSACAD_AC1032_CHILD_COLOR_DIRECT_RGB&&
            s->raw_has_rgb&&!(s->raw_rgb&UINT32_C(0xff000000)))){
            if(!(r->risk_mask&(MUSACAD_AC1032_LINE_RISK_HIDDEN|
                               MUSACAD_AC1032_LINE_RISK_TRANSPARENCY|
                               MUSACAD_AC1032_LINE_RISK_COLOR_REFERENCE|
                               MUSACAD_AC1032_LINE_RISK_RGB_PROFILE))){
                r->direct_encoded_color_source_only=1u;
                v.direct_encoded_color_source_count++;
            }
        }
    }
    if(line_index!=v.source_line_count)return 0;
    v.original_source_style_join_verified=1u;
    *out=v;return 1;
}
#endif
