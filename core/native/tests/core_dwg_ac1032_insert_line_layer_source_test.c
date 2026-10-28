/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original untouched R2018 Dynblocks.dwg. Source-only child LINE layer
 * Unicode/status and raw color join; never grants final paint. */
#include "musacad_dwg_ac1032_insert_line_source_segments.h"
#include "musacad_dwg_ac1032_complete_block_line_style_risk.h"
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
    uint64_t first_insert=0u,first_child=0u;
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
            if(musa_dwg_ac1032_block_owner_coverage_complete(&coverage)){
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
    if(ok)ok=complete_style_positive>0u&&partial_style_rejected>0u&&
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
