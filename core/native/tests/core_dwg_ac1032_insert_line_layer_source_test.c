/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original untouched R2018 Dynblocks.dwg. Source-only child LINE layer
 * Unicode/status and raw color join; never grants final paint. */
#include "musacad_dwg_ac1032_insert_line_source_segments.h"
#include "musacad_dwg_ac1032_complete_block_line_style_risk.h"
#include "musacad_dwg_ac1032_block_owner_gap_census.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int genuine(const char *filename){
    FILE *f=fopen(filename,"rb");
    if(!f)return 0;
    int ok=fseek(f,0,SEEK_END)==0;
    const long n=ok?ftell(f):-1L;
    if(n<256L||n>32L*1024L*1024L||fseek(f,0,SEEK_SET)!=0)ok=0;
    uint8_t *bytes=ok?(uint8_t*)malloc((size_t)n):NULL;
    uint8_t *copy=bytes?(uint8_t*)malloc((size_t)n):NULL;
    if(!bytes||!copy)ok=0;
    if(ok)ok=fread(bytes,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(ok)memcpy(copy,bytes,(size_t)n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(bytes,"AC1032",6u)&&
        musa_dwg_ac1032_scene_index_open(bytes,(size_t)n,&scene)&&
        !memcmp(bytes,copy,(size_t)n);
    free(bytes);free(copy);
    if(!ok){musa_dwg_ac1032_scene_index_close(scene);return 0;}
    size_t inserts=0u,source_rows=0u,joined=0u,unsupported=0u;
    size_t complete_style_positive=0u,partial_style_rejected=0u;
    size_t census_inventories=0u,census_unreadable=0u;
    size_t ot77_h12_verified=0u;
    uint32_t circle_unreadable_stages[6]={0};
    size_t circle_unreadable=0u;
    uint32_t unreadable_by_type[MUSACAD_AC1032_OWNER_GAP_FAMILIES]={0};
    uint64_t first_insert=0u,first_child=0u;
    uint64_t first_unreadable_ot18=0u,first_unreadable_ot77=0u;
    uint64_t first_ot77_handle=0u;
    for(size_t i=0u;ok&&i<musa_dwg_ac1032_scene_index_count(scene);i++){
        MusaDwgAc1032HandleEntry entry={0};
        uint16_t type=0u;
        ok=musa_dwg_ac1032_scene_index_entry(scene,i,&entry)&&
           musa_dwg_ac1032_scene_index_object_type(scene,entry.handle,&type);
        if(!ok)break;
        if(type!=7u)continue;
        inserts++;
        MusaDwgAc1032InsertBlockOwnerCoverage coverage={0};
        if(musa_dwg_ac1032_scene_index_insert_block_owner_coverage(
               scene,entry.handle,&coverage)){
            if(coverage.child_count){
                MusaDwgAc1032BlockOwnerGapCensus census={0};
                if(!musa_dwg_ac1032_block_owner_gap_census(
                       &coverage,&census)||
                   !census.source_inventory_and_statuses_verified||
                   census.final_visibility_color_and_paint_accepted||
                   census.insert_handle!=entry.handle||
                   census.block_header_handle!=coverage.block_header_handle||
                   census.child_count!=coverage.child_count){ok=0;break;}
                census_inventories++;
                for(uint32_t k=0u;k<coverage.child_count;k++){
                    if(coverage.child_types[k]!=18u||
                       coverage.child_status[k]!=
                           MUSACAD_AC1032_BLOCK_OWNER_UNREADABLE)continue;
                    MusaDwgAc1032CircleDecodeStage stage={0};
                    const uint64_t child=coverage.child_handles[k];
                    if(!musa_dwg_ac1032_scene_index_circle_decode_stage(
                           scene,child,&stage)||
                       stage.indexed_handle!=child||
                       stage.raw_object_type!=18u||
                       stage.fetch_stage>5u||
                       !stage.original_record_crc_verified||
                       stage.owner_layer_visibility_and_paint_accepted){
                        ok=0;break;
                    }
                    MusaDwgAc1032CirclePrefixProbe prefix={0};
                    if(!musa_dwg_ac1032_scene_index_circle_prefix_probe(
                           scene,child,&prefix)||
                       prefix.indexed_handle!=child||
                       prefix.fetch_stage!=stage.fetch_stage||
                       prefix.owner_layer_visibility_and_paint_accepted||
                       (prefix.fetch_stage<4u&&prefix.prefix_reason!=0u)||
                       prefix.fetch_stage!=5u||prefix.prefix_reason!=9u||
                       prefix.reactor_count!=1u){
                        ok=0;break;
                    }
                    fprintf(stderr,"DIAG genuine OT18 H=%llx stage=%u "
                        "prefix_reason=%u embedded=%llx mode=%u eed=%u "
                        "preview=%u reactors=%u xdict_missing=%u ds=%u\n",
                        (unsigned long long)child,prefix.fetch_stage,
                        prefix.prefix_reason,
                        (unsigned long long)prefix.embedded_self_handle,
                        prefix.entity_mode,prefix.eed_bytes,prefix.preview,
                        prefix.reactor_count,prefix.xdictionary_missing,
                        prefix.has_binary_data_store);
                    MusaDwgAc1032CirclePrefixProbe untouched_prefix;
                    memset(&untouched_prefix,0xa5,sizeof(untouched_prefix));
                    unsigned char prefix_before[sizeof(untouched_prefix)];
                    memcpy(prefix_before,&untouched_prefix,
                           sizeof(prefix_before));
                    if(musa_dwg_ac1032_scene_index_circle_prefix_probe(
                           scene,0u,&untouched_prefix)||
                       memcmp(&untouched_prefix,prefix_before,
                              sizeof(prefix_before))||
                       musa_dwg_ac1032_scene_index_circle_prefix_probe(
                           scene,UINT64_MAX,&untouched_prefix)||
                       memcmp(&untouched_prefix,prefix_before,
                              sizeof(prefix_before))){ok=0;break;}
                    circle_unreadable_stages[stage.fetch_stage]++;
                    circle_unreadable++;
                    MusaDwgAc1032CircleDecodeStage untouched;
                    memset(&untouched,0xa5,sizeof(untouched));
                    unsigned char before[sizeof(untouched)];
                    memcpy(before,&untouched,sizeof(before));
                    if(musa_dwg_ac1032_scene_index_circle_decode_stage(
                           scene,0u,&untouched)||
                       memcmp(&untouched,before,sizeof(before))||
                       musa_dwg_ac1032_scene_index_circle_decode_stage(
                           scene,UINT64_MAX,&untouched)||
                       memcmp(&untouched,before,sizeof(before))){
                        ok=0;break;
                    }
                }
                if(!ok)break;
                for(uint32_t k=0u;k<coverage.child_count;k++){
                    if(coverage.child_types[k]!=77u)continue;
                    const uint64_t child=coverage.child_handles[k];
                    if(!first_ot77_handle)first_ot77_handle=child;
                    if(coverage.child_status[k]!=MUSACAD_AC1032_BLOCK_OWNER_VERIFIED)
                        continue;
                    MusaDwgAc1032EntityBlockOwnerLayerSource edge={0};
                    if(!musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
                           scene,child,&edge)||
                       !musa_dwg_ac1032_block_child_owner_edge_matches(
                           &edge,child,coverage.block_header_handle,77u)){
                        ok=0;break;
                    }
                    if(edge.original_owner_wire_code==12u){
                        if(!edge.original_relative_h12_owner_verified){ok=0;break;}
                        MusaDwgAc1032EntityBlockOwnerLayerSource forged=edge;
                        forged.owner_block_header_handle^=1u;
                        if(musa_dwg_ac1032_block_child_owner_edge_matches(
                               &forged,child,coverage.block_header_handle,77u)){
                            ok=0;break;
                        }
                        ot77_h12_verified++;
                    }
                }
                if(!ok)break;
                if(census.unreadable[1]&&!first_unreadable_ot18)
                    first_unreadable_ot18=census.first_unreadable_handle[1];
                if(census.unreadable[5]&&!first_unreadable_ot77)
                    first_unreadable_ot77=census.first_unreadable_handle[5];
                for(unsigned t=0u;t<MUSACAD_AC1032_OWNER_GAP_FAMILIES;t++){
                    unreadable_by_type[t]+=census.unreadable[t];
                    census_unreadable+=census.unreadable[t];
                    if(census.unreadable[t]&&!census.first_unreadable_handle[t]){
                        ok=0;break;
                    }
                }
                if(!ok)break;
                MusaDwgAc1032BlockOwnerGapCensus untouched;
                memset(&untouched,0xa5,sizeof(untouched));
                const MusaDwgAc1032BlockOwnerGapCensus before=untouched;
                MusaDwgAc1032InsertBlockOwnerCoverage forged=coverage;
                forged.verified_count++;
                if(musa_dwg_ac1032_block_owner_gap_census(
                       &forged,&untouched)||
                   memcmp(&untouched,&before,sizeof(untouched))){ok=0;break;}
                forged=coverage;
                forged.child_handles[0]=0u;
                if(musa_dwg_ac1032_block_owner_gap_census(
                       &forged,&untouched)||
                   memcmp(&untouched,&before,sizeof(untouched))){ok=0;break;}
                forged=coverage;
                forged.final_display_accepted=1u;
                if(musa_dwg_ac1032_block_owner_gap_census(
                       &forged,&untouched)||
                   memcmp(&untouched,&before,sizeof(untouched))){ok=0;break;}
            }
            uint32_t line_children=0u;
            for(uint32_t k=0u;k<coverage.child_count;k++)
                if(coverage.child_types[k]==19u)line_children++;
            if(musa_dwg_ac1032_block_owner_coverage_complete(&coverage)&&
               line_children){
                MusaDwgAc1032CompleteBlockLineStyleRisk batch={0};
                if(!musa_dwg_ac1032_complete_block_line_style_risk(
                       scene,entry.handle,&batch)||
                   !batch.complete_owner_and_style_risk_verified||
                   batch.final_visibility_color_and_paint_accepted||
                   !batch.line_count||
                   batch.insert_handle!=entry.handle||
                   batch.block_header_handle!=coverage.block_header_handle){
                    ok=0;break;
                }
                for(uint32_t j=0u;j<batch.line_count;j++){
                    const MusaDwgAc1032InsertLineStyleRiskRow *r=
                        &batch.rows[j];
                    if(!r->child_handle||!r->source_layer_handle||
                       !r->original_layer_flags_verified||
                       r->final_visibility_and_paint_accepted||
                       (r->risk_mask&(
                           MUSACAD_AC1032_LINE_RISK_LAYER_VISIBILITY|
                           MUSACAD_AC1032_LINE_RISK_SPACE_VIEWPORT))!=(
                           MUSACAD_AC1032_LINE_RISK_LAYER_VISIBILITY|
                           MUSACAD_AC1032_LINE_RISK_SPACE_VIEWPORT)){
                        ok=0;break;
                    }
                }
                if(!ok)break;
                complete_style_positive++;
            }else if(musa_dwg_ac1032_block_owner_coverage_complete(
                          &coverage)){
                MusaDwgAc1032InsertBlockLineSources empty={0};
                if(line_children||
                   !musa_dwg_ac1032_scene_index_complete_block_line_sources(
                       scene,entry.handle,&empty)||
                   empty.line_count||
                   !empty.original_source_geometry_verified||
                   empty.final_display_accepted){ok=0;break;}
            }else if(coverage.child_count){
                MusaDwgAc1032CompleteBlockLineStyleRisk untouched;
                memset(&untouched,0xa5,sizeof(untouched));
                const MusaDwgAc1032CompleteBlockLineStyleRisk before=untouched;
                if(musa_dwg_ac1032_complete_block_line_style_risk(
                       scene,entry.handle,&untouched)||
                   memcmp(&untouched,&before,sizeof(untouched))){
                    ok=0;break;
                }
                partial_style_rejected++;
            }
        }
        MusaDwgAc1032InsertLineSourceSegments rows={0};
        if(!musa_dwg_ac1032_insert_line_source_segments(
               scene,entry.handle,&rows))continue;
        for(uint32_t j=0u;ok&&j<rows.verified_planar_line_count;j++){
            const MusaDwgAc1032InsertLineSourceRow *row=&rows.rows[j];
            source_rows++;
            MusaDwgAc1032InsertLineLayerSource result={0};
            if(!musa_dwg_ac1032_insert_line_layer_source(
                   scene,entry.handle,row->child_handle,&result)){
                unsupported++;
                continue;
            }
            joined++;
            if(!first_child){
                first_insert=entry.handle;
                first_child=row->child_handle;
            }
            ok=result.source_line_layer_and_owner_verified&&
               !result.final_visibility_color_and_paint_accepted&&
               result.insert_handle==entry.handle&&
               result.block_header_handle==rows.block_header_handle&&
               result.child_handle==row->child_handle&&
               result.layer_handle==row->source_layer_handle&&
               result.layer_name.indexed_handle==row->source_layer_handle&&
               result.raw_layer_flags.indexed_handle==row->source_layer_handle&&
               result.layer_name.original_record_crc_verified&&
               result.raw_layer_flags.source_crc_verified&&
               result.layer_name.utf8_bytes>0u&&
               result.source_color_index==row->source_color_index&&
               result.source_rgb24==row->source_rgb24&&
               result.source_style_class==row->source_style_class;
            for(unsigned k=0u;ok&&k<3u;k++)
                ok=isfinite(result.world_start[k])&&
                   isfinite(result.world_end[k])&&
                   result.world_start[k]==row->world_start[k]&&
                   result.world_end[k]==row->world_end[k];
            if(ok){
                MusaDwgAc1032InsertLineLayerSource repeat={0};
                ok=musa_dwg_ac1032_insert_line_layer_source(
                    scene,entry.handle,row->child_handle,&repeat)&&
                   !memcmp(&repeat,&result,sizeof(result));
            }
        }
    }
    if(ok)ok=census_inventories>0u&&census_unreadable>0u&&
             circle_unreadable>0u&&
             ot77_h12_verified>0u&&
             complete_style_positive>0u&&partial_style_rejected>0u&&
             inserts>0u&&source_rows>0u&&joined>0u&&
             joined+unsupported==source_rows;
    if(ok){
        MusaDwgAc1032InsertLineLayerSource untouched;
        memset(&untouched,0xa5,sizeof(untouched));
        const MusaDwgAc1032InsertLineLayerSource before=untouched;
        ok=!musa_dwg_ac1032_insert_line_layer_source(
               scene,first_insert,0u,&untouched)&&
           !memcmp(&untouched,&before,sizeof(before))&&
           !musa_dwg_ac1032_insert_line_layer_source(
               scene,first_insert,UINT64_MAX,&untouched)&&
           !memcmp(&untouched,&before,sizeof(before))&&
           !musa_dwg_ac1032_insert_line_layer_source(
               scene,UINT64_MAX,first_child,&untouched)&&
           !memcmp(&untouched,&before,sizeof(before))&&
           !musa_dwg_ac1032_insert_line_layer_source(
               NULL,first_insert,first_child,&untouched)&&
           !memcmp(&untouched,&before,sizeof(before))&&
           !musa_dwg_ac1032_insert_line_layer_source(
               scene,first_insert,first_child,NULL);
    }
    /* Original OT77 negative owner evidence; no owner role is inferred. */
    if(ok&&(first_unreadable_ot77||first_ot77_handle)){
        const uint64_t sample=first_unreadable_ot77?
            first_unreadable_ot77:first_ot77_handle;
        MusaDwgAc1032LWPolylineHTailProbe p={0};
        ok=musa_dwg_ac1032_scene_index_lwpolyline_h_tail_probe(
               scene,sample,&p)&&
           p.original_record_crc_verified&&
           p.indexed_handle==sample&&
           p.raw_object_type==77u&&
           !p.final_owner_layer_visibility_and_paint_accepted;
        if(ok){
            fprintf(stderr,"DIAG genuine OT77 source H=%llx "
                "first=%u/%u/%llx second=%u/%u/%llx "
                "residual=%llu zero_pad=%u one_pad=%u "
                "entity_mode=%u reactor=%u xdict_missing=%u\n",
                (unsigned long long)p.indexed_handle,
                p.first_ref_decoded,p.first_ref_code,
                (unsigned long long)p.first_raw_target,
                p.second_ref_decoded,p.second_ref_code,
                (unsigned long long)p.second_raw_target,
                (unsigned long long)p.residual_bits_after_two_refs,
                p.zero_tail_padding,p.one_tail_padding,
                p.entity_mode,p.reactor_count,p.xdictionary_missing);
            MusaDwgAc1032LWPolylineHTailProbe untouched;
            memset(&untouched,0xa5,sizeof(untouched));
            unsigned char before[sizeof(untouched)];
            memcpy(before,&untouched,sizeof(before));
            const int r0=musa_dwg_ac1032_scene_index_lwpolyline_h_tail_probe(
                scene,0u,&untouched);
            const int b0=!memcmp(&untouched,before,sizeof(before));
            const int r1=musa_dwg_ac1032_scene_index_lwpolyline_h_tail_probe(
                scene,UINT64_MAX,&untouched);
            const int b1=!memcmp(&untouched,before,sizeof(before));
            const int r2=musa_dwg_ac1032_scene_index_lwpolyline_h_tail_probe(
                NULL,sample,&untouched);
            const int b2=!memcmp(&untouched,before,sizeof(before));
            const int r3=musa_dwg_ac1032_scene_index_lwpolyline_h_tail_probe(
                scene,sample,NULL);
            fprintf(stderr,"DIAG genuine OT77 negative probes "
                "r=%d/%d/%d/%d unchanged=%d/%d/%d\n",
                r0,r1,r2,r3,b0,b1,b2);
            ok=!r0&&b0&&!r1&&b1&&!r2&&b2&&!r3;
        }
    }
    /* An unreadable original OT18 may be outside the strict CIRCLE
     * H-tail profile. Log that limitation without pretending it passed. */
    if(ok&&first_unreadable_ot18){
        MusaDwgAc1032CircleHTailProfile p={0};
        const int profile_ok=musa_dwg_ac1032_scene_index_circle_h_tail_profile(
            scene,first_unreadable_ot18,&p);
        fprintf(stderr,"DIAG genuine OT18 unreadable H=%llx "
            "profile_readable=%d status=%u first=%u/%u/%llx "
            "second=%u/%u/%llx residual=%llu\n",
            (unsigned long long)first_unreadable_ot18,profile_ok,
            p.profile_status,p.first_ref_decoded,p.first_ref_code,
            (unsigned long long)p.first_raw_target,
            p.second_ref_decoded,p.second_ref_code,
            (unsigned long long)p.second_raw_target,
            (unsigned long long)p.residual_bits_after_two_refs);
        if(profile_ok)
            ok=p.original_record_crc_verified&&
               p.indexed_handle==first_unreadable_ot18&&
               p.raw_object_type==18u&&
               !p.final_visibility_and_paint_accepted;
        MusaDwgAc1032CircleHTailProfile untouched;
        memset(&untouched,0xa5,sizeof(untouched));
        unsigned char before[sizeof(untouched)];
        memcpy(before,&untouched,sizeof(before));
        ok=ok&&!musa_dwg_ac1032_scene_index_circle_h_tail_profile(
                scene,0u,&untouched)&&
           !memcmp(&untouched,before,sizeof(before))&&
           !musa_dwg_ac1032_scene_index_circle_h_tail_profile(
                scene,UINT64_MAX,&untouched)&&
           !memcmp(&untouched,before,sizeof(before));
    }
    fprintf(stderr,"DIAG genuine R2018 OT18 unreadable fetch stages: "
                   "stage0=%u stage1=%u stage2=%u stage3=%u "
                   "stage4=%u stage5=%u total=%zu\n",
            circle_unreadable_stages[0],circle_unreadable_stages[1],
            circle_unreadable_stages[2],circle_unreadable_stages[3],
            circle_unreadable_stages[4],circle_unreadable_stages[5],
            circle_unreadable);
    fprintf(stderr,"DIAG genuine OT77 verified H12 owner edges=%zu\n",
            ot77_h12_verified);
    fprintf(stderr,"DIAG genuine R2018 owner gap by OT: "
                   "OT17=%u OT18=%u OT19=%u OT27=%u OT35=%u OT77=%u "
                   "total=%zu inventories=%zu\n",
            unreadable_by_type[0],unreadable_by_type[1],
            unreadable_by_type[2],unreadable_by_type[3],
            unreadable_by_type[4],unreadable_by_type[5],
            census_unreadable,census_inventories);
    fprintf(stderr,"DIAG genuine R2018 complete-owner LINE risk: "
                   "positive=%zu partial_rejected=%zu\n",
            complete_style_positive,partial_style_rejected);
    fprintf(stderr,"%s genuine R2018 INSERT LINE source OT51 layer: "
                   "%zu INSERT, %zu source LINE, %zu joined, "
                   "%zu unsupported; no paint\n",
            ok?"PASS":"FAIL",inserts,source_rows,joined,unsupported);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    return genuine(argv[1])?0:1;
}
