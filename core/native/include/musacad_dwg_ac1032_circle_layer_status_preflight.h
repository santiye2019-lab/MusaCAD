/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_CIRCLE_LAYER_STATUS_PREFLIGHT_H
#define MUSACAD_DWG_AC1032_CIRCLE_LAYER_STATUS_PREFLIGHT_H
#include "musacad_dwg_ac1032_insert_circle_bylayer_crosscheck.h"
#include <stdint.h>
/* Authenticated original INSERT -> CIRCLE -> LAYER status bit preflight.
 * Decode the raw DWG VALUES(70/290/370) candidate bits without mistaking
 * them for DXF group 70 or accepting actual viewport visibility.
 * The original source bitfield is preserved, including unmapped bits.
 * This API is a fail-closed evidence bridge, NOT a renderer gate pass. */
typedef struct MusaDwgAc1032CircleLayerStatusPreflight {
    MusaDwgAc1032InsertCircleByLayerCrosscheck source;
    MusaDwgAc1032LayerBaseBitPreflight candidate_bits;
    uint8_t original_layer_status_bits_crc_verified;
    uint8_t dxf_group70_and_viewport_semantics_verified; /* ALWAYS 0 */
    uint8_t effective_color_and_visible_paint_accepted; /* ALWAYS 0 */
} MusaDwgAc1032CircleLayerStatusPreflight;
static inline int musa_dwg_ac1032_circle_layer_status_preflight(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    uint64_t circle_handle,MusaDwgAc1032CircleLayerStatusPreflight *out){
    if(!scene||!insert_handle||!circle_handle||!out)return 0;
    MusaDwgAc1032InsertCircleByLayerCrosscheck source={0};
    if(!musa_dwg_ac1032_insert_circle_bylayer_crosscheck(
           scene,insert_handle,circle_handle,&source)||
       !source.source_entity_bylayer_and_layer_cmc_agree||
       source.layer_state_and_viewport_verified||
       source.final_visible_paint_accepted||
       !source.original_layer_flags.source_crc_verified||
       source.original_layer_flags.indexed_handle!=source.chain.layer_handle)
        return 0;
    MusaDwgAc1032LayerBaseBitPreflight bits={0};
    if(!musa_dwg_ac1032_layer_base_bits_decode(
           source.original_layer_flags.raw_values_70_290_370,&bits)||
       bits.raw_values!=source.original_layer_flags.raw_values_70_290_370||
       bits.source_crc_verified||bits.display_accepted)return 0;
    /* CRC applies to original flags from the indexed scene, NOT to the
     * numeric-only bit decoder in isolation. */
    bits.layer_handle=source.chain.layer_handle;
    bits.source_crc_verified=1u;
    MusaDwgAc1032CircleLayerStatusPreflight result={0};
    result.source=source;
    result.candidate_bits=bits;
    result.original_layer_status_bits_crc_verified=1u;
    *out=result;
    return 1;
}
#endif
