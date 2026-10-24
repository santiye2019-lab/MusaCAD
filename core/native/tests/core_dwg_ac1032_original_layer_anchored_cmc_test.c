/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual AC1032 R2018 LAYER 0 source -> CMC C3+ACI7, with opaque
 * 7-bit gap preserved: do not infer general BYLAYER color fidelity. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int readall(const char *name,uint8_t **out,size_t *len){
    FILE *f=fopen(name,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *bytes=(uint8_t*)malloc((size_t)n);
    if(!bytes){fclose(f);return 0;}
    int ok=fread(bytes,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(bytes);return 0;}
    *out=bytes;*len=(size_t)n;return 1;
}
static void toggle(uint8_t *data,size_t bit){
    data[bit/8u]^=(uint8_t)(1u<<(7u-bit%8u));
}
static int test_file(const char *name){
    uint8_t *input=NULL,*original=NULL,*record=NULL;size_t n=0u;
    if(!readall(name,&input,&n))return 0;
    original=(uint8_t*)malloc(n);
    if(!original){free(input);return 0;}
    memcpy(original,input,n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(input,n,&scene);
    MusaDwgAc1032LayerCmcEvidence evidence={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_layer_anchored_cmc_evidence(
         scene,UINT64_C(0x10),&evidence)&&
         evidence.indexed_handle==UINT64_C(0x10)&&
         evidence.opaque_field_start_bit==45u&&
         evidence.opaque_field_bits==7u&&
         evidence.cmc_start_bit==52u&&evidence.cmc_end_bit==96u&&
         evidence.packed_color_method==UINT32_C(0xC3000007)&&
         evidence.aci_index==7u&&evidence.original_record_crc_verified;
    if(ok){
        const MusaDwgAc1032LayerCmcEvidence saved=evidence;
        ok=!musa_dwg_ac1032_scene_index_layer_anchored_cmc_evidence(
               scene,UINT64_C(0x1ca),&evidence)&&
           !memcmp(&saved,&evidence,sizeof(evidence))&&
           !musa_dwg_ac1032_scene_index_layer_anchored_cmc_evidence(
               scene,UINT64_MAX,&evidence)&&
           !memcmp(&saved,&evidence,sizeof(evidence))&&
           !musa_dwg_ac1032_scene_index_layer_anchored_cmc_evidence(
               scene,UINT64_C(0x10),NULL);
    }
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    size_t need=0u,copied=0u;
    if(ok)ok=musa_dwg_ac1032_object_payload_file(
         input,n,UINT64_C(0x10),NULL,0u,&need,&meta)&&
         musa_dwg_ac1032_stream_layout_file(
         input,n,UINT64_C(0x10),&layout)&&
         meta.original_record_crc_verified&&
         need>0u&&need<65536u;
    if(ok){
        record=(uint8_t*)malloc(need);
        ok=record&&musa_dwg_ac1032_object_payload_file(
            input,n,UINT64_C(0x10),record,need,&copied,&again)&&
            copied==need;
    }
    if(ok){
        MusaDwgAc1032LayerCmcEvidence result={0};
        ok=musa_dwg_ac1032_layer_ot51_anchored_cmc_evidence_at(
            record,need,&layout,&result)&&
            !memcmp(&result,&evidence,sizeof(result));
    }
    if(ok){
        const MusaDwgAc1032LayerCmcEvidence saved=evidence;
        toggle(record,51u); /* change one of the unexplained source bits */
        ok=!musa_dwg_ac1032_layer_ot51_anchored_cmc_evidence_at(
               record,need,&layout,&evidence)&&
           !memcmp(&saved,&evidence,sizeof(evidence));
        toggle(record,51u);
    }
    if(ok){
        const MusaDwgAc1032LayerCmcEvidence saved=evidence;
        toggle(record,80u); /* change C3 method high byte */
        ok=!musa_dwg_ac1032_layer_ot51_anchored_cmc_evidence_at(
               record,need,&layout,&evidence)&&
           !memcmp(&saved,&evidence,sizeof(evidence));
        toggle(record,80u);
    }
    if(ok){
        const MusaDwgAc1032LayerCmcEvidence saved=evidence;
        ok=!musa_dwg_ac1032_layer_ot51_anchored_cmc_evidence_at(
              record,need-1u,&layout,&evidence)&&
           !memcmp(&saved,&evidence,sizeof(evidence))&&
           !musa_dwg_ac1032_layer_ot51_anchored_cmc_evidence_at(
              record,need,&layout,NULL);
    }
    ok=ok&&!memcmp(input,original,n);
    fprintf(stderr,"%s actual R2018 OT51 LAYER 0 CMC method C3 "
      "ACI7 precisely source-bound [52,96), opaque [45,52) unresolved: %s\n",
      ok?"PASS":"FAIL",name);
    free(record);free(input);free(original);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1){
        MusaDwgAc1032LayerCmcEvidence sentinel={0};
        sentinel.aci_index=123u;
        return !musa_dwg_ac1032_scene_index_layer_anchored_cmc_evidence(
                 NULL,UINT64_C(0x10),&sentinel)&&
               sentinel.aci_index==123u?0:1;
    }
    if(argc!=6)return 2;
    int ok=1;
    for(int i=1;i<6;i++) if(!test_file(argv[i]))ok=0;
    return ok?0:1;
}
