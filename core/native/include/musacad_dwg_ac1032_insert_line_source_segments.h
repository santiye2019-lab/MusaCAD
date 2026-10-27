/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_LINE_SOURCE_SEGMENTS_H
#define MUSACAD_DWG_AC1032_INSERT_LINE_SOURCE_SEGMENTS_H
#include "musacad_dwg_ac1032_insert_reverse_owner_source_join.h"
#include <float.h>
#include <math.h>
#include <stdint.h>

/* Portable first-party C11 ORIGINAL R2018 INSERT->LINE source packet.
 * Exact H/type/order joins of the separately authenticated INSERT placement,
 * OT49 child candidate list, OT51 layer and strict H4 child->OT49 owner.
 * Only MATCHed LINE children with both genuinely transformed endpoints,
 * finite float-representable planar WCS XY and a verified layer enter rows.
 * FOREIGN/UNRESOLVED owners, 3D/nonfinite lines and unsupported child types
 * are never emitted as line segments. This is a read-only GEOMETRY packet,
 * NOT a draw list: layer freeze/off, viewport, nested INSERT, XREF, font,
 * effective BYLAYER/BYBLOCK color, stroke and final paint are unverified. */
typedef struct MusaDwgAc1032InsertLineSourceRow {
    uint64_t child_handle,verified_block_owner_handle,source_layer_handle;
    double world_start[3],world_end[3];
    uint32_t source_rgb24;
    uint16_t source_color_index;
    uint8_t source_style_class;
    uint8_t original_line_h4_h5_and_world_xy_verified;
    uint8_t final_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertLineSourceRow;

typedef struct MusaDwgAc1032InsertLineSourceSegments {
    uint64_t insert_handle,block_header_handle;
    uint32_t child_count,line_candidate_count,verified_planar_line_count;
    uint32_t unadmitted_line_count,non_line_child_count;
    MusaDwgAc1032InsertLineSourceRow rows[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t original_source_line_packet_verified;
    uint8_t model_viewport_layer_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertLineSourceSegments;

/* Every rejected line is counted, not represented by guessed endpoints or
 * default white color. Caller output is never modified on failure. */
static inline int musa_dwg_ac1032_insert_line_source_segments(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    MusaDwgAc1032InsertLineSourceSegments *out){
    if(!scene||!out||!insert_handle)return 0;
    MusaDwgAc1032InsertChildSourceJoin sources={0};
    MusaDwgAc1032InsertReverseOwnerJoin owners={0};
    if(!musa_dwg_ac1032_insert_child_source_join(
           scene,insert_handle,&sources)||
       !musa_dwg_ac1032_insert_reverse_owner_source_join(
           scene,insert_handle,&owners)||
       !sources.original_insert_block_child_source_join_verified||
       !owners.original_insert_and_reverse_source_join_verified||
       sources.reverse_ownership_verified||sources.space_and_viewport_verified||
       sources.final_paint_accepted||owners.complete_block_ownership_verified||
       owners.space_visibility_and_paint_accepted||
       !sources.block_header_handle||
       sources.block_header_handle!=owners.block_header_handle||
       sources.child_count!=owners.child_count||
       sources.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;

    MusaDwgAc1032InsertLineSourceSegments v={0};
    v.insert_handle=insert_handle;
    v.block_header_handle=sources.block_header_handle;
    v.child_count=sources.child_count;
    for(uint32_t i=0u;i<v.child_count;i++){
        const MusaDwgAc1032InsertChildSourceRow *s=&sources.rows[i];
        const MusaDwgAc1032InsertReverseOwnerRow *o=&owners.rows[i];
        if(!s->child_handle||s->child_handle!=o->child_handle||
           !s->raw_object_type||s->raw_object_type!=o->raw_object_type||
           s->source_layer_handle!=o->source_layer_handle||
           s->source_style_class!=o->source_style_class||
           s->coordinate_mapped!=o->source_coordinate_mapped||
           s->reverse_owner_verified||s->visibility_and_paint_accepted||
           o->final_paint_accepted)return 0;
        if(s->raw_object_type!=19u){
            v.non_line_child_count++;
            continue;
        }
        v.line_candidate_count++;
        if(!s->coordinate_mapped||!s->line_end_mapped||
           !s->original_common_verified||!s->source_layer_handle||
           !o->original_reverse_source_edge_verified||
           o->reverse_owner_status!=MUSACAD_AC1032_INSERT_OWNER_MATCH||
           o->verified_owner_handle!=v.block_header_handle){
            v.unadmitted_line_count++;
            continue;
        }
        int planar=1;
        for(unsigned k=0u;k<3u;k++){
            if(!isfinite(s->world_anchor[k])||
               !isfinite(s->world_line_end[k]))planar=0;
            if(k<2u&&(fabs(s->world_anchor[k])>FLT_MAX||
                         fabs(s->world_line_end[k])>FLT_MAX))planar=0;
        }
        if(fabs(s->world_anchor[2])>1e-9||
           fabs(s->world_line_end[2])>1e-9)planar=0;
        if(!planar){
            v.unadmitted_line_count++;
            continue;
        }
        MusaDwgAc1032InsertLineSourceRow *r=
            &v.rows[v.verified_planar_line_count++];
        r->child_handle=s->child_handle;
        r->verified_block_owner_handle=o->verified_owner_handle;
        r->source_layer_handle=s->source_layer_handle;
        r->source_color_index=s->raw_color_index;
        r->source_rgb24=s->raw_rgb;
        r->source_style_class=s->source_style_class;
        for(unsigned k=0u;k<3u;k++){
            r->world_start[k]=s->world_anchor[k];
            r->world_end[k]=s->world_line_end[k];
        }
        r->original_line_h4_h5_and_world_xy_verified=1u;
    }
    if(v.line_candidate_count+v.non_line_child_count!=v.child_count||
       v.verified_planar_line_count+v.unadmitted_line_count!=
          v.line_candidate_count)return 0;
    v.original_source_line_packet_verified=1u;
    *out=v;
    return 1;
}

/* One ORIGINAL R2018 INSERT->BLOCK->LINE source join with its actual
 * OT51 Unicode layer name and raw layer status. This carries the
 * authenticated raw LINE color fields but does not resolve BYLAYER,
 * BYBLOCK, viewport overrides, frozen/off or final paint. Fail atomic. */
typedef struct MusaDwgAc1032InsertLineLayerSource {
    uint64_t insert_handle,block_header_handle,child_handle,layer_handle;
    MusaDwgAc1032LayerName layer_name;
    MusaDwgAc1032LayerFlagsEvidence raw_layer_flags;
    double world_start[3],world_end[3];
    uint32_t source_rgb24;
    uint16_t source_color_index;
    uint8_t source_style_class;
    uint8_t source_line_layer_and_owner_verified;
    uint8_t final_visibility_color_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertLineLayerSource;
static inline int musa_dwg_ac1032_insert_line_layer_source(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    uint64_t child_handle,MusaDwgAc1032InsertLineLayerSource *out){
    if(!scene||!insert_handle||!child_handle||!out)return 0;
    MusaDwgAc1032InsertLineSourceSegments segments={0};
    if(!musa_dwg_ac1032_insert_line_source_segments(
           scene,insert_handle,&segments)||
       !segments.original_source_line_packet_verified||
       segments.model_viewport_layer_visibility_and_paint_accepted)
        return 0;
    for(uint32_t i=0u;i<segments.verified_planar_line_count;i++){
        const MusaDwgAc1032InsertLineSourceRow *row=&segments.rows[i];
        if(row->child_handle!=child_handle)continue;
        if(!row->original_line_h4_h5_and_world_xy_verified||
           row->final_visibility_and_paint_accepted||
           row->verified_block_owner_handle!=segments.block_header_handle||
           !row->source_layer_handle)return 0;
        MusaDwgAc1032InsertLineLayerSource local={0};
        if(!musa_dwg_ac1032_scene_index_read_layer_name(
               scene,row->source_layer_handle,&local.layer_name)||
           !musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
               scene,row->source_layer_handle,&local.raw_layer_flags)||
           !local.layer_name.original_record_crc_verified||
           !local.raw_layer_flags.source_crc_verified||
           local.layer_name.indexed_handle!=row->source_layer_handle||
           local.raw_layer_flags.indexed_handle!=row->source_layer_handle||
           !local.layer_name.utf8_bytes||
           local.layer_name.utf8_bytes>=sizeof(local.layer_name.utf8)||
           local.layer_name.utf8[local.layer_name.utf8_bytes]!='\0')
            return 0;
        local.insert_handle=insert_handle;
        local.block_header_handle=segments.block_header_handle;
        local.child_handle=child_handle;
        local.layer_handle=row->source_layer_handle;
        local.source_color_index=row->source_color_index;
        local.source_rgb24=row->source_rgb24;
        local.source_style_class=row->source_style_class;
        for(unsigned k=0u;k<3u;k++){
            local.world_start[k]=row->world_start[k];
            local.world_end[k]=row->world_end[k];
        }
        local.source_line_layer_and_owner_verified=1u;
        *out=local;
        return 1;
    }
    return 0;
}
#endif
