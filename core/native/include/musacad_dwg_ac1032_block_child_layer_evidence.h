/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_BLOCK_CHILD_LAYER_EVIDENCE_H
#define MUSACAD_DWG_AC1032_BLOCK_CHILD_LAYER_EVIDENCE_H
#include "musacad_dwg_ac1032_block_marker_commons.h"
#include <stdint.h>
#include <stddef.h>

/* These classifications describe ORIGINAL source evidence, not paint.
 * 1: the child's source H graph, original layer OT51 and Unicode name agree.
 * 2: its raw type has a known layer reader but its exact H profile rejects.
 * 3: no supported source layer reader exists for this raw object type.
 * Never synthesize layer 0 or ACI 7 for either unresolved classification. */
/* Reject malformed UTF-8 before exposing an OT51 Unicode layer label.
 * The caller supplies the actual array capacity: never probe beyond it.
 * Reject overlong encodings, surrogates, non-scalars, truncation and
 * embedded NUL; a valid name is nonempty and terminated at exactly n. */
static inline int musa_dwg_ac1032_block_layer_utf8_valid(
    const char *text,size_t n,size_t capacity){
    if(!text||!n||n>=capacity)return 0;
    size_t i=0u;
    while(i<n){
        const unsigned char lead=(unsigned char)text[i];
        if(!lead)return 0;
        if(lead<0x80u){i++;continue;}
        uint32_t scalar=0u,min_scalar=0u;
        size_t tails=0u;
        if(lead>=0xc2u&&lead<=0xdfu){
            scalar=(uint32_t)(lead&0x1fu);min_scalar=0x80u;tails=1u;
        }else if(lead>=0xe0u&&lead<=0xefu){
            scalar=(uint32_t)(lead&0x0fu);min_scalar=0x800u;tails=2u;
        }else if(lead>=0xf0u&&lead<=0xf4u){
            scalar=(uint32_t)(lead&0x07u);min_scalar=0x10000u;tails=3u;
        }else return 0;
        if(tails>n-i-1u)return 0;
        for(size_t k=1u;k<=tails;k++){
            const unsigned char tail=(unsigned char)text[i+k];
            if((tail&0xc0u)!=0x80u)return 0;
            scalar=(scalar<<6u)|(uint32_t)(tail&0x3fu);
        }
        if(scalar<min_scalar||scalar>0x10ffffu||
           (scalar>=0xd800u&&scalar<=0xdfffu))return 0;
        i+=tails+1u;
    }
    return text[n]=='\\0';
}

#define MUSACAD_AC1032_BLOCK_LAYER_VERIFIED 1u
#define MUSACAD_AC1032_BLOCK_LAYER_PROFILE_UNRESOLVED 2u
#define MUSACAD_AC1032_BLOCK_LAYER_TYPE_UNSUPPORTED 3u

typedef struct MusaDwgAc1032BlockChildLayerRow {
    uint64_t child_handle;
    uint64_t layer_handle; /* ZERO unless independent source is verified */
    uint16_t raw_object_type;
    uint8_t layer_evidence_status;
} MusaDwgAc1032BlockChildLayerRow;

typedef struct MusaDwgAc1032BlockChildLayerInventory {
    uint64_t block_header_handle;
    uint32_t child_count;
    uint32_t verified_layer_count;
    uint32_t unresolved_profile_count;
    uint32_t unsupported_type_count;
    MusaDwgAc1032BlockChildLayerRow children[MUSACAD_AC1032_OT49_CHILD_LIMIT];
    uint8_t marker_and_child_source_crc_verified;
    uint8_t reverse_child_owner_links_verified; /* ALWAYS ZERO */
    uint8_t layer_visibility_color_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockChildLayerInventory;

/* A local block's bounded, ordered candidate children -> their own
 * independently CRC/H-authenticated layer references and Unicode names.
 * OT49 forward H-prefix is NOT proof of reverse child ownership.
 * Each unsupported/rejected layer is explicitly recorded as unresolved;
 * layer visibility, BYLAYER/BYBLOCK colors, XREF, viewport, block expansion
 * and final paint are never accepted. Fail atomic even on wrong-type H.
 * Read-only portable C11: Android, iOS and Windows, no external CAD codec. */
static inline int musa_dwg_ac1032_block_child_layer_evidence(
    const MusaDwgAc1032SceneIndex *scene,uint64_t block_header_handle,
    MusaDwgAc1032BlockChildLayerInventory *out){
    if(!scene||!out||!block_header_handle)return 0;
    MusaDwgAc1032BlockMarkerCommons markers={0};
    MusaDwgAc1032BlockHeaderTypedChildren typed={0};
    if(!musa_dwg_ac1032_block_marker_commons(
           scene,block_header_handle,&markers)||
       !musa_dwg_ac1032_scene_index_block_header_typed_children(
           scene,block_header_handle,&typed)||
       !markers.marker_records_crc_verified||
       !markers.unique_prefix_handles_verified||
       markers.reverse_owner_links_verified||
       markers.final_display_accepted||
       !typed.all_child_record_types_crc_verified||
       typed.reverse_owner_links_verified||
       typed.final_display_accepted||
       typed.prefix.indexed_handle!=block_header_handle||
       markers.block_header_handle!=block_header_handle||
       typed.prefix.child_count!=markers.child_count||
       typed.prefix.block_begin_handle!=markers.block_begin_handle||
       typed.prefix.endblk_handle!=markers.endblk_handle||
       typed.prefix.child_count>MUSACAD_AC1032_OT49_CHILD_LIMIT)
        return 0;

    MusaDwgAc1032BlockChildLayerInventory result={0};
    result.block_header_handle=block_header_handle;
    result.child_count=typed.prefix.child_count;
    for(uint32_t i=0u;i<result.child_count;i++){
        MusaDwgAc1032BlockChildLayerRow *row=&result.children[i];
        const uint16_t type=typed.child_object_types[i];
        const uint64_t h=typed.prefix.child_handles[i];
        if(!h||!type||type==4u||type==5u)return 0;
        row->child_handle=h;
        row->raw_object_type=type;
        /* Source entity-layer graph does NOT support child owner profiles
         * indiscriminately. A failed exact-profile read is not a color. */
        if(type==1u||type==17u||type==18u||type==19u||
           type==27u||type==35u||type==44u||type==77u){
            MusaDwgAc1032EntityLayerName link={0};
            if(musa_dwg_ac1032_scene_index_entity_layer_name(scene,h,&link)&&
               link.source_graph_and_name_verified&&
               link.edge.entity_handle==h&&
               link.edge.raw_entity_type==type&&
               link.edge.raw_layer_type==51u&&
               link.edge.source_handle_ref_verified&&
               link.edge.original_entity_crc_verified&&
               link.edge.original_layer_crc_verified&&
               link.layer.original_record_crc_verified&&
               link.layer.indexed_handle==link.edge.layer_handle&&
               link.edge.layer_handle&&link.edge.layer_handle!=h&&
               /* A verified OT51 record without a bounded, terminated
                * Unicode layer name is not enough source evidence. */
               link.layer.utf8_bytes>0u&&
               link.layer.utf8_bytes<sizeof(link.layer.utf8)&&
               link.layer.utf8[link.layer.utf8_bytes]=='\0'&&
               musa_dwg_ac1032_block_layer_utf8_valid(
                   link.layer.utf8,link.layer.utf8_bytes,
                   sizeof(link.layer.utf8))){
                row->layer_handle=link.edge.layer_handle;
                row->layer_evidence_status=MUSACAD_AC1032_BLOCK_LAYER_VERIFIED;
                result.verified_layer_count++;
            }else if(type==17u||type==18u||type==19u||
                     type==27u||type==35u||type==77u){
                /* A block-owned geometric entity may have an explicit H4
                 * owner (or an authenticated OT18/OT19 relative H12)
                 * before its H5 layer. The no-owner layer reader must
                 * reject that valid grammar.
                 * Recover only from the independently CRC/type-checked
                 * owner+layer source, and require this exact OT49 parent
                 * plus the original OT51 Unicode name. Never infer
                 * layer 0, BYLAYER color or drawing visibility. */
                MusaDwgAc1032EntityBlockOwnerLayerSource edge={0};
                MusaDwgAc1032LayerName layer={0};
                if(musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
                       scene,h,&edge)&&
                   edge.child_handle==h&&edge.raw_entity_type==type&&
                   edge.owner_block_header_handle==block_header_handle&&
                   edge.layer_handle&&edge.layer_handle!=h&&
                   edge.original_child_record_crc_verified&&
                   edge.original_owner_and_layer_records_crc_verified&&
                   edge.exact_h4_owner_and_h5_layer_source_verified&&
                   !edge.visible_scene_and_paint_accepted&&
                   musa_dwg_ac1032_scene_index_read_layer_name(
                       scene,edge.layer_handle,&layer)&&
                   layer.indexed_handle==edge.layer_handle&&
                   layer.original_record_crc_verified&&
                   layer.utf8_bytes>0u&&
                   layer.utf8_bytes<sizeof(layer.utf8)&&
                   layer.utf8[layer.utf8_bytes]=='\0'&&
                   musa_dwg_ac1032_block_layer_utf8_valid(
                       layer.utf8,layer.utf8_bytes,sizeof(layer.utf8))){
                    row->layer_handle=edge.layer_handle;
                    row->layer_evidence_status=
                        MUSACAD_AC1032_BLOCK_LAYER_VERIFIED;
                    result.verified_layer_count++;
                }else{
                    row->layer_evidence_status=
                        MUSACAD_AC1032_BLOCK_LAYER_PROFILE_UNRESOLVED;
                    result.unresolved_profile_count++;
                }
            }else{
                row->layer_evidence_status=
                    MUSACAD_AC1032_BLOCK_LAYER_PROFILE_UNRESOLVED;
                result.unresolved_profile_count++;
            }
        }else{
            row->layer_evidence_status=
                MUSACAD_AC1032_BLOCK_LAYER_TYPE_UNSUPPORTED;
            result.unsupported_type_count++;
        }
    }
    if(result.verified_layer_count+result.unresolved_profile_count+
       result.unsupported_type_count!=result.child_count)return 0;
    result.marker_and_child_source_crc_verified=1u;
    *out=result;
    return 1;
}
/* Metadata-only, source-authenticated layer label for one original block
 * child. Revalidate OT49 membership and the child's OT51 edge on demand;
 * do not trust a caller-supplied row or infer Layer0 for missing evidence.
 * This is NOT a per-frame rendering API: cache the result in the caller
 * after its source scene is validated. No visibility, color, or paint grant.
 * Failure leaves the caller's output byte-for-byte unchanged. */
typedef struct MusaDwgAc1032BlockChildVerifiedLayerName {
    uint64_t block_header_handle;
    uint64_t child_handle;
    MusaDwgAc1032LayerName layer;
    uint8_t source_graph_and_unicode_verified;
    uint8_t layer_visibility_color_and_paint_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockChildVerifiedLayerName;

static inline int musa_dwg_ac1032_block_child_verified_layer_name(
    const MusaDwgAc1032SceneIndex *scene,uint64_t block_header_handle,
    uint64_t child_handle,MusaDwgAc1032BlockChildVerifiedLayerName *out){
    if(!scene||!out||!block_header_handle||!child_handle)return 0;
    MusaDwgAc1032BlockChildLayerInventory inventory={0};
    if(!musa_dwg_ac1032_block_child_layer_evidence(
           scene,block_header_handle,&inventory)||
       !inventory.marker_and_child_source_crc_verified||
       inventory.reverse_child_owner_links_verified||
       inventory.layer_visibility_color_and_paint_accepted)return 0;
    for(uint32_t i=0u;i<inventory.child_count;i++){
        const MusaDwgAc1032BlockChildLayerRow *row=&inventory.children[i];
        if(row->child_handle!=child_handle)continue;
        if(row->layer_evidence_status!=MUSACAD_AC1032_BLOCK_LAYER_VERIFIED||
           !row->layer_handle||row->layer_handle==child_handle)return 0;
        MusaDwgAc1032LayerName name={0};
        if(!musa_dwg_ac1032_scene_index_read_layer_name(
               scene,row->layer_handle,&name)||
           !name.original_record_crc_verified||
           name.indexed_handle!=row->layer_handle||
           !name.utf8_bytes||name.utf8_bytes>=sizeof(name.utf8)||
           name.utf8[name.utf8_bytes]!='\0'||
           !musa_dwg_ac1032_block_layer_utf8_valid(
               name.utf8,name.utf8_bytes,sizeof(name.utf8)))return 0;
        MusaDwgAc1032BlockChildVerifiedLayerName result={0};
        result.block_header_handle=block_header_handle;
        result.child_handle=child_handle;
        result.layer=name;
        result.source_graph_and_unicode_verified=1u;
        *out=result;
        return 1;
    }
    return 0;
}

#endif
