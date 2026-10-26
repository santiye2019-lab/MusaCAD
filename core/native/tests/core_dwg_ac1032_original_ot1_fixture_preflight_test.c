/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Separate genuine R2018 OT1 TEXT candidate: sample_2018.dwg, H=8E.
 * External same-corpus sample_2018.dxf lists TEXT handle 8E and
 * "Jen teksto simpla, cxu ne?". Do NOT confuse 2018/Text.dwg H1C0,
 * which is OT44 MTEXT. This only proves source H/OT identity, NOT TU
 * Unicode, geometry, font, layer, owner, color or rendering. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_sections.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int genuine(const char *filename){
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
    ok=ok&&memcmp(src,"AC1032",6u)==0;
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
         src,(size_t)n,&scene)&&
         memcmp(copy,src,(size_t)n)==0;
    if(!ok){
        size_t sections=0u,handles=0u;
        int catalog=musa_dwg_ac1032_section_catalog(
            src,(size_t)n,NULL,0u,&sections);
        int index=musa_dwg_ac1032_handles_index(
            src,(size_t)n,NULL,0u,&handles);
        fprintf(stderr,"OT1_SAMPLE_OPEN_FAIL: bytes=%ld catalog=%d "
             "sections=%zu handles_index=%d handles=%zu\n",
             n,catalog,sections,index,handles);
    }
    free(src);free(copy);
    if(!ok)return 0;
    const uint64_t text_handle=UINT64_C(0x8e);
    uint16_t type=0u;
    ok=musa_dwg_ac1032_scene_index_object_type(
         scene,text_handle,&type)&&type==1u;
    size_t found=0u,others=0u;
    const size_t count=musa_dwg_ac1032_scene_index_count(scene);
    for(size_t i=0u;i<count&&ok;i++){
        MusaDwgAc1032HandleEntry e={0};
        uint16_t current=0u;
        ok=musa_dwg_ac1032_scene_index_entry(scene,i,&e)&&
           musa_dwg_ac1032_scene_index_object_type(
              scene,e.handle,&current);
        if(!ok)break;
        if(current==1u){found++;if(e.handle!=text_handle)others++;}
    }
    ok=ok&&found>=1u;
    fprintf(stderr,"%s genuine R2018 sample OT1 TEXT H=8E "
       "indexed_text=%zu other_text=%zu; string/font/geometry UNVERIFIED\n",
       ok?"PASS":"FAIL",found,others);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0; /* host smoke: no external fixture */
    if(argc!=2)return 2;
    return genuine(argv[1])?0:1;
}
