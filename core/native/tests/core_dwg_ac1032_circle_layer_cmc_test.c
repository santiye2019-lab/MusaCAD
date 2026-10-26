/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent pinned original R2018 DWG and DXF H1BB CIRCLE H8A
 * Unicode LAYER ACI4 oracle. This does NOT authorize visible BYLAYER. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct Oracle {
    char layer[256],circle_layer[256];
    unsigned layer_count,circle_count,layer_color_count;
    unsigned circle_color_count;
    int layer_aci;
    double radius;
} Oracle;
static void trim(char *s){
    size_t n=strlen(s);
    while(n&&(s[n-1]=='\n'||s[n-1]=='\r'||s[n-1]==' '||
              s[n-1]=='\t'))s[--n]='\0';
    size_t i=0u;
    while(s[i]==' '||s[i]=='\t')i++;
    if(i)memmove(s,s+i,strlen(s+i)+1u);
}
static void accept(Oracle *o,const char *type,uint64_t h,
                   const char *name,const char *layer,
                   unsigned has_62,int aci,unsigned has_420,
                   double radius,unsigned has_radius){
    if(!strcmp(type,"LAYER")&&h==UINT64_C(0x8a)){
        o->layer_count++;
        if(strlen(name)<sizeof(o->layer))strcpy(o->layer,name);
        if(has_62){o->layer_color_count++;o->layer_aci=aci;}
    }
    if(!strcmp(type,"CIRCLE")&&h==UINT64_C(0x1bb)){
        o->circle_count++;
        if(strlen(layer)<sizeof(o->circle_layer))
            strcpy(o->circle_layer,layer);
        o->circle_color_count+=has_62+has_420;
        o->radius=has_radius?radius:0.0;
    }
}
static int oracle(const char *path,Oracle *o){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    char code[128],value[1024],type[64]={0};
    char name[256]={0},layer[256]={0};
    uint64_t h=0u;
    unsigned has_62=0u,has_420=0u,has_radius=0u;
    int aci=0,valid=1;
    double radius=0.0;
    while(fgets(code,sizeof(code),f)&&fgets(value,sizeof(value),f)){
        trim(code);trim(value);
        char *end=NULL;long group=strtol(code,&end,10);
        if(end==code||*end){valid=0;break;}
        if(group==0){
            accept(o,type,h,name,layer,has_62,aci,has_420,
                   radius,has_radius);
            if(strlen(value)>=sizeof(type)){valid=0;break;}
            strcpy(type,value);
            h=0u;name[0]='\0';layer[0]='\0';
            has_62=0u;has_420=0u;has_radius=0u;
            aci=0;radius=0.0;
        }else if(group==5){
            char *tail=NULL;unsigned long long v=strtoull(value,&tail,16);
            if(tail!=value&&!*tail)h=(uint64_t)v;
        }else if(group==2&&!strcmp(type,"LAYER")){
            if(strlen(value)<sizeof(name))strcpy(name,value);
        }else if(group==8&&!strcmp(type,"CIRCLE")){
            if(strlen(value)<sizeof(layer))strcpy(layer,value);
        }else if(group==62){
            char *tail=NULL;long v=strtol(value,&tail,10);
            if(tail==value||*tail||v<-32768L||v>32767L){
                valid=0;break;
            }
            has_62=1u;aci=(int)v;
        }else if(group==420&&!strcmp(type,"CIRCLE")){
            has_420=1u;
        }else if(group==40&&!strcmp(type,"CIRCLE")){
            char *tail=NULL;double v=strtod(value,&tail);
            if(tail==value||*tail||!isfinite(v)){valid=0;break;}
            has_radius=1u;radius=v;
        }
    }
    if(valid)accept(o,type,h,name,layer,has_62,aci,has_420,
                    radius,has_radius);
    if(ferror(f))valid=0;
    if(fclose(f)!=0)valid=0;
    return valid&&o->layer_count==1u&&o->circle_count==1u&&
           o->layer_color_count==1u&&o->layer_aci==4&&
           !o->circle_color_count&&
           fabs(o->radius-130.0490046078758)<1e-6&&
           !strcmp(o->layer,o->circle_layer);
}
static int run(const char *dwg,const char *dxf){
    Oracle expected={0};
    if(!oracle(dxf,&expected))return 0;
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
    MusaDwgAc1032CircleLayerCmcSource source={0};
    ok=musa_dwg_ac1032_scene_index_circle_layer_cmc_source(
           scene,UINT64_C(0x1bb),&source)&&
       source.source.edge.circle_handle==UINT64_C(0x1bb)&&
       source.source.edge.owner_block_header_handle==UINT64_C(0x1b8)&&
       source.source.edge.layer_handle==UINT64_C(0x8a)&&
       source.source.source_geometry_and_layer_verified&&
       !strcmp(source.source.layer_name.utf8,expected.layer)&&
       source.layer_cmc.indexed_handle==UINT64_C(0x8a)&&
       source.layer_cmc.original_record_crc_verified&&
       source.layer_cmc.aci_index==(uint8_t)expected.layer_aci&&
       source.layer_cmc.packed_color_method==UINT32_C(0xC3000004)&&
       source.original_layer_cmc_aci_verified&&
       !source.final_visible_paint_accepted;
    if(ok){
        MusaDwgAc1032CircleLayerCmcSource repeat={0};
        ok=musa_dwg_ac1032_scene_index_circle_layer_cmc_source(
            scene,UINT64_C(0x1bb),&repeat)&&
           !memcmp(&repeat,&source,sizeof(source));
    }
    if(ok){
        MusaDwgAc1032CircleLayerCmcSource untouched;
        memset(&untouched,0xa5,sizeof(untouched));
        const MusaDwgAc1032CircleLayerCmcSource before=untouched;
        const uint64_t wrong[]={0u,UINT64_MAX,UINT64_C(0x1b8),
                                 UINT64_C(0x8a),UINT64_C(0x1c8)};
        for(size_t i=0u;ok&&i<sizeof(wrong)/sizeof(wrong[0]);i++)
            ok=!musa_dwg_ac1032_scene_index_circle_layer_cmc_source(
                  scene,wrong[i],&untouched)&&
               !memcmp(&untouched,&before,sizeof(before));
        if(ok)ok=!musa_dwg_ac1032_scene_index_circle_layer_cmc_source(
                 scene,UINT64_C(0x1bb),NULL);
    }
    fprintf(stderr,"%s original R2018 H1BB/H8A CMC source ACI4, "
            "visible paint NOT certified\n",ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=3)return 2;
    return run(argv[1],argv[2])?0:1;
}
