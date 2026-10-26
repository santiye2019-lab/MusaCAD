/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_BLOCK_MARKER_COMMONS_H
#define MUSACAD_DWG_AC1032_BLOCK_MARKER_COMMONS_H
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>

/* Source-only, original R2018 BLOCK/ENDBLK marker evidence.  These marker
 * records are never drawable.  Neither an H prefix nor an entity common
 * proves reverse ownership, space, visibility or a resolved paint style. */
typedef struct MusaDwgAc1032BlockMarkerCommons {
    uint64_t block_header_handle;
    uint64_t block_begin_handle;
    uint64_t endblk_handle;
    uint32_t child_count;
    MusaDwgAc1032EntityCommon begin_common;
    MusaDwgAc1032EntityCommon end_common;
    uint8_t marker_records_crc_verified;
    uint8_t unique_prefix_handles_verified;
    uint8_t reverse_owner_links_verified; /* ALWAYS ZERO */
    uint8_t final_display_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockMarkerCommons;

/* Fail atomic: no output writes until both independent source marker
 * records and the bounded canonical H prefix have passed verification. */
static inline int musa_dwg_ac1032_block_marker_commons(
    const MusaDwgAc1032SceneIndex *scene,uint64_t block_header_handle,
    MusaDwgAc1032BlockMarkerCommons *out){
    if(!scene||!out||!block_header_handle)return 0;
    MusaDwgAc1032BlockHeaderTypedChildren typed={0};
    if(!musa_dwg_ac1032_scene_index_block_header_typed_children(
           scene,block_header_handle,&typed)||
       typed.prefix.indexed_handle!=block_header_handle||
       !typed.prefix.original_record_crc_verified||
       !typed.prefix.canonical_h_roles_verified||
       !typed.all_child_record_types_crc_verified||
       typed.reverse_owner_links_verified||typed.final_display_accepted||
       typed.prefix.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)return 0;
    const uint64_t begin=typed.prefix.block_begin_handle;
    const uint64_t end=typed.prefix.endblk_handle;
    const uint64_t control=typed.prefix.block_control_handle;
    if(!control||!begin||!end||begin==end||
       control==begin||control==end||control==block_header_handle||
       begin==block_header_handle||end==block_header_handle)return 0;
    for(uint32_t i=0u;i<typed.prefix.child_count;i++){
        const uint64_t h=typed.prefix.child_handles[i];
        if(!h||h==control||h==begin||h==end||
           h==block_header_handle)return 0;
        for(uint32_t j=0u;j<i;j++)
            if(h==typed.prefix.child_handles[j])return 0;
    }
    MusaDwgAc1032BlockMarkerCommons value={0};
    if(!musa_dwg_ac1032_scene_index_read_block_common(
           scene,begin,&value.begin_common)||
       !musa_dwg_ac1032_scene_index_read_endblk_common(
           scene,end,&value.end_common)||
       !value.begin_common.common_decoded||
       !value.end_common.common_decoded||
       !value.begin_common.prefix.prefix_decoded||
       !value.end_common.prefix.prefix_decoded||
       value.begin_common.prefix.indexed_handle!=begin||
       value.end_common.prefix.indexed_handle!=end||
       value.begin_common.prefix.raw_object_type!=4u||
       value.end_common.prefix.raw_object_type!=5u)return 0;
    value.block_header_handle=block_header_handle;
    value.block_begin_handle=begin;
    value.endblk_handle=end;
    value.child_count=typed.prefix.child_count;
    value.marker_records_crc_verified=1u;
    value.unique_prefix_handles_verified=1u;
    *out=value;
    return 1;
}
#endif
