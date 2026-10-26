/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent unchanged R2018 DWG -> INSERT H graph -> LAYER CMC ACI,
 * compared with the companion DXF LAYER group 5/62 numeric oracle.
 * This is source-only: not block expansion, BYLAYER display or viewport. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void trim(char *s){
    size_t n=strlen(s);
    while(n&&(s[n-1]=='\n'||s[n-1]=='\r'||
              s[n-1]==' '||s[n-1]=='\t'))s[--n]='\0';
    size_t i=0u;
    while(s[i]&&isspace((unsigned char)s[i]))i++;
    if(i)memmove(s,s+i,strlen(s+i)+1u);
}
static int oracle_aci(const char *file,uint64_t wanted,int *out){
    FILE *f=fopen(file,"rb");
    if(!f||!out){if(f)fclose(f);return 0;}
    char group[64],value[1024];
    int in_layer=0,has_handle=0,has_aci=0,aci=0,ok=0;
    uint64_t handle=0u;
    while(fgets(group,sizeof(group),f)&&fgets(value,sizeof(value),f)){
        trim(group);trim(value);
        char *end=NULL;
        const long code=strtol(group,&end,10);
        if(!end||*end)break;
        if(code==0){
            if(in_layer&&has_handle&&has_aci&&handle==wanted){
                *out=aci;ok=1;break;
            }
            in_layer=!strcmp(value,"LAYER");
            has_handle=has_aci=0;handle=0u;aci=0;
        }else if(in_layer&&code==5){
            char *last=NULL;
            const unsigned long long v=strtoull(value,&last,16);
            if(last&&last!=value&&!*last){
                handle=(uint64_t)v;has_handle=1;
            }
        }else if(in_layer&&code==62){
            char *last=NULL;
            const long v=strtol(value,&last,10);
            if(last&&last!=value&&!*last&&v>=-255&&v<=255&&v){
                aci=(int)(v<0?-v:v);has_aci=1;
            }
        }
    }
    if(!ok&&in_layer&&has_handle&&has_aci&&handle==wanted){
        *out=aci;ok=1;
    }
    fclose(f);return ok;
}
static int genuine(const char *dwg,const char *dxf){
    int expected_aci=0;
    if(!oracle_aci(dxf,UINT64_C(0x8a),&expected_aci))return 0;
    FILE *f=fopen(dwg,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    const long len=ftell(f);
    if(len<256L||len>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *bytes=(uint8_t*)malloc((size_t)len);
    if(!bytes){fclose(f);return 0;}
    int ok=fread(bytes,1u,(size_t)len,f)==(size_t)len;
    if(fclose(f))ok=0;
    ok=ok&&!memcmp(bytes,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
        bytes,(size_t)len,&scene);
    free(bytes);
    if(!ok)return 0;
    const uint64_t inserts[]={
        UINT64_C(0x1c8),UINT64_C(0x190),UINT64_C(0x756)
    };
    for(size_t i=0u;ok&&i<sizeof(inserts)/sizeof(inserts[0]);i++){
        MusaDwgAc1032InsertLayerCmcSource v={0};
        ok=musa_dwg_ac1032_scene_index_insert_layer_cmc_source(
               scene,inserts[i],&v)&&
           v.source.graph.indexed_handle==inserts[i]&&
           v.source.graph.layer_handle==UINT64_C(0x8a)&&
           v.source.source_layer_and_entity_verified&&
           !v.source.final_visibility_color_and_paint_accepted&&
           v.cmc.indexed_handle==UINT64_C(0x8a)&&
           v.cmc.original_record_crc_verified&&
           v.cmc.packed_color_method==0xc3u&&
           v.cmc.aci_index==(uint8_t)expected_aci&&
           v.source_aci_verified&&!v.resolved_color_and_paint_accepted;
        if(!ok)fprintf(stderr,
            "FAIL R2018 INSERT H%llx -> LAYER H8A CMC vs DXF ACI=%d\n",
            (unsigned long long)inserts[i],expected_aci);
    }
    if(ok){
        MusaDwgAc1032InsertLayerCmcSource untouched;
        memset(&untouched,0x5a,sizeof(untouched));
        const MusaDwgAc1032InsertLayerCmcSource saved=untouched;
        const uint64_t bad[]={0u,UINT64_MAX,UINT64_C(0x192),
                              UINT64_C(0x8a),UINT64_C(0x1b8)};
        for(size_t i=0u;ok&&i<sizeof(bad)/sizeof(bad[0]);i++)
            ok=!musa_dwg_ac1032_scene_index_insert_layer_cmc_source(
                   scene,bad[i],&untouched)&&
               !memcmp(&untouched,&saved,sizeof(saved));
    }
    fprintf(stderr,"%s original R2018 INSERT source layer C3 ACI=%d; "
        "BYLAYER, owner, viewport and paint UNVERIFIED\n",
        ok?"PASS":"FAIL",expected_aci);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc!=3)return 2;
    return genuine(argv[1],argv[2])?0:1;
}
