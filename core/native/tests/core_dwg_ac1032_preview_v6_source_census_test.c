/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Strict original-source AC1032 2D preview admission census: a bulged
 * polyline remains ONE source object even when many v6 primitives result.
 * No full drawing equivalence, no guessed model space or layer visibility. */
#include "musacad_dwg_ac1032_preview_v6.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_ORIGINAL_DWG (64u*1024u*1024u)

static int read_source(const char *path,uint8_t **out,size_t *bytes){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long length=ftell(f);
    if(length<256||length>(long)MAX_ORIGINAL_DWG||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *p=(uint8_t*)malloc((size_t)length);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)length,f)==(size_t)length;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(p);return 0;}
    *out=p;*bytes=(size_t)length;return 1;
}
static int inspect(const char *path,uint16_t expected_type,
                   uint64_t handle){
    uint8_t *raw=NULL,*same=NULL;
    size_t n=0u;
    if(!read_source(path,&raw,&n))return 0;
    same=(uint8_t*)malloc(n);
    if(!same){free(raw);return 0;}
    memcpy(same,raw,n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(raw,n,&scene);
    if(ok)ok=!memcmp(raw,same,n);
    free(raw);free(same);
    raw=NULL;same=NULL;
    uint16_t type=0u;
    if(ok)ok=musa_dwg_ac1032_scene_index_object_type(
         scene,handle,&type)&&type==expected_type;
    MusaAc1032PreviewV6SourceCensus c={0},saved={0};
    MusaAc1032PreviewV6Stats p={0};
    if(ok)ok=musa_dwg_ac1032_preview_v6_source_census(
             scene,&c)&&
           musa_dwg_ac1032_preview_v6_planar_source_aci(
             scene,NULL,0u,&p)&&
           c.total_indexed_records==musa_dwg_ac1032_scene_index_count(scene)&&
           c.candidate_source_records>0u&&
           c.packet_admitted_source_records>0u&&
           c.packet_admitted_source_records<=c.candidate_source_records&&
           c.candidate_source_records==c.packet_admitted_source_records+
             c.candidate_not_admitted_records&&
           c.other_indexed_records>0u&&
           c.candidate_source_records+c.other_indexed_records==
             c.total_indexed_records&&
           c.candidate_not_admitted_records+c.other_indexed_records==
             c.omitted_v6_records&&
           c.emitted_v6_primitives>=c.packet_admitted_source_records&&
           c.emitted_v6_primitives==p.rendered_primitives&&
           c.omitted_v6_records==p.omitted_records&&
           c.total_indexed_records==p.total_indexed_records&&
           c.complete_dwg_display_accepted==0u&&
           p.partial_geometry==1u;
    if(ok){
        saved=c;
        ok=!musa_dwg_ac1032_preview_v6_source_census(scene,NULL)&&
           !musa_dwg_ac1032_preview_v6_source_census(
             NULL,&c)&&
           !memcmp(&saved,&c,sizeof(c));
    }
    if(ok){
        MusaAc1032PreviewV6SourceCensus recheck={0};
        ok=musa_dwg_ac1032_preview_v6_source_census(scene,&recheck)&&
           !memcmp(&c,&recheck,sizeof(c));
    }
    fprintf(stderr,"%s R2018 original type=%u indexed=%zu "
       "candidates=%zu admitted-source=%zu rejected-candidate=%zu "
       "other=%zu emitted-primitives=%zu, NO full-display claim\n",
       ok?"PASS":"FAIL",(unsigned)expected_type,
       c.total_indexed_records,c.candidate_source_records,
       c.packet_admitted_source_records,
       c.candidate_not_admitted_records,c.other_indexed_records,
       c.emitted_v6_primitives);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    MusaAc1032PreviewV6SourceCensus sentinel={0};
    sentinel.total_indexed_records=9191u;
    if(musa_dwg_ac1032_preview_v6_source_census(NULL,&sentinel)||
       musa_dwg_ac1032_preview_v6_source_census(NULL,NULL)||
       sentinel.total_indexed_records!=9191u)return 1;
    if(argc==1)return 0;
    if(argc!=7)return 2;
    return (inspect(argv[1],19u,UINT64_C(0x1ca))&&
            inspect(argv[2],27u,UINT64_C(0x1c0))&&
            inspect(argv[3],17u,UINT64_C(0x1bd))&&
            inspect(argv[4],18u,UINT64_C(0x1bd))&&
            inspect(argv[5],77u,UINT64_C(0x1bd))&&
            inspect(argv[6],16u,UINT64_C(0x1c1)))?0:1;
}
