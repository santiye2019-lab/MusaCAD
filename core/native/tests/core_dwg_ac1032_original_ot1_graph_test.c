/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine R2018 sample_2018.dwg H8E OT1 TEXT source H graph,
 * not the unrelated 2018/Text.dwg H1C0 OT44 MTEXT graph. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int sample(const char *filename){
    FILE *f=fopen(filename,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *src=(uint8_t*)malloc((size_t)n);
    uint8_t *copy=(uint8_t*)malloc((size_t)n);
    if(!src||!copy){free(src);free(copy);fclose(f);return 0;}
    int ok=fread(src,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(ok)memcpy(copy,src,(size_t)n);
    ok=ok&&!memcmp(src,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
         src,(size_t)n,&scene)&&!memcmp(src,copy,(size_t)n);
    free(src);free(copy);
    if(!ok)return 0;
    const uint64_t h=UINT64_C(0x8e);
    MusaDwgAc1032TextGraph graph={0};
    MusaDwgAc1032TextSource text={0};
    MusaDwgAc1032EntityCommon common={0};
    MusaDwgAc1032LayerName layer={0};
    ok=musa_dwg_ac1032_scene_index_read_text_common(
           scene,h,&common)&&
       musa_dwg_ac1032_scene_index_read_text_source(
           scene,h,&text)&&
       !strcmp(text.utf8,"Jen teksto simpla, cxu ne?")&&
       musa_dwg_ac1032_scene_index_text_graph(
           scene,h,&graph)&&graph.graph_edges_verified&&
       graph.original_entity_crc_verified&&
       graph.original_layer_crc_verified&&
       graph.original_style_crc_verified&&
       !graph.font_and_display_accepted&&
       graph.indexed_handle==h&&
       graph.layer_object_type==51u&&
       musa_dwg_ac1032_scene_index_read_layer_name(
           scene,graph.layer_handle,&layer)&&
       layer.original_record_crc_verified&&
       !strcmp(layer.utf8,"Tavolo 1");
    if(ok){
        MusaDwgAc1032TextGraph bad;
        memset(&bad,0xa5,sizeof(bad));
        const MusaDwgAc1032TextGraph before=bad;
        ok=!musa_dwg_ac1032_scene_index_text_graph(
                scene,UINT64_MAX,&bad)&&
           !memcmp(&bad,&before,sizeof(bad));
    }
    fprintf(stderr,"%s original OT1 TEXT H8E mode=%u "
        "layer=%llX style=%llX layer_name='%s' "
        "font/owner/model-space/viewport UNVERIFIED\n",
        ok?"PASS":"FAIL",(unsigned)common.prefix.entity_mode,
        (unsigned long long)graph.layer_handle,
        (unsigned long long)graph.style_handle,layer.utf8);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return sample(argv[1])?0:1;
}
