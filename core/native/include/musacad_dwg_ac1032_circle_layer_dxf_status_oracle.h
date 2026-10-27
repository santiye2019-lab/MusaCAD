/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_CIRCLE_LAYER_DXF_STATUS_ORACLE_H
#define MUSACAD_DWG_AC1032_CIRCLE_LAYER_DXF_STATUS_ORACLE_H
#include "musacad_dwg_ac1032_circle_layer_status_preflight.h"
#include <stdint.h>
/* Companion DXF is an INDEPENDENT TEST ORACLE, never a DWG runtime fallback.
 * Signed DXF LAYER 62: negative means OFF; magnitude is the ACI.
 * DXF LAYER 70: bit 0 frozen, bit 1 frozen in new viewports, bit 2 locked.
 * These are NOT the packed R2018 DWG VALUES(70/290/370) bits.  Even when
 * the source color matches, this does NOT certify model-space membership,
 * nested INSERT inheritance, xref state or any viewport override.
 * The caller must separately authenticate and parse the companion DXF. */
typedef struct MusaDwgAc1032CircleLayerDxfOracle {
    uint64_t layer_handle;
    int32_t dxf_group70;
    int32_t dxf_signed_group62;
    uint8_t exactly_one_layer_record;
    uint8_t has_group70;
    uint8_t has_group62;
} MusaDwgAc1032CircleLayerDxfOracle;
typedef struct MusaDwgAc1032CircleLayerDxfStatusEvidence {
    uint64_t layer_handle;
    uint16_t original_dwg_packed_values;
    uint8_t original_dwg_crc_and_chain_verified;
    uint8_t dxf_layer_aci_matches_source;
    uint8_t external_dxf_frozen;
    uint8_t external_dxf_frozen_in_new_viewports;
    uint8_t external_dxf_locked;
    uint8_t external_dxf_off;
    uint8_t dwg_layer_visibility_semantics_verified; /* ALWAYS ZERO */
    uint8_t viewport_and_final_paint_accepted;       /* ALWAYS ZERO */
} MusaDwgAc1032CircleLayerDxfStatusEvidence;
/* Fail-atomic: unsupported source, malformed external oracle, color/handle
 * mismatch leave caller output untouched.  Never map DXF status to a
 * DWG source bit position based on a single fixture. */
static inline int musa_dwg_ac1032_circle_layer_dxf_status_oracle(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    uint64_t circle_handle,const MusaDwgAc1032CircleLayerDxfOracle *oracle,
    MusaDwgAc1032CircleLayerDxfStatusEvidence *out){
    if(!scene||!insert_handle||!circle_handle||!oracle||!out||
       !oracle->exactly_one_layer_record||!oracle->has_group70||
       !oracle->has_group62||!oracle->layer_handle||
       oracle->dxf_group70<0||oracle->dxf_group70>127||
       (oracle->dxf_group70&8)!=0||
       oracle->dxf_signed_group62==0||
       oracle->dxf_signed_group62 < -255||
       oracle->dxf_signed_group62 > 255)return 0;
    MusaDwgAc1032CircleLayerStatusPreflight source={0};
    if(!musa_dwg_ac1032_circle_layer_status_preflight(
          scene,insert_handle,circle_handle,&source)||
       !source.original_layer_status_bits_crc_verified||
       !source.source.source_entity_bylayer_and_layer_cmc_agree||
       !source.source.circle_color.entity_by_layer||
       source.source.circle_color.source_entity_invisible||
       source.dxf_group70_and_viewport_semantics_verified||
       source.effective_color_and_visible_paint_accepted||
       source.candidate_bits.display_accepted||
       oracle->layer_handle!=source.source.chain.layer_handle)return 0;
    int32_t magnitude=oracle->dxf_signed_group62<0?
        -oracle->dxf_signed_group62:oracle->dxf_signed_group62;
    if(magnitude!=(int32_t)source.source.chain.original_layer_aci)
        return 0;
    MusaDwgAc1032CircleLayerDxfStatusEvidence v={0};
    v.layer_handle=oracle->layer_handle;
    v.original_dwg_packed_values=source.candidate_bits.raw_values;
    v.original_dwg_crc_and_chain_verified=1u;
    v.dxf_layer_aci_matches_source=1u;
    v.external_dxf_frozen=(uint8_t)((oracle->dxf_group70&1)!=0);
    v.external_dxf_frozen_in_new_viewports=
        (uint8_t)((oracle->dxf_group70&2)!=0);
    v.external_dxf_locked=(uint8_t)((oracle->dxf_group70&4)!=0);
    v.external_dxf_off=(uint8_t)(oracle->dxf_signed_group62<0);
    *out=v;
    return 1;
}
#endif
