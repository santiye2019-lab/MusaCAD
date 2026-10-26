/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine unchanged R2018 example_2018.dwg vs independent companion DXF:
 * INSERT H1C8/H190/H756, LAYER/BLOCK_RECORD, ATTRIB and SEQEND handles.
 * Source H/CRC and insertion-point math only. No nested block expansion or visible scene claim. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_insert_binding.h"
#include "musacad_dwg_ac1032_block_header_identity.h"
#include "musacad_dwg_ac1032_insert_transform.h"
#include "musacad_dwg_ac1032_insert_original_base_map.h"
#include "musacad_dwg_ac1032_block_marker_commons.h"
#include "musacad_dwg_ac1032_block_child_layer_evidence.h"
#include "musacad_dwg_ac1032_block_child_geometry_audit.h"
#include "musacad_dwg_ac1032_insert_child_coordinate_probe.h"
#include "musacad_dwg_ac1032_block_child_color_evidence.h"
#include "musacad_dwg_ac1032_block_child_layer_style_provenance.h"
#include "musacad_dwg_ac1032_block_pair_source_isolation.h"
#include "musacad_dwg_ac1032_insert_child_source_join.h"
#include "musacad_dwg_ac1032_insert_instance_source_pair.h"
#include "musacad_dwg_ac1032_block_reverse_owner_source_audit.h"
#include "musacad_dwg_ac1032_block_child_text_source_inventory.h"
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct Expected {
    uint64_t h,layer,block,attrib,seqend;
    uint32_t count;
} Expected;
/* Independent fixed binary oracle: 0xB2 0x6D 0xE0 at bit 3 for
 * 10 bits is 0x93 0x40, not the LSB-first 0x46-style packing. */
static int bit_order_oracle(void){
    const uint8_t source[3]={0xb2u,0x6du,0xe0u};
    uint8_t out[2]={0xa5u,0x5au};
    if(!musa_dwg_ac1032_ot49_copy_msb_bits(
          source,sizeof(source),3u,10u,out,sizeof(out))||
       out[0]!=0x93u||out[1]!=0x40u)return 0;
    const uint8_t before[2]={out[0],out[1]};
    return !musa_dwg_ac1032_ot49_copy_msb_bits(
                source,sizeof(source),20u,10u,out,sizeof(out))&&
           !memcmp(out,before,sizeof(out));
}
static int genuine(const char *file){
    FILE *f=fopen(file,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    const long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *src=(uint8_t*)malloc((size_t)n);
    uint8_t *saved=(uint8_t*)malloc((size_t)n);
    if(!src||!saved){free(src);free(saved);fclose(f);return 0;}
    int ok=fread(src,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(ok)memcpy(saved,src,(size_t)n);
    ok=ok&&!memcmp(src,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
        src,(size_t)n,&scene)&&!memcmp(src,saved,(size_t)n);
    free(src);free(saved);
    if(!ok)return 0;
    /* Handles from independent DXF AcDbBlockReference / BLOCK_RECORD,
     * ATTRIB and SEQEND group 5, never guessed from adjacent offsets. */
    const Expected expected[]={
        {UINT64_C(0x1c8),UINT64_C(0x8a),UINT64_C(0x1b8),0u,0u,0u},
        {UINT64_C(0x190),UINT64_C(0x8a),UINT64_C(0x17d),UINT64_C(0x192),UINT64_C(0x191),1u},
        {UINT64_C(0x756),UINT64_C(0x8a),UINT64_C(0x17d),UINT64_C(0x757),UINT64_C(0x759),1u}
    };
    for(size_t i=0u;ok&&i<sizeof(expected)/sizeof(expected[0]);i++){
        MusaDwgAc1032InsertSourceGraph g={0};
        const Expected *e=&expected[i];
        ok=musa_dwg_ac1032_scene_index_insert_source_graph(
            scene,e->h,&g)&&g.indexed_handle==e->h&&
            g.layer_handle==e->layer&&
            g.block_header_handle==e->block&&
            g.attrib_handle==e->attrib&&g.seqend_handle==e->seqend&&
            g.owned_attrib_count==e->count&&
            g.original_entity_and_targets_crc_verified&&
            g.source_edges_verified&&!g.block_expansion_and_display_accepted;
        if(ok){
            MusaDwgAc1032InsertBinding binding={0};
            ok=musa_dwg_ac1032_insert_bind_source(scene,e->h,&binding)&&
               binding.source_binding_verified&&
               !binding.block_expansion_accepted&&
               binding.graph.block_header_handle==e->block&&
               binding.graph.layer_handle==e->layer&&
               binding.placement.indexed_handle==e->h;
        }
        if(ok){
            MusaDwgAc1032InsertBinding binding={0};
            MusaDwgAc1032BlockBaseEvidence missing={0};
            double world[3]={123.0,456.0,789.0};
            const double original_world[3]={123.0,456.0,789.0};
            const double local[3]={0.0,0.0,0.0};
            ok=musa_dwg_ac1032_insert_bind_source(
                   scene,e->h,&binding)&&
               !musa_dwg_ac1032_insert_map_verified_base(
                   &binding,&missing,local,world)&&
               !memcmp(world,original_world,sizeof(world));
            /* A foreign BLOCK_HEADER H must never authorize geometry.
             * This evidence is synthetic and deliberately not accepted. */
            missing.block_header_handle=e->block+1u;
            missing.original_block_header_crc_verified=1u;
            missing.base_point_decoded=1u;
            if(ok)ok=!musa_dwg_ac1032_insert_map_verified_base(
                         &binding,&missing,local,world)&&
                     !memcmp(world,original_world,sizeof(world));
        }
        if(ok){
            MusaDwgAc1032BlockHeaderIdentity block={0};
            ok=musa_dwg_ac1032_scene_index_read_block_header_identity(
                   scene,e->block,&block)&&
               block.indexed_handle==e->block&&
               block.original_record_crc_verified&&
               block.embedded_self_handle_verified&&
               block.empty_eed_verified&&
               block.no_reactors_verified&&
               !block.base_point_decoded;
        }
        if(ok){
            MusaDwgAc1032InsertBlockHeaderSource joined={0};
            ok=musa_dwg_ac1032_insert_bind_block_header_source(
                   scene,e->h,&joined)&&
               joined.source_join_verified&&
               joined.insert.graph.indexed_handle==e->h&&
               joined.block_header.indexed_handle==e->block&&
               joined.block_header.original_record_crc_verified&&
               !joined.block_base_decoded&&
               !joined.block_header.base_point_decoded;
            /* The wrong object type must not mutate the caller output. */
            if(ok){
                const MusaDwgAc1032InsertBlockHeaderSource before=joined;
                ok=!musa_dwg_ac1032_insert_bind_block_header_source(
                       scene,e->block,&joined)&&
                   !memcmp(&before,&joined,sizeof(joined));
            }
        }
        if(ok){
            MusaDwgAc1032BlockHeaderPrefixEvidence prefix={0};
            ok=musa_dwg_ac1032_scene_index_read_block_header_prefix_evidence(
                   scene,e->block,&prefix)&&
               prefix.indexed_handle==e->block&&
               prefix.original_record_crc_verified&&
               prefix.fields_start_bit>10u&&
               prefix.prefix_fnv1a64!=0u&&
               !prefix.base_point_decoded;
            /* Repeated extraction from immutable original bytes is
             * deterministic, with the same verified handle and prefix. */
            if(ok){
                MusaDwgAc1032BlockHeaderPrefixEvidence repeat={0};
                ok=musa_dwg_ac1032_scene_index_read_block_header_prefix_evidence(
                       scene,e->block,&repeat)&&
                   repeat.indexed_handle==prefix.indexed_handle&&
                   repeat.fields_start_bit==prefix.fields_start_bit&&
                   repeat.prefix_fnv1a64==prefix.prefix_fnv1a64&&
                   repeat.original_record_crc_verified&&
                   !repeat.base_point_decoded;
            }
            if(ok){
                const MusaDwgAc1032BlockHeaderPrefixEvidence before=prefix;
                ok=!musa_dwg_ac1032_scene_index_read_block_header_prefix_evidence(
                        scene,e->h,&prefix)&&
                   !memcmp(&prefix,&before,sizeof(prefix));
            }
        }
        if(ok){
            MusaDwgAc1032BlockHeaderFieldSpan span={0};
            ok=musa_dwg_ac1032_scene_index_read_block_header_field_span(
                   scene,e->block,&span)&&
               span.indexed_handle==e->block&&
               span.original_record_crc_verified&&
               span.first_bit>10u&&
               span.first_bit<span.end_bit&&
               !span.base_point_decoded;
            if(ok){
                const MusaDwgAc1032BlockHeaderFieldSpan before=span;
                ok=!musa_dwg_ac1032_scene_index_read_block_header_field_span(
                        scene,e->h,&span)&&
                   !memcmp(&before,&span,sizeof(span));
            }
        }
        if(ok){
            /* Genuine R2018: first authenticated field bits must be
             * reproducible, without claiming that they encode XYZ. */
            MusaDwgAc1032BlockHeaderFieldSpan span={0};
            if(ok)ok=musa_dwg_ac1032_scene_index_read_block_header_field_span(
                   scene,e->block,&span);
            if(ok){
                MusaDwgAc1032BlockHeaderFieldSpan repeat={0};
                ok=musa_dwg_ac1032_scene_index_read_block_header_field_span(
                       scene,e->block,&repeat)&&
                   repeat.indexed_handle==span.indexed_handle&&
                   repeat.first_bit==span.first_bit&&
                   repeat.end_bit==span.end_bit&&
                   repeat.original_record_crc_verified&&
                   !repeat.base_point_decoded;
            }
        }
        if(ok){
            /* Original R2018 H1B8/H17D: the same source DATA bits must
             * agree for 16-bit and two 8-bit reads. This is an exact
             * binary extraction oracle, NOT a decoded block base. */
            MusaDwgAc1032BlockHeaderFieldSpan span={0};
            uint8_t pair[2]={0xa5u,0x5au};
            uint8_t hi=0u,lo=0u;
            ok=musa_dwg_ac1032_scene_index_read_block_header_field_span(
                   scene,e->block,&span)&&
               span.end_bit-span.first_bit>=16u&&
               musa_dwg_ac1032_scene_index_copy_block_header_field_bits(
                   scene,e->block,span.first_bit,16u,pair,sizeof(pair))&&
               musa_dwg_ac1032_scene_index_copy_block_header_field_bits(
                   scene,e->block,span.first_bit,8u,&hi,1u)&&
               musa_dwg_ac1032_scene_index_copy_block_header_field_bits(
                   scene,e->block,span.first_bit+8u,8u,&lo,1u)&&
               pair[0]==hi&&pair[1]==lo;
            if(ok){
                const uint8_t before[2]={pair[0],pair[1]};
                ok=!musa_dwg_ac1032_scene_index_copy_block_header_field_bits(
                       scene,e->h,span.first_bit,16u,pair,sizeof(pair))&&
                   !memcmp(pair,before,sizeof(pair))&&
                   !musa_dwg_ac1032_scene_index_copy_block_header_field_bits(
                       scene,e->block,span.end_bit,16u,pair,sizeof(pair))&&
                   !memcmp(pair,before,sizeof(pair));
            }
        }
        if(ok){
            /* Independent example_2018.dxf BLOCK/ENDBLK ownership oracle:
             * H1B8 CIRKLO_PUNKTOJ has 13 children, H17D bloko has 5;
             * both BLOCK group 10/20/30 origins are (0,0,0).
             * These expectations do not come from our binary decoder.\n             * Do NOT equate the unverified DWG raw has-ATTDEF bit with\n             * the DXF BLOCK 70 attribute-presence flag. */
            MusaDwgAc1032BlockHeaderBase base={0};
            const uint32_t expected_owned=
                e->block==UINT64_C(0x1b8)?13u:5u;
            const int decoded=musa_dwg_ac1032_scene_index_read_block_header_base(
                   scene,e->block,&base);
            ok=decoded&&
               base.indexed_handle==e->block&&
               base.original_record_crc_verified&&
               base.base_point_decoded&&
               !base.block_children_and_display_accepted&&
               base.owned_object_count==expected_owned&&
               !base.is_xref&&!base.xref_overlaid&&
               !base.xref_dependent&&
               fabs(base.base_xyz[0])<1e-12&&
               fabs(base.base_xyz[1])<1e-12&&
               fabs(base.base_xyz[2])<1e-12;
            if(ok){
                const MusaDwgAc1032BlockHeaderBase before=base;
                ok=!musa_dwg_ac1032_scene_index_read_block_header_base(
                        scene,e->h,&base)&&
                   !memcmp(&before,&base,sizeof(base));
            }
        }
        if(ok){
            /* Independent companion example_2018.dxf defines 13 and 5
             * entities in these local blocks. H candidates are decoded
             * from the unchanged binary DWG, not generated from DXF. */
            MusaDwgAc1032BlockHeaderChildPrefix prefix={0};
            const uint32_t expected_children=
                e->block==UINT64_C(0x1b8)?13u:5u;
            ok=musa_dwg_ac1032_scene_index_block_header_child_prefix(
                   scene,e->block,&prefix)&&
               prefix.indexed_handle==e->block&&
               prefix.original_record_crc_verified&&
               prefix.h_prefix_decoded&&
               prefix.canonical_h_roles_verified&&
               !prefix.full_handle_tail_verified&&
               !prefix.final_child_ownership_and_paint_accepted&&
               prefix.child_count==expected_children&&
               prefix.block_control_handle&&
               prefix.block_begin_handle&&prefix.endblk_handle&&
               prefix.block_begin_handle!=prefix.endblk_handle;
            if(ok){
                uint16_t type=0u;
                ok=musa_dwg_ac1032_scene_index_object_type(
                       scene,prefix.block_begin_handle,&type)&&type==4u&&
                   musa_dwg_ac1032_scene_index_object_type(
                       scene,prefix.endblk_handle,&type)&&type==5u;
                for(uint32_t j=0u;ok&&j<prefix.child_count;j++)
                    ok=musa_dwg_ac1032_scene_index_object_type(
                           scene,prefix.child_handles[j],&type)&&
                       type!=4u&&type!=5u;
            }
            if(ok){
                const MusaDwgAc1032BlockHeaderChildPrefix before=prefix;
                ok=!musa_dwg_ac1032_scene_index_block_header_child_prefix(
                       scene,e->h,&prefix)&&
                   !memcmp(&before,&prefix,sizeof(prefix));
            }
        }
        if(ok){
            /* Independent companion DXF block sizes 13/5; each binary
             * source H is independently checked for CRC and actual OT.
             * No reverse-owner or final paint acceptance is inferred. */
            MusaDwgAc1032BlockHeaderTypedChildren typed={0};
            const uint32_t expected_count=
                e->block==UINT64_C(0x1b8)?13u:5u;
            ok=musa_dwg_ac1032_scene_index_block_header_typed_children(
                   scene,e->block,&typed)&&
               typed.prefix.indexed_handle==e->block&&
               typed.prefix.child_count==expected_count&&
               typed.prefix.canonical_h_roles_verified&&
               typed.all_child_record_types_crc_verified&&
               !typed.reverse_owner_links_verified&&
               !typed.final_display_accepted;
            for(uint32_t j=0u;ok&&j<typed.prefix.child_count;j++){
                uint16_t type=0u;
                ok=musa_dwg_ac1032_scene_index_object_type(
                      scene,typed.prefix.child_handles[j],&type)&&
                   type==typed.child_object_types[j]&&
                   type!=4u&&type!=5u;
            }
            if(ok){
                const MusaDwgAc1032BlockHeaderTypedChildren before=typed;
                ok=!musa_dwg_ac1032_scene_index_block_header_typed_children(
                        scene,e->h,&typed)&&
                   !memcmp(&before,&typed,sizeof(typed));
            }
        }
        if(ok){
            /* Unchanged original example_2018.dwg H1B8/H17D; independent
             * companion example_2018.dxf counts 13/5.  Read actual OT4
             * BLOCK and OT5 ENDBLK common prefixes at source H targets.
             * This is marker authentication, never an owner/paint claim. */
            MusaDwgAc1032BlockMarkerCommons markers={0};
            const uint32_t expected_children=
                e->block==UINT64_C(0x1b8)?13u:5u;
            ok=musa_dwg_ac1032_block_marker_commons(
                   scene,e->block,&markers)&&
               markers.block_header_handle==e->block&&
               markers.child_count==expected_children&&
               markers.block_begin_handle!=markers.endblk_handle&&
               markers.marker_records_crc_verified&&
               markers.unique_prefix_handles_verified&&
               markers.begin_common.prefix.raw_object_type==4u&&
               markers.end_common.prefix.raw_object_type==5u&&
               !markers.reverse_owner_links_verified&&
               !markers.final_display_accepted;
            if(ok){
                MusaDwgAc1032BlockMarkerCommons repeated={0};
                ok=musa_dwg_ac1032_block_marker_commons(
                       scene,e->block,&repeated)&&
                   repeated.block_begin_handle==markers.block_begin_handle&&
                   repeated.endblk_handle==markers.endblk_handle&&
                   repeated.child_count==markers.child_count&&
                   repeated.marker_records_crc_verified&&
                   repeated.unique_prefix_handles_verified;
            }
            if(ok){
                const MusaDwgAc1032BlockMarkerCommons before=markers;
                ok=!musa_dwg_ac1032_block_marker_commons(
                       scene,e->h,&markers)&&
                   !memcmp(&before,&markers,sizeof(markers))&&
                   !musa_dwg_ac1032_block_marker_commons(
                       scene,UINT64_MAX,&markers)&&
                   !memcmp(&before,&markers,sizeof(markers))&&
                   !musa_dwg_ac1032_block_marker_commons(
                       scene,before.block_begin_handle,&markers)&&
                   !memcmp(&before,&markers,sizeof(markers))&&
                   !musa_dwg_ac1032_block_marker_commons(
                       scene,before.endblk_handle,&markers)&&
                   !memcmp(&before,&markers,sizeof(markers));
            }
        }
        if(ok){
            /* Original R2018 local BLOCK candidates: never treat a raw
             * H/OT match as ownership or fabricate layer 0 / ACI 7.
             * Independently read an actual OT51 + TU Unicode layer for
             * only the exact child profiles understood by this reader. */
            MusaDwgAc1032BlockChildLayerInventory inv={0};
            const uint32_t expected_children=
                e->block==UINT64_C(0x1b8)?13u:5u;
            ok=musa_dwg_ac1032_block_child_layer_evidence(
                   scene,e->block,&inv)&&
               inv.block_header_handle==e->block&&
               inv.child_count==expected_children&&
               inv.marker_and_child_source_crc_verified&&
               !inv.reverse_child_owner_links_verified&&
               !inv.layer_visibility_color_and_paint_accepted&&
               inv.verified_layer_count+inv.unresolved_profile_count+
               inv.unsupported_type_count==expected_children;
            if(ok){
                MusaDwgAc1032BlockHeaderTypedChildren typed={0};
                ok=musa_dwg_ac1032_scene_index_block_header_typed_children(
                       scene,e->block,&typed);
                for(uint32_t j=0u;ok&&j<inv.child_count;j++){
                    const MusaDwgAc1032BlockChildLayerRow *row=&inv.children[j];
                    ok=row->child_handle==typed.prefix.child_handles[j]&&
                       row->raw_object_type==typed.child_object_types[j];
                    if(!ok)break;
                    if(row->layer_evidence_status==
                       MUSACAD_AC1032_BLOCK_LAYER_VERIFIED){
                        uint16_t t=0u;
                        ok=row->layer_handle&&
                           musa_dwg_ac1032_scene_index_object_type(
                              scene,row->layer_handle,&t)&&t==51u;
                    }else{
                        ok=!row->layer_handle&&
                           (row->layer_evidence_status==
                              MUSACAD_AC1032_BLOCK_LAYER_PROFILE_UNRESOLVED||
                            row->layer_evidence_status==
                              MUSACAD_AC1032_BLOCK_LAYER_TYPE_UNSUPPORTED);
                    }
                }
            }
            if(ok){
                MusaDwgAc1032BlockChildLayerInventory repeated={0};
                ok=musa_dwg_ac1032_block_child_layer_evidence(
                       scene,e->block,&repeated)&&
                   !memcmp(&inv,&repeated,sizeof(inv));
            }
            if(ok){
                const MusaDwgAc1032BlockChildLayerInventory before=inv;
                ok=!musa_dwg_ac1032_block_child_layer_evidence(
                       scene,e->h,&inv)&&
                   !memcmp(&before,&inv,sizeof(inv))&&
                   !musa_dwg_ac1032_block_child_layer_evidence(
                       scene,UINT64_MAX,&inv)&&
                   !memcmp(&before,&inv,sizeof(inv));
            }
        }
        if(ok){
            /* Genuine unchanged example_2018.dwg H1B8/H17D. Independent
             * example_2018.dxf has exactly 13/5 block child objects.
             * A native numeric decode is counted ONLY when the exact
             * source H/CRC/type and geometry parser agree; other types
             * remain explicitly unrendered and never gain ownership. */
            MusaDwgAc1032BlockChildGeometryAudit audit={0};
            const uint32_t expected_count=
                e->block==UINT64_C(0x1b8)?13u:5u;
            ok=musa_dwg_ac1032_block_child_geometry_audit(
                   scene,e->block,&audit)&&
               audit.block_header_handle==e->block&&
               audit.child_count==expected_count&&
               audit.original_h_crc_type_verified&&
               !audit.reverse_ownership_verified&&
               !audit.visibility_and_paint_accepted&&
               audit.numeric_geometry_verified+
               audit.numeric_geometry_rejected+
               audit.unsupported_object_types==expected_count;
            for(uint32_t j=0u;ok&&j<audit.child_count;j++){
                uint16_t actual=0u;
                ok=musa_dwg_ac1032_scene_index_object_type(
                       scene,audit.child_handles[j],&actual)&&
                   actual==audit.child_types[j]&&
                   audit.numeric_geometry_ok[j]<=1u;
            }
            if(ok){
                MusaDwgAc1032BlockChildGeometryAudit repeated={0};
                ok=musa_dwg_ac1032_block_child_geometry_audit(
                       scene,e->block,&repeated)&&
                   !memcmp(&repeated,&audit,sizeof(audit));
            }
            if(ok){
                const MusaDwgAc1032BlockChildGeometryAudit before=audit;
                ok=!musa_dwg_ac1032_block_child_geometry_audit(
                        scene,e->h,&audit)&&
                   !memcmp(&before,&audit,sizeof(audit))&&
                   !musa_dwg_ac1032_block_child_geometry_audit(
                        scene,UINT64_MAX,&audit)&&
                   !memcmp(&before,&audit,sizeof(audit));
            }
        }
        if(ok){
            /* Original INSERT H1C8/H190/H756 and DXF-independent
             * BLOCK_RECORD H1B8/H17D. Coordinates are candidate numeric
             * probes only, not reverse-owned child paint or a scene. */
            MusaDwgAc1032InsertChildCoordinateProbe probe={0};
            const uint32_t expected_count=
                e->block==UINT64_C(0x1b8)?13u:5u;
            ok=musa_dwg_ac1032_insert_child_coordinate_probe(
                   scene,e->h,&probe)&&
               probe.insert_handle==e->h&&
               probe.block_header_handle==e->block&&
               probe.child_count==expected_count&&
               probe.original_source_graph_verified&&
               !probe.reverse_child_owner_verified&&
               !probe.display_accepted&&
               probe.mapped_candidates+probe.unmapped_candidates==
                   expected_count;
            for(uint32_t j=0u;ok&&j<probe.child_count;j++){
                uint16_t type=0u;
                ok=musa_dwg_ac1032_scene_index_object_type(
                       scene,probe.child_handles[j],&type)&&
                   type==probe.child_types[j]&&
                   (!probe.line_end_mapped[j]||
                     (probe.anchor_mapped[j]&&type==19u));
                if(ok&&probe.anchor_mapped[j])
                    for(unsigned k=0u;k<3u;k++)
                        if(!isfinite(probe.world_anchor[j][k]))ok=0;
            }
            if(ok){
                MusaDwgAc1032InsertChildCoordinateProbe repeat={0};
                ok=musa_dwg_ac1032_insert_child_coordinate_probe(
                       scene,e->h,&repeat)&&
                   !memcmp(&repeat,&probe,sizeof(probe));
            }
            if(ok){
                const MusaDwgAc1032InsertChildCoordinateProbe before=probe;
                ok=!musa_dwg_ac1032_insert_child_coordinate_probe(
                        scene,e->block,&probe)&&
                   !memcmp(&before,&probe,sizeof(probe))&&
                   !musa_dwg_ac1032_insert_child_coordinate_probe(
                        scene,UINT64_MAX,&probe)&&
                   !memcmp(&before,&probe,sizeof(probe));
            }
        }
        if(ok){
            /* New native numeric decoding must refer to the SAME ordered
             * source H/OT candidates that the already merged #597 layer
             * evidence uses. Independent source provenance is mandatory;
             * neither subsystem may silently invent layer/paint/owner. */
            MusaDwgAc1032BlockChildLayerInventory layers={0};
            MusaDwgAc1032BlockChildGeometryAudit geo={0};
            MusaDwgAc1032InsertChildCoordinateProbe probe={0};
            ok=musa_dwg_ac1032_block_child_layer_evidence(
                   scene,e->block,&layers)&&
               musa_dwg_ac1032_block_child_geometry_audit(
                   scene,e->block,&geo)&&
               musa_dwg_ac1032_insert_child_coordinate_probe(
                   scene,e->h,&probe)&&
               layers.marker_and_child_source_crc_verified&&
               geo.original_h_crc_type_verified&&
               probe.original_source_graph_verified&&
               layers.child_count==geo.child_count&&
               geo.child_count==probe.child_count&&
               !layers.reverse_child_owner_links_verified&&
               !geo.reverse_ownership_verified&&
               !probe.reverse_child_owner_verified&&
               !layers.layer_visibility_color_and_paint_accepted&&
               !geo.visibility_and_paint_accepted&&
               !probe.display_accepted;
            for(uint32_t j=0u;ok&&j<geo.child_count;j++){
                ok=layers.children[j].child_handle==geo.child_handles[j]&&
                   layers.children[j].raw_object_type==geo.child_types[j]&&
                   geo.child_handles[j]==probe.child_handles[j]&&
                   geo.child_types[j]==probe.child_types[j]&&
                   (!probe.anchor_mapped[j]||geo.numeric_geometry_ok[j])&&
                   (!probe.line_end_mapped[j]||probe.anchor_mapped[j]);
            }
        }
        if(ok){
            /* Original R2018 block child TEXT/MTEXT is not silently
             * accepted on raw OT alone. Every source TU, numeric position,
             * graph edge is decoded independently or remains unresolved.
             * Independent DXF: H1B8 13 children, H17D 5 children. */
            MusaDwgAc1032BlockChildTextSourceInventory textinv={0};
            const uint32_t count=e->block==UINT64_C(0x1b8)?13u:5u;
            ok=musa_dwg_ac1032_block_child_text_source_inventory(
                   scene,e->block,&textinv)&&
               textinv.block_header_handle==e->block&&
               textinv.child_count==count&&
               textinv.original_child_source_graph_verified&&
               !textinv.complete_reverse_ownership_verified&&
               !textinv.font_layout_and_paint_accepted&&
               textinv.utf8_source_count<=textinv.text_family_count&&
               textinv.numeric_placement_count<=textinv.text_family_count&&
               textinv.h_graph_count<=textinv.text_family_count&&
               textinv.matching_owner_edge_count<=textinv.h_graph_count&&
               textinv.matching_layer_edge_count<=textinv.h_graph_count;
            if(ok){
                MusaDwgAc1032BlockChildLayerInventory layers={0};
                ok=musa_dwg_ac1032_block_child_layer_evidence(
                       scene,e->block,&layers);
                uint32_t text_types=0u;
                for(uint32_t j=0u;ok&&j<textinv.child_count;j++){
                    const MusaDwgAc1032BlockTextChildRow *row=&
                        textinv.children[j];
                    ok=row->child_handle==layers.children[j].child_handle&&
                       row->raw_object_type==layers.children[j].raw_object_type&&
                       !row->text_and_display_accepted;
                    if(!ok)break;
                    if(row->raw_object_type!=1u&&row->raw_object_type!=44u){
                        ok=!row->utf8_source_verified&&
                           !row->numeric_placement_verified&&
                           !row->source_h_graph_verified&&
                           !row->reverse_owner_edge_matches_block&&
                           !row->source_layer_edge_matches;
                        continue;
                    }
                    text_types++;
                    if(row->reverse_owner_edge_matches_block)
                        ok=row->source_h_graph_verified&&
                           row->source_owner_handle==e->block;
                    if(ok&&row->source_layer_edge_matches)
                        ok=row->source_h_graph_verified&&
                           layers.children[j].layer_evidence_status==
                             MUSACAD_AC1032_BLOCK_LAYER_VERIFIED&&
                           row->source_layer_handle==
                             layers.children[j].layer_handle;
                }
                if(ok)ok=text_types==textinv.text_family_count;
            }
            if(ok){
                MusaDwgAc1032BlockChildTextSourceInventory again={0};
                ok=musa_dwg_ac1032_block_child_text_source_inventory(
                        scene,e->block,&again)&&
                   !memcmp(&again,&textinv,sizeof(again));
            }
            if(ok){
                const MusaDwgAc1032BlockChildTextSourceInventory
                    before=textinv;
                ok=!musa_dwg_ac1032_block_child_text_source_inventory(
                       scene,e->h,&textinv)&&
                   !memcmp(&before,&textinv,sizeof(before))&&
                   !musa_dwg_ac1032_block_child_text_source_inventory(
                       scene,UINT64_MAX,&textinv)&&
                   !memcmp(&before,&textinv,sizeof(before));
            }
        }
        if(ok){
            /* The independent companion DXF says both referenced BLOCK
             * origins are (0,0,0). Use the actual OT49 base reader and
             * original INSERT placement; do not inject a synthetic base. */
            MusaDwgAc1032InsertPlacement placement={0};
            const double local[3]={0.0,0.0,0.0};
            double mapped[3]={NAN,NAN,NAN};
            ok=musa_dwg_ac1032_scene_index_read_insert_placement(
                   scene,e->h,&placement)&&
               musa_dwg_ac1032_insert_map_original_block_base(
                   scene,e->h,local,mapped);
            if(ok){
                for(unsigned k=0u;k<3u;k++)
                    if(fabs(mapped[k]-placement.insertion[k])>1e-9)ok=0;
            }
            if(ok){
                const double before[3]={mapped[0],mapped[1],mapped[2]};
                const double bad_local[3]={NAN,0.0,0.0};
                ok=!musa_dwg_ac1032_insert_map_original_block_base(
                       scene,e->block,local,mapped)&&
                   !memcmp(mapped,before,sizeof(mapped))&&
                   !musa_dwg_ac1032_insert_map_original_block_base(
                       scene,e->h,bad_local,mapped)&&
                   !memcmp(mapped,before,sizeof(mapped));
            }
        }
        /* A verified source H graph must agree with the independently
         * decoded INSERT DATA attribute flag/count for the same handle.
         * This is source consistency, not block ownership or rendering. */
        if(ok){
            MusaDwgAc1032InsertPlacement p={0};
            ok=musa_dwg_ac1032_scene_index_read_insert_placement(
                 scene,e->h,&p)&&p.indexed_handle==e->h&&
               p.original_record_crc_verified&&p.numeric_fields_decoded&&
               p.owned_attrib_count==g.owned_attrib_count&&
               p.has_attribs==(uint8_t)(e->count!=0u)&&
               !p.block_graph_and_display_accepted;
            /* Genuine source INSERT + independent transform oracle:
             * identity block base is synthetic; not yet a BLOCK_HEADER
             * base-point or final scene-geometry verification. */
            if(ok&&e->h==UINT64_C(0x1c8)){
                const double base[3]={0.0,0.0,0.0};
                const double origin[3]={0.0,0.0,0.0};
                double mapped[3]={NAN,NAN,NAN};
                ok=musa_dwg_ac1032_insert_xy_world_z(
                    &p,base,origin,mapped)&&
                    fabs(mapped[0]-p.insertion[0])<1e-9&&
                    fabs(mapped[1]-p.insertion[1])<1e-9&&
                    fabs(mapped[2]-p.insertion[2])<1e-9;
            }
        }
    }
    if(ok){
        MusaDwgAc1032InsertBinding unchanged_binding;
        memset(&unchanged_binding,0x5a,sizeof(unchanged_binding));
        const MusaDwgAc1032InsertBinding original_binding=unchanged_binding;
        ok=!musa_dwg_ac1032_insert_bind_source(
                scene,UINT64_MAX,&unchanged_binding)&&
           !memcmp(&unchanged_binding,&original_binding,
                   sizeof(unchanged_binding))&&
           !musa_dwg_ac1032_insert_bind_source(
                scene,UINT64_C(0x192),&unchanged_binding)&&
           !memcmp(&unchanged_binding,&original_binding,
                   sizeof(unchanged_binding))&&
           !musa_dwg_ac1032_insert_bind_source(
                scene,UINT64_C(0x1b8),&unchanged_binding)&&
           !memcmp(&unchanged_binding,&original_binding,
                   sizeof(unchanged_binding));
    }
    /* An OT49 parser must reject an unauthenticated payload before
     * attempting any BLOCK_HEADER base-point interpretation. */
    if(ok){
        const uint8_t fake[2]={0,0};
        MusaDwgAc1032ObjectStreams layout={0};
        MusaDwgAc1032BlockHeaderIdentity identity;
        memset(&identity,0x5a,sizeof(identity));
        const MusaDwgAc1032BlockHeaderIdentity saved_identity=identity;
        layout.indexed_handle=UINT64_C(0x1b8);
        layout.raw_object_type=49u;
        layout.object_type_field_bits=10u;
        layout.total_payload_bits=16u;
        layout.data_stream_bits=12u;
        layout.handle_stream_start_bit=12u;
        layout.handle_stream_bits=4u;
        ok=!musa_dwg_ac1032_block_header_ot49_identity(
              fake,sizeof(fake),&layout,&identity)&&
           !memcmp(&identity,&saved_identity,sizeof(identity));
        /* A CRC bit alone must not authorize an OT49 identity: a
         * mismatched object type and malformed prefix both fail closed. */
        layout.original_record_crc_verified=1u;
        if(ok)ok=!musa_dwg_ac1032_block_header_ot49_identity(
                    fake,sizeof(fake),&layout,&identity)&&
                  !memcmp(&identity,&saved_identity,sizeof(identity));
        layout.raw_object_type=51u;
        if(ok)ok=!musa_dwg_ac1032_block_header_ot49_identity(
                    fake,sizeof(fake),&layout,&identity)&&
                  !memcmp(&identity,&saved_identity,sizeof(identity));
        layout.raw_object_type=49u;
        /* Aliased output/payload is never accepted, even with a CRC
         * claim, and no bytes of the original buffer may be changed. */
        {
            uint8_t shared[sizeof(MusaDwgAc1032BlockHeaderIdentity)+2u];
            memset(shared,0x3c,sizeof(shared));
            uint8_t saved_shared[sizeof(shared)];
            memcpy(saved_shared,shared,sizeof(shared));
            if(ok)ok=!musa_dwg_ac1032_block_header_ot49_identity(
                 shared,sizeof(shared),&layout,
                 (MusaDwgAc1032BlockHeaderIdentity *)(void *)shared)&&
                 !memcmp(shared,saved_shared,sizeof(shared));
        }
        {
            MusaDwgAc1032BlockHeaderPrefixEvidence prefix;
            memset(&prefix,0x5a,sizeof(prefix));
            const MusaDwgAc1032BlockHeaderPrefixEvidence original=prefix;
            layout.original_record_crc_verified=0u;
            if(ok)ok=!musa_dwg_ac1032_block_header_ot49_prefix_evidence(
                  fake,sizeof(fake),&layout,&prefix)&&
                  !memcmp(&prefix,&original,sizeof(prefix));
            layout.original_record_crc_verified=1u;
            if(ok)ok=!musa_dwg_ac1032_block_header_ot49_prefix_evidence(
                  fake,sizeof(fake),&layout,&prefix)&&
                  !memcmp(&prefix,&original,sizeof(prefix));
        }
        {
            MusaDwgAc1032BlockHeaderFieldSpan span;
            memset(&span,0x5a,sizeof(span));
            const MusaDwgAc1032BlockHeaderFieldSpan before=span;
            if(ok)ok=!musa_dwg_ac1032_block_header_ot49_field_span(
                  fake,sizeof(fake),&layout,&span)&&
                  !memcmp(&span,&before,sizeof(span));
        }
        {
            uint8_t output[2]={0xa5u,0x5au};
            if(ok)ok=!musa_dwg_ac1032_block_header_ot49_copy_field_bits(
                   fake,sizeof(fake),&layout,10u,8u,output,sizeof(output))&&
                   output[0]==0xa5u&&output[1]==0x5au;
        }
        {
            MusaDwgAc1032BlockHeaderBase base;
            memset(&base,0x5au,sizeof(base));
            const MusaDwgAc1032BlockHeaderBase before=base;
            if(ok)ok=!musa_dwg_ac1032_block_header_ot49_base_at(
                  fake,sizeof(fake),&layout,&base)&&
                  !memcmp(&base,&before,sizeof(base));
        }
        layout.handle_stream_start_bit=11u;
        if(ok)ok=!musa_dwg_ac1032_block_header_ot49_identity(
                    fake,sizeof(fake),&layout,&identity)&&
                  !memcmp(&identity,&saved_identity,sizeof(identity));
    }
    if(ok){
        MusaDwgAc1032InsertSourceGraph unchanged;
        memset(&unchanged,0x5a,sizeof(unchanged));
        const MusaDwgAc1032InsertSourceGraph saved=unchanged;
        ok=!musa_dwg_ac1032_scene_index_insert_source_graph(
             scene,UINT64_MAX,&unchanged)&&
           !memcmp(&unchanged,&saved,sizeof(unchanged))&&
           !musa_dwg_ac1032_scene_index_insert_source_graph(
             scene,UINT64_C(0x192),&unchanged)&&
           !memcmp(&unchanged,&saved,sizeof(unchanged))&&
           /* Reject valid, CRC-authenticated non-INSERT source types.
            * Every failed read must leave the caller's output intact. */
           !musa_dwg_ac1032_scene_index_insert_source_graph(
             scene,UINT64_C(0x8a),&unchanged)&&
           !memcmp(&unchanged,&saved,sizeof(unchanged))&&
           !musa_dwg_ac1032_scene_index_insert_source_graph(
             scene,UINT64_C(0x1b8),&unchanged)&&
           !memcmp(&unchanged,&saved,sizeof(unchanged))&&
           !musa_dwg_ac1032_scene_index_insert_source_graph(
             scene,UINT64_C(0x191),&unchanged)&&
           !memcmp(&unchanged,&saved,sizeof(unchanged));
    }
    if(ok){
        /* Unmodified R2018 example_2018.dwg, with independent DXF
         * BLOCK_RECORD child counts: H1B8=13 and H17D=5.
         * This proves source-color classification, NOT child paint. */
        const uint64_t blocks[]={UINT64_C(0x1b8),UINT64_C(0x17d)};
        const uint32_t counts[]={13u,5u};
        for(size_t b=0u;ok&&b<2u;b++){
            MusaDwgAc1032BlockChildColorEvidence colors={0};
            MusaDwgAc1032BlockChildColorEvidence again={0};
            MusaDwgAc1032BlockChildGeometryAudit geo={0};
            MusaDwgAc1032BlockChildLayerInventory layers={0};
            ok=musa_dwg_ac1032_block_child_color_evidence(
                   scene,blocks[b],&colors)&&
               musa_dwg_ac1032_block_child_color_evidence(
                   scene,blocks[b],&again)&&
               !memcmp(&colors,&again,sizeof(colors))&&
               musa_dwg_ac1032_block_child_geometry_audit(
                   scene,blocks[b],&geo)&&
               musa_dwg_ac1032_block_child_layer_evidence(
                   scene,blocks[b],&layers)&&
               colors.block_header_handle==blocks[b]&&
               colors.child_count==counts[b]&&
               colors.original_child_and_layer_evidence_verified&&
               !colors.reverse_ownership_verified&&
               !colors.visibility_and_paint_accepted&&
               colors.direct_aci_count+colors.direct_rgb_count+
               colors.inherited_count+colors.hidden_count+
               colors.unresolved_count==counts[b];
            for(uint32_t i=0u;ok&&i<colors.child_count;i++){
                const MusaDwgAc1032BlockChildColorRow *r=&colors.rows[i];
                ok=r->child_handle==geo.child_handles[i]&&
                   r->raw_object_type==geo.child_types[i]&&
                   r->child_handle==layers.children[i].child_handle&&
                   !r->final_paint_accepted;
                if(ok&&r->source_common_verified){
                    ok=geo.numeric_geometry_ok[i]&&
                       layers.children[i].layer_evidence_status==
                           MUSACAD_AC1032_BLOCK_LAYER_VERIFIED&&
                       r->source_layer_handle==
                           layers.children[i].layer_handle;
                }
                if(ok&&(r->source_color_status==
                         MUSACAD_AC1032_CHILD_COLOR_DIRECT_ACI||
                        r->source_color_status==
                         MUSACAD_AC1032_CHILD_COLOR_DIRECT_RGB))
                    ok=r->source_common_verified&&
                       !r->raw_invisibility&&
                       !r->raw_has_transparency&&
                       !r->raw_has_color_reference;
            }
            if(ok){
                const MusaDwgAc1032BlockChildColorEvidence saved=colors;
                ok=!musa_dwg_ac1032_block_child_color_evidence(
                       scene,UINT64_C(0x1c8),&colors)&&
                   !memcmp(&saved,&colors,sizeof(colors))&&
                   !musa_dwg_ac1032_block_child_color_evidence(
                       scene,UINT64_MAX,&colors)&&
                   !memcmp(&saved,&colors,sizeof(colors));
            }
        }
    }
    if(ok){
        /* Deterministic standalone color semantics: never turn hidden,
         * BYLAYER/BYBLOCK or transparency into a direct drawable color. */
        MusaDwgAc1032EntityCommon c={0};
        c.common_decoded=1u;c.prefix.prefix_decoded=1u;
        c.color_index=5u;
        ok=musa_dwg_ac1032_block_child_direct_color_class(&c)==
             MUSACAD_AC1032_CHILD_COLOR_DIRECT_ACI;
        c.color_index=256u;
        if(ok)ok=musa_dwg_ac1032_block_child_direct_color_class(&c)==
                 MUSACAD_AC1032_CHILD_COLOR_INHERITED;
        c.color_index=0u;
        if(ok)ok=musa_dwg_ac1032_block_child_direct_color_class(&c)==
                 MUSACAD_AC1032_CHILD_COLOR_INHERITED;
        c.has_rgb=1u;c.rgb=0x00112233u;
        if(ok)ok=musa_dwg_ac1032_block_child_direct_color_class(&c)==
                 MUSACAD_AC1032_CHILD_COLOR_DIRECT_RGB;
        c.invisibility=1u;
        if(ok)ok=musa_dwg_ac1032_block_child_direct_color_class(&c)==
                 MUSACAD_AC1032_CHILD_COLOR_HIDDEN;
        c.invisibility=0u;c.has_transparency=1u;
        if(ok)ok=musa_dwg_ac1032_block_child_direct_color_class(&c)==
                 MUSACAD_AC1032_CHILD_COLOR_UNRESOLVED;
        c.has_transparency=0u;c.has_color_reference=1u;
        if(ok)ok=musa_dwg_ac1032_block_child_direct_color_class(&c)==
                 MUSACAD_AC1032_CHILD_COLOR_UNRESOLVED;
    }
    if(ok){
        /* Authentic R2018 source OT49 child H -> original OT51 LAYER
         * numeric flags + anchored default-profile CMC. This is NOT a
         * viewport visibility/plot decision or a BYLAYER paint result.
         * Independent DXF oracle: H1B8=13, H17D=5 children. */
        const uint64_t handles[]={UINT64_C(0x1b8),UINT64_C(0x17d)};
        const uint32_t child_counts[]={13u,5u};
        for(size_t b=0u;ok&&b<2u;b++){
            MusaDwgAc1032BlockChildLayerStyleProvenance styled={0};
            MusaDwgAc1032BlockChildLayerStyleProvenance repeat={0};
            MusaDwgAc1032BlockChildLayerInventory layer={0};
            MusaDwgAc1032BlockChildColorEvidence colors={0};
            ok=musa_dwg_ac1032_block_child_layer_style_provenance(
                   scene,handles[b],&styled)&&
               musa_dwg_ac1032_block_child_layer_style_provenance(
                   scene,handles[b],&repeat)&&
               !memcmp(&styled,&repeat,sizeof(styled))&&
               musa_dwg_ac1032_block_child_layer_evidence(
                   scene,handles[b],&layer)&&
               musa_dwg_ac1032_block_child_color_evidence(
                   scene,handles[b],&colors)&&
               styled.block_header_handle==handles[b]&&
               styled.child_count==child_counts[b]&&
               styled.original_children_joined&&
               !styled.reverse_ownership_accepted&&
               !styled.layer_visibility_and_paint_accepted&&
               styled.layer_flags_source_count<=styled.source_layer_count&&
               styled.layer_cmc_source_count<=styled.source_layer_count&&
               styled.explicit_aci_count+styled.explicit_rgb_count+
               styled.bylayer_count+styled.byblock_count+
               styled.hidden_count+styled.unresolved_count==
                   child_counts[b];
            for(uint32_t i=0u;ok&&i<styled.child_count;i++){
                const MusaDwgAc1032BlockChildLayerStyleRow *row=&
                    styled.rows[i];
                ok=row->child_handle==layer.children[i].child_handle&&
                   row->raw_object_type==layer.children[i].raw_object_type&&
                   row->child_handle==colors.rows[i].child_handle&&
                   row->layer_handle==layer.children[i].layer_handle&&
                   !row->effective_color_and_visibility_accepted;
                if(!ok)break;
                if(row->original_layer_flags_verified){
                    MusaDwgAc1032LayerFlagsEvidence flags={0};
                    ok=row->layer_handle&&
                       musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
                         scene,row->layer_handle,&flags)&&
                       flags.source_crc_verified&&
                       flags.indexed_handle==row->layer_handle&&
                       flags.raw_values_70_290_370==
                           row->source_layer_raw_flags;
                }
                if(ok&&row->original_layer_cmc_aci_verified){
                    MusaDwgAc1032LayerCmcEvidence cmc={0};
                    ok=row->layer_handle&&
                       musa_dwg_ac1032_scene_index_layer_anchored_cmc_evidence(
                         scene,row->layer_handle,&cmc)&&
                       cmc.original_record_crc_verified&&
                       cmc.indexed_handle==row->layer_handle&&
                       cmc.aci_index==row->original_layer_cmc_aci;
                }
            }
            if(ok){
                const MusaDwgAc1032BlockChildLayerStyleProvenance
                    original=styled;
                ok=!musa_dwg_ac1032_block_child_layer_style_provenance(
                        scene,UINT64_C(0x1c8),&styled)&&
                   !memcmp(&original,&styled,sizeof(original))&&
                   !musa_dwg_ac1032_block_child_layer_style_provenance(
                        scene,UINT64_MAX,&styled)&&
                   !memcmp(&original,&styled,sizeof(original));
            }
        }
    }
    if(ok){
        /* One original inherited status is NOT enough to choose color.
         * Index 0 is BYBLOCK; 256 is BYLAYER, never direct ACI7.
         * The source classification never authorizes final paint. */
        MusaDwgAc1032BlockChildColorRow c={0};
        c.source_common_verified=1u;
        c.source_layer_handle=UINT64_C(0x8a);
        c.source_color_status=MUSACAD_AC1032_CHILD_COLOR_INHERITED;
        c.raw_color_index=0u;
        ok=musa_dwg_ac1032_child_source_style_class(&c)==
             MUSACAD_AC1032_SOURCE_STYLE_BYBLOCK;
        c.raw_color_index=256u;
        if(ok)ok=musa_dwg_ac1032_child_source_style_class(&c)==
                     MUSACAD_AC1032_SOURCE_STYLE_BYLAYER;
        c.source_color_status=MUSACAD_AC1032_CHILD_COLOR_DIRECT_ACI;
        c.raw_color_index=5u;
        if(ok)ok=musa_dwg_ac1032_child_source_style_class(&c)==
                     MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_ACI;
        c.source_color_status=MUSACAD_AC1032_CHILD_COLOR_DIRECT_RGB;
        if(ok)ok=musa_dwg_ac1032_child_source_style_class(&c)==
                     MUSACAD_AC1032_SOURCE_STYLE_EXPLICIT_RGB;
        c.source_color_status=MUSACAD_AC1032_CHILD_COLOR_HIDDEN;
        if(ok)ok=musa_dwg_ac1032_child_source_style_class(&c)==
                     MUSACAD_AC1032_SOURCE_STYLE_HIDDEN;
        c.source_color_status=MUSACAD_AC1032_CHILD_COLOR_UNRESOLVED;
        if(ok)ok=musa_dwg_ac1032_child_source_style_class(&c)==
                     MUSACAD_AC1032_SOURCE_STYLE_UNRESOLVED;
        c.source_common_verified=0u;
        if(ok)ok=musa_dwg_ac1032_child_source_style_class(&c)==
                     MUSACAD_AC1032_SOURCE_STYLE_UNRESOLVED;
        c.source_common_verified=1u;c.source_layer_handle=0u;
        if(ok)ok=musa_dwg_ac1032_child_source_style_class(&c)==
                     MUSACAD_AC1032_SOURCE_STYLE_UNRESOLVED;
    }
    if(ok){
        /* Independent original example_2018.dwg / companion DXF:
         * OT49 H1B8 has 13 children, H17D has 5. Their separately
         * authenticated header, BLOCK, ENDBLK and candidate child Hs
         * must not alias. The shared BLOCK_CONTROL is not an entity. */
        MusaDwgAc1032BlockPairSourceIsolation pair={0},reverse={0};
        ok=musa_dwg_ac1032_block_pair_source_isolation(
               scene,UINT64_C(0x1b8),UINT64_C(0x17d),&pair)&&
           musa_dwg_ac1032_block_pair_source_isolation(
               scene,UINT64_C(0x17d),UINT64_C(0x1b8),&reverse)&&
           pair.first_block_header==UINT64_C(0x1b8)&&
           pair.second_block_header==UINT64_C(0x17d)&&
           pair.first_children==13u&&pair.second_children==5u&&
           reverse.first_children==5u&&reverse.second_children==13u&&
           pair.original_records_crc_and_types_verified&&
           pair.cross_block_source_handles_disjoint&&
           reverse.original_records_crc_and_types_verified&&
           reverse.cross_block_source_handles_disjoint&&
           !pair.reverse_ownership_verified&&!pair.display_accepted&&
           !reverse.reverse_ownership_verified&&!reverse.display_accepted&&
           pair.first_begin==reverse.second_begin&&
           pair.first_end==reverse.second_end&&
           pair.second_begin==reverse.first_begin&&
           pair.second_end==reverse.first_end;
        if(ok){
            const MusaDwgAc1032BlockPairSourceIsolation before=pair;
            ok=!musa_dwg_ac1032_block_pair_source_isolation(
                    scene,UINT64_C(0x1b8),UINT64_C(0x1b8),&pair)&&
               !memcmp(&pair,&before,sizeof(pair))&&
               !musa_dwg_ac1032_block_pair_source_isolation(
                    scene,UINT64_C(0x1b8),UINT64_C(0x1c8),&pair)&&
               !memcmp(&pair,&before,sizeof(pair))&&
               !musa_dwg_ac1032_block_pair_source_isolation(
                    scene,UINT64_C(0x1b8),UINT64_MAX,&pair)&&
               !memcmp(&pair,&before,sizeof(pair))&&
               !musa_dwg_ac1032_block_pair_source_isolation(
                    scene,UINT64_C(0x1b8),UINT64_C(0x17d),NULL)&&
               !memcmp(&pair,&before,sizeof(pair));
        }
    }
    if(ok){
        /* Independent original example_2018.dwg + companion DXF oracle:
         * INSERT H1C8->H1B8 (13), H190/H756->H17D (5).
         * Every candidate coordinate, original OT51 layer and encoded
         * source color must join on the EXACT child H/type and order.
         * Neither a successful join nor explicit color permits paint. */
        const uint64_t inserts[]={UINT64_C(0x1c8),
                                  UINT64_C(0x190),UINT64_C(0x756)};
        const uint64_t blocks[]={UINT64_C(0x1b8),
                                 UINT64_C(0x17d),UINT64_C(0x17d)};
        const uint32_t counts[]={13u,5u,5u};
        for(size_t b=0u;ok&&b<3u;b++){
            MusaDwgAc1032InsertChildSourceJoin joined={0},repeat={0};
            MusaDwgAc1032InsertChildCoordinateProbe probe={0};
            MusaDwgAc1032BlockChildLayerStyleProvenance styles={0};
            MusaDwgAc1032BlockChildColorEvidence colors={0};
            ok=musa_dwg_ac1032_insert_child_source_join(
                   scene,inserts[b],&joined)&&
               musa_dwg_ac1032_insert_child_source_join(
                   scene,inserts[b],&repeat)&&
               !memcmp(&joined,&repeat,sizeof(joined))&&
               musa_dwg_ac1032_insert_child_coordinate_probe(
                   scene,inserts[b],&probe)&&
               musa_dwg_ac1032_block_child_layer_style_provenance(
                   scene,blocks[b],&styles)&&
               musa_dwg_ac1032_block_child_color_evidence(
                   scene,blocks[b],&colors)&&
               joined.insert_handle==inserts[b]&&
               joined.block_header_handle==blocks[b]&&
               joined.child_count==counts[b]&&
               joined.mapped_count==probe.mapped_candidates&&
               joined.unmapped_count==probe.unmapped_candidates&&
               joined.original_insert_block_child_source_join_verified&&
               !joined.reverse_ownership_verified&&
               !joined.space_and_viewport_verified&&
               !joined.final_paint_accepted;
            for(uint32_t i=0u;ok&&i<joined.child_count;i++){
                const MusaDwgAc1032InsertChildSourceRow *r=&joined.rows[i];
                const MusaDwgAc1032BlockChildLayerStyleRow *st=&styles.rows[i];
                const MusaDwgAc1032BlockChildColorRow *co=&colors.rows[i];
                ok=r->child_handle==probe.child_handles[i]&&
                   r->child_handle==st->child_handle&&
                   r->child_handle==co->child_handle&&
                   r->raw_object_type==probe.child_types[i]&&
                   r->raw_object_type==st->raw_object_type&&
                   r->raw_object_type==co->raw_object_type&&
                   r->source_layer_handle==st->layer_handle&&
                   r->source_layer_handle==co->source_layer_handle&&
                   r->raw_color_index==co->raw_color_index&&
                   r->raw_rgb==co->raw_rgb&&
                   r->source_style_class==st->source_style_class&&
                   r->source_color_status==co->source_color_status&&
                   r->coordinate_mapped==probe.anchor_mapped[i]&&
                   r->line_end_mapped==probe.line_end_mapped[i]&&
                   !r->reverse_owner_verified&&
                   !r->visibility_and_paint_accepted;
                if(ok&&r->coordinate_mapped){
                    ok=!memcmp(r->world_anchor,probe.world_anchor[i],
                               sizeof(r->world_anchor));
                    if(ok&&r->line_end_mapped)
                        ok=!memcmp(r->world_line_end,probe.world_line_end[i],
                                   sizeof(r->world_line_end));
                }
                if(ok&&(r->source_style_class==
                           MUSACAD_AC1032_SOURCE_STYLE_BYLAYER||
                        r->source_style_class==
                           MUSACAD_AC1032_SOURCE_STYLE_BYBLOCK||
                        r->source_style_class==
                           MUSACAD_AC1032_SOURCE_STYLE_HIDDEN))
                    ok=!r->visibility_and_paint_accepted;
            }
            if(ok){
                const MusaDwgAc1032InsertChildSourceJoin before=joined;
                ok=!musa_dwg_ac1032_insert_child_source_join(
                        scene,blocks[b],&joined)&&
                   !memcmp(&joined,&before,sizeof(joined))&&
                   !musa_dwg_ac1032_insert_child_source_join(
                        scene,UINT64_MAX,&joined)&&
                   !memcmp(&joined,&before,sizeof(joined))&&
                   !musa_dwg_ac1032_insert_child_source_join(
                        scene,inserts[b],NULL)&&
                   !memcmp(&joined,&before,sizeof(joined));
            }
        }
    }
    if(ok){
        /* Independent original example_2018.dwg/DXF: two distinct INSERTs
         * H190 and H756 target the SAME OT49 H17D with five children.
         * Original layer/color provenance must not change with instance
         * transform. H1C8 targets a DIFFERENT block and must be rejected. */
        MusaDwgAc1032InsertInstanceSourcePair pair={0},reverse={0};
        ok=musa_dwg_ac1032_insert_instance_source_pair(
               scene,UINT64_C(0x190),UINT64_C(0x756),&pair)&&
           musa_dwg_ac1032_insert_instance_source_pair(
               scene,UINT64_C(0x756),UINT64_C(0x190),&reverse)&&
           pair.first_insert==UINT64_C(0x190)&&
           pair.second_insert==UINT64_C(0x756)&&
           pair.shared_block_header==UINT64_C(0x17d)&&
           pair.matched_child_sources==5u&&
           pair.original_child_provenance_instance_invariant&&
           !pair.distinct_transform_verified&&
           !pair.owner_and_visibility_verified&&
           !pair.final_paint_accepted&&
           reverse.first_insert==pair.second_insert&&
           reverse.second_insert==pair.first_insert&&
           reverse.shared_block_header==pair.shared_block_header&&
           reverse.matched_child_sources==pair.matched_child_sources&&
           reverse.first_mapped==pair.second_mapped&&
           reverse.second_mapped==pair.first_mapped;
        if(ok){
            const MusaDwgAc1032InsertInstanceSourcePair before=pair;
            ok=!musa_dwg_ac1032_insert_instance_source_pair(
                    scene,UINT64_C(0x190),UINT64_C(0x1c8),&pair)&&
               !memcmp(&pair,&before,sizeof(pair))&&
               !musa_dwg_ac1032_insert_instance_source_pair(
                    scene,UINT64_C(0x190),UINT64_C(0x190),&pair)&&
               !memcmp(&pair,&before,sizeof(pair))&&
               !musa_dwg_ac1032_insert_instance_source_pair(
                    scene,UINT64_C(0x190),UINT64_MAX,&pair)&&
               !memcmp(&pair,&before,sizeof(pair))&&
               !musa_dwg_ac1032_insert_instance_source_pair(
                    scene,UINT64_C(0x190),UINT64_C(0x756),NULL)&&
               !memcmp(&pair,&before,sizeof(pair));
        }
    }
    if(ok){
        /* Independently sourced example_2018.dxf counts for the original
         * untouched R2018 DWG OT49 block candidate child H inventory.
         * Only an actual verified H4 child->OT49 owner counts, never H
         * prefix position or guessed group adjacency. Unsupported H
         * profiles remain explicitly unresolved, not "owned". */
        const uint64_t blocks[]={UINT64_C(0x1b8),UINT64_C(0x17d)};
        const uint32_t counts[]={13u,5u};
        for(size_t b=0u;ok&&b<2u;b++){
            MusaDwgAc1032BlockReverseOwnerSourceAudit owned={0};
            MusaDwgAc1032BlockReverseOwnerSourceAudit again={0};
            MusaDwgAc1032BlockHeaderTypedChildren typed={0};
            ok=musa_dwg_ac1032_block_reverse_owner_source_audit(
                   scene,blocks[b],&owned)&&
               musa_dwg_ac1032_block_reverse_owner_source_audit(
                   scene,blocks[b],&again)&&
               !memcmp(&owned,&again,sizeof(owned))&&
               musa_dwg_ac1032_scene_index_block_header_typed_children(
                   scene,blocks[b],&typed)&&
               owned.block_header_handle==blocks[b]&&
               owned.child_count==counts[b]&&
               owned.block_child_h_crc_types_verified&&
               !owned.full_block_reverse_ownership_accepted&&
               !owned.layout_visibility_and_paint_accepted&&
               owned.verified_reverse_source_count+
                 owned.unresolved_owner_count==counts[b]&&
               owned.matching_reverse_owner_count+
                 owned.foreign_owner_count==owned.verified_reverse_source_count;
            for(uint32_t j=0u;ok&&j<owned.child_count;j++){
                const MusaDwgAc1032BlockReverseOwnerSourceRow *r=
                    &owned.rows[j];
                uint16_t raw_type=0u;
                ok=r->child_handle==typed.prefix.child_handles[j]&&
                   r->raw_object_type==typed.child_object_types[j]&&
                   musa_dwg_ac1032_scene_index_object_type(
                     scene,r->child_handle,&raw_type)&&
                   raw_type==r->raw_object_type&&
                   !r->child_visible_and_paint_accepted;
                if(!ok)break;
                if(r->strict_reverse_edge_source_verified){
                    uint16_t owner_type=0u,layer_type=0u;
                    ok=r->verified_source_owner_handle&&
                       r->verified_source_layer_handle&&
                       musa_dwg_ac1032_scene_index_object_type(
                         scene,r->verified_source_owner_handle,&owner_type)&&
                       owner_type==49u&&
                       musa_dwg_ac1032_scene_index_object_type(
                         scene,r->verified_source_layer_handle,&layer_type)&&
                       layer_type==51u&&
                       r->reverse_owner_matches_block+
                       r->reverse_owner_conflicts_block==1u&&
                       r->reverse_owner_matches_block==
                         (uint8_t)(r->verified_source_owner_handle==blocks[b]);
                }else{
                    ok=!r->verified_source_owner_handle&&
                       !r->verified_source_layer_handle&&
                       !r->reverse_owner_matches_block&&
                       !r->reverse_owner_conflicts_block;
                }
            }
            if(ok){
                const MusaDwgAc1032BlockReverseOwnerSourceAudit before=owned;
                ok=!musa_dwg_ac1032_block_reverse_owner_source_audit(
                        scene,UINT64_C(0x1c8),&owned)&&
                   !memcmp(&before,&owned,sizeof(before))&&
                   !musa_dwg_ac1032_block_reverse_owner_source_audit(
                        scene,UINT64_MAX,&owned)&&
                   !memcmp(&before,&owned,sizeof(before))&&
                   !musa_dwg_ac1032_block_reverse_owner_source_audit(
                        scene,blocks[b],NULL)&&
                   !memcmp(&before,&owned,sizeof(before));
            }
        }
    }
    if(ok){
        /* Exact geometry entity-mode/H4/H5 reader must fail atomic for
         * valid non-geometry OT49 and unrelated source H values. */
        MusaDwgAc1032EntityBlockOwnerLayerSource edge;
        memset(&edge,0x5a,sizeof(edge));
        const MusaDwgAc1032EntityBlockOwnerLayerSource before=edge;
        ok=!musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
               scene,UINT64_C(0x1b8),&edge)&&
           !memcmp(&before,&edge,sizeof(edge))&&
           !musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
               scene,UINT64_C(0x190),&edge)&&
           !memcmp(&before,&edge,sizeof(edge))&&
           !musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
               scene,UINT64_MAX,&edge)&&
           !memcmp(&before,&edge,sizeof(edge));
    }
    fprintf(stderr,"%s genuine R2018 INSERT H1C8/H190/H756 source H graph and H1C8 origin transform; "
        "block expansion, owner, viewport, final paint UNVERIFIED\n",
        ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return bit_order_oracle()?0:1;
    if(argc!=2)return 2;
    return bit_order_oracle()&&genuine(argv[1])?0:1;
}
