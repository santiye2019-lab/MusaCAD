/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine unchanged R2018 H1C8/H1BB/H8A DWG versus independently
 * parsed DXF: CIRCLE BYLAYER with LAYER ACI4. No final paint. */
#include "musacad_dwg_ac1032_insert_circle_bylayer_crosscheck.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct Oracle {
    unsigned insert_count,circle_count,layer_count;
    unsigned circle_62,circle_420,circle_60,layer_62;
    int layer_aci;
    double circle_radius;
    char circle_layer[256],layer_name[256];
} Oracle;
static void trim(char *s){
    size_t n=strlen(s);
    while(n&&(s[n-1]=='\n'||s[n-1]=='\r'||s[n-1]==' '||
              s[n-1]=='\t'))s[--n]='\0';
    size_t i=0u;
    while(s[i]==' '||s[i]=='\t')i++;
    if(i)memmove(s,s+i,strlen(s+i)+1u);
}
static void record(Oracle *o,const char *type,uint64_t h,
                   const char *name,const char *layer,
                   unsigned has62,int aci,unsigned has420,
                   unsigned has60,double radius){
    if(!strcmp(type,"INSERT")&&h==UINT64_C(0x1c8))o->insert_count++;
    if(!strcmp(type,"CIRCLE")&&h==UINT64_C(0x1bb)){
        o->circle_count++;
        o->circle_62+=has62;o->circle_420+=has420;o->circle_60+=has60;
        o->circle_radius=radius;
        if(strlen(layer)<sizeof(o->circle_layer))
            strcpy(o->circle_layer,layer);
    }
    if(!strcmp(type,"LAYER")&&h==UINT64_C(0x8a)){
        o->layer_count++;
        o->layer_62+=has62;o->layer_aci=aci;
        if(strlen(name)<sizeof(o->layer_name))
            strcpy(o->layer_name,name);
    }
}
static int dxf_oracle(const char *path,Oracle *o){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    char code[128],value[1024],type[64]={0};
    char name[256]={0},layer[256]={0};
    uint64_t h=0u;
    unsigned has62=0u,has420=0u,has60=0u;
    int aci=0,ok=1;
    double radius=0.0;
    while(fgets(code,sizeof(code),f)&&fgets(value,sizeof(value),f)){
        trim(code);trim(value);
        char *end=NULL;
        long group=strtol(code,&end,10);
        if(end==code||*end){ok=0;break;}
        if(group==0){
            record(o,type,h,name,layer,has62,aci,has420,has60,radius);
            if(strlen(value)>=sizeof(type)){ok=0;break;}
            strcpy(type,value);
            h=0u;name[0]='\0';layer[0]='\0';
            has62=has420=has60=0u;aci=0;radius=0.0;
        }else if(group==5){
            char *tail=NULL;
            unsigned long long v=strtoull(value,&tail,16);
            if(tail!=value&&!*tail)h=(uint64_t)v;
        }else if(group==2&&!strcmp(type,"LAYER")){
            if(strlen(value)<sizeof(name))strcpy(name,value);
        }else if(group==8&&!strcmp(type,"CIRCLE")){
            if(strlen(value)<sizeof(layer))strcpy(layer,value);
        }else if(group==62){
            char *tail=NULL;long v=strtol(value,&tail,10);
            if(tail==value||*tail||v<-32768L||v>32767L){ok=0;break;}
            has62=1u;aci=(int)v;
        }else if(group==420&&!strcmp(type,"CIRCLE")){
            has420=1u;
        }else if(group==60&&!strcmp(type,"CIRCLE")){
            has60=1u;
        }else if(group==40&&!strcmp(type,"CIRCLE")){
            char *tail=NULL;double v=strtod(value,&tail);
            if(tail==value||*tail||!isfinite(v)){ok=0;break;}
            radius=v;
        }
    }
    if(ok)record(o,type,h,name,layer,has62,aci,has420,has60,radius);
    if(ferror(f))ok=0;
    if(fclose(f)!=0)ok=0;
    return ok&&o->insert_count==1u&&o->circle_count==1u&&
           o->layer_count==1u&&!o->circle_62&&!o->circle_420&&
           !o->circle_60&&o->layer_62==1u&&o->layer_aci==4&&
           fabs(o->circle_radius-130.0490046078758)<1e-6&&
           !strcmp(o->circle_layer,o->layer_name);
}
static int genuine(const char *dwg,const char *dxf){
    Oracle oracle={0};
    if(!dxf_oracle(dxf,&oracle))return 0;
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
    MusaDwgAc1032InsertCircleByLayerCrosscheck proof={0};
    ok=musa_dwg_ac1032_insert_circle_bylayer_crosscheck(
         scene,UINT64_C(0x1c8),UINT64_C(0x1bb),&proof)&&
       proof.chain.insert_handle==UINT64_C(0x1c8)&&
       proof.chain.circle_handle==UINT64_C(0x1bb)&&
       proof.chain.block_header_handle==UINT64_C(0x1b8)&&
       proof.chain.layer_handle==UINT64_C(0x8a)&&
       proof.chain.original_layer_aci==(uint8_t)oracle.layer_aci&&
       proof.circle_color.entity_by_layer&&
       proof.circle_color.entity_aci==256u&&
       !proof.circle_color.source_entity_invisible&&
       proof.original_layer_flags.source_crc_verified&&
       proof.original_layer_flags.indexed_handle==UINT64_C(0x8a)&&
       proof.source_entity_bylayer_and_layer_cmc_agree&&
       !proof.layer_state_and_viewport_verified&&
       !proof.final_visible_paint_accepted;
    if(ok){
        MusaDwgAc1032InsertCircleByLayerCrosscheck repeat={0};
        ok=musa_dwg_ac1032_insert_circle_bylayer_crosscheck(
             scene,UINT64_C(0x1c8),UINT64_C(0x1bb),&repeat)&&
           !memcmp(&repeat,&proof,sizeof(proof));
    }
    if(ok){
        MusaDwgAc1032InsertCircleByLayerCrosscheck unchanged;
        memset(&unchanged,0xa5,sizeof(unchanged));
        const MusaDwgAc1032InsertCircleByLayerCrosscheck before=unchanged;
        const uint64_t bad[][2]={
            {0u,UINT64_C(0x1bb)},
            {UINT64_C(0x1c8),0u},
            {UINT64_C(0x1b8),UINT64_C(0x1bb)},
            {UINT64_C(0x1c8),UINT64_C(0x8a)},
            {UINT64_MAX,UINT64_C(0x1bb)}
        };
        for(size_t i=0u;ok&&i<sizeof(bad)/sizeof(bad[0]);i++)
            ok=!musa_dwg_ac1032_insert_circle_bylayer_crosscheck(
                   scene,bad[i][0],bad[i][1],&unchanged)&&
               !memcmp(&unchanged,&before,sizeof(before));
        if(ok)ok=!musa_dwg_ac1032_insert_circle_bylayer_crosscheck(
                  scene,UINT64_C(0x1c8),UINT64_C(0x1bb),NULL);
    }
    fprintf(stderr,"%s original INSERT-CIRCLE ENC BYLAYER and LAYER ACI4 "
            "source crosscheck; viewport/layer state/paint UNKNOWN\n",
            ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=3)return 2;
    return genuine(argv[1],argv[2])?0:1;
}
