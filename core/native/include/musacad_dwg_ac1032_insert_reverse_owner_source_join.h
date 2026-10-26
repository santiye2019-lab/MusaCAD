/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_REVERSE_OWNER_SOURCE_JOIN_H
#define MUSACAD_DWG_AC1032_INSERT_REVERSE_OWNER_SOURCE_JOIN_H
#include "musacad_dwg_ac1032_insert_child_source_join.h"
#include "musacad_dwg_ac1032_block_reverse_owner_source_audit.h"
#include <stdint.h>

/* Join two INDEPENDENT original R2018 DWG source proofs by exact child H
 * and OT, preserving the three-valued reverse ownership result:
 * MATCH / FOREIGN / UNRESOLVED. Never turn a candidate H list into a
 * displayable block. Layout, nested ownership, layer visibility, viewport,
 * XREF and final paint remain unverified, even for a matching H4 owner. */
typedef enum MusaDwgAc1032InsertReverseOwnerStatus {
    MUSACAD_AC1032_INSERT_OWNER_UNRESOLVED=0,
    MUSACAD_AC1032_INSERT_OWNER_MATCH=1,
    MUSACAD_AC1032_INSERT_OWNER_FOREIGN=2
} MusaDwgAc1032InsertReverseOwnerStatus;

typedef struct MusaDwgAc1032InsertReverseOwnerRow {
    uint64_t child_handle,source_layer_handle,verified_owner_handle;
    uint16_t raw_object_type;
    uint8_t source_coordinate_mapped;
    uint8_t source_style_class;
    uint8_t reverse_owner_status;
    uint8_t original_reverse_source_edge_verified;
    uint8_t final_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertReverseOwnerRow;

typedef struct MusaDwgAc1032InsertReverseOwnerJoin {
    uint64_t insert_handle,block_header_handle;
    uint32_t child_count,matched_owner_count,foreign_owner_count;
    uint32_t unresolved_owner_count,mapped_candidate_count;
    MusaDwgAc1032InsertReverseOwnerRow rows[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t original_insert_and_reverse_source_join_verified;
    uint8_t complete_block_ownership_verified; /* ALWAYS ZERO */
    uint8_t space_visibility_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertReverseOwnerJoin;

/* Fail-atomic output. Reject even a single conflicting independently
 * verified H5 layer edge; never substitute BYLAYER/BYBLOCK colors. */
static inline int musa_dwg_ac1032_insert_reverse_owner_source_join(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    MusaDwgAc1032InsertReverseOwnerJoin *out){
    if(!scene||!insert_handle||!out)return 0;
    MusaDwgAc1032InsertChildSourceJoin sources={0};
    if(!musa_dwg_ac1032_insert_child_source_join(
           scene,insert_handle,&sources)||
       !sources.original_insert_block_child_source_join_verified||
       sources.reverse_ownership_verified||sources.space_and_viewport_verified||
       sources.final_paint_accepted||!sources.block_header_handle||
       sources.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    MusaDwgAc1032BlockReverseOwnerSourceAudit owners={0};
    if(!musa_dwg_ac1032_block_reverse_owner_source_audit(
           scene,sources.block_header_handle,&owners)||
       !owners.block_child_h_crc_types_verified||
       owners.full_block_reverse_ownership_accepted||
       owners.layout_visibility_and_paint_accepted||
       owners.block_header_handle!=sources.block_header_handle||
       owners.child_count!=sources.child_count)return 0;
    MusaDwgAc1032InsertReverseOwnerJoin v={0};
    v.insert_handle=insert_handle;
    v.block_header_handle=sources.block_header_handle;
    v.child_count=sources.child_count;
    for(uint32_t i=0u;i<v.child_count;i++){
        const MusaDwgAc1032InsertChildSourceRow *s=&sources.rows[i];
        const MusaDwgAc1032BlockReverseOwnerSourceRow *o=&owners.rows[i];
        MusaDwgAc1032InsertReverseOwnerRow *r=&v.rows[i];
        if(!s->child_handle||s->child_handle!=o->child_handle||
           !s->raw_object_type||s->raw_object_type!=o->raw_object_type||
           s->reverse_owner_verified||s->visibility_and_paint_accepted||
           o->child_visible_and_paint_accepted||
           (o->strict_reverse_edge_source_verified&&
            (!o->verified_source_owner_handle||
             !o->verified_source_layer_handle||
             (s->source_layer_handle&&
              s->source_layer_handle!=o->verified_source_layer_handle))))return 0;
        r->child_handle=s->child_handle;
        r->raw_object_type=s->raw_object_type;
        r->source_layer_handle=s->source_layer_handle;
        r->source_coordinate_mapped=s->coordinate_mapped;
        r->source_style_class=s->source_style_class;
        v.mapped_candidate_count+=r->source_coordinate_mapped;
        if(o->strict_reverse_edge_source_verified){
            if(o->reverse_owner_matches_block+
               o->reverse_owner_conflicts_block!=1u||
               o->reverse_owner_matches_block!=
                  (uint8_t)(o->verified_source_owner_handle==
                            sources.block_header_handle))return 0;
            r->verified_owner_handle=o->verified_source_owner_handle;
            r->original_reverse_source_edge_verified=1u;
            if(o->reverse_owner_matches_block){
                r->reverse_owner_status=MUSACAD_AC1032_INSERT_OWNER_MATCH;
                v.matched_owner_count++;
            }else{
                r->reverse_owner_status=MUSACAD_AC1032_INSERT_OWNER_FOREIGN;
                v.foreign_owner_count++;
            }
        }else{
            if(o->verified_source_owner_handle||
               o->verified_source_layer_handle||
               o->reverse_owner_matches_block||
               o->reverse_owner_conflicts_block)return 0;
            r->reverse_owner_status=MUSACAD_AC1032_INSERT_OWNER_UNRESOLVED;
            v.unresolved_owner_count++;
        }
    }
    if(v.mapped_candidate_count!=sources.mapped_count||
       v.matched_owner_count!=owners.matching_reverse_owner_count||
       v.foreign_owner_count!=owners.foreign_owner_count||
       v.unresolved_owner_count!=owners.unresolved_owner_count||
       v.matched_owner_count+v.foreign_owner_count+
          v.unresolved_owner_count!=v.child_count)return 0;
    v.original_insert_and_reverse_source_join_verified=1u;
    *out=v;
    return 1;
}
#endif
