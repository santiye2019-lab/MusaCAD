/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Five immutable original AutoCAD R2018 files: source joined H/CRC layer
 * color+raw flags+name for 2D entities, NEVER certified display styles. */
#include "musacad_dwg_ac1032_style_evidence.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int load(const char *name,uint8_t **out,size_t *len){
    FILE *f=fopen(name,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long n=ftell(f);
    if(n<256||n>64L*1024L*1024L||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *raw=(uint8_t*)malloc((size_t)n);
    if(!raw){fclose(f);return 0;}
    int ok=fread(raw,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(raw);return 0;}
    *out=raw;*len=(size_t)n;
    return 1;
}
static int check(const char *file,uint64_t entity,uint16_t type){
    uint8_t *raw=NULL,*saved=NULL;
    size_t len=0u;
    if(!load(file,&raw,&len))return 0;
    saved=(uint8_t*)malloc(len);
    if(!saved){free(raw);return 0;}
    memcpy(saved,raw,len);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(raw,len,&scene)&&
           memcmp(raw,saved,len)==0;
    free(raw);free(saved);
    MusaAc1032PlanarStyleEvidence bundle={0};
    if(ok){
        ok=musa_dwg_ac1032_scene_planar_style_evidence(
             scene,entity,&bundle)&&
           bundle.source_layer_join_verified==1u&&
           bundle.color.entity_handle==entity&&
           bundle.color.raw_entity_type==type&&
           bundle.color.layer_handle==UINT64_C(0x10)&&
           bundle.color.entity_and_layer_crc_verified==1u&&
           bundle.color.source_layer_aci_supported==1u&&
           bundle.color.source_layer_aci==7u&&
           bundle.color.rendering_accepted==0u&&
           (!bundle.color.source_entity_invisible||
              (bundle.color.entity_complex_color_or_transparency&&
               !bundle.color.entity_has_direct_rgb24))&&
           (!bundle.color.entity_has_direct_rgb24||
             (bundle.color.entity_complex_color_or_transparency&&
              bundle.color.entity_source_rgb24<=UINT32_C(0x00ffffff)))&&
           (bundle.color.entity_has_direct_rgb24||
             bundle.color.entity_source_rgb24==0u)&&
           bundle.raw_layer_status.indexed_handle==UINT64_C(0x10)&&
           bundle.raw_layer_status.source_crc_verified==1u&&
           bundle.raw_layer_status.numeric_flags_start_bit==31u&&
           bundle.raw_layer_status.numeric_flags_end_bit==45u&&
           bundle.raw_layer_status.raw_values_70_290_370==224u&&
           bundle.raw_layer_status.xref_index_plus_one==0u&&
           bundle.raw_layer_status.xref_dependent==0u&&
           bundle.original_layer_name.indexed_handle==UINT64_C(0x10)&&
           bundle.original_layer_name.original_record_crc_verified==1u&&
           bundle.original_layer_name.utf8_bytes==1u&&
           strcmp(bundle.original_layer_name.utf8,"0")==0&&
           bundle.visibility_resolved==0u&&
           bundle.display_style_resolved==0u;
    }
    if(ok){
        const MusaAc1032PlanarStyleEvidence untouched=bundle;
        ok=!musa_dwg_ac1032_scene_planar_style_evidence(
               scene,UINT64_C(0x10),&bundle)&&
           memcmp(&bundle,&untouched,sizeof(bundle))==0&&
           !musa_dwg_ac1032_scene_planar_style_evidence(
               scene,UINT64_MAX,&bundle)&&
           memcmp(&bundle,&untouched,sizeof(bundle))==0&&
           !musa_dwg_ac1032_scene_planar_style_evidence(
               scene,entity,NULL);
    }
    fprintf(stderr,"%s R2018 original OT%u H=%llX LAYER0:"
        " Unicode=0 raw-flag=224 source-ACI=7; display NOT accepted\n",
        ok?"PASS":"FAIL",(unsigned)type,(unsigned long long)entity);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1){
        MusaAc1032PlanarStyleEvidence sentinel={0};
        sentinel.raw_layer_status.raw_values_70_290_370=321u;
        return !musa_dwg_ac1032_scene_planar_style_evidence(
             NULL,UINT64_C(0x1ca),&sentinel)&&
             sentinel.raw_layer_status.raw_values_70_290_370==321u?0:1;
    }
    if(argc!=6)return 2;
    int ok=1;
    ok=check(argv[1],UINT64_C(0x1ca),19u)&&ok;
    ok=check(argv[2],UINT64_C(0x1c0),27u)&&ok;
    ok=check(argv[3],UINT64_C(0x1bd),17u)&&ok;
    ok=check(argv[4],UINT64_C(0x1bd),18u)&&ok;
    /* OT77 must agree with the same independent H/CRC layer proof.
     * No ABI promotion into renderer visibility or palette selection. */
    ok=check(argv[5],UINT64_C(0x1bd),77u)&&ok;
    return ok?0:1;
}
