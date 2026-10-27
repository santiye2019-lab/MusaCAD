/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_CIRCLE_SOURCE_ENVELOPE_H
#define MUSACAD_DWG_AC1032_CIRCLE_SOURCE_ENVELOPE_H
#include "musacad_dwg_ac1032_circle_bylayer_inventory.h"
#include <math.h>
#include <stdint.h>
/* Authenticated source ACI + analytic WCS XY envelope for ONE original
 * INSERT/CIRCLE instance. This is for source audits and future culling,
 * NOT a renderable scene or proof of effective BYLAYER paint. */
typedef struct MusaDwgAc1032CircleSourceEnvelope {
    uint64_t insert_handle,block_header_handle,circle_handle,layer_handle;
    double min_xy[2],max_xy[2];
    uint16_t original_layer_packed_values;
    uint8_t original_layer_aci;
    uint8_t source_crc_wcs_and_layer_aci_verified;
    uint8_t final_visible_paint_accepted; /* MUST remain zero. */
    uint32_t unresolved_original_dwg_requirements;
} MusaDwgAc1032CircleSourceEnvelope;

/* All failure paths leave *out untouched, including non-finite or
 * overflowed geometry, malformed inventory and unverified source color.
 * Outward nextafter bounds contain floating-point extrema. */
static inline int musa_dwg_ac1032_circle_source_envelope(
    const MusaDwgAc1032CircleByLayerInventory *inventory,
    size_t index,MusaDwgAc1032CircleSourceEnvelope *out){
    if(!inventory||!out||
       !inventory->original_source_inventory_only||
       inventory->final_visible_paint_accepted||
       inventory->verified_source_color_count>
         MUSACAD_AC1032_CIRCLE_BYLAYER_INVENTORY_MAX||
       index>=inventory->verified_source_color_count||
       inventory->verified_source_color_count>
         inventory->circle_candidate_count||
       inventory->unresolved_circle_count!=
         inventory->circle_candidate_count-
           inventory->verified_source_color_count||
       inventory->source_indexed_insert_count>inventory->insert_count||
       inventory->unresolved_insert_count!=
         inventory->insert_count-inventory->source_indexed_insert_count)
        return 0;
    const MusaDwgAc1032CircleByLayerInventoryRow *row=
        &inventory->rows[index];
    if(!row->insert_handle||!row->block_header_handle||
       !row->circle_handle||!row->layer_handle||
       !row->original_layer_aci||
       !row->original_source_crc_and_wcs_verified||
       row->final_visible_paint_accepted)
        return 0;
    MusaDwgAc1032CircleSourceEnvelope value={0};
    value.insert_handle=row->insert_handle;
    value.block_header_handle=row->block_header_handle;
    value.circle_handle=row->circle_handle;
    value.layer_handle=row->layer_handle;
    value.original_layer_aci=row->original_layer_aci;
    value.original_layer_packed_values=row->original_layer_packed_values;
    double u2=0.0,v2=0.0;
    for(unsigned k=0;k<3u;k++){
        const double c=row->world_center[k];
        const double u=row->world_axis_u[k];
        const double v=row->world_axis_v[k];
        if(!isfinite(c)||!isfinite(u)||!isfinite(v))return 0;
        u2=hypot(u2,u);v2=hypot(v2,v);
        if(!isfinite(u2)||!isfinite(v2))return 0;
    }
    if(u2<=0.0||v2<=0.0)return 0;
    for(unsigned k=0;k<2u;k++){
        const double radius=hypot(row->world_axis_u[k],
                                  row->world_axis_v[k]);
        const double lo=row->world_center[k]-radius;
        const double hi=row->world_center[k]+radius;
        if(!isfinite(radius)||!isfinite(lo)||!isfinite(hi)||
           lo>hi)return 0;
        value.min_xy[k]=nextafter(lo,-INFINITY);
        value.max_xy[k]=nextafter(hi,INFINITY);
        if(!isfinite(value.min_xy[k])||
           !isfinite(value.max_xy[k]))return 0;
    }
    value.source_crc_wcs_and_layer_aci_verified=1u;
    value.unresolved_original_dwg_requirements=
        MUSA_CIRCLE_DWG_UNRESOLVED_LAYER_BITS|
        MUSA_CIRCLE_DWG_UNRESOLVED_OWNER_SPACE|
        MUSA_CIRCLE_DWG_UNRESOLVED_NESTED_INSERT|
        MUSA_CIRCLE_DWG_UNRESOLVED_VIEWPORT|
        MUSA_CIRCLE_DWG_UNRESOLVED_XREF;
    *out=value;
    return 1;
}
#endif
