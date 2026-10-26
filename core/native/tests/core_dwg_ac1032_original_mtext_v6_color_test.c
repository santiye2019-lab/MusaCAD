/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent original AutoCAD R2018 Text.dwg H1C0 OT44 -> v6 color
 * transport regression. This genuine file is BYLAYER (not direct ACI/RGB).
 * Direct ACI/RGB and invalid style classes use synthetic common fields
 * ONLY; they are not falsely presented as independent original fixtures.
 * Original font/layout, owner, layer/viewport visibility remain unknown. */
#include "musacad_dwg_ac1032_preview_v6.h"
#include "musacad_dwg_ac1032_color_provenance.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int select_color(void){
    MusaDwgAc1032EntityCommon c={0};
    float code=0.f,rgb=0.f;
    uint8_t extra=0u,exact=0u;
    c.common_decoded=1u;c.color_index=3u;
    if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
        &c,6u,&code,&rgb,&extra,&exact)||
       code!=1539.f||extra!=0u||exact!=1u)return 0;
    c.color_index=256u;c.has_rgb=1u;c.rgb=0x10203u;
    if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
        &c,6u,&code,&rgb,&extra,&exact)||
       code!=-1538.f||rgb!=66051.f||extra!=1u||exact!=1u)
       return 0;
    c.has_rgb=0u;
    if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
        &c,6u,&code,&rgb,&extra,&exact)||
       code!=1543.f||extra!=0u||exact!=0u)return 0;
    c.color_index=0u; /* BYBLOCK */
    if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
        &c,6u,&code,&rgb,&extra,&exact)||
       code!=1543.f||exact!=0u)return 0;
    c.color_index=4u;c.has_color_reference=1u;
    if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
        &c,6u,&code,&rgb,&extra,&exact)||
       code!=1543.f||exact!=0u)return 0;
    c.has_color_reference=0u;c.has_transparency=1u;
    if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
        &c,6u,&code,&rgb,&extra,&exact)||
       code!=1543.f||exact!=0u)return 0;
    c.has_transparency=0u;c.invisibility=1u;
    if(!musa_dwg_ac1032_preview_v6_entity_source_style_code(
        &c,6u,&code,&rgb,&extra,&exact)||
       code!=1543.f||exact!=0u)return 0;
    code=99.f;rgb=99.f;extra=99u;exact=99u;
    if(musa_dwg_ac1032_preview_v6_entity_source_style_code(
        &c,5u,&code,&rgb,&extra,&exact)||
       code!=99.f||rgb!=99.f||extra!=99u||exact!=99u)return 0;
    return 1;
}
static int packet(const float *v,size_t count,
                  const MusaDwgAc1032MTextSource *source){
    if(!v||count<18u||v[0]!=6.f||v[6]!=1.f)return 0;
    size_t i=7u,primitives=0u,mtext=0u;
    while(i<count){
        float raw=v[i++];
        if(!isfinite(raw)||raw==0.f||
           fabsf(raw)>16383.f||floorf(fabsf(raw))!=fabsf(raw))
            return 0;
        unsigned type=((unsigned)fabsf(raw))>>8u;
        if(type<1u||type>6u)return 0;
        if(raw<0.f){
            if(i>=count||!isfinite(v[i])||v[i]<0.f||
               v[i]>16777215.f||floorf(v[i])!=v[i])return 0;
            i++;
        }
        if(type==1u){if(count-i<4u)return 0;i+=4u;}
        else if(type==3u){if(count-i<2u)return 0;i+=2u;}
        else if(type==4u){if(count-i<8u)return 0;i+=8u;}
        else if(type==2u||type==5u){
            if(i>=count||!isfinite(v[i])||fabsf(v[i])>100000.f||
               floorf(v[i])!=v[i])return 0;
            size_t n=(size_t)fabsf(v[i++]);
            if(n<2u||n>(count-i)/2u)return 0;
            i+=n*2u;
        }else{
            /* Genuine Text.dwg H1C0 source is BYLAYER: NEVER promote
             * LAYER0 ACI7 to an exact inherited or visible style. */
            if(raw!=1543.f||count-i<9u)return 0;
            const float *g=v+i;
            if(!isfinite(g[8])||floorf(g[8])!=g[8]||
               g[8]!=(float)source->utf8_bytes)return 0;
            size_t bytes=(size_t)g[8];
            if(bytes==0u||bytes>=2048u)return 0;
            i+=9u;
            size_t packed=(bytes+2u)/3u;
            if(packed>count-i)return 0;
            char actual[2048]={0};
            for(size_t b=0u;b<packed;b++){
                if(!isfinite(v[i+b])||v[i+b]<0.f||
                   v[i+b]>16777215.f||
                   floorf(v[i+b])!=v[i+b])return 0;
                uint32_t value=(uint32_t)v[i+b];
                for(unsigned j=0u;j<3u;j++){
                    size_t at=b*3u+j;
                    if(at<bytes)
                        actual[at]=(char)((value>>(8u*j))&0xffu);
                }
            }
            i+=packed;
            if(memcmp(actual,source->utf8,bytes)!=0)return 0;
            mtext++;
        }
        primitives++;
    }
    return i==count&&mtext==1u&&primitives==(size_t)v[1];
}
static int original(const char *filename){
    FILE *f=fopen(filename,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *bytes=(uint8_t*)malloc((size_t)n);
    uint8_t *snapshot=(uint8_t*)malloc((size_t)n);
    if(!bytes||!snapshot){
        free(bytes);free(snapshot);fclose(f);return 0;
    }
    int ok=fread(bytes,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    memcpy(snapshot,bytes,(size_t)n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
        bytes,(size_t)n,&scene)&&
        memcmp(bytes,snapshot,(size_t)n)==0;
    free(bytes);free(snapshot);
    if(!ok)return 0;
    const uint64_t h=UINT64_C(0x1c0);
    MusaDwgAc1032EntityCommon common={0};
    MusaDwgAc1032MTextSource source={0};
    MusaAc1032ColorProvenance color={0};
    MusaAc1032PreviewV6Stats planned={0},done={0};
    ok=musa_dwg_ac1032_scene_index_read_mtext_common(
         scene,h,&common)&&
       musa_dwg_ac1032_scene_index_read_mtext_source(
         scene,h,&source)&&
       musa_dwg_ac1032_scene_planar_color_provenance(
         scene,h,&color)&&
       source.original_record_crc_verified&&
       source.indexed_handle==h&&
       common.prefix.raw_object_type==44u&&
       common.color_index==256u&&
       common.invisibility==0u&&
       color.entity_handle==h&&
       color.raw_entity_type==44u&&
       color.layer_handle==UINT64_C(0x10)&&
       color.entity_and_layer_crc_verified&&
       color.entity_by_layer==1u&&
       !color.entity_has_direct_aci&&
       !color.entity_has_direct_rgb24&&
       !color.source_entity_invisible&&
       !color.rendering_accepted&&
       musa_dwg_ac1032_preview_v6_planar_source_with_mtext(
         scene,NULL,0u,&planned)&&
       planned.partial_geometry==1u&&
       planned.provisional_aci_style==1u&&
       planned.required_float_words>=18u;
    if(ok){
        float *v=(float*)malloc(planned.required_float_words*sizeof(float));
        if(!v)ok=0;
        else{
            float small[4]={31.f,31.f,31.f,31.f};
            MusaAc1032PreviewV6Stats unchanged=planned;
            if(musa_dwg_ac1032_preview_v6_planar_source_with_mtext(
                scene,small,4u,&unchanged)||
               small[0]!=31.f||
               memcmp(&unchanged,&planned,sizeof(planned))!=0)ok=0;
            if(ok)ok=musa_dwg_ac1032_preview_v6_planar_source_with_mtext(
                scene,v,planned.required_float_words,&done)&&
                memcmp(&done,&planned,sizeof(done))==0&&
                packet(v,planned.required_float_words,&source);
            free(v);
        }
    }
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    int ok=select_color();
    if(argc==2)ok=ok&&original(argv[1]);
    else if(argc!=1) return 2;
    fprintf(stderr,"%s OT44 direct ACI/RGB selector (synthetic) + "
            "genuine R2018 H1C0 BYLAYER remains provisional v6; "
            "visibility/owner/font NOT resolved\n",ok?"PASS":"FAIL");
    return ok?0:1;
}
