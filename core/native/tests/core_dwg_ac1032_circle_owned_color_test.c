/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent R2018 DXF oracle for original H1BB OT18 CIRCLE and H8A
 * LAYER. Original DWG is decoded only by first-party C11. This fixture
 * has no entity 62/420/60 overrides: its own source is BYLAYER, but
 * effective layer color, model space and viewport paint remain unknown. */
#include "musacad_dwg_ac1032_color_provenance.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct Oracle {
    char layer_name[256],circle_layer[256];
    unsigned layer_count,circle_count,layer_aci_present;
    unsigned circle_aci_present,circle_rgb_present,circle_hidden_present;
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
static void finish_record(Oracle *o,const char *type,uint64_t h,
                          const char *name,const char *layer,
                          unsigned has_62,int aci,unsigned has_420,
                          unsigned has_60,double radius,unsigned has_radius){
    if(!strcmp(type,"LAYER")&&h==UINT64_C(0x8a)){
        o->layer_count++;
        if(strlen(name)<sizeof(o->layer_name))
            strcpy(o->layer_name,name);
        o->layer_aci_present=has_62;
        o->layer_aci=aci;
    }
    if(!strcmp(type,"CIRCLE")&&h==UINT64_C(0x1bb)){
        o->circle_count++;
        if(strlen(layer)<sizeof(o->circle_layer))
            strcpy(o->circle_layer,layer);
        o->circle_aci_present=has_62;
        o->circle_rgb_present=has_420;
        o->circle_hidden_present=has_60;
        o->radius=has_radius?radius:0.0;
    }
}
static int read_oracle(const char *path,Oracle *o){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    char code[128],value[1024],type[64]={0};
    char name[256]={0},layer[256]={0};
    uint64_t h=0u;
    unsigned has_62=0u,has_420=0u,has_60=0u,has_radius=0u;
    int aci=0,valid=1;
    double radius=0.0;
    while(fgets(code,sizeof(code),f)&&fgets(value,sizeof(value),f)){
        trim(code);trim(value);
        char *end=NULL;
        long group=strtol(code,&end,10);
        if(end==code||*end){valid=0;break;}
        if(group==0){
            finish_record(o,type,h,name,layer,has_62,aci,has_420,
                          has_60,radius,has_radius);
            if(strlen(value)>=sizeof(type)){valid=0;break;}
            strcpy(type,value);
            h=0u;name[0]='\0';layer[0]='\0';
            has_62=0u;has_420=0u;has_60=0u;has_radius=0u;
            aci=0;radius=0.0;
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
            if(tail!=value&&!*tail&&v>=-32768L&&v<=32767L){
                has_62=1u;aci=(int)v;
            }else valid=0;
        }else if(group==420&&!strcmp(type,"CIRCLE")){
            has_420=1u;
        }else if(group==60&&!strcmp(type,"CIRCLE")){
            has_60=1u;
        }else if(group==40&&!strcmp(type,"CIRCLE")){
            char *tail=NULL;double v=strtod(value,&tail);
            if(tail!=value&&!*tail&&isfinite(v)){
                has_radius=1u;radius=v;
            }else valid=0;
        }
    }
    if(valid)finish_record(o,type,h,name,layer,has_62,aci,has_420,
                          has_60,radius,has_radius);
    if(ferror(f)||fclose(f)!=0)valid=0;
    return valid&&o->layer_count==1u&&o->circle_count==1u&&
           o->layer_aci_present&&o->layer_aci==4&&
           !o->circle_aci_present&&!o->circle_rgb_present&&
           !o->circle_hidden_present&&
           fabs(o->radius-130.0490046078758)<1e-6&&
           !strcmp(o->layer_name,o->circle_layer);
}
static int real_source(const char *dwg,const char *dxf){
    Oracle oracle={0};
    if(!read_oracle(dxf,&oracle))return 0;
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
    MusaAc1032ColorProvenance color={0};
    MusaDwgAc1032CircleLayerProvenance source={0};
    const uint64_t h=UINT64_C(0x1bb);
    ok=musa_dwg_ac1032_scene_circle_owned_color_provenance(
           scene,h,&color)&&
       musa_dwg_ac1032_scene_index_circle_layer_provenance(
           scene,h,&source)&&
       color.entity_handle==h&&color.raw_entity_type==18u&&
       color.layer_handle==UINT64_C(0x8a)&&
       color.entity_and_layer_crc_verified&&
       color.entity_aci==256u&&color.entity_by_layer==1u&&
       !color.entity_by_block&&!color.entity_has_direct_aci&&
       !color.entity_has_direct_rgb24&&
       !color.entity_complex_color_or_transparency&&
       !color.source_entity_invisible&&!color.rendering_accepted&&
       source.edge.owner_block_header_handle==UINT64_C(0x1b8)&&
       source.edge.original_forward_block_child_membership_verified&&
       !strcmp(source.layer_name.utf8,oracle.layer_name)&&
       source.circle.common.color_index==color.entity_aci&&
       (!color.source_layer_aci_supported||
        color.source_layer_aci==(uint8_t)oracle.layer_aci);
    if(ok){
        MusaAc1032ColorProvenance again={0};
        ok=musa_dwg_ac1032_scene_circle_owned_color_provenance(
            scene,h,&again)&&!memcmp(&color,&again,sizeof(color));
    }
    if(ok){
        MusaAc1032ColorProvenance untouched;
        memset(&untouched,0xa5,sizeof(untouched));
        const MusaAc1032ColorProvenance before=untouched;
        const uint64_t wrong[]={0u,UINT64_MAX,UINT64_C(0x1b8),
                                 UINT64_C(0x8a),UINT64_C(0x1c8)};
        for(size_t i=0u;ok&&i<sizeof(wrong)/sizeof(wrong[0]);i++)
            ok=!musa_dwg_ac1032_scene_circle_owned_color_provenance(
                   scene,wrong[i],&untouched)&&
               !memcmp(&before,&untouched,sizeof(before));
        if(ok)ok=!musa_dwg_ac1032_scene_circle_owned_color_provenance(
                   scene,h,NULL);
    }
    fprintf(stderr,"%s original R2018 H1BB relative H(C) owner, H8A "
           "DXF layer ACI4; CIRCLE ENC BYLAYER, final paint=0\n",
           ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0; /* host C11 ABI smoke */
    if(argc!=3)return 2;
    return real_source(argv[1],argv[2])?0:1;
}
