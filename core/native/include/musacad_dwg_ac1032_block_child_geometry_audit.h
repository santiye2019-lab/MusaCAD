/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_BLOCK_CHILD_GEOMETRY_AUDIT_H
#define MUSACAD_DWG_AC1032_BLOCK_CHILD_GEOMETRY_AUDIT_H
#include "musacad_dwg_ac1032_block_marker_commons.h"
#include <stdint.h>

/* Original R2018 OT49 H children, authenticated one by one against their
 * original CRC/H/OT and optional native numeric geometry. This is a
 * diagnostic audit, NOT proof of reverse ownership, space, visibility,
 * layer inheritance or permission to draw any child. */
typedef struct MusaDwgAc1032BlockChildGeometryAudit {
    uint64_t block_header_handle;
    uint32_t child_count;
    uint32_t numeric_geometry_verified;
    uint32_t numeric_geometry_rejected;
    uint32_t unsupported_object_types;
    uint64_t child_handles[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint16_t child_types[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t numeric_geometry_ok[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t original_h_crc_type_verified;
    uint8_t reverse_ownership_verified; /* ALWAYS ZERO */
    uint8_t visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockChildGeometryAudit;

/* All-or-nothing output: unsupported/undecodable geometry is explicitly
 * counted, never silently promoted to a drawable entity. The underlying
 * original H/type inventory must pass completely or this call fails. */
static inline int musa_dwg_ac1032_block_child_geometry_audit(
    const MusaDwgAc1032SceneIndex *scene,uint64_t block_header_handle,
    MusaDwgAc1032BlockChildGeometryAudit *out){
    if(!scene||!out||!block_header_handle)return 0;
    MusaDwgAc1032BlockMarkerCommons markers={0};
    MusaDwgAc1032BlockHeaderTypedChildren typed={0};
    if(!musa_dwg_ac1032_block_marker_commons(
           scene,block_header_handle,&markers)||
       !musa_dwg_ac1032_scene_index_block_header_typed_children(
           scene,block_header_handle,&typed)||
       !markers.marker_records_crc_verified||
       !markers.unique_prefix_handles_verified||
       markers.reverse_owner_links_verified||markers.final_display_accepted||
       !typed.all_child_record_types_crc_verified||
       typed.reverse_owner_links_verified||typed.final_display_accepted||
       markers.child_count!=typed.prefix.child_count||
       typed.prefix.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    MusaDwgAc1032BlockChildGeometryAudit v={0};
    v.block_header_handle=block_header_handle;
    v.child_count=typed.prefix.child_count;
    for(uint32_t i=0u;i<v.child_count;i++){
        const uint64_t h=typed.prefix.child_handles[i];
        const uint16_t t=typed.child_object_types[i];
        if(!h||!t||h==block_header_handle||
           h==markers.block_begin_handle||h==markers.endblk_handle||
           h==typed.prefix.block_control_handle)return 0;
        v.child_handles[i]=h;
        v.child_types[i]=t;
        int decoded=0;
        switch(t){
        case 19u: {
            MusaDwgAc1032NativeLine line={0};
            decoded=musa_dwg_ac1032_scene_index_read_line(scene,h,&line)&&
                    line.geometry.coordinates_decoded&&
                    line.geometry.indexed_handle==h;
            break;
        }
        case 17u: case 18u: {
            MusaDwgAc1032NativeCurve curve={0};
            decoded=musa_dwg_ac1032_scene_index_read_curve(scene,h,&curve)&&
                    curve.geometry_decoded&&curve.indexed_handle==h&&
                    curve.object_type==t;
            break;
        }
        case 35u: {
            MusaDwgAc1032NativeEllipse ellipse={0};
            decoded=musa_dwg_ac1032_scene_index_read_ellipse(
                       scene,h,&ellipse)&&
                    ellipse.geometry_decoded&&ellipse.indexed_handle==h;
            break;
        }
        case 27u: {
            MusaDwgAc1032NativePoint point={0};
            decoded=musa_dwg_ac1032_scene_index_read_point(scene,h,&point)&&
                    point.geometry_decoded&&point.indexed_handle==h;
            break;
        }
        default:
            v.unsupported_object_types++;
            continue;
        }
        if(decoded){
            v.numeric_geometry_ok[i]=1u;
            v.numeric_geometry_verified++;
        }else v.numeric_geometry_rejected++;
    }
    if(v.numeric_geometry_verified+v.numeric_geometry_rejected+
       v.unsupported_object_types!=v.child_count)return 0;
    v.original_h_crc_type_verified=1u;
    *out=v;
    return 1;
}
#endif
