/* SPDX-License-Identifier: GPL-3.0-or-later */
/* First-party original R2018 direct ACI -> v6, with source line/point
 * independently decoded from actual CRC/H/OT-backed binary objects.
 * No runtime LibreDWG code. Synthetic selector cases validate edge ACI. */
#include "musacad_dwg_ac1032_preview_v6.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int close_to(float a,double b){
    return isfinite(a)&&fabs((double)a-b)<0.0002;
}
static int selector(void){
    MusaDwgAc1032EntityCommon c={0};
    c.common_decoded=1u;
    float code=-111.0f;uint8_t exact=3u;
    const uint16_t indices[]={1u,2u,7u,255u};
    for(unsigned kind=1u;kind<=4u;kind++){
        for(size_t i=0;i<sizeof(indices)/sizeof(indices[0]);i++){
            c.color_index=indices[i];c.has_rgb=0u;
            c.has_color_reference=0u;c.has_transparency=0u;
            c.invisibility=0u;
            if(!musa_dwg_ac1032_preview_v6_entity_aci_code(
                    &c,(uint8_t)kind,&code,&exact)||
               code!=(float)(kind*256u+indices[i])||exact!=1u)
               return 0;
        }
        const uint16_t inherited[]={0u,256u};
        for(size_t i=0;i<2u;i++){
            c.color_index=inherited[i];
            if(!musa_dwg_ac1032_preview_v6_entity_aci_code(
                 &c,(uint8_t)kind,&code,&exact)||
               code!=(float)(kind*256u+7u)||exact!=0u)return 0;
        }
        c.color_index=5u;
        c.has_rgb=1u;
        if(!musa_dwg_ac1032_preview_v6_entity_aci_code(
                &c,(uint8_t)kind,&code,&exact)||
            code!=(float)(kind*256u+7u)||exact)return 0;
        c.has_rgb=0u;c.has_color_reference=1u;
        if(!musa_dwg_ac1032_preview_v6_entity_aci_code(
                &c,(uint8_t)kind,&code,&exact)||
            code!=(float)(kind*256u+7u)||exact)return 0;
        c.has_color_reference=0u;c.has_transparency=1u;
        if(!musa_dwg_ac1032_preview_v6_entity_aci_code(
                &c,(uint8_t)kind,&code,&exact)||
            code!=(float)(kind*256u+7u)||exact)return 0;
        c.has_transparency=0u;c.invisibility=1u;
        if(!musa_dwg_ac1032_preview_v6_entity_aci_code(
                &c,(uint8_t)kind,&code,&exact)||
            code!=(float)(kind*256u+7u)||exact)return 0;
        c.invisibility=0u;
    }
    code=13579.0f;exact=17u;
    if(musa_dwg_ac1032_preview_v6_entity_aci_code(
        &c,0u,&code,&exact)||code!=13579.0f||exact!=17u)return 0;
    if(musa_dwg_ac1032_preview_v6_entity_aci_code(
        &c,5u,&code,&exact)||code!=13579.0f||exact!=17u)return 0;
    if(musa_dwg_ac1032_preview_v6_entity_aci_code(
        NULL,1u,&code,&exact)||code!=13579.0f||exact!=17u)return 0;
    return 1;
}
static int load(const char *name,uint8_t **raw,size_t *n){
    FILE *f=fopen(name,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    const long bytes=ftell(f);
    if(bytes<256||bytes>64L*1024*1024||
       fseek(f,0,SEEK_SET)){fclose(f);return 0;}
    uint8_t *p=(uint8_t*)malloc((size_t)bytes);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)bytes,f)==(size_t)bytes;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(p);return 0;}
    *raw=p;*n=(size_t)bytes;return 1;
}
static int original(const char *name,uint64_t handle,uint8_t kind,
                    double x,double y){
    uint8_t *raw=NULL,*same=NULL;size_t n=0u;
    if(!load(name,&raw,&n))return 0;
    same=(uint8_t*)malloc(n);
    if(!same){free(raw);return 0;}
    memcpy(same,raw,n);
    MusaDwgAc1032SceneIndex *index=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(raw,n,&index);
    MusaDwgAc1032EntityCommon common={0};
    if(ok&&kind==1u){
        MusaDwgAc1032NativeLine line={0};
        ok=musa_dwg_ac1032_scene_index_read_line(
           index,handle,&line)&&
           close_to((float)line.geometry.start[0],x)&&
           close_to((float)line.geometry.start[1],y);
        if(ok)common=line.common;
    }else if(ok&&kind==3u){
        MusaDwgAc1032NativePoint point={0};
        ok=musa_dwg_ac1032_scene_index_read_point(
           index,handle,&point)&&
           close_to((float)point.point[0],x)&&
           close_to((float)point.point[1],y);
        if(ok)common=point.common;
    }else ok=0;
    ok=ok&&!memcmp(raw,same,n);
    free(raw);free(same);
    raw=NULL;
    float expected_aci=0.0f;
    uint8_t exact=0u;
    if(ok)ok=musa_dwg_ac1032_preview_v6_entity_aci_code(
             &common,kind,&expected_aci,&exact);
    MusaAc1032PreviewV6Stats before={0},after={0};
    if(ok)ok=musa_dwg_ac1032_preview_v6_planar_source_aci(
          index,NULL,0u,&before)&&
          before.required_float_words>=10u&&
          before.rendered_primitives>=1u&&
          before.partial_geometry!=0u;
    float *words=NULL,*copy=NULL;
    if(ok){
        words=(float*)malloc(before.required_float_words*sizeof(float));
        copy=(float*)malloc(before.required_float_words*sizeof(float));
        if(!words||!copy)ok=0;
    }
    if(ok){
        for(size_t i=0u;i<before.required_float_words;i++)
            words[i]=-5.0f;
        memcpy(copy,words,before.required_float_words*sizeof(float));
        MusaAc1032PreviewV6Stats unchanged={0};
        unchanged.total_indexed_records=99u;
        ok=!musa_dwg_ac1032_preview_v6_planar_source_aci(
             index,words,before.required_float_words-1u,&unchanged)&&
             unchanged.total_indexed_records==99u&&
             !memcmp(copy,words,before.required_float_words*sizeof(float));
    }
    if(ok)ok=musa_dwg_ac1032_preview_v6_planar_source_aci(
           index,words,before.required_float_words,&after)&&
           !memcmp(&before,&after,sizeof(before))&&
           words[0]==6.0f&&words[6]==1.0f;
    int found=0;
    for(size_t at=7u;ok&&at<after.required_float_words;){
        const int code=(int)words[at++];
        const int type=code>>8;
        if(code<=0||(code&255)==0){ok=0;break;}
        if(type==1){
            if(at+4u>after.required_float_words){ok=0;break;}
            if(kind==1u&&(float)code==expected_aci&&
               close_to(words[at],x)&&close_to(words[at+1u],y))
               found++;
            at+=4u;
        }else if(type==3){
            if(at+2u>after.required_float_words){ok=0;break;}
            if(kind==3u&&(float)code==expected_aci&&
               close_to(words[at],x)&&close_to(words[at+1u],y))
               found++;
            at+=2u;
        }else if(type==4){
            if(at+8u>after.required_float_words){ok=0;break;}
            at+=8u;
        }else if(type==2){
            if(at>=after.required_float_words){ok=0;break;}
            const int signed_count=(int)words[at++];
            const size_t count=(size_t)abs(signed_count);
            if(count<2u||count>100000u||
               count>(after.required_float_words-at)/2u){ok=0;break;}
            at+=2u*count;
        }else ok=0;
    }
    ok=ok&&found>0;
    fprintf(stderr,"%s original AC1032 %s direct ACI v6="
         "%.0f verified exact=%u rendered=%zu partial=%u\n",
         ok?"PASS":"FAIL",kind==1u?"LINE":"POINT",
         expected_aci,(unsigned)exact,after.rendered_primitives,
         (unsigned)after.partial_geometry);
    free(words);free(copy);
    musa_dwg_ac1032_scene_index_close(index);
    return ok;
}
int main(int argc,char **argv){
    if(!selector())return 1;
    if(argc==1){
        MusaAc1032PreviewV6Stats s={0};
        s.total_indexed_records=12u;
        return (!musa_dwg_ac1032_preview_v6_planar_source_aci(
            NULL,NULL,0u,&s)&&s.total_indexed_records==12u)?0:1;
    }
    if(argc!=3)return 2;
    return original(argv[1],UINT64_C(0x1ca),1u,21.6078,2.81192)&&
           original(argv[2],UINT64_C(0x1bd),3u,27.928,8.65308)?0:1;
}
