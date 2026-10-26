/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_INSTANCE_SOURCE_PAIR_H
#define MUSACAD_DWG_AC1032_INSERT_INSTANCE_SOURCE_PAIR_H
#include "musacad_dwg_ac1032_insert_child_source_join.h"
#include <stdint.h>

/* Two original INSERT instances of the SAME original OT49 block must have
 * the same child H/type, source OT51 layer and raw color evidence. Their
 * world coordinates remain INSTANCE-SPECIFIC and are not compared here.
 * This does not prove nested INSERT expansion, ownership or visibility. */
typedef struct MusaDwgAc1032InsertInstanceSourcePair {
    uint64_t first_insert,second_insert,shared_block_header;
    uint32_t matched_child_sources;
    uint32_t first_mapped,second_mapped;
    uint8_t original_child_provenance_instance_invariant;
    uint8_t distinct_transform_verified; /* ALWAYS ZERO */
    uint8_t owner_and_visibility_verified; /* ALWAYS ZERO */
    uint8_t final_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032InsertInstanceSourcePair;

/* Fail atomic on a different source block, mismatched child H/OT/layer,
 * raw entity color or style classification, or invalid input. */
static inline int musa_dwg_ac1032_insert_instance_source_pair(
    const MusaDwgAc1032SceneIndex *scene,
    uint64_t first,uint64_t second,
    MusaDwgAc1032InsertInstanceSourcePair *out){
    if(!scene||!out||!first||!second||first==second)return 0;
    MusaDwgAc1032InsertChildSourceJoin a={0},b={0};
    if(!musa_dwg_ac1032_insert_child_source_join(scene,first,&a)||
       !musa_dwg_ac1032_insert_child_source_join(scene,second,&b)||
       !a.original_insert_block_child_source_join_verified||
       !b.original_insert_block_child_source_join_verified||
       a.reverse_ownership_verified||b.reverse_ownership_verified||
       a.space_and_viewport_verified||b.space_and_viewport_verified||
       a.final_paint_accepted||b.final_paint_accepted||
       !a.block_header_handle||a.block_header_handle!=b.block_header_handle||
       a.child_count!=b.child_count||
       a.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    for(uint32_t i=0u;i<a.child_count;i++){
        const MusaDwgAc1032InsertChildSourceRow *x=&a.rows[i],*y=&b.rows[i];
        if(!x->child_handle||x->child_handle!=y->child_handle||
           !x->raw_object_type||x->raw_object_type!=y->raw_object_type||
           x->source_layer_handle!=y->source_layer_handle||
           x->raw_color_index!=y->raw_color_index||
           x->raw_rgb!=y->raw_rgb||
           x->source_style_class!=y->source_style_class||
           x->source_color_status!=y->source_color_status||
           x->original_common_verified!=y->original_common_verified||
           x->original_layer_flags_verified!=y->original_layer_flags_verified||
           x->original_layer_cmc_verified!=y->original_layer_cmc_verified||
           x->raw_has_rgb!=y->raw_has_rgb||
           x->raw_invisibility!=y->raw_invisibility||
           x->raw_has_transparency!=y->raw_has_transparency||
           x->raw_has_color_reference!=y->raw_has_color_reference||
           x->reverse_owner_verified||y->reverse_owner_verified||
           x->visibility_and_paint_accepted||
           y->visibility_and_paint_accepted)return 0;
    }
    MusaDwgAc1032InsertInstanceSourcePair v={0};
    v.first_insert=first;
    v.second_insert=second;
    v.shared_block_header=a.block_header_handle;
    v.matched_child_sources=a.child_count;
    v.first_mapped=a.mapped_count;
    v.second_mapped=b.mapped_count;
    v.original_child_provenance_instance_invariant=1u;
    *out=v;return 1;
}
#endif
