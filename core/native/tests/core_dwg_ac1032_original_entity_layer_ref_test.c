/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original independent AutoCAD R2018 source -> strict H-code5
 * entity-to-LAYER(OT=51) source-proven edge. This does NOT decode
 * the LAYER's color or assert BYLAYER color completeness. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int load(const char *name,uint8_t **bytes,size_t *count){
    FILE *f=fopen(name,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256||n>64L*1024*1024||fseek(f,0,SEEK_SET)){
       fclose(f);return 0;
    }
    uint8_t *data=(uint8_t*)malloc((size_t)n);
    if(!data){fclose(f);return 0;}
    int ok=fread(data,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(data);return 0;}
    *bytes=data;*count=(size_t)n;return 1;
}
static int check(const char *path,uint64_t entity,uint16_t type){
    uint8_t *bytes=NULL,*saved=NULL;size_t n=0u;
    if(!load(path,&bytes,&n))return 0;
    saved=(uint8_t*)malloc(n);
    if(!saved){free(bytes);return 0;}
    memcpy(saved,bytes,n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(bytes,n,&scene)&&
           !memcmp(bytes,saved,n);
    free(bytes);free(saved);
    MusaDwgAc1032EntityLayerRef ref={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_entity_layer_ref(
        scene,entity,&ref);
    fprintf(stderr,"R2018 original entity layer source"
       " H=%llX type=%u lookup=%d layerH=%llX layerOT=%u\n",
       (unsigned long long)entity,(unsigned)type,ok,
       (unsigned long long)ref.layer_handle,
       (unsigned)ref.raw_layer_type);
    if(ok)ok=ref.entity_handle==entity&&
       ref.raw_entity_type==type&&
       ref.layer_handle==UINT64_C(0x10)&&
       ref.raw_layer_type==51u&&
       ref.original_entity_crc_verified&&
       ref.original_layer_crc_verified&&
       ref.source_handle_ref_verified;
    if(ok){
        const MusaDwgAc1032EntityLayerRef keep=ref;
        ok=!musa_dwg_ac1032_scene_index_entity_layer_ref(
            scene,UINT64_MAX,&ref)&&
            !memcmp(&keep,&ref,sizeof(ref))&&
            !musa_dwg_ac1032_scene_index_entity_layer_ref(
                scene,ref.layer_handle,&ref)&&
            !memcmp(&keep,&ref,sizeof(ref))&&
            !musa_dwg_ac1032_scene_index_entity_layer_ref(
                scene,entity,NULL);
    }
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc!=3)return argc==1?0:2;
    const int line=check(argv[1],UINT64_C(0x1ca),19u);
    const int point=check(argv[2],UINT64_C(0x1c0),27u);
    fprintf(stderr,"%s authentic R2018 LINE+POINT source H5->OT51 LAYER"
       " (not resolved color)\n",line&&point?"PASS":"FAIL");
    return line&&point?0:1;
}
