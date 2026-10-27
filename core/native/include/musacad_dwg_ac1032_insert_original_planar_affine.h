/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_INSERT_ORIGINAL_PLANAR_AFFINE_H
#define MUSACAD_DWG_AC1032_INSERT_ORIGINAL_PLANAR_AFFINE_H
#include "musacad_dwg_ac1032_insert_binding.h"

/* One affine per CRC/H-authenticated original R2018 INSERT -> OT49 source
 * edge, using the *decoded* original block base rather than an assumed
 * origin. Pure numeric evidence; never authorizes drawing a block child.
 * Reverse owner links, model/paper space, nested blocks, XREF visibility,
 * BYBLOCK/BYLAYER, viewport state and final paint remain unverified.
 * Output is unchanged on any failure. */
static inline int musa_dwg_ac1032_insert_original_planar_affine(
    const MusaDwgAc1032SceneIndex *scene,uint64_t insert_handle,
    double out_six[6]){
    if(!scene||!insert_handle||!out_six)return 0;
    MusaDwgAc1032InsertBinding binding={0};
    MusaDwgAc1032BlockHeaderBase block={0};
    if(!musa_dwg_ac1032_insert_bind_source(
           scene,insert_handle,&binding)||
       !binding.source_binding_verified||binding.block_expansion_accepted||
       binding.graph.indexed_handle!=insert_handle||
       binding.placement.indexed_handle!=insert_handle||
       !binding.graph.block_header_handle||
       !musa_dwg_ac1032_scene_index_read_block_header_base(
           scene,binding.graph.block_header_handle,&block)||
       block.indexed_handle!=binding.graph.block_header_handle||
       !block.original_record_crc_verified||!block.base_point_decoded||
       block.block_children_and_display_accepted||
       block.is_xref||block.xref_overlaid||block.xref_dependent||
       block.xref_index_plus_one)return 0;
    return musa_dwg_ac1032_insert_planar_affine(
        &binding.placement,block.base_xyz,out_six);
}
#endif
