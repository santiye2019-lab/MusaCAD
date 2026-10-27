/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original R2018 Text.dwg OT44 H1C0 genuine Unicode -> provisional native
 * v6 type 6 packet. IMPORTANT: this is NOT native AutoCAD MTEXT formatting,
 * font rendering, BYLAYER resolving or model-space visibility acceptance.
 * No LibreDWG runtime. */
#include "musacad_dwg_ac1032_preview_v6.h"
#include "musacad_dwg_ac1032_color_provenance.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
static int near(double a,double b,double eps){
    return isfinite(a)&&fabs(a-b)<eps;
}
static int check(const float *words,size_t length,
                 const MusaDwgAc1032MTextSource *source,
                 const MusaAc1032ColorProvenance *color){
    if(!words||length<18u||words[0]!=6.0f||
       words[1]<1.0f||words[6]!=1.0f)return 0;
    size_t at=7u,parsed=0u,found=0u;
    while(at<length){
        const float raw=words[at++];
        if(!isfinite(raw)||raw==0.0f||fabsf(raw)>16383.0f)
            return 0;
        const float direct_rgb_word=raw<0.0f&&at<length?words[at]:0.0f;
        const uint32_t packed=(uint32_t)fabsf(raw);
        const uint32_t type=packed>>8u;
        if(type<1u||type>6u)return 0;
        if(raw<0.0f){
            if(at>=length||!isfinite(words[at])||
               words[at]<0.0f||words[at]>16777215.0f)return 0;
            at++;
        }
        if(type==1u){if(length-at<4u)return 0;at+=4u;}
        else if(type==3u){if(length-at<2u)return 0;at+=2u;}
        else if(type==4u){if(length-at<8u)return 0;at+=8u;}
        else if(type==2u||type==5u){
            if(at>=length)return 0;
            int n=(int)fabsf(words[at++]);
            if(n<2||n>100000||(size_t)n>(length-at)/2u)return 0;
            at+=2u*(size_t)n;
        }else if(type==6u){
            if(length-at<9u)return 0;
            const float *g=words+at;
            const int bytes=(int)g[8];
            at+=9u;
            if(bytes<=0||bytes>2047||((size_t)bytes+2u)/3u>
                     length-at)return 0;
            char buffer[2048]={0};
            const size_t n=((size_t)bytes+2u)/3u;
            for(size_t k=0;k<n;k++){
                const float f=words[at+k];
                if(!isfinite(f)||f<0.0f||f>16777215.0f||
                   floorf(f)!=f)return 0;
                const uint32_t bits=(uint32_t)f;
                for(unsigned j=0u;j<3u;j++){
                    const size_t target=k*3u+j;
                    if(target<(size_t)bytes)
                        buffer[target]=(char)((bits>>(8u*j))&0xffu);
                }
            }
            at+=n;buffer[bytes]='\0';
            if(bytes==(int)source->utf8_bytes&&
               strcmp(buffer,source->utf8)==0){
                /* Independent original CRC/H OT44 -> OT51 ENC oracle:
                 * verify the actual native v6 first-frame style word. */
                const float expected=color->entity_has_direct_rgb24?
                    -1538.0f:color->entity_has_direct_aci?
                    (float)(1536u+color->entity_aci):1543.0f;
                if(raw!=expected||
                   (color->entity_has_direct_rgb24&&
                    direct_rgb_word!=(float)color->entity_source_rgb24))
                    return 0;
                if(!near(g[0],7.33298,.0003)||
                   !near(g[1],14.1158,.0003)||
                   !near(g[2],.2,.00001)||
                   !near(g[3],0.,.00001)||
                   !near(g[4],0.,.00001)||
                   !near(g[5],-.2,.00001)||
                   !near(g[6],0.,.00001)||
                   !near(g[7],3.,.00001)||
                   !strstr(buffer,"multiline text"))
                    return 0;
                found++;
            }
        }
        parsed++;
    }
    return at==length&&parsed==(size_t)words[1]&&found==1u;
}
int main(int argc,char **argv){
    if(argc==1)return 0; /* CTest smoke without external golden fixture */
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");
    if(!f)return 1;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 1;}
    long file_bytes=ftell(f);
    if(file_bytes<256L||file_bytes>64L*1024L*1024L||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 1;}
    uint8_t *src=(uint8_t*)malloc((size_t)file_bytes);
    uint8_t *snapshot=(uint8_t*)malloc((size_t)file_bytes);
    if(!src||!snapshot){free(src);free(snapshot);fclose(f);return 1;}
    int ok=fread(src,1u,(size_t)file_bytes,f)==(size_t)file_bytes;
    if(fclose(f)!=0)ok=0;
    memcpy(snapshot,src,(size_t)file_bytes);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
                 src,(size_t)file_bytes,&scene)&&
              memcmp(snapshot,src,(size_t)file_bytes)==0;
    free(src);free(snapshot);
    if(!ok){fprintf(stderr,"FAIL authentic MTEXT source open\n");return 1;}
    MusaDwgAc1032MTextSource text={0};
    MusaAc1032ColorProvenance color={0};
    MusaAc1032PreviewV6Stats planned={0},done={0};
    MusaAc1032PreviewV6SourceCensus census={0};
    ok=musa_dwg_ac1032_scene_index_read_mtext_source(
           scene,UINT64_C(0x1c0),&text)&&
       text.utf8_bytes>0u&&
       text.original_record_crc_verified&&
       musa_dwg_ac1032_scene_planar_color_provenance(
           scene,UINT64_C(0x1c0),&color)&&
       color.entity_handle==UINT64_C(0x1c0)&&
       color.raw_entity_type==44u&&
       color.entity_and_layer_crc_verified&&
       !color.source_entity_invisible&&
       !color.rendering_accepted&&
       !text.geometry_style_and_glyphs_accepted&&
       musa_dwg_ac1032_preview_v6_planar_source_with_mtext(
           scene,NULL,0u,&planned)&&
       planned.partial_geometry==1u&&planned.rendered_primitives>=1u&&
       planned.required_float_words>=18u&&
       musa_dwg_ac1032_preview_v6_source_census_with_mtext(
           scene,&census)&&
       census.emitted_v6_primitives==planned.rendered_primitives&&
       census.packet_admitted_source_records>=1u&&
       !census.complete_dwg_display_accepted;
    /* Original R2018 MTEXT is retained as positive gold. These synthetic
     * Unicode controls are negative cases, never claimed as DWG fixtures. */
    if(ok){
        static const char bidi_override[]="A\\xE2\\x80\\xAE""B";
        static const char bidi_isolate[]="A\\xE2\\x81\\xA7""B";
        static const char zero_width[]="A\\xE2\\x80\\x8B""B";
        static const char direction_mark[]="A\\xD8\\x9C""B";
        ok=!musa_dwg_ac1032_mtext_v6_plain_source_preflight(
                 bidi_override,sizeof(bidi_override)-1u)&&
           !musa_dwg_ac1032_mtext_v6_plain_source_preflight(
                 bidi_isolate,sizeof(bidi_isolate)-1u)&&
           !musa_dwg_ac1032_mtext_v6_plain_source_preflight(
                 zero_width,sizeof(zero_width)-1u)&&
           !musa_dwg_ac1032_mtext_v6_plain_source_preflight(
                 direction_mark,sizeof(direction_mark)-1u)&&
           !musa_dwg_ac1032_mtext_v6_plain_source_preflight(
                 "A\\xC2\\xAD""B",sizeof("A\\xC2\\xAD""B")-1u)&&
           !musa_dwg_ac1032_mtext_v6_plain_source_preflight(
                 "A\\xE2\\x81\\xA0""B",sizeof("A\\xE2\\x81\\xA0""B")-1u)&&
           !musa_dwg_ac1032_mtext_v6_plain_source_preflight(
                 "A\\xEF\\xBF\\xB9""B",sizeof("A\\xEF\\xBF\\xB9""B")-1u)&&
           musa_dwg_ac1032_mtext_v6_plain_source_preflight(
                 "Plain MTEXT",sizeof("Plain MTEXT")-1u);
    }
    if(ok){
        float *packet=(float*)malloc(planned.required_float_words*
                                     sizeof(*packet));
        if(!packet)ok=0;
        else{
            float small[8]={77.f,77.f,77.f,77.f,77.f,77.f,77.f,77.f};
            MusaAc1032PreviewV6Stats before=planned;
            if(musa_dwg_ac1032_preview_v6_planar_source_with_mtext(
                  scene,small,8u,&before)||small[0]!=77.f||
               memcmp(&before,&planned,sizeof(before))!=0)ok=0;
            if(ok)ok=musa_dwg_ac1032_preview_v6_planar_source_with_mtext(
                scene,packet,planned.required_float_words,&done)&&
                memcmp(&planned,&done,sizeof(done))==0&&
                check(packet,planned.required_float_words,&text,&color);
            free(packet);
        }
    }
    fprintf(stderr,"%s source R2018 original MTEXT H1C0 -> real "
           "Unicode WCS to PROVISIONAL native v6 TEXT; "
           "font/paragraph/colors/visibility UNVERIFIED\n",
           ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok?0:1;
}
