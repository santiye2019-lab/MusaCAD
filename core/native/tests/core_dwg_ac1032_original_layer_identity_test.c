/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exact original AutoCAD R2018 LINE.dwg H=0x10 LAYER identity:
 * independently authenticated source bit positions before layer name,
 * flags/color are intentionally NOT parsed as established semantics. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static int load(const char *path,uint8_t **out,size_t *count){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long k=ftell(f);
    if(k<256||k>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *x=(uint8_t*)malloc((size_t)k);
    if(!x){fclose(f);return 0;}
    int ok=fread(x,1u,(size_t)k,f)==(size_t)k;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(x);return 0;}
    *out=x;*count=(size_t)k;return 1;
}
int main(int argc,char **argv){
    if(argc!=2)return argc==1?0:2;
    uint8_t *raw=NULL,*backup=NULL;
    size_t n=0u;
    if(!load(argv[1],&raw,&n))return 2;
    backup=(uint8_t*)malloc(n);
    if(!backup){free(raw);return 2;}
    memcpy(backup,raw,n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(raw,n,&scene)&&
        !memcmp(raw,backup,n);
    MusaDwgAc1032LayerIdentity layer={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_layer_identity(
          scene,UINT64_C(0x10),&layer)&&
          layer.indexed_handle==UINT64_C(0x10)&&
          layer.object_type==51u&&
          layer.data_fields_start_bit==31u&&
          layer.original_record_crc_verified&&
          layer.embedded_self_handle_verified&&
          layer.empty_eed_verified&&
          layer.no_reactors_verified&&layer.xdictionary_missing;
    MusaDwgAc1032EntityLayerRef edge={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_entity_layer_ref(
            scene,UINT64_C(0x1ca),&edge)&&
            edge.layer_handle==layer.indexed_handle;
    if(ok){
       MusaDwgAc1032LayerIdentity saved=layer;
       ok=!musa_dwg_ac1032_scene_index_read_layer_identity(
               scene,UINT64_C(0x1ca),&layer)&&
          !memcmp(&layer,&saved,sizeof(layer))&&
          !musa_dwg_ac1032_scene_index_read_layer_identity(
               scene,UINT64_MAX,&layer)&&
          !memcmp(&layer,&saved,sizeof(layer))&&
          !musa_dwg_ac1032_scene_index_read_layer_identity(
               scene,UINT64_C(0x10),NULL);
    }
    size_t required=0u,filled=0u;
    MusaDwgAc1032PayloadInfo info={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    uint8_t *payload=NULL;
    if(ok)ok=musa_dwg_ac1032_object_payload_file(
           raw,n,UINT64_C(0x10),NULL,0u,&required,&info)&&
          musa_dwg_ac1032_stream_layout_file(
           raw,n,UINT64_C(0x10),&layout)&&
          info.original_record_crc_verified&&
          info.raw_object_type==51u&&required>10u&&required<65536u;
    if(ok){
       payload=(uint8_t*)malloc(required);
       ok=payload&&musa_dwg_ac1032_object_payload_file(
           raw,n,UINT64_C(0x10),payload,required,&filled,&again)&&
           filled==required;
    }
    if(ok){
       MusaDwgAc1032LayerIdentity second={0};
       ok=musa_dwg_ac1032_layer_ot51_identity_at(
           payload,required,&layout,&second)&&
           !memcmp(&layer,&second,sizeof(layer));
       if(ok){
           const MusaDwgAc1032LayerIdentity saved=second;
           const uint8_t original=payload[2];
           payload[2]^=0x10u; /* corrupt self-H, not OT */
           ok=!musa_dwg_ac1032_layer_ot51_identity_at(
                payload,required,&layout,&second)&&
              !memcmp(&saved,&second,sizeof(second));
           payload[2]=original;
           if(ok)ok=!musa_dwg_ac1032_layer_ot51_identity_at(
                payload,required-1u,&layout,&second)&&
                !memcmp(&saved,&second,sizeof(second));
       }
    }
    if(ok)ok=!memcmp(raw,backup,n);
    fprintf(stderr,"%s real original R2018 CRC/H LAYER OT51 "
         "self=0x10 name/flag DATA start bit=%zu; COLOR STILL UNRESOLVED\n",
         ok?"PASS":"FAIL",layer.data_fields_start_bit);
    free(payload);free(raw);free(backup);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok?0:1;
}
