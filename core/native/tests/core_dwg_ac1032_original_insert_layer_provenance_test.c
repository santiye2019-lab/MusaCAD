/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Unmodified original R2018 example_2018.dwg and independent DXF layer
 * record oracle. Source-only INSERT -> OT51 layer Unicode/raw status/ENC.
 * Never claims block expansion, BYLAYER paint, or viewport visibility. */
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
/* Read only DXF group pairs for an independently named LAYER table
 * record. This does not use a DWG decoder or guess layer names. */
static int oracle_layer_name(const char *path,uint64_t wanted,
                             char *out,size_t cap){
    FILE *f=fopen(path,"rb");
    if(!f||!out||!cap){if(f)fclose(f);return 0;}
    char code[64],value[1024],name[512]={0};
    uint64_t handle=0u;
    int in_layer=0,has_handle=0,has_name=0,ok=0;
    while(fgets(code,sizeof(code),f)&&fgets(value,sizeof(value),f)){
        trim(code);trim(value);
        char *end=NULL;
        long group=strtol(code,&end,10);
        if(!end||*end)break;
        if(group==0){
            if(in_layer&&has_handle&&has_name&&handle==wanted){
                const size_t len=strlen(name);
                if(len&&len<cap){memcpy(out,name,len+1u);ok=1;}
                break;
            }
            in_layer=!strcmp(value,"LAYER");
            has_handle=0;has_name=0;handle=0u;name[0]='\0';
        }else if(in_layer&&group==5){
            char *last=NULL;
            unsigned long long h=strtoull(value,&last,16);
            if(last&&last!=value&&!*last){
                handle=(uint64_t)h;has_handle=1;
            }
        }else if(in_layer&&group==2){
            const size_t len=strlen(value);
            if(len&&len<sizeof(name)){
                memcpy(name,value,len+1u);has_name=1;
            }
        }
    }
    if(!ok&&in_layer&&has_handle&&has_name&&handle==wanted){
        const size_t len=strlen(name);
        if(len&&len<cap){memcpy(out,name,len+1u);ok=1;}
    }
    fclose(f);
    return ok;
}
static int original(const char *dwg,const char *dxf){
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
    free(bytes); /* All further reads MUST use the cached original H index. */
    if(!ok)return 0;

    char oracle_name[512]={0};
    ok=oracle_layer_name(dxf,UINT64_C(0x8a),
                         oracle_name,sizeof(oracle_name));
    const uint64_t handles[]={
        UINT64_C(0x1c8),UINT64_C(0x190),UINT64_C(0x756)
    };
    uint16_t first_flags=0u;
    for(size_t i=0u;ok&&i<sizeof(handles)/sizeof(handles[0]);i++){
        MusaDwgAc1032InsertLayerProvenance v={0};
        ok=musa_dwg_ac1032_scene_index_insert_layer_provenance(
            scene,handles[i],&v)&&
           v.graph.indexed_handle==handles[i]&&
           v.graph.layer_handle==UINT64_C(0x8a)&&
           v.graph.original_entity_and_targets_crc_verified&&
           v.graph.source_edges_verified&&
           !v.graph.block_expansion_and_display_accepted&&
           v.layer_name.indexed_handle==UINT64_C(0x8a)&&
           v.layer_name.original_record_crc_verified&&
           v.layer_name.utf8_bytes==strlen(oracle_name)&&
           !strcmp(v.layer_name.utf8,oracle_name)&&
           v.raw_layer_flags.indexed_handle==UINT64_C(0x8a)&&
           v.raw_layer_flags.source_crc_verified&&
           v.raw_layer_flags.numeric_flags_end_bit>
               v.raw_layer_flags.numeric_flags_start_bit&&
           v.insert_common.common_decoded&&
           v.insert_common.prefix.indexed_handle==handles[i]&&
           v.insert_common.prefix.raw_object_type==7u&&
           v.source_layer_and_entity_verified&&
           !v.final_visibility_color_and_paint_accepted;
        if(ok){
            if(i&&first_flags!=v.raw_layer_flags.raw_values_70_290_370)
                ok=0;
            first_flags=v.raw_layer_flags.raw_values_70_290_370;
        }
        if(!ok)fprintf(stderr,
            "FAIL INSERT layer provenance H%llx / DXF LAYER H8A\n",
            (unsigned long long)handles[i]);
    }
    if(ok){
        MusaDwgAc1032InsertLayerProvenance untouched;
        memset(&untouched,0xa5,sizeof(untouched));
        const MusaDwgAc1032InsertLayerProvenance before=untouched;
        const uint64_t wrong[]={0u,UINT64_MAX,
                                  UINT64_C(0x192),UINT64_C(0x8a),
                                  UINT64_C(0x1b8)};
        for(size_t i=0u;ok&&i<sizeof(wrong)/sizeof(wrong[0]);i++)
            ok=!musa_dwg_ac1032_scene_index_insert_layer_provenance(
                    scene,wrong[i],&untouched)&&
               !memcmp(&untouched,&before,sizeof(before));
        if(ok)ok=!musa_dwg_ac1032_scene_index_insert_layer_provenance(
                       scene,handles[0],NULL);
    }
    fprintf(stderr,"%s genuine R2018 INSERT source layer H8A "
        "Unicode/DXF name, raw status and ENC; final paint UNVERIFIED\n",
        ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc!=3)return 2;
    return original(argv[1],argv[2])?0:1;
}
