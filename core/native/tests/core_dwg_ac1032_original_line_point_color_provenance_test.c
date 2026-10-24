/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real AutoCAD R2018 LINE+POINT+ARC+CIRCLE original DWG full source-to-source
 * entity ENC color + LAYER0 CMC evidence. Never certifies visibility. */
#include "musacad_dwg_ac1032_color_provenance.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int load(const char *name,uint8_t **out,size_t *len){
    FILE *f=fopen(name,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *bytes=(uint8_t*)malloc((size_t)n);
    if(!bytes){fclose(f);return 0;}
    int ok=fread(bytes,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(bytes);return 0;}
    *out=bytes;*len=(size_t)n;return 1;
}
static int fixture(const char *file,uint64_t handle,uint16_t type){
    uint8_t *raw=NULL,*copy=NULL;size_t len=0u;
    if(!load(file,&raw,&len))return 0;
    copy=(uint8_t*)malloc(len);
    if(!copy){free(raw);return 0;}
    memcpy(copy,raw,len);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(raw,len,&scene)&&
           !memcmp(copy,raw,len);
    /* This cached scene must remain valid after original input is freed. */
    free(copy);free(raw);raw=NULL;copy=NULL;
    MusaAc1032ColorProvenance p={0};
    if(ok)ok=musa_dwg_ac1032_scene_planar_color_provenance(
         scene,handle,&p)&&
         p.entity_handle==handle&&
         p.layer_handle==UINT64_C(0x10)&&
         p.raw_entity_type==type&&
         p.entity_and_layer_crc_verified&&
         p.source_layer_aci_supported==1u&&
         p.source_layer_aci==7u&&
         p.rendering_accepted==0u;
    if(ok){
        const unsigned classes=(unsigned)p.entity_has_direct_aci+
          (unsigned)p.entity_by_layer+(unsigned)p.entity_by_block+
          (unsigned)p.entity_complex_color_or_transparency;
        ok=classes==1u;
        if(ok&&p.entity_by_layer)ok=p.entity_aci==256u;
        if(ok&&p.entity_by_block)ok=p.entity_aci==0u;
        if(ok&&p.entity_has_direct_aci)
            ok=p.entity_aci>=1u&&p.entity_aci<=255u;
    }
    if(ok){
        const MusaAc1032ColorProvenance copy=p;
        MusaAc1032ColorProvenance legacy={0};
        if(type==17u||type==18u){
            ok=!musa_dwg_ac1032_scene_line_point_color_provenance(
                 scene,handle,&p)&&!memcmp(&copy,&p,sizeof(p));
        }else{
            ok=musa_dwg_ac1032_scene_line_point_color_provenance(
                 scene,handle,&legacy)&&
                 !memcmp(&legacy,&copy,sizeof(legacy));
        }
    }
    if(ok){
        const MusaAc1032ColorProvenance old=p;
        ok=!musa_dwg_ac1032_scene_planar_color_provenance(
              scene,UINT64_C(0x10),&p)&&
           !memcmp(&old,&p,sizeof(p))&&
           !musa_dwg_ac1032_scene_planar_color_provenance(
              scene,UINT64_MAX,&p)&&
           !memcmp(&old,&p,sizeof(p))&&
           !musa_dwg_ac1032_scene_planar_color_provenance(
              scene,handle,NULL);
    }
    fprintf(stderr,"%s source REAL R2018 OT%u entity H=0x%llX"
       " ENC=%u direct=%u BYLAYER=%u BYBLOCK=%u complex=%u "
       "linked LAYER0 ACI=%u; renderer NOT accepted\n",
       ok?"PASS":"FAIL",(unsigned)type,(unsigned long long)handle,
       (unsigned)p.entity_aci,(unsigned)p.entity_has_direct_aci,
       (unsigned)p.entity_by_layer,(unsigned)p.entity_by_block,
       (unsigned)p.entity_complex_color_or_transparency,
       (unsigned)p.source_layer_aci);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1){
        MusaAc1032ColorProvenance p={0};p.source_layer_aci=119u;
        return !musa_dwg_ac1032_scene_line_point_color_provenance(
                 NULL,UINT64_C(0x1ca),&p)&&
               p.source_layer_aci==119u?0:1;
    }
    if(argc!=5)return 2;
    const int line=fixture(argv[1],UINT64_C(0x1ca),19u);
    const int point=fixture(argv[2],UINT64_C(0x1c0),27u);
    const int arc=fixture(argv[3],UINT64_C(0x1bd),17u);
    const int circle=fixture(argv[4],UINT64_C(0x1bd),18u);
    return line&&point&&arc&&circle?0:1;
}
