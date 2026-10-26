/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original unchanged R2018 example_2018.dwg against independent
 * companion DXF: INSERT H1C8, CIRCLE H1BB, LAYER H8A ACI4.
 * Source provenance plus numeric WCS only, NEVER final visible paint. */
#include "musacad_dwg_ac1032_insert_circle_layer_cmc_chain.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct Oracle {
    unsigned inserts,circles,layers,circle_explicit_color;
    unsigned layer_has_aci;
    int aci;
    double radius;
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
static void finish(Oracle *o,const char *type,uint64_t h,
                   const char *name,const char *layer,
                   unsigned has_62,int aci,unsigned has_420,
                   unsigned has_radius,double radius){
    if(!strcmp(type,"INSERT")&&h==UINT64_C(0x1c8))o->inserts++;
    if(!strcmp(type,"CIRCLE")&&h==UINT64_C(0x1bb)){
        o->circles++;
        if(strlen(layer)<sizeof(o->circle_layer))
            strcpy(o->circle_layer,layer);
        o->circle_explicit_color+=has_62+has_420;
        if(has_radius)o->radius=radius;
    }
    if(!strcmp(type,"LAYER")&&h==UINT64_C(0x8a)){
        o->layers++;
        if(strlen(name)<sizeof(o->layer_name))
            strcpy(o->layer_name,name);
        if(has_62){o->layer_has_aci++;o->aci=aci;}
    }
}
static int independent_dxf(const char *path,Oracle *o){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    char code[128],value[1024],type[64]={0};
    char name[256]={0},layer[256]={0};
    uint64_t h=0u;
    unsigned has_62=0u,has_420=0u,has_radius=0u;
    int aci=0,ok=1;
    double radius=0.0;
    while(fgets(code,sizeof(code),f)&&fgets(value,sizeof(value),f)){
        trim(code);trim(value);
        char *end=NULL;
        const long group=strtol(code,&end,10);
        if(end==code||*end){ok=0;break;}
        if(group==0){
            finish(o,type,h,name,layer,has_62,aci,has_420,
                   has_radius,radius);
            if(strlen(value)>=sizeof(type)){ok=0;break;}
            strcpy(type,value);
            h=0u;name[0]='\0';layer[0]='\0';
            has_62=has_420=has_radius=0u;aci=0;radius=0.0;
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
            if(tail==value||*tail||v<-32768L||v>32767L){
                ok=0;break;
            }
            has_62=1u;aci=(int)v;
        }else if(group==420&&!strcmp(type,"CIRCLE")){
            has_420=1u;
        }else if(group==40&&!strcmp(type,"CIRCLE")){
            char *tail=NULL;double v=strtod(value,&tail);
            if(tail==value||*tail||!isfinite(v)){ok=0;break;}
            has_radius=1u;radius=v;
        }
    }
    if(ok)finish(o,type,h,name,layer,has_62,aci,has_420,
                 has_radius,radius);
    if(ferror(f))ok=0;
    if(fclose(f)!=0)ok=0;
    return ok&&o->inserts==1u&&o->circles==1u&&o->layers==1u&&
           o->layer_has_aci==1u&&o->aci==4&&
           !o->circle_explicit_color&&
           fabs(o->radius-130.0490046078758)<1e-6&&
           !strcmp(o->circle_layer,o->layer_name);
}
static int genuine(const char *dwg,const char *dxf){
    Oracle expected={0};
    if(!independent_dxf(dxf,&expected))return 0;
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
    MusaDwgAc1032InsertCircleLayerCmcChain chain={0};
    ok=musa_dwg_ac1032_insert_circle_layer_cmc_chain(
           scene,UINT64_C(0x1c8),UINT64_C(0x1bb),&chain)&&
       chain.insert_handle==UINT64_C(0x1c8)&&
       chain.circle_handle==UINT64_C(0x1bb)&&
       chain.block_header_handle==UINT64_C(0x1b8)&&
       chain.layer_handle==UINT64_C(0x8a)&&
       chain.original_layer_aci==(uint8_t)expected.aci&&
       fabs(chain.original_circle_radius-expected.radius)<1e-6&&
       chain.source_chain_and_wcs_verified&&
       !chain.final_color_visibility_and_paint_accepted;
    for(unsigned j=0u;ok&&j<3u;j++)
        ok=isfinite(chain.world_center[j])&&
           isfinite(chain.world_axis_u[j])&&
           isfinite(chain.world_axis_v[j]);
    if(ok){
        MusaDwgAc1032InsertCircleLayerCmcChain repeat={0};
        ok=musa_dwg_ac1032_insert_circle_layer_cmc_chain(
             scene,UINT64_C(0x1c8),UINT64_C(0x1bb),&repeat)&&
           !memcmp(&repeat,&chain,sizeof(chain));
    }
    if(ok){
        MusaDwgAc1032InsertCircleLayerCmcChain unchanged;
        memset(&unchanged,0xa5,sizeof(unchanged));
        const MusaDwgAc1032InsertCircleLayerCmcChain before=unchanged;
        const uint64_t wrong[][2]={
            {0u,UINT64_C(0x1bb)},
            {UINT64_C(0x1c8),0u},
            {UINT64_C(0x190),UINT64_C(0x1bb)},
            {UINT64_C(0x1c8),UINT64_C(0x8a)},
            {UINT64_C(0x1b8),UINT64_C(0x1bb)},
            {UINT64_MAX,UINT64_C(0x1bb)}
        };
        for(size_t i=0u;ok&&i<sizeof(wrong)/sizeof(wrong[0]);i++)
            ok=!musa_dwg_ac1032_insert_circle_layer_cmc_chain(
                   scene,wrong[i][0],wrong[i][1],&unchanged)&&
               !memcmp(&unchanged,&before,sizeof(before));
        if(ok)ok=!musa_dwg_ac1032_insert_circle_layer_cmc_chain(
                   scene,UINT64_C(0x1c8),UINT64_C(0x1bb),NULL);
    }
    fprintf(stderr,"%s original INSERT H1C8 -> CIRCLE H1BB -> "
            "LAYER H8A ACI4 with numeric WCS; paint blocked\n",
            ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=3)return 2;
    return genuine(argv[1],argv[2])?0:1;
}
