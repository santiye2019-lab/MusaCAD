/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_CHILD_COORDINATE_PROBE_H
#define MUSACAD_DWG_AC1032_INSERT_CHILD_COORDINATE_PROBE_H
#include "musacad_dwg_ac1032_block_child_geometry_audit.h"
#include "musacad_dwg_ac1032_insert_original_base_map.h"
#include <stdint.h>

/* Source-backed candidate coordinates, NOT drawable block expansion.
 * OT49 child H list is not yet reverse-owner verified. For LINE both
 * endpoints are mapped; ARC/CIRCLE/ELLIPSE/POINT only the center/point
 * is mapped, never radius, axes, arc sweep or transformed stroke. */
typedef struct MusaDwgAc1032InsertChildCoordinateProbe {
    uint64_t insert_handle,block_header_handle;
    uint32_t child_count;
    uint32_t mapped_candidates;
    uint32_t unmapped_candidates;
    uint64_t child_handles[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint16_t child_types[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    double world_anchor[MUSACAD_AC1032_OT49_CHILD_LIMIT][3];
    double world_line_end[MUSACAD_AC1032_OT49_CHILD_LIMIT][3];
    uint8_t anchor_mapped[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t line_end_mapped[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t original_source_graph_verified;
    uint8_t reverse_child_owner_verified; /* ALWAYS ZERO */
    uint8_t display_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertChildCoordinateProbe;

static inline int musa_dwg_ac1032_insert_child_coordinate_probe(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    MusaDwgAc1032InsertChildCoordinateProbe *out){
    if(!scene||!out||!insert_handle)return 0;
    MusaDwgAc1032InsertSourceGraph graph={0};
    if(!musa_dwg_ac1032_scene_index_insert_source_graph(
           scene,insert_handle,&graph)||
       !graph.original_entity_and_targets_crc_verified||
       !graph.source_edges_verified||
       graph.block_expansion_and_display_accepted||
       graph.indexed_handle!=insert_handle||
       !graph.block_header_handle)return 0;
    MusaDwgAc1032BlockChildGeometryAudit audit={0};
    if(!musa_dwg_ac1032_block_child_geometry_audit(
           scene,graph.block_header_handle,&audit)||
       !audit.original_h_crc_type_verified||
       audit.reverse_ownership_verified||
       audit.visibility_and_paint_accepted||
       audit.block_header_handle!=graph.block_header_handle)return 0;
    MusaDwgAc1032InsertChildCoordinateProbe v={0};
    v.insert_handle=insert_handle;
    v.block_header_handle=graph.block_header_handle;
    v.child_count=audit.child_count;
    for(uint32_t i=0u;i<v.child_count;i++){
        const uint64_t h=audit.child_handles[i];
        const uint16_t t=audit.child_types[i];
        v.child_handles[i]=h;
        v.child_types[i]=t;
        if(!audit.numeric_geometry_ok[i]){
            v.unmapped_candidates++;
            continue;
        }
        double local[3]={0.0,0.0,0.0};
        double local_end[3]={0.0,0.0,0.0};
        int valid=0,has_end=0;
        switch(t){
        case 19u:{
            MusaDwgAc1032NativeLine line={0};
            if(musa_dwg_ac1032_scene_index_read_line(scene,h,&line)&&
               line.geometry.coordinates_decoded&&
               line.geometry.indexed_handle==h){
                for(unsigned k=0u;k<3u;k++){
                    local[k]=line.geometry.start[k];
                    local_end[k]=line.geometry.end[k];
                }
                valid=1;has_end=1;
            }
            break;
        }
        case 17u:case 18u:{
            MusaDwgAc1032NativeCurve curve={0};
            if(musa_dwg_ac1032_scene_index_read_curve(scene,h,&curve)&&
               curve.geometry_decoded&&curve.indexed_handle==h&&
               curve.object_type==t){
                for(unsigned k=0u;k<3u;k++)local[k]=curve.center[k];
                valid=1;
            }
            break;
        }
        case 35u:{
            MusaDwgAc1032NativeEllipse ellipse={0};
            if(musa_dwg_ac1032_scene_index_read_ellipse(scene,h,&ellipse)&&
               ellipse.geometry_decoded&&ellipse.indexed_handle==h){
                for(unsigned k=0u;k<3u;k++)local[k]=ellipse.center[k];
                valid=1;
            }
            break;
        }
        case 27u:{
            MusaDwgAc1032NativePoint point={0};
            if(musa_dwg_ac1032_scene_index_read_point(scene,h,&point)&&
               point.geometry_decoded&&point.indexed_handle==h){
                for(unsigned k=0u;k<3u;k++)local[k]=point.point[k];
                valid=1;
            }
            break;
        }
        default:break;
        }
        double world[3]={0.0,0.0,0.0};
        double world_end[3]={0.0,0.0,0.0};
        if(!valid||
           !musa_dwg_ac1032_insert_map_original_block_base(
              scene,insert_handle,local,world)||
           (has_end&&!musa_dwg_ac1032_insert_map_original_block_base(
              scene,insert_handle,local_end,world_end))){
            v.unmapped_candidates++;
            continue;
        }
        for(unsigned k=0u;k<3u;k++){
            v.world_anchor[i][k]=world[k];
            if(has_end)v.world_line_end[i][k]=world_end[k];
        }
        v.anchor_mapped[i]=1u;
        v.line_end_mapped[i]=(uint8_t)has_end;
        v.mapped_candidates++;
    }
    if(v.mapped_candidates+v.unmapped_candidates!=v.child_count)return 0;
    v.original_source_graph_verified=1u;
    *out=v;
    return 1;
}
#endif
