/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original AC1032 fixtures: independent entity -> typed LAYER H=0x10 ->
 * Unicode TU name '0'. Not a guessed layer name or RGB/ACI resolver. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int load(const char *path,uint8_t **out,size_t *len){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    const long n=ftell(f);
    if(n<256||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
       fclose(f);return 0;
    }
    uint8_t *raw=(uint8_t*)malloc((size_t)n);
    if(!raw){fclose(f);return 0;}
    int ok=fread(raw,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(raw);return 0;}
    *out=raw;*len=(size_t)n;return 1;
}
static int check(const char *name,uint64_t h,uint16_t type){
    uint8_t *bytes=NULL,*unchanged=NULL;size_t len=0u;
    if(!load(name,&bytes,&len))return 0;
    unchanged=(uint8_t*)malloc(len);
    if(!unchanged){free(bytes);return 0;}
    memcpy(unchanged,bytes,len);
    MusaDwgAc1032SceneIndex *s=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(bytes,len,&s)&&
           !memcmp(bytes,unchanged,len);
    /* Cached index owns extracted data after opening; original source
     * may be released before any source graph/name query is executed. */
    free(bytes);free(unchanged);bytes=NULL;unchanged=NULL;
    MusaDwgAc1032EntityLayerName answer={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_entity_layer_name(
         s,h,&answer)&&
         answer.source_graph_and_name_verified&&
         answer.edge.entity_handle==h&&
         answer.edge.layer_handle==UINT64_C(0x10)&&
         answer.edge.raw_entity_type==type&&
         answer.edge.raw_layer_type==51u&&
         answer.edge.source_handle_ref_verified&&
         answer.edge.original_entity_crc_verified&&
         answer.edge.original_layer_crc_verified&&
         answer.layer.original_record_crc_verified&&
         answer.layer.indexed_handle==answer.edge.layer_handle&&
         answer.layer.utf16_code_units==1u&&
         answer.layer.utf8_bytes==1u&&
         strcmp(answer.layer.utf8,"0")==0;
    if(ok){
        const MusaDwgAc1032EntityLayerName saved=answer;
        ok=!musa_dwg_ac1032_scene_index_entity_layer_name(
                s,UINT64_C(0x10),&answer)&&
           !memcmp(&answer,&saved,sizeof(answer))&&
           !musa_dwg_ac1032_scene_index_entity_layer_name(
                s,UINT64_MAX,&answer)&&
           !memcmp(&answer,&saved,sizeof(answer))&&
           !musa_dwg_ac1032_scene_index_entity_layer_name(
                s,h,NULL);
    }
    fprintf(stderr,"%s original R2018 OT%u H=0x%llX -> LAYER H=0x10 "
        "TU='0' with source H/CRC checks\n",
        ok?"PASS":"FAIL",(unsigned)type,
        (unsigned long long)h);
    musa_dwg_ac1032_scene_index_close(s);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1){
        MusaDwgAc1032EntityLayerName untouched={0};
        untouched.layer.utf8[0]='A';
        return !musa_dwg_ac1032_scene_index_entity_layer_name(
            NULL,UINT64_C(0x1ca),&untouched)&&
            untouched.layer.utf8[0]=='A'?0:1;
    }
    if(argc!=6)return 2;
    int ok=1;
    ok=check(argv[1],UINT64_C(0x1ca),19u)&&ok;
    ok=check(argv[2],UINT64_C(0x1c0),27u)&&ok;
    ok=check(argv[3],UINT64_C(0x1bd),17u)&&ok;
    ok=check(argv[4],UINT64_C(0x1bd),18u)&&ok;
    ok=check(argv[5],UINT64_C(0x1bd),77u)&&ok;
    return ok?0:1;
}
