/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_BLOCK_PAIR_SOURCE_ISOLATION_H
#define MUSACAD_DWG_AC1032_BLOCK_PAIR_SOURCE_ISOLATION_H
#include "musacad_dwg_ac1032_block_marker_commons.h"
#include <stdint.h>

/* Original R2018 OT49 forward H lists only. Disjointness of two source
 * block records is a necessary integrity check, NOT reverse ownership,
 * model-space membership, nested INSERT expansion, or visibility/paint.
 * A common BLOCK_CONTROL is expected and intentionally not compared. */
typedef struct MusaDwgAc1032BlockPairSourceIsolation {
    uint64_t first_block_header,second_block_header;
    uint64_t first_begin,first_end,second_begin,second_end;
    uint32_t first_children,second_children;
    uint8_t original_records_crc_and_types_verified;
    uint8_t cross_block_source_handles_disjoint;
    uint8_t reverse_ownership_verified; /* ALWAYS ZERO */
    uint8_t display_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockPairSourceIsolation;

/* Fail atomic. Both blocks must be individually authenticated by the
 * existing marker/type readers; then reject any cross-block alias of a
 * header, BLOCK/ENDBLK marker or candidate child. No invented H edges. */
static inline int musa_dwg_ac1032_block_pair_source_isolation(
    const MusaDwgAc1032SceneIndex *scene,
    uint64_t first,uint64_t second,
    MusaDwgAc1032BlockPairSourceIsolation *out){
    if(!scene||!out||!first||!second||first==second)return 0;
    MusaDwgAc1032BlockMarkerCommons a={0},b={0};
    MusaDwgAc1032BlockHeaderTypedChildren ta={0},tb={0};
    if(!musa_dwg_ac1032_block_marker_commons(scene,first,&a)||
       !musa_dwg_ac1032_block_marker_commons(scene,second,&b)||
       !musa_dwg_ac1032_scene_index_block_header_typed_children(
           scene,first,&ta)||
       !musa_dwg_ac1032_scene_index_block_header_typed_children(
           scene,second,&tb)||
       !a.marker_records_crc_verified||!b.marker_records_crc_verified||
       !a.unique_prefix_handles_verified||!b.unique_prefix_handles_verified||
       !ta.all_child_record_types_crc_verified||
       !tb.all_child_record_types_crc_verified||
       a.reverse_owner_links_verified||b.reverse_owner_links_verified||
       ta.reverse_owner_links_verified||tb.reverse_owner_links_verified||
       a.final_display_accepted||b.final_display_accepted||
       ta.final_display_accepted||tb.final_display_accepted||
       ta.prefix.indexed_handle!=first||
       tb.prefix.indexed_handle!=second||
       a.block_header_handle!=first||b.block_header_handle!=second||
       ta.prefix.child_count!=a.child_count||
       tb.prefix.child_count!=b.child_count||
       ta.prefix.block_begin_handle!=a.block_begin_handle||
       tb.prefix.block_begin_handle!=b.block_begin_handle||
       ta.prefix.endblk_handle!=a.endblk_handle||
       tb.prefix.endblk_handle!=b.endblk_handle||
       a.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT||
       b.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    uint64_t left[MUSACAD_AC1032_OT49_CHILD_LIMIT+3u]={0};
    uint64_t right[MUSACAD_AC1032_OT49_CHILD_LIMIT+3u]={0};
    left[0]=first;left[1]=a.block_begin_handle;left[2]=a.endblk_handle;
    right[0]=second;right[1]=b.block_begin_handle;right[2]=b.endblk_handle;
    for(uint32_t i=0u;i<a.child_count;i++){
        if(!ta.prefix.child_handles[i]||!ta.child_object_types[i])return 0;
        left[i+3u]=ta.prefix.child_handles[i];
    }
    for(uint32_t i=0u;i<b.child_count;i++){
        if(!tb.prefix.child_handles[i]||!tb.child_object_types[i])return 0;
        right[i+3u]=tb.prefix.child_handles[i];
    }
    for(uint32_t i=0u;i<a.child_count+3u;i++)
        for(uint32_t j=0u;j<b.child_count+3u;j++)
            if(left[i]==right[j])return 0;
    MusaDwgAc1032BlockPairSourceIsolation result={0};
    result.first_block_header=first;
    result.second_block_header=second;
    result.first_begin=a.block_begin_handle;
    result.first_end=a.endblk_handle;
    result.second_begin=b.block_begin_handle;
    result.second_end=b.endblk_handle;
    result.first_children=a.child_count;
    result.second_children=b.child_count;
    result.original_records_crc_and_types_verified=1u;
    result.cross_block_source_handles_disjoint=1u;
    *out=result;
    return 1;
}
#endif
