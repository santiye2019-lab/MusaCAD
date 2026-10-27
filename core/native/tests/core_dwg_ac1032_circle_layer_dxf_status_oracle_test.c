/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine R2018 original H1C8 INSERT/H1BB CIRCLE/H8A LAYER source status.
 * DXF group 70 is separately read, NOT equated to DWG packed 70/290/370.
 * Unknown viewport, frozen/off and overrides continue to veto paint. */
#include "musacad_dwg_ac1032_circle_layer_visibility_veto.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct DxfOracle {
    unsigned layer_count,circle_count,insert_count,layer70_count,layer62_count;
    unsigned circle62_count,circle420_count,circle60_count;
    long group70,aci;
} DxfOracle;
static void trim(char *s){
    size_t n=strlen(s);
    while(n&&(s[n-1]=='\n'||s[n-1]=='\r'||s[n-1]==' '||
              s[n-1]=='\t'))s[--n]='\0';
    size_t i=0u;
    while(s[i]==' '||s[i]=='\t')i++;
    if(i)memmove(s,s+i,strlen(s+i)+1u);
}
static void finish(DxfOracle *o,const char *type,uint64_t h,
                   unsigned has70,long group70,unsigned has62,long aci,
                   unsigned has420,unsigned has60){
    if(!strcmp(type,"INSERT")&&h==UINT64_C(0x1c8))o->insert_count++;
    if(!strcmp(type,"CIRCLE")&&h==UINT64_C(0x1bb)){
        o->circle_count++;
        o->circle62_count+=has62;o->circle420_count+=has420;
        o->circle60_count+=has60;
    }
    if(!strcmp(type,"LAYER")&&h==UINT64_C(0x8a)){
        o->layer_count++;
        o->layer70_count+=has70;o->layer62_count+=has62;
        o->group70=group70;o->aci=aci;
    }
}
static int independent_dxf(const char *path,DxfOracle *o){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    char code[128],value[1024],type[64]={0};
    uint64_t h=0u;
    unsigned has70=0u,has62=0u,has420=0u,has60=0u;
    long group70=0L,aci=0L;
    int ok=1;
    while(fgets(code,sizeof(code),f)&&fgets(value,sizeof(value),f)){
        trim(code);trim(value);
        char *end=NULL;
        long group=strtol(code,&end,10);
        if(end==code||*end){ok=0;break;}
        if(group==0){
            finish(o,type,h,has70,group70,has62,aci,has420,has60);
            if(strlen(value)>=sizeof(type)){ok=0;break;}
            strcpy(type,value);
            h=0u;has70=has62=has420=has60=0u;
            group70=aci=0L;
        }else if(group==5){
            char *tail=NULL;
            unsigned long long v=strtoull(value,&tail,16);
            if(tail!=value&&!*tail)h=(uint64_t)v;
        }else if(group==70&&!strcmp(type,"LAYER")){
            char *tail=NULL;long v=strtol(value,&tail,10);
            if(tail==value||*tail||v<0L||v>255L){ok=0;break;}
            has70=1u;group70=v;
        }else if(group==62){
            char *tail=NULL;long v=strtol(value,&tail,10);
            if(tail==value||*tail||v<-32768L||v>32767L){ok=0;break;}
            has62=1u;aci=v;
        }else if(group==420&&!strcmp(type,"CIRCLE")){
            has420=1u;
        }else if(group==60&&!strcmp(type,"CIRCLE")){
            has60=1u;
        }
    }
    if(ok)finish(o,type,h,has70,group70,has62,aci,has420,has60);
    if(ferror(f))ok=0;
    if(fclose(f)!=0)ok=0;
    return ok&&o->insert_count==1u&&o->circle_count==1u&&
           o->layer_count==1u&&o->layer70_count==1u&&
           o->layer62_count==1u&&o->aci==4L&&
           !o->circle62_count&&!o->circle420_count&&!o->circle60_count;
}
static int run(const char *dwg,const char *dxf){
    DxfOracle oracle={0};
    if(!independent_dxf(dxf,&oracle))return 0;
    FILE *f=fopen(dwg,"rb");
    if(!f)return 0;
    int ok=fseek(f,0,SEEK_END)==0;
    long n=ok?ftell(f):-1L;
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)!=0)ok=0;
    uint8_t *bytes=ok?(uint8_t*)malloc((size_t)n):NULL;
    uint8_t *copy=bytes?(uint8_t*)malloc((size_t)n):NULL;
    if(!bytes||!copy)ok=0;
    if(ok)ok=fread(bytes,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(ok)memcpy(copy,bytes,(size_t)n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(bytes,"AC1032",6u)&&
       musa_dwg_ac1032_scene_index_open(bytes,(size_t)n,&scene)&&
       !memcmp(bytes,copy,(size_t)n);
    free(bytes);free(copy);
    if(!ok){musa_dwg_ac1032_scene_index_close(scene);return 0;}
    MusaDwgAc1032CircleLayerStatusPreflight v={0};
    ok=musa_dwg_ac1032_circle_layer_status_preflight(
           scene,UINT64_C(0x1c8),UINT64_C(0x1bb),&v)&&
       v.source.chain.layer_handle==UINT64_C(0x8a)&&
       v.source.chain.original_layer_aci==(uint8_t)oracle.aci&&
       v.candidate_bits.layer_handle==UINT64_C(0x8a)&&
       v.candidate_bits.source_crc_verified&&
       v.candidate_bits.raw_values==
         v.source.original_layer_flags.raw_values_70_290_370&&
       v.candidate_bits.frozen_bit==
         ((v.candidate_bits.raw_values>>0u)&1u)&&
       v.candidate_bits.on_bit==
         ((v.candidate_bits.raw_values>>1u)&1u)&&
       v.candidate_bits.locked_bit==
         ((v.candidate_bits.raw_values>>3u)&1u)&&
       v.original_layer_status_bits_crc_verified&&
       !v.dxf_group70_and_viewport_semantics_verified&&
       !v.candidate_bits.display_accepted&&
       !v.effective_color_and_visible_paint_accepted;
    if(ok){
        MusaDwgAc1032CircleLayerStatusPreflight again={0};
        ok=musa_dwg_ac1032_circle_layer_status_preflight(
              scene,UINT64_C(0x1c8),UINT64_C(0x1bb),&again)&&
           !memcmp(&again,&v,sizeof(v));
    }
    if(ok){
        MusaDwgAc1032CircleLayerStatusPreflight untouched;
        memset(&untouched,0xa5,sizeof(untouched));
        const MusaDwgAc1032CircleLayerStatusPreflight before=untouched;
        const uint64_t bad[][2]={
            {0u,UINT64_C(0x1bb)},
            {UINT64_C(0x1c8),0u},
            {UINT64_C(0x1b8),UINT64_C(0x1bb)},
            {UINT64_C(0x1c8),UINT64_C(0x8a)},
            {UINT64_MAX,UINT64_C(0x1bb)}
        };
        for(size_t i=0u;ok&&i<sizeof(bad)/sizeof(bad[0]);i++)
            ok=!musa_dwg_ac1032_circle_layer_status_preflight(
                  scene,bad[i][0],bad[i][1],&untouched)&&
               !memcmp(&before,&untouched,sizeof(before));
        if(ok)ok=!musa_dwg_ac1032_circle_layer_status_preflight(
                  scene,UINT64_C(0x1c8),UINT64_C(0x1bb),NULL);
    }
    if(ok){
        MusaDwgAc1032CircleLayerDxfOracle external={
            UINT64_C(0x8a),(int32_t)oracle.group70,(int32_t)oracle.aci,
            1u,1u,1u
        };
        MusaDwgAc1032CircleLayerDxfStatusEvidence evidence={0};
        ok=musa_dwg_ac1032_circle_layer_dxf_status_oracle(
             scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
             &external,&evidence)&&
           evidence.layer_handle==UINT64_C(0x8a)&&
           evidence.dxf_layer_aci_matches_source&&
           evidence.original_dwg_crc_and_chain_verified&&
           evidence.external_dxf_frozen==((oracle.group70&1L)!=0L)&&
           evidence.external_dxf_frozen_in_new_viewports==
               ((oracle.group70&2L)!=0L)&&
           evidence.external_dxf_locked==((oracle.group70&4L)!=0L)&&
           !evidence.external_dxf_off&&
           !evidence.dwg_layer_visibility_semantics_verified&&
           !evidence.viewport_and_final_paint_accepted;
        if(ok){
            MusaDwgAc1032CircleLayerDxfStatusEvidence repeat={0};
            ok=musa_dwg_ac1032_circle_layer_dxf_status_oracle(
                 scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
                 &external,&repeat)&&
               !memcmp(&repeat,&evidence,sizeof(evidence));
        }
        if(ok){
            MusaDwgAc1032CircleLayerDxfStatusEvidence untouched;
            memset(&untouched,0xa5,sizeof(untouched));
            const MusaDwgAc1032CircleLayerDxfStatusEvidence before=untouched;
            const MusaDwgAc1032CircleLayerDxfOracle invalid[]={
                {UINT64_C(0x8b),0,4,1,1,1},
                {UINT64_C(0x8a),8,4,1,1,1},
                {UINT64_C(0x8a),0,5,1,1,1},
                {UINT64_C(0x8a),0,0,1,1,1},
                {UINT64_C(0x8a),0,4,0,1,1},
                {UINT64_C(0x8a),0,4,1,0,1},
                {UINT64_C(0x8a),0,4,1,1,0},
                {UINT64_C(0x8a),128,4,1,1,1},
                {UINT64_C(0x8a),0,-256,1,1,1}
            };
            for(size_t i=0u;ok&&i<sizeof(invalid)/sizeof(invalid[0]);i++)
                ok=!musa_dwg_ac1032_circle_layer_dxf_status_oracle(
                    scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
                    &invalid[i],&untouched)&&
                   !memcmp(&before,&untouched,sizeof(before));
            if(ok){
                MusaDwgAc1032CircleLayerDxfOracle synthetic=external;
                synthetic.dxf_signed_group62=-4;
                MusaDwgAc1032CircleLayerDxfStatusEvidence off={0};
                ok=musa_dwg_ac1032_circle_layer_dxf_status_oracle(
                     scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
                     &synthetic,&off)&&off.external_dxf_off&&
                   !off.viewport_and_final_paint_accepted;
            }
        }
    }
    /* Authentic immutable R2018 DWG/DXF oracle: classification is NOT
     * a runtime visibility permission, even if the companion layer is ON. */
    if(ok){
        MusaDwgAc1032CircleLayerDxfOracle external={
            UINT64_C(0x8a),(int32_t)oracle.group70,(int32_t)oracle.aci,
            1u,1u,1u
        };
        MusaDwgAc1032CircleLayerVisibilityVeto evidence={0};
        ok=musa_dwg_ac1032_circle_layer_visibility_veto(
             scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
             &external,&evidence)&&
           evidence.original_layer_handle==UINT64_C(0x8a)&&
           evidence.original_dwg_chain_crc_verified&&
           !evidence.final_visible_paint_accepted&&
           evidence.unresolved_original_dwg_requirements==
             (MUSA_CIRCLE_DWG_UNRESOLVED_LAYER_BITS|
              MUSA_CIRCLE_DWG_UNRESOLVED_OWNER_SPACE|
              MUSA_CIRCLE_DWG_UNRESOLVED_NESTED_INSERT|
              MUSA_CIRCLE_DWG_UNRESOLVED_VIEWPORT|
              MUSA_CIRCLE_DWG_UNRESOLVED_XREF)&&
           evidence.external_unconditional_veto==
             (uint32_t)(((oracle.aci<0L)?
               MUSA_CIRCLE_DXF_VETO_LAYER_OFF:0u)|
              ((oracle.group70&1L)?
               MUSA_CIRCLE_DXF_VETO_LAYER_FROZEN:0u))&&
           evidence.external_dxf_locked_for_editing==
             (uint8_t)((oracle.group70&4L)!=0L);
        if(ok){
            MusaDwgAc1032CircleLayerVisibilityVeto again={0};
            ok=musa_dwg_ac1032_circle_layer_visibility_veto(
                scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
                &external,&again)&&
               !memcmp(&evidence,&again,sizeof(evidence));
        }
        /* These are synthetic DXF statuses, NOT claims about the fixture.
         * Exercise OFF, frozen, new-viewport, locked and xref flags. */
        if(ok){
            const int32_t flags[]={0,1,2,4,16,32,64,119};
            for(size_t i=0u;ok&&i<sizeof(flags)/sizeof(flags[0]);i++){
                MusaDwgAc1032CircleLayerDxfOracle synthetic=external;
                synthetic.dxf_group70=flags[i];
                synthetic.dxf_signed_group62=(i==0u)?-4:4;
                MusaDwgAc1032CircleLayerVisibilityVeto v={0};
                const uint32_t expected_veto=
                    (uint32_t)(((i==0u)?
                      MUSA_CIRCLE_DXF_VETO_LAYER_OFF:0u)|
                     ((flags[i]&1)?
                      MUSA_CIRCLE_DXF_VETO_LAYER_FROZEN:0u));
                const uint32_t expected_risks=
                    (uint32_t)(((flags[i]&2)?
                      MUSA_CIRCLE_DXF_RISK_NEW_VIEWPORT_FROZEN:0u)|
                     ((flags[i]&16)?
                      MUSA_CIRCLE_DXF_RISK_XREF_DEPENDENT:0u)|
                     ((flags[i]&32)?
                      MUSA_CIRCLE_DXF_RISK_XREF_RESOLVED:0u)|
                     ((flags[i]&64)?
                      MUSA_CIRCLE_DXF_RISK_REFERENCED:0u));
                ok=musa_dwg_ac1032_circle_layer_visibility_veto(
                    scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
                    &synthetic,&v)&&
                   v.external_unconditional_veto==expected_veto&&
                   v.external_conditional_risks==expected_risks&&
                   v.external_dxf_locked_for_editing==
                       (uint8_t)((flags[i]&4)!=0)&&
                   !v.final_visible_paint_accepted;
            }
        }
        if(ok){
            MusaDwgAc1032CircleLayerVisibilityVeto untouched;
            memset(&untouched,0xa5,sizeof(untouched));
            const MusaDwgAc1032CircleLayerVisibilityVeto before=untouched;
            MusaDwgAc1032CircleLayerDxfOracle invalid=external;
            invalid.dxf_group70=8; /* reserved DXF LAYER status */
            ok=!musa_dwg_ac1032_circle_layer_visibility_veto(
                   scene,UINT64_C(0x1c8),UINT64_C(0x1bb),
                   &invalid,&untouched)&&
               !memcmp(&before,&untouched,sizeof(before));
            if(ok)ok=!musa_dwg_ac1032_circle_layer_visibility_veto(
                   scene,UINT64_C(0x1c8),UINT64_C(0x8a),
                   &external,&untouched)&&
               !memcmp(&before,&untouched,sizeof(before));
        }
    }
    fprintf(stderr,"%s original DWG vs independent DXF signed LAYER oracle, group70=%ld "
            "is separate; viewport/paint UNKNOWN\n",
            ok?"PASS":"FAIL",oracle.group70);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=3)return 2;
    return run(argv[1],argv[2])?0:1;
}
