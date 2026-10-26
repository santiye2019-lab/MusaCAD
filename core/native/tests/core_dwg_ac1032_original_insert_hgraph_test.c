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
#include "musacad_dwg_ac1032_block_child_geometry_audit.h"
#include "musacad_dwg_ac1032_insert_child_coordinate_probe.h"
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
