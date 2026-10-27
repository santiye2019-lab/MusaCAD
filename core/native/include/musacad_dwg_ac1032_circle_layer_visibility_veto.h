/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_CIRCLE_LAYER_VISIBILITY_VETO_H
#define MUSACAD_DWG_AC1032_CIRCLE_LAYER_VISIBILITY_VETO_H
#include "musacad_dwg_ac1032_circle_layer_dxf_status_oracle.h"
#include <stdint.h>
/* This is a diagnostic VETO, never a permission to draw. Companion DXF
 * is an independent test oracle, NOT a runtime DWG fallback. The authentic
 * DWG packed layer values cannot be equated with DXF group 70 flags.
 * No model-space, nested INSERT, viewport override or xref display proof. */
enum {
    MUSA_CIRCLE_DXF_VETO_LAYER_OFF = 1u,
    MUSA_CIRCLE_DXF_VETO_LAYER_FROZEN = 2u
};
enum {
    MUSA_CIRCLE_DXF_RISK_NEW_VIEWPORT_FROZEN = 1u,
    MUSA_CIRCLE_DXF_RISK_XREF_DEPENDENT = 2u,
    MUSA_CIRCLE_DXF_RISK_XREF_RESOLVED = 4u,
    MUSA_CIRCLE_DXF_RISK_REFERENCED = 8u
};
enum {
    MUSA_CIRCLE_DWG_UNRESOLVED_LAYER_BITS = 1u,
    MUSA_CIRCLE_DWG_UNRESOLVED_OWNER_SPACE = 2u,
    MUSA_CIRCLE_DWG_UNRESOLVED_NESTED_INSERT = 4u,
    MUSA_CIRCLE_DWG_UNRESOLVED_VIEWPORT = 8u,
    MUSA_CIRCLE_DWG_UNRESOLVED_XREF = 16u
};
typedef struct MusaDwgAc1032CircleLayerVisibilityVeto {
    uint64_t original_layer_handle;
    uint16_t original_dwg_packed_values;
    uint32_t external_unconditional_veto;
    uint32_t external_conditional_risks;
    uint32_t unresolved_original_dwg_requirements;
    uint8_t original_dwg_chain_crc_verified;
    uint8_t external_dxf_locked_for_editing; /* Lock does not hide paint. */
    uint8_t final_visible_paint_accepted;    /* ALWAYS ZERO. */
} MusaDwgAc1032CircleLayerVisibilityVeto;
/* Fail-atomic: source/independent-oracle rejection leaves output untouched.
 * A positive return proves only a conservative evidence classification. */
static inline int musa_dwg_ac1032_circle_layer_visibility_veto(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    uint64_t circle_handle,const MusaDwgAc1032CircleLayerDxfOracle *oracle,
    MusaDwgAc1032CircleLayerVisibilityVeto *out){
    if(!out||!oracle)return 0;
    MusaDwgAc1032CircleLayerDxfStatusEvidence e={0};
    if(!musa_dwg_ac1032_circle_layer_dxf_status_oracle(
           scene,insert_handle,circle_handle,oracle,&e)||
       !e.original_dwg_crc_and_chain_verified||
       !e.dxf_layer_aci_matches_source||
       e.dwg_layer_visibility_semantics_verified||
       e.viewport_and_final_paint_accepted)return 0;
    MusaDwgAc1032CircleLayerVisibilityVeto v={0};
    v.original_layer_handle=e.layer_handle;
    v.original_dwg_packed_values=e.original_dwg_packed_values;
    v.original_dwg_chain_crc_verified=1u;
    if(e.external_dxf_off)
        v.external_unconditional_veto|=MUSA_CIRCLE_DXF_VETO_LAYER_OFF;
    if(e.external_dxf_frozen)
        v.external_unconditional_veto|=MUSA_CIRCLE_DXF_VETO_LAYER_FROZEN;
    if(e.external_dxf_frozen_in_new_viewports)
        v.external_conditional_risks|=MUSA_CIRCLE_DXF_RISK_NEW_VIEWPORT_FROZEN;
    if((oracle->dxf_group70&16)!=0)
        v.external_conditional_risks|=MUSA_CIRCLE_DXF_RISK_XREF_DEPENDENT;
    if((oracle->dxf_group70&32)!=0)
        v.external_conditional_risks|=MUSA_CIRCLE_DXF_RISK_XREF_RESOLVED;
    if((oracle->dxf_group70&64)!=0)
        v.external_conditional_risks|=MUSA_CIRCLE_DXF_RISK_REFERENCED;
    v.external_dxf_locked_for_editing=e.external_dxf_locked;
    v.unresolved_original_dwg_requirements=
        MUSA_CIRCLE_DWG_UNRESOLVED_LAYER_BITS|
        MUSA_CIRCLE_DWG_UNRESOLVED_OWNER_SPACE|
        MUSA_CIRCLE_DWG_UNRESOLVED_NESTED_INSERT|
        MUSA_CIRCLE_DWG_UNRESOLVED_VIEWPORT|
        MUSA_CIRCLE_DWG_UNRESOLVED_XREF;
    *out=v;
    return 1;
}
#endif
