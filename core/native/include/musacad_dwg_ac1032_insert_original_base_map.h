/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_ORIGINAL_BASE_MAP_H
#define MUSACAD_DWG_AC1032_INSERT_ORIGINAL_BASE_MAP_H
#include "musacad_dwg_ac1032_insert_binding.h"

/* Map a caller-supplied BLOCK-local coordinate using the actual R2018
 * CRC/H-verified INSERT -> OT49 BLOCK_HEADER graph and decoded base XYZ.
 * No synthetic zero base, foreign handle, XREF or unauthenticated source.
 * This proves coordinate arithmetic ONLY: block child membership, entity
 * owner/model-space, visibility, nested inserts and paint remain unverified.
 * A failed read must never alter the caller's world coordinate buffer. */
static inline int musa_dwg_ac1032_insert_map_original_block_base(
    const MusaDwgAc1032SceneIndex *scene,
    uint64_t insert_handle,const double local[3],double world[3]){
    if(!scene||!insert_handle||!local||!world)return 0;
    MusaDwgAc1032InsertBinding binding={0};
    MusaDwgAc1032BlockHeaderBase block={0};
    if(!musa_dwg_ac1032_insert_bind_source(
           scene,insert_handle,&binding)||
       !binding.source_binding_verified||
       binding.block_expansion_accepted||
       !musa_dwg_ac1032_scene_index_read_block_header_base(
           scene,binding.graph.block_header_handle,&block)||
       block.indexed_handle!=binding.graph.block_header_handle||
       !block.original_record_crc_verified||!block.base_point_decoded||
       block.block_children_and_display_accepted||
       block.is_xref||block.xref_overlaid||block.xref_dependent||
       block.xref_index_plus_one)return 0;
    MusaDwgAc1032BlockBaseEvidence base={0};
    base.block_header_handle=block.indexed_handle;
    base.base[0]=block.base_xyz[0];
    base.base[1]=block.base_xyz[1];
    base.base[2]=block.base_xyz[2];
    base.original_block_header_crc_verified=1u;
    base.base_point_decoded=1u;
    return musa_dwg_ac1032_insert_map_verified_base(
        &binding,&base,local,world);
}
#endif
