/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine original R2018 sample_2018.dwg OT1 TEXT H8E native scene v6
 * opt-in. Source TEXT remains provisional: font/owner/viewport/visibility
 * and inherited layer color are NOT verified. */
#include "musacad_dwg_ac1032_text_v6_packet.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int original(const char *filename){
    FILE *f=fopen(filename,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *src=(uint8_t*)malloc((size_t)n);
    uint8_t *copy=(uint8_t*)malloc((size_t)n);
    if(!src||!copy){free(src);free(copy);fclose(f);return 0;}
    int ok=fread(src,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(ok)memcpy(copy,src,(size_t)n);
    ok=ok&&!memcmp(src,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(src,(size_t)n,&scene)&&
       !memcmp(src,copy,(size_t)n);
    free(src);free(copy);
    if(!ok)return 0;
    MusaAc1032TextV6PacketStats one={0};
    MusaAc1032PreviewV6Stats old={0},all={0},written={0};
    MusaAc1032PreviewV6SourceCensus census={0};
    ok=musa_dwg_ac1032_text_v6_packet(
         scene,UINT64_C(0x8e),NULL,0u,&one)&&
       musa_dwg_ac1032_preview_v6_planar_source_with_mtext(
         scene,NULL,0u,&old)&&
       musa_dwg_ac1032_preview_v6_planar_source_with_text(
         scene,NULL,0u,&all)&&
       musa_dwg_ac1032_preview_v6_source_census_with_text(
         scene,&census)&&
       all.rendered_primitives>old.rendered_primitives&&
       all.required_float_words>old.required_float_words&&
       all.partial_geometry==1u&&
       all.provisional_aci_style==1u&&
       census.emitted_v6_primitives==all.rendered_primitives&&
       census.omitted_v6_records==all.omitted_records&&
       !census.complete_dwg_display_accepted;
    if(ok){
        float *packet=(float*)malloc(one.float_words*sizeof(float));
        float *scene_words=(float*)malloc(all.required_float_words*sizeof(float));
        if(!packet||!scene_words)ok=0;
        else{
            MusaAc1032TextV6PacketStats again={0};
            ok=musa_dwg_ac1032_text_v6_packet(
                 scene,UINT64_C(0x8e),packet,one.float_words,&again)&&
               !memcmp(&one,&again,sizeof(one))&&
               musa_dwg_ac1032_preview_v6_planar_source_with_text(
                 scene,scene_words,all.required_float_words,&written)&&
               !memcmp(&written,&all,sizeof(all));
            if(ok){
                size_t found=0u;
                for(size_t i=7u;i+one.float_words<=all.required_float_words;i++)
                    if(!memcmp(scene_words+i,packet,
                         one.float_words*sizeof(float)))found++;
                ok=found==1u&&scene_words[0]==6.f&&
                   scene_words[1]==(float)all.rendered_primitives&&
                   scene_words[6]==1.f;
            }
            float tiny[2]={31.f,31.f};
            MusaAc1032PreviewV6Stats before=all;
            if(musa_dwg_ac1032_preview_v6_planar_source_with_text(
                 scene,tiny,2u,&all)||tiny[0]!=31.f||tiny[1]!=31.f||
               memcmp(&all,&before,sizeof(all)))ok=0;
        }
        free(packet);free(scene_words);
    }
    fprintf(stderr,"%s genuine OT1 H8E appears ONCE in opt-in native "
        "v6 scene; Unicode/WCS source exact, glyph/owner/viewport provisional\n",
        ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return original(argv[1])?0:1;
}
