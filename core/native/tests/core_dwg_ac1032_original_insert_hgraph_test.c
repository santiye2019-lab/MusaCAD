/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine unchanged R2018 example_2018.dwg vs independent companion DXF:
 * INSERT H1C8/H190/H756, LAYER/BLOCK_RECORD, ATTRIB and SEQEND handles.
 * Source H/CRC and insertion-point math only. No nested block expansion or visible scene claim. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_insert_binding.h"
#include "musacad_dwg_ac1032_insert_transform.h"
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct Expected {
    uint64_t h,layer,block,attrib,seqend;
    uint32_t count;
} Expected;
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
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return genuine(argv[1])?0:1;
}
