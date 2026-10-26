/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent untouched R2018 sample_2018.dwg H8E AcDbText -> native v6
 * packet. Companion sample_2018.dxf is the independent text/anchor oracle.
 * No claim of font equivalence, viewport/owner or BYLAYER resolution. */
#include "musacad_dwg_ac1032_text_v6_packet.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int near(float a,double b){return isfinite(a)&&fabs((double)a-b)<0.00002;}
static int original(const char *filename){
    FILE *f=fopen(filename,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *bytes=(uint8_t*)malloc((size_t)n);
    uint8_t *copy=(uint8_t*)malloc((size_t)n);
    if(!bytes||!copy){free(bytes);free(copy);fclose(f);return 0;}
    int ok=fread(bytes,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(ok)memcpy(copy,bytes,(size_t)n);
    ok=ok&&!memcmp(bytes,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(bytes,(size_t)n,&scene)&&
       !memcmp(bytes,copy,(size_t)n);
    free(bytes);free(copy);
    if(!ok)return 0;
    const uint64_t h=UINT64_C(0x8e);
    MusaAc1032TextV6PacketStats planned={0},done={0};
    MusaDwgAc1032TextBundle b={0};
    MusaDwgAc1032EntityCommon c={0};
    ok=musa_dwg_ac1032_scene_text_bundle(scene,h,&b)&&
       musa_dwg_ac1032_scene_index_read_text_common(scene,h,&c)&&
       !strcmp(b.unicode.utf8,"Jen teksto simpla, cxu ne?")&&
       !strcmp(b.layer.utf8,"Tavolo 1")&&
       b.world.horizontal_alignment==4u&&
       musa_dwg_ac1032_text_v6_packet(scene,h,NULL,0u,&planned)&&
       planned.source_handle==h&&
       planned.source_layer_handle==UINT64_C(0x81)&&
       planned.source_crc_and_graph_verified&&
       planned.provisional_font_and_visibility&&
       !planned.final_display_accepted&&
       planned.utf8_bytes==26u&&
       planned.float_words>=19u;
    if(ok){
        float packet[1024]={0};
        float small[3]={17.f,17.f,17.f};
        MusaAc1032TextV6PacketStats before=planned;
        ok=!musa_dwg_ac1032_text_v6_packet(
             scene,h,small,3u,&planned)&&
           small[0]==17.f&&small[1]==17.f&&small[2]==17.f&&
           !memcmp(&before,&planned,sizeof(before))&&
           !musa_dwg_ac1032_text_v6_packet(
             scene,UINT64_MAX,small,3u,&planned)&&
           !memcmp(&before,&planned,sizeof(before))&&
           musa_dwg_ac1032_text_v6_packet(
             scene,h,packet,1024u,&done)&&
           !memcmp(&before,&done,sizeof(done));
        if(ok){
            float code=0.f,rgb=0.f;uint8_t extra=0u,exact=0u;
            ok=musa_dwg_ac1032_preview_v6_entity_source_style_code(
                &c,6u,&code,&rgb,&extra,&exact)&&
               packet[0]==code&&
               (!extra||packet[1]==rgb)&&
               done.source_direct_style_exact==exact;
            size_t at=(size_t)extra;
            ok=ok&&near(packet[at+1u],9.264596078552017)&&
               near(packet[at+2u],136.3624257905797)&&
               packet[at+3u]==5.f&&packet[at+4u]==0.f&&
               packet[at+5u]==0.f&&packet[at+6u]==-5.f&&
               packet[at+7u]==4.f&&packet[at+8u]==0.f&&
               packet[at+9u]==26.f&&
               done.float_words==10u+extra+(26u+2u)/3u;
            char recovered[27]={0};
            for(size_t i=0u;i<(26u+2u)/3u;i++){
                float raw=packet[at+10u+i];
                if(!isfinite(raw)||raw<0.f||raw>16777215.f||
                   floorf(raw)!=raw){ok=0;break;}
                uint32_t w=(uint32_t)raw;
                for(size_t j=0u;j<3u;j++){
                    size_t k=3u*i+j;
                    if(k<26u)recovered[k]=(char)((w>>(8u*j))&255u);
                }
            }
            ok=ok&&!strcmp(recovered,"Jen teksto simpla, cxu ne?");
        }
    }
    fprintf(stderr,"%s genuine R2018 OT1 H8E -> provisional native v6 "
        "TEXT; original Unicode and WCS alignment, generic font ONLY; "
        "owner/viewport/layer/final color unresolved\n",ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return original(argv[1])?0:1;
}
