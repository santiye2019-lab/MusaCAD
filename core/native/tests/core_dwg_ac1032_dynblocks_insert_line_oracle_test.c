/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authentic untouched R2018 Dynblocks.dwg: positive source-only INSERT
 * LINE owner + affine evidence, checked against the independently
 * authenticated first-party H4/H5/OT49 source-segment join. This is NOT
 * a visibility, BYLAYER/BYBLOCK or final paint acceptance test. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_insert_line_source_segments.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int near(double a,double b){
    return isfinite(a)&&isfinite(b)&&fabs(a-b)<=1e-5;
}
static int original(const char *filename){
    FILE *f=fopen(filename,"rb");
    if(!f)return 0;
    int ok=fseek(f,0,SEEK_END)==0;
    long n=ok?ftell(f):-1L;
    if(n<256L||n>16L*1024L*1024L||fseek(f,0,SEEK_SET))ok=0;
    uint8_t *src=ok?(uint8_t*)malloc((size_t)n):NULL;
    uint8_t *copy=src?(uint8_t*)malloc((size_t)n):NULL;
    if(!src||!copy)ok=0;
    if(ok)ok=fread(src,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(ok)memcpy(copy,src,(size_t)n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(src,"AC1032",6u)&&
       musa_dwg_ac1032_scene_index_open(src,(size_t)n,&scene)&&
       !memcmp(src,copy,(size_t)n);
    free(src);free(copy);
    if(!ok){musa_dwg_ac1032_scene_index_close(scene);return 0;}
    size_t inserts=0u,admitted=0u,verified=0u;
    size_t graph_ok=0u,inventory_ok=0u,affine_ok=0u;
    size_t line_sources_ok=0u,segments_ok=0u;
    size_t inventory_line_candidates=0u,inventory_with_line=0u;
    size_t inventory_line_owner_readable=0u,inventory_line_owner_matches=0u;
    size_t inventory_line_geometry_readable=0u,inventory_line_planar=0u;
    size_t first_line_diagnostics=0u;
    size_t tail_ok_count=0u,tail_h4=0u,tail_h12=0u,tail_other=0u;
    size_t admitted_line_sources=0u,source_segments_positive=0u;
    size_t source_segments_total=0u,source_segments_lines=0u;
    size_t graph_inventory_affine_ok=0u;
    size_t owner_audit_ok=0u,owner_audit_verified=0u;
    size_t owner_audit_complete=0u,owner_audit_unsupported=0u;
    size_t owner_audit_rejected=0u,owner_audit_unreadable=0u;
    size_t owner_audit_mismatched=0u,owner_audit_unsupported_on_reject=0u;
    size_t coverage_ok=0u,coverage_verified=0u,coverage_unreadable=0u;
    size_t coverage_mismatched=0u,coverage_unsupported=0u;
    size_t coverage_complete=0u;
    uint64_t first_rejected_insert=0u;
    size_t source_graph_first_failure=0u;
    size_t insert_type_lookup_failures=0u;
    size_t line_samples=0u,line_sample_ok=0u;
    size_t first_mismatches=0u;
    size_t insert_common_ok=0u,insert_placement_ok=0u;
    size_t fetch_stages[6]={0};
    size_t prefix_reasons[10]={0};
    MusaDwgAc1032InsertPrefixProbe first_prefix={0};
    unsigned first_fetch_stage=255u;
    size_t entity_mode_0=0u,entity_mode_1=0u,entity_mode_2=0u;
    size_t entity_mode_other=0u,placement_has_attribs=0u;
    unsigned first_entity_mode=255u,first_reactors=0u;
    unsigned first_xdict=0u,first_has_color_ref=0u;
    unsigned first_ltype=0u,first_plotstyle=0u;
    unsigned first_material=0u,first_visualstyle=0u;
    uint64_t first_insert=0u,first_line=0u;
    double first_xy[4]={0};
    for(size_t i=0u;ok&&i<musa_dwg_ac1032_scene_index_count(scene);i++){
        MusaDwgAc1032HandleEntry entry={0};
        uint16_t type=0u;
        ok=musa_dwg_ac1032_scene_index_entry(scene,i,&entry)&&
           musa_dwg_ac1032_scene_index_object_type(
               scene,entry.handle,&type);
        if(!ok){insert_type_lookup_failures++;break;} /* Fail closed. */
        if(type==19u&&line_samples<10u){
            MusaDwgAc1032NativeLine line={0};
            line_samples++;
            if(musa_dwg_ac1032_scene_index_read_line(
                   scene,entry.handle,&line))line_sample_ok++;
        }
        if(type!=7u)continue;
        inserts++;
        uint8_t fetch_stage=255u;
        if(!musa_dwg_ac1032_scene_index_insert_decode_stage(
               scene,entry.handle,&fetch_stage)||fetch_stage>5u){
            ok=0;break;
        }
        fetch_stages[fetch_stage]++;
        if(inserts==1u)first_fetch_stage=(unsigned)fetch_stage;
        MusaDwgAc1032InsertPrefixProbe probe={0};
        if(!musa_dwg_ac1032_scene_index_insert_prefix_probe(
               scene,entry.handle,&probe)||
           probe.indexed_handle!=entry.handle||
           probe.fetch_stage!=fetch_stage||
           probe.prefix_reason>9u){ok=0;break;}
        prefix_reasons[probe.prefix_reason]++;
        if(probe.prefix_reason==2u&&first_mismatches<5u){
            fprintf(stderr,"DIAG self mismatch %zu: index=H%llx "
                           "embedded=H%llx length=%u\n",
                    first_mismatches+1u,
                    (unsigned long long)entry.handle,
                    (unsigned long long)probe.embedded_self_handle,
                    (unsigned)probe.embedded_handle_length);
            first_mismatches++;
        }
        if(inserts==1u)first_prefix=probe;
        MusaDwgAc1032EntityCommon common={0};
        if(musa_dwg_ac1032_scene_index_read_insert_common(
               scene,entry.handle,&common)){
            insert_common_ok++;
            switch(common.prefix.entity_mode){
                case 0u:entity_mode_0++;break;
                case 1u:entity_mode_1++;break;
                case 2u:entity_mode_2++;break;
                default:entity_mode_other++;break;
            }
            if(inserts==1u){
                first_entity_mode=(unsigned)common.prefix.entity_mode;
                first_reactors=(unsigned)common.prefix.reactor_count;
                first_xdict=(unsigned)common.prefix.xdictionary_missing;
                first_has_color_ref=(unsigned)common.has_color_reference;
                first_ltype=(unsigned)common.linetype_flags;
                first_plotstyle=(unsigned)common.plotstyle_flags;
                first_material=(unsigned)common.material_flags;
                first_visualstyle=(unsigned)common.visualstyle_flags;
            }
        }
        MusaDwgAc1032InsertPlacement placement={0};
        if(musa_dwg_ac1032_scene_index_read_insert_placement(
               scene,entry.handle,&placement)){
            insert_placement_ok++;
            if(placement.has_attribs)placement_has_attribs++;
        }
        if(!first_rejected_insert)first_rejected_insert=entry.handle;
        MusaDwgAc1032InsertSourceGraph graph={0};
        MusaDwgAc1032InsertBlockChildInventory inventory={0};
        double affine[6]={0};
        if(musa_dwg_ac1032_scene_index_insert_source_graph(
               scene,entry.handle,&graph))graph_ok++;
        else if(!source_graph_first_failure)source_graph_first_failure=i+1u;
        if(musa_dwg_ac1032_scene_index_insert_block_child_inventory(
               scene,entry.handle,&inventory)){
            inventory_ok++;
            MusaDwgAc1032InsertBlockOwnerCoverage coverage={0};
            if(!musa_dwg_ac1032_scene_index_insert_block_owner_coverage(
                   scene,entry.handle,&coverage)||
               !coverage.source_inventory_verified||
               coverage.final_display_accepted||
               coverage.insert_handle!=entry.handle||
               coverage.block_header_handle!=
                   inventory.insert.block_header_handle||
               coverage.child_count!=inventory.block.prefix.child_count||
               coverage.verified_count+coverage.unsupported_count+
               coverage.unreadable_count+coverage.mismatched_count!=
                   coverage.child_count){ok=0;break;}
            coverage_ok++;
            coverage_verified+=coverage.verified_count;
            coverage_unreadable+=coverage.unreadable_count;
            coverage_mismatched+=coverage.mismatched_count;
            coverage_unsupported+=coverage.unsupported_count;
            if(coverage.complete_reverse_owner_links_verified)
                coverage_complete++;
            for(uint32_t z=0u;z<coverage.child_count;z++){
                if(coverage.child_handles[z]!=
                       inventory.block.prefix.child_handles[z]||
                   coverage.child_types[z]!=
                       inventory.block.child_object_types[z]||
                   coverage.child_status[z]<MUSACAD_AC1032_BLOCK_OWNER_VERIFIED||
                   coverage.child_status[z]>MUSACAD_AC1032_BLOCK_OWNER_MISMATCH){
                    ok=0;break;
                }
            }
            if(!ok)break;
            MusaDwgAc1032InsertBlockOwnerAudit audit={0};
            if(musa_dwg_ac1032_scene_index_insert_block_owner_audit(
                   scene,entry.handle,&audit)){
                owner_audit_ok++;
                owner_audit_verified+=audit.supported_child_count;
                owner_audit_unsupported+=audit.unsupported_child_count;
                if(audit.complete_reverse_owner_links_verified)
                    owner_audit_complete++;
                if(audit.insert_handle!=entry.handle||
                   audit.block_header_handle!=
                       inventory.insert.block_header_handle||
                   audit.child_count!=inventory.block.prefix.child_count||
                   !audit.supported_reverse_owner_links_verified||
                   audit.final_display_accepted||
                   audit.supported_child_count+audit.unsupported_child_count!=
                       audit.child_count){ok=0;break;}
            }else{
                owner_audit_rejected++;
                for(uint32_t z=0u;z<inventory.block.prefix.child_count;z++){
                    const uint16_t t=inventory.block.child_object_types[z];
                    if(t!=17u&&t!=18u&&t!=19u&&
                       t!=27u&&t!=35u&&t!=77u){
                        owner_audit_unsupported_on_reject++;
                        continue;
                    }
                    MusaDwgAc1032EntityBlockOwnerLayerSource edge={0};
                    const uint64_t child=inventory.block.prefix.child_handles[z];
                    if(!musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
                           scene,child,&edge))owner_audit_unreadable++;
                    else if(!musa_dwg_ac1032_block_child_owner_edge_matches(
                               &edge,child,inventory.insert.block_header_handle,t))
                        owner_audit_mismatched++;
                }
            }
            size_t candidates=0u;
            for(uint32_t z=0u;z<inventory.block.prefix.child_count;z++)
                if(inventory.block.child_object_types[z]==19u)candidates++;
            for(uint32_t z=0u;z<inventory.block.prefix.child_count;z++){
                if(inventory.block.child_object_types[z]!=19u)continue;
                const uint64_t child=inventory.block.prefix.child_handles[z];
                MusaDwgAc1032EntityBlockOwnerLayerSource owner={0};
                MusaDwgAc1032NativeLine line={0};
                const int owner_ok=
                    musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
                        scene,child,&owner);
                const int line_ok=musa_dwg_ac1032_scene_index_read_line(
                    scene,child,&line);
                if(owner_ok){
                    inventory_line_owner_readable++;
                    if(owner.owner_block_header_handle==
                       inventory.insert.block_header_handle)
                        inventory_line_owner_matches++;
                }
                if(line_ok){
                    inventory_line_geometry_readable++;
                    if(line.geometry.start[2]==0.0&&
                       line.geometry.end[2]==0.0&&
                       line.geometry.extrusion[0]==0.0&&
                       line.geometry.extrusion[1]==0.0&&
                       line.geometry.extrusion[2]==1.0&&
                       line.geometry.thickness==0.0)
                        inventory_line_planar++;
                }
                MusaDwgAc1032LineHTailProbe tail={0};
                const int tail_ok=musa_dwg_ac1032_scene_index_line_h_tail_probe(
                    scene,child,&tail);
                if(tail_ok){
                    tail_ok_count++;
                    if(tail.first_code==4u)tail_h4++;
                    else if(tail.first_code==12u)tail_h12++;
                    else tail_other++;
                }
                if(first_line_diagnostics<8u){
                    fprintf(stderr,"DIAG LINE H-tail H%llx mode=%u "
                                   "first=%u/H%llx second=%u/H%llx "
                                   "remaining=%u\n",
                        (unsigned long long)child,
                        tail_ok?(unsigned)tail.entity_mode:255u,
                        tail_ok?(unsigned)tail.first_code:255u,
                        (unsigned long long)tail.first_target,
                        tail_ok?(unsigned)tail.second_code:255u,
                        (unsigned long long)tail.second_target,
                        tail.residual_bits);
                    fprintf(stderr,"DIAG LINE H%llx block H%llx "
                                   "owner_ok=%d owner_H%llx "
                                   "line_ok=%d invis=%u\n",
                        (unsigned long long)child,
                        (unsigned long long)inventory.insert.block_header_handle,
                        owner_ok,
                        (unsigned long long)owner.owner_block_header_handle,
                        line_ok,line_ok?(unsigned)line.common.invisibility:0u);
                    first_line_diagnostics++;
                }
            }
            inventory_line_candidates+=candidates;
            if(candidates)inventory_with_line++;
            if(inventory_ok<=10u)
                fprintf(stderr,"DIAG inventory H%llx block H%llx "
                               "children=%u LINE=%zu\n",
                        (unsigned long long)entry.handle,
                        (unsigned long long)inventory.insert.block_header_handle,
                        inventory.block.prefix.child_count,candidates);
        }
        if(musa_dwg_ac1032_scene_index_original_insert_block_affine(
               scene,entry.handle,affine))affine_ok++;
        if(musa_dwg_ac1032_scene_index_insert_source_graph(
               scene,entry.handle,&graph)&&
           musa_dwg_ac1032_scene_index_insert_block_child_inventory(
               scene,entry.handle,&inventory)&&
           musa_dwg_ac1032_scene_index_original_insert_block_affine(
               scene,entry.handle,affine))graph_inventory_affine_ok++;
        MusaDwgAc1032InsertLineSourceSegments independent={0};
        if(musa_dwg_ac1032_insert_line_source_segments(
               scene,entry.handle,&independent)){
            source_segments_total++;
            source_segments_lines+=independent.verified_planar_line_count;
            if(independent.verified_planar_line_count)source_segments_positive++;
        }
        MusaDwgAc1032InsertBlockLineSources got={0};
        if(!musa_dwg_ac1032_scene_index_insert_block_line_sources(
               scene,entry.handle,&got))continue;
        line_sources_ok++;
        admitted_line_sources+=got.line_count;
        if(!got.original_source_geometry_verified||
           got.reverse_owner_links_verified||got.final_display_accepted||
           got.insert_handle!=entry.handle||
           got.line_count>MUSACAD_AC1032_OT49_CHILD_LIMIT){ok=0;break;}
        MusaDwgAc1032InsertLineSourceSegments ref={0};
        if(!musa_dwg_ac1032_insert_line_source_segments(
               scene,entry.handle,&ref)||
           !ref.original_source_line_packet_verified||
           ref.model_viewport_layer_visibility_and_paint_accepted||
           ref.block_header_handle!=got.block_header_handle){ok=0;break;}
        segments_ok++;
        verified++;
        for(uint32_t k=0u;ok&&k<got.line_count;k++){
            /* Real source LINE handles must be nonzero and unique.
             * The same handle appended twice must be rejected. */
            if(!musa_dwg_ac1032_source_line_handle_admissible(
                    got.child_handles,k,got.child_handles[k])||
               musa_dwg_ac1032_source_line_handle_admissible(
                    got.child_handles,k+1u,got.child_handles[k])){
                ok=0;break;
            }
            int matches=0;
            for(uint32_t j=0u;j<ref.verified_planar_line_count;j++){
                const MusaDwgAc1032InsertLineSourceRow *row=&ref.rows[j];
                if(row->child_handle!=got.child_handles[k])continue;
                matches++;
                if(!row->original_line_h4_h5_and_world_xy_verified||
                   row->verified_block_owner_handle!=got.block_header_handle||
                   row->final_visibility_and_paint_accepted||
                   !near(row->world_start[0],got.endpoints[k][0])||
                   !near(row->world_start[1],got.endpoints[k][1])||
                   !near(row->world_end[0],got.endpoints[k][2])||
                   !near(row->world_end[1],got.endpoints[k][3]))ok=0;
            }
            if(matches!=1)ok=0;
            if(!first_line){
                first_insert=entry.handle;
                first_line=got.child_handles[k];
                for(unsigned z=0u;z<4u;z++)first_xy[z]=got.endpoints[k][z];
            }
            admitted++;
        }
    }
    fprintf(stderr,"DIAG genuine OT19 LINE sample decoded=%zu/%zu\n",
            line_sample_ok,line_samples);
    fprintf(stderr,"DIAG original OT7 prefix gates: "
                   "unavailable=%zu Hheader=%zu self=%zu EED=%zu "
                   "preview=%zu mode=%zu reactors=%zu xdict=%zu "
                   "datastore=%zu canonical=%zu\n",
            prefix_reasons[0],prefix_reasons[1],prefix_reasons[2],
            prefix_reasons[3],prefix_reasons[4],prefix_reasons[5],
            prefix_reasons[6],prefix_reasons[7],prefix_reasons[8],
            prefix_reasons[9]);
    fprintf(stderr,"DIAG first OT7 prefix: reason=%u eed=%u "
                   "preview=%u mode=%u reactors=%u xdict=%u datastore=%u\n",
            (unsigned)first_prefix.prefix_reason,
            (unsigned)first_prefix.eed_bytes,
            (unsigned)first_prefix.preview,
            (unsigned)first_prefix.entity_mode,
            (unsigned)first_prefix.reactor_count,
            (unsigned)first_prefix.xdictionary_missing,
            (unsigned)first_prefix.has_binary_data_store);
    fprintf(stderr,"DIAG original OT7 fetch stage first=%u "
                   "not_indexed=%zu indexed=%zu payload=%zu layout=%zu "
                   "copied=%zu common=%zu\n",
            first_fetch_stage,fetch_stages[0],fetch_stages[1],
            fetch_stages[2],fetch_stages[3],fetch_stages[4],
            fetch_stages[5]);
    fprintf(stderr,"DIAG original OT7 common=%zu placement=%zu "
                   "mode0=%zu mode1=%zu mode2=%zu other=%zu attrib=%zu\n",
            insert_common_ok,insert_placement_ok,entity_mode_0,
            entity_mode_1,entity_mode_2,entity_mode_other,
            placement_has_attribs);
    fprintf(stderr,"DIAG first OT7 common mode=%u reactors=%u xdict=%u "
                   "color_ref=%u ltype=%u plot=%u material=%u visual=%u\n",
            first_entity_mode,first_reactors,first_xdict,
            first_has_color_ref,first_ltype,first_plotstyle,
            first_material,first_visualstyle);
    fprintf(stderr,"DIAG index/type lookup failures=%zu\n",
            insert_type_lookup_failures);
    fprintf(stderr,"DIAG first source graph rejection scene-entry index=%zu\n",
            source_graph_first_failure);
    fprintf(stderr,"DIAG genuine R2018 first INSERT handle=H%llx\n",
            (unsigned long long)first_rejected_insert);
    fprintf(stderr,"DIAG LINE H-tail verified=%zu H4=%zu H12=%zu "
                   "other=%zu\n",tail_ok_count,tail_h4,tail_h12,tail_other);
    fprintf(stderr,"DIAG LINE evidence owner_readable=%zu "
                   "owner_matches=%zu geometry_readable=%zu planar=%zu\n",
            inventory_line_owner_readable,inventory_line_owner_matches,
            inventory_line_geometry_readable,inventory_line_planar);
    fprintf(stderr,"DIAG OT49 inventory=%zu with_LINE=%zu "
                   "LINE_candidates=%zu source_line_helpers=%zu "
                   "helper_lines=%zu independent_packets=%zu "
                   "independent_positive=%zu independent_lines=%zu\n",
            inventory_ok,inventory_with_line,inventory_line_candidates,
            line_sources_ok,admitted_line_sources,source_segments_total,
            source_segments_positive,source_segments_lines);
    fprintf(stderr,"DIAG genuine R2018 INSERT gates: total=%zu graph=%zu "
                   "inventory=%zu affine=%zu all_three=%zu line_sources=%zu segments=%zu\n",
            inserts,graph_ok,inventory_ok,affine_ok,graph_inventory_affine_ok,
            line_sources_ok,segments_ok);
    fprintf(stderr,"DIAG OT49 reverse owner audit: successful=%zu "
                   "verified=%zu unsupported=%zu complete=%zu "
                   "rejected=%zu unreadable_edges=%zu mismatched_edges=%zu "
                   "unsupported_on_reject=%zu\n",
            owner_audit_ok,owner_audit_verified,owner_audit_unsupported,
            owner_audit_complete,owner_audit_rejected,owner_audit_unreadable,
            owner_audit_mismatched,owner_audit_unsupported_on_reject);
    fprintf(stderr,"DIAG OT49 owner per-child coverage: inventories=%zu "
                   "verified=%zu unreadable=%zu mismatch=%zu "
                   "unsupported=%zu complete=%zu\n",
            coverage_ok,coverage_verified,coverage_unreadable,
            coverage_mismatched,coverage_unsupported,coverage_complete);
    if(ok)ok=owner_audit_ok>0u&&owner_audit_verified>0u&&
             owner_audit_complete>0u&&
             owner_audit_ok+owner_audit_rejected==inventory_ok&&
             coverage_ok==inventory_ok&&
             coverage_verified>=owner_audit_verified&&
             coverage_unreadable>0u&&
             coverage_complete==owner_audit_complete;
    /* Genuine positive R2018 H12 owner/H5 layer; a BLOCK_HEADER must
     * never be misread as a LINE, and failed output is unchanged. */
    if(ok){
        MusaDwgAc1032EntityBlockOwnerLayerSource edge={0};
        ok=musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
              scene,UINT64_C(0xbcad),&edge)&&
           edge.child_handle==UINT64_C(0xbcad)&&
           edge.owner_block_header_handle==UINT64_C(0xbcaa)&&
           edge.layer_handle==UINT64_C(0xbf6f)&&
           edge.original_owner_wire_code==12u&&
           edge.original_relative_h12_owner_verified&&
           edge.exact_h4_owner_and_h5_layer_source_verified&&
           !edge.visible_scene_and_paint_accepted;
        if(ok){
            MusaDwgAc1032EntityBlockOwnerLayerSource bad;
            memset(&bad,0xa5,sizeof(bad));
            const MusaDwgAc1032EntityBlockOwnerLayerSource before=bad;
            ok=!musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
                  scene,UINT64_C(0xbcaa),&bad)&&
               !memcmp(&bad,&before,sizeof(bad));
        }
    }
    if(ok){
        MusaDwgAc1032EntityBlockOwnerLayerSource edge={0};
        ok=musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
              scene,UINT64_C(0xbcad),&edge)&&
           musa_dwg_ac1032_block_child_owner_edge_matches(
              &edge,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u);
        if(ok){
            MusaDwgAc1032EntityBlockOwnerLayerSource changed=edge;
            changed.owner_block_header_handle^=1u;
            ok=!musa_dwg_ac1032_block_child_owner_edge_matches(
                  &changed,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u);
            changed=edge;changed.child_handle^=1u;
            ok=ok&&!musa_dwg_ac1032_block_child_owner_edge_matches(
                  &changed,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u);
            changed=edge;changed.original_child_record_crc_verified=0u;
            ok=ok&&!musa_dwg_ac1032_block_child_owner_edge_matches(
                  &changed,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u);
            changed=edge;changed.visible_scene_and_paint_accepted=1u;
            ok=ok&&!musa_dwg_ac1032_block_child_owner_edge_matches(
                  &changed,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u);
            changed=edge;changed.layer_handle=0u;
            ok=ok&&!musa_dwg_ac1032_block_child_owner_edge_matches(
                  &changed,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u);
            changed=edge;changed.raw_entity_type=17u;
            ok=ok&&!musa_dwg_ac1032_block_child_owner_edge_matches(
                  &changed,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u);
            changed=edge;changed.original_owner_wire_code=3u;
            ok=ok&&!musa_dwg_ac1032_block_child_owner_edge_matches(
                  &changed,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u);
            changed=edge;changed.original_relative_h12_owner_verified=0u;
            ok=ok&&!musa_dwg_ac1032_block_child_owner_edge_matches(
                  &changed,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u);
            changed=edge;changed.original_owner_and_layer_records_crc_verified=0u;
            ok=ok&&!musa_dwg_ac1032_block_child_owner_edge_matches(
                  &changed,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u);
        }
    }
    if(ok){
        MusaDwgAc1032EntityBlockOwnerLayerSource edge={0};
        ok=musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
              scene,UINT64_C(0xbcad),&edge)&&
           musa_dwg_ac1032_block_child_owner_status(
              &edge,1,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u)==
              MUSACAD_AC1032_BLOCK_OWNER_VERIFIED;
        if(ok){
            MusaDwgAc1032EntityBlockOwnerLayerSource changed=edge;
            changed.owner_block_header_handle^=1u;
            ok=musa_dwg_ac1032_block_child_owner_status(
                   &changed,1,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u)==
                   MUSACAD_AC1032_BLOCK_OWNER_MISMATCH&&
               musa_dwg_ac1032_block_child_owner_status(
                   &edge,0,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u)==
                   MUSACAD_AC1032_BLOCK_OWNER_UNREADABLE&&
               musa_dwg_ac1032_block_child_owner_status(
                   NULL,1,UINT64_C(0xbcad),UINT64_C(0xbcaa),19u)==
                   MUSACAD_AC1032_BLOCK_OWNER_UNREADABLE&&
               musa_dwg_ac1032_block_child_owner_status(
                   &edge,1,UINT64_C(0xbcad),UINT64_C(0xbcaa),999u)==
                   MUSACAD_AC1032_BLOCK_OWNER_UNSUPPORTED;
        }
    }
    if(ok){
        MusaDwgAc1032InsertBlockOwnerCoverage bad;
        memset(&bad,0xa5,sizeof(bad));
        const MusaDwgAc1032InsertBlockOwnerCoverage before=bad;
        ok=!musa_dwg_ac1032_scene_index_insert_block_owner_coverage(
              scene,UINT64_MAX,&bad)&&!memcmp(&bad,&before,sizeof(bad))&&
           !musa_dwg_ac1032_scene_index_insert_block_owner_coverage(
              NULL,UINT64_C(0xbcad),&bad)&&
           !memcmp(&bad,&before,sizeof(bad));
    }
    if(ok){
        MusaDwgAc1032InsertBlockOwnerAudit bad;
        memset(&bad,0xa5,sizeof(bad));
        const MusaDwgAc1032InsertBlockOwnerAudit before=bad;
        ok=!musa_dwg_ac1032_scene_index_insert_block_owner_audit(
              scene,UINT64_MAX,&bad)&&!memcmp(&bad,&before,sizeof(bad));
    }
    if(ok){
        uint8_t stage=255u;
        ok=musa_dwg_ac1032_scene_index_insert_decode_stage(
              scene,UINT64_MAX,&stage)&&stage==0u;
    }
    if(ok){
        MusaDwgAc1032InsertBlockLineSources out;
        memset(&out,0xa5,sizeof(out));
        const MusaDwgAc1032InsertBlockLineSources before=out;
        ok=!musa_dwg_ac1032_scene_index_insert_block_line_sources(
              scene,UINT64_MAX,&out)&&
           !memcmp(&out,&before,sizeof(out));
    }
    if(ok){
        const uint64_t valid[3]={UINT64_C(0x123),UINT64_C(0x456),UINT64_C(0x789)};
        const uint64_t repeated[3]={UINT64_C(0x123),UINT64_C(0x456),UINT64_C(0x123)};
        const uint64_t missing[3]={UINT64_C(0x123),0u,UINT64_C(0x789)};
        ok=musa_dwg_ac1032_block_child_handles_unique(valid,3u)&&
           musa_dwg_ac1032_block_child_handles_unique(NULL,0u)&&
           !musa_dwg_ac1032_block_child_handles_unique(NULL,1u)&&
           !musa_dwg_ac1032_block_child_handles_unique(repeated,3u)&&
           !musa_dwg_ac1032_block_child_handles_unique(missing,3u)&&
           !musa_dwg_ac1032_block_child_handles_unique(
               valid,MUSACAD_AC1032_OT49_CHILD_LIMIT+1u);
    }
    if(ok){
        const uint64_t previous[2]={UINT64_C(0x123),UINT64_C(0x456)};
        const uint64_t invalid_previous[2]={UINT64_C(0x123),0u};
        ok=musa_dwg_ac1032_source_line_handle_admissible(
               previous,2u,UINT64_C(0x789))&&
           !musa_dwg_ac1032_source_line_handle_admissible(
               previous,2u,UINT64_C(0x123))&&
           !musa_dwg_ac1032_source_line_handle_admissible(
               previous,2u,UINT64_C(0x456))&&
           !musa_dwg_ac1032_source_line_handle_admissible(
               previous,2u,0u)&&
           !musa_dwg_ac1032_source_line_handle_admissible(
               invalid_previous,2u,UINT64_C(0x789))&&
           !musa_dwg_ac1032_source_line_handle_admissible(
               NULL,0u,UINT64_C(0x789))&&
           !musa_dwg_ac1032_source_line_handle_admissible(
               previous,MUSACAD_AC1032_OT49_CHILD_LIMIT,
               UINT64_C(0x789))&&
           !musa_dwg_ac1032_source_line_handle_admissible(
               previous,MUSACAD_AC1032_OT49_CHILD_LIMIT+1u,
               UINT64_C(0x789));
    }
    fprintf(stderr,"%s genuine R2018 Dynblocks INSERT source LINE: "
                   "%zu INSERT, %zu strict source joins, %zu lines; "
                   "first H%llx child H%llx (%.8f %.8f)-(%.8f %.8f); "
                   "paint never accepted\n",
            (ok&&inserts>0u&&verified>0u&&admitted>0u)?"PASS":"FAIL",
            inserts,verified,admitted,
            (unsigned long long)first_insert,(unsigned long long)first_line,
            first_xy[0],first_xy[1],first_xy[2],first_xy[3]);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok&&inserts>0u&&verified>0u&&admitted>0u;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    return original(argv[1])?0:1;
}
