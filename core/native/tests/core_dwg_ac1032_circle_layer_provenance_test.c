/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent R2018 original DWG vs companion DXF CIRCLE H1BB,
 * LAYER H8A Unicode, source-only numeric flags and raw ENC.
 * Never authorizes BYLAYER/BYBLOCK, frozen/off, viewport or final paint. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void trim(char *s){
    size_t n=strlen(s);
    while(n&&(s[n-1]=='\n'||s[n-1]=='\r'||
              s[n-1]==' '||s[n-1]=='\t'))s[--n]='\0';
    size_t i=0u;
    while(s[i]==' '||s[i]=='\t')i++;
    if(i)memmove(s,s+i,strlen(s+i)+1u);
}
typedef struct Oracle {
    char layer_name[512],circle_layer[512];
    unsigned layer_found,circle_found;
} Oracle;
static void accept_record(Oracle *o,const char *type,uint64_t h,
                          const char *name,const char *layer,
                          double radius,int has_radius){
    if(!strcmp(type,"LAYER")&&h==UINT64_C(0x8a)&&name[0]){
        size_t n=strlen(name);
        if(n<sizeof(o->layer_name)){
            memcpy(o->layer_name,name,n+1u);
            o->layer_found++;
        }
    }
    if(!strcmp(type,"CIRCLE")&&h==UINT64_C(0x1bb)&&
       has_radius&&fabs(radius-130.0490046078758)<1e-6&&layer[0]){
        size_t n=strlen(layer);
        if(n<sizeof(o->circle_layer)){
            memcpy(o->circle_layer,layer,n+1u);
            o->circle_found++;
        }
    }
}
static int oracle_dxf(const char *path,Oracle *o){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    char code[128],value[1024],type[64]={0};
    char name[512]={0},layer[512]={0};
    uint64_t h=0u;
    double radius=0.0;
    int has_radius=0,valid=1;
    while(fgets(code,sizeof(code),f)&&fgets(value,sizeof(value),f)){
        trim(code);trim(value);
        char *end=NULL;
        long group=strtol(code,&end,10);
        if(end==code||*end){valid=0;break;}
        if(group==0){
            accept_record(o,type,h,name,layer,radius,has_radius);
            size_t n=strlen(value);
            if(n>=sizeof(type)){valid=0;break;}
            memcpy(type,value,n+1u);
            h=0u;name[0]='\0';layer[0]='\0';
            radius=0.0;has_radius=0;
        }else if(group==5){
            char *last=NULL;
            unsigned long long candidate=strtoull(value,&last,16);
            if(last!=value&&!*last)h=(uint64_t)candidate;
        }else if(group==2&&!strcmp(type,"LAYER")){
            size_t n=strlen(value);
            if(n<sizeof(name))memcpy(name,value,n+1u);
        }else if(group==8&&!strcmp(type,"CIRCLE")){
            size_t n=strlen(value);
            if(n<sizeof(layer))memcpy(layer,value,n+1u);
        }else if(group==40&&!strcmp(type,"CIRCLE")){
            char *last=NULL;
            double v=strtod(value,&last);
            if(last!=value&&!*last&&isfinite(v)){
                radius=v;has_radius=1;
            }
        }
    }
    if(valid)accept_record(o,type,h,name,layer,radius,has_radius);
    if(ferror(f))valid=0;
    if(fclose(f)!=0)valid=0;
    return valid&&o->layer_found==1u&&o->circle_found==1u&&
           !strcmp(o->layer_name,o->circle_layer);
}
static int genuine(const char *dwg,const char *dxf){
    Oracle oracle={0};
    if(!oracle_dxf(dxf,&oracle))return 0;
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
    MusaDwgAc1032CircleLayerProvenance v={0};
    ok=musa_dwg_ac1032_scene_index_circle_layer_provenance(
           scene,UINT64_C(0x1bb),&v)&&
       v.edge.circle_handle==UINT64_C(0x1bb)&&
       v.edge.owner_block_header_handle==UINT64_C(0x1b8)&&
       v.edge.layer_handle==UINT64_C(0x8a)&&
       v.edge.exact_relative_owner_and_layer_source_verified&&
       v.edge.original_forward_block_child_membership_verified&&
       !v.edge.model_space_viewport_and_paint_accepted&&
       v.layer_name.indexed_handle==UINT64_C(0x8a)&&
       v.layer_name.original_record_crc_verified&&
       !strcmp(v.layer_name.utf8,oracle.layer_name)&&
       v.raw_layer_flags.indexed_handle==UINT64_C(0x8a)&&
       v.raw_layer_flags.source_crc_verified&&
       v.circle.indexed_handle==UINT64_C(0x1bb)&&
       v.circle.object_type==18u&&v.circle.geometry_decoded&&
       fabs(v.circle.radius-130.0490046078758)<1e-6&&
       v.circle.common.common_decoded&&
       v.source_geometry_and_layer_verified&&
       !v.final_visibility_color_and_paint_accepted;
    if(ok){
        MusaDwgAc1032CircleLayerProvenance repeat={0};
        ok=musa_dwg_ac1032_scene_index_circle_layer_provenance(
             scene,UINT64_C(0x1bb),&repeat)&&
           !memcmp(&repeat,&v,sizeof(v));
    }
    if(ok){
        MusaDwgAc1032CircleLayerProvenance untouched;
        memset(&untouched,0xa5,sizeof(untouched));
        const MusaDwgAc1032CircleLayerProvenance before=untouched;
        const uint64_t wrong[]={0u,UINT64_MAX,UINT64_C(0x1c8),
                                 UINT64_C(0x1b8),UINT64_C(0x8a)};
        for(size_t i=0u;ok&&i<sizeof(wrong)/sizeof(wrong[0]);i++)
            ok=!musa_dwg_ac1032_scene_index_circle_layer_provenance(
                   scene,wrong[i],&untouched)&&
               !memcmp(&untouched,&before,sizeof(before));
        if(ok)ok=!musa_dwg_ac1032_scene_index_circle_layer_provenance(
                      scene,UINT64_C(0x1bb),NULL);
    }
    fprintf(stderr,"%s original R2018 H1BB CIRCLE H1B8/H8A "
            "DXF Unicode layer, raw flags and ENC; final paint=0\n",
            ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0; /* portable C11 ABI smoke */
    if(argc!=3)return 2;
    return genuine(argv[1],argv[2])?0:1;
}
