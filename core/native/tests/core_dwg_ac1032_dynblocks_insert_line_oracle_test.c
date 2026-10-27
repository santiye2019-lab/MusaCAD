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
    uint64_t first_insert=0u,first_line=0u;
    double first_xy[4]={0};
    for(size_t i=0u;ok&&i<musa_dwg_ac1032_scene_index_count(scene);i++){
        MusaDwgAc1032HandleEntry entry={0};
        uint16_t type=0u;
        ok=musa_dwg_ac1032_scene_index_entry(scene,i,&entry)&&
           musa_dwg_ac1032_scene_index_object_type(
               scene,entry.handle,&type);
        if(!ok)break; /* Index/type failures must not be skipped. */
        if(type!=7u)continue;
        inserts++;
        MusaDwgAc1032InsertSourceGraph graph={0};
        MusaDwgAc1032InsertBlockChildInventory inventory={0};
        double affine[6]={0};
        if(musa_dwg_ac1032_scene_index_insert_source_graph(
               scene,entry.handle,&graph))graph_ok++;
        if(musa_dwg_ac1032_scene_index_insert_block_child_inventory(
               scene,entry.handle,&inventory))inventory_ok++;
        if(musa_dwg_ac1032_scene_index_original_insert_block_affine(
               scene,entry.handle,affine))affine_ok++;
        MusaDwgAc1032InsertBlockLineSources got={0};
        if(!musa_dwg_ac1032_scene_index_insert_block_line_sources(
               scene,entry.handle,&got))continue;
        line_sources_ok++;
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
    fprintf(stderr,"DIAG genuine R2018 INSERT gates: total=%zu graph=%zu "
                   "inventory=%zu affine=%zu line_sources=%zu segments=%zu\\n",
            inserts,graph_ok,inventory_ok,affine_ok,line_sources_ok,
            segments_ok);
    if(ok){
        MusaDwgAc1032InsertBlockLineSources out;
        memset(&out,0xa5,sizeof(out));
        const MusaDwgAc1032InsertBlockLineSources before=out;
        ok=!musa_dwg_ac1032_scene_index_insert_block_line_sources(
              scene,UINT64_MAX,&out)&&
           !memcmp(&out,&before,sizeof(out));
    }
    fprintf(stderr,"%s genuine R2018 Dynblocks INSERT source LINE: "
                   "%zu INSERT, %zu strict source joins, %zu lines; "
                   "first H%llx child H%llx (%.8f %.8f)-(%.8f %.8f); "
                   "paint never accepted\n",
            ok?"PASS":"FAIL",inserts,verified,admitted,
            (unsigned long long)first_insert,(unsigned long long)first_line,
            first_xy[0],first_xy[1],first_xy[2],first_xy[3]);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok&&inserts>0u&&verified>0u&&admitted>0u;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    return original(argv[1])?0:1;
}
