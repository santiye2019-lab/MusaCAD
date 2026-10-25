/* SPDX-License-Identifier: GPL-3.0-or-later */
/* R2018 genuine layer0 H=0x10, exact source data bits, but NEVER
 * infer AutoCAD on-screen visibility without independent viewport tests.
 * The original data contains raw 0xE0; do not "fix" on=0 to visible. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int pure(void){
    MusaDwgAc1032LayerBaseBitPreflight b={0};
    if(!musa_dwg_ac1032_layer_base_bits_decode(
            UINT16_C(0x3ff),&b)||
       !b.frozen_bit||!b.on_bit||
       !b.frozen_in_new_viewports_bit||!b.locked_bit||
       !b.plotting_bit||b.lineweight_field!=31u||
       b.unmapped_bits||b.source_crc_verified||b.display_accepted)return 0;
    if(!musa_dwg_ac1032_layer_base_bits_decode(
            UINT16_C(0xe0),&b)||
       b.raw_values!=224u||b.frozen_bit||b.on_bit||
       b.frozen_in_new_viewports_bit||b.locked_bit||
       b.plotting_bit||b.lineweight_field!=7u||
       b.unmapped_bits||b.source_crc_verified||b.display_accepted)return 0;
    if(!musa_dwg_ac1032_layer_base_bits_decode(
            UINT16_C(0xfc00),&b)||
       b.unmapped_bits!=UINT16_C(0xfc00)||b.frozen_bit||
       b.on_bit||b.frozen_in_new_viewports_bit||b.locked_bit||
       b.plotting_bit||b.lineweight_field!=0u||b.display_accepted)return 0;
    const MusaDwgAc1032LayerBaseBitPreflight old=b;
    if(musa_dwg_ac1032_layer_base_bits_decode(5u,NULL)||
       memcmp(&old,&b,sizeof(b))!=0)return 0;
    for(unsigned bit=0;bit<16u;bit++){
        if(!musa_dwg_ac1032_layer_base_bits_decode(
                (uint16_t)(1u<<bit),&b)||b.display_accepted||
           b.source_crc_verified)return 0;
        const unsigned marked=(unsigned)b.frozen_bit+
          (unsigned)b.on_bit+
          (unsigned)b.frozen_in_new_viewports_bit+
          (unsigned)b.locked_bit+
          (unsigned)b.plotting_bit;
        if(bit<5u&&marked!=1u)return 0;
        if(bit>=5u&&marked!=0u)return 0;
        if(bit<10u&&b.unmapped_bits)return 0;
        if(bit>=10u&&b.unmapped_bits!=(uint16_t)(1u<<bit))
            return 0;
    }
    return 1;
}
static int load(const char *name,uint8_t **out,size_t *size){
    FILE *f=fopen(name,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)!=0){
        fclose(f);return 0;
    }
    uint8_t *p=(uint8_t*)malloc((size_t)n);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(p);return 0;}
    *out=p;*size=(size_t)n;return 1;
}
static int original(const char *path){
    uint8_t *raw=NULL,*backup=NULL,*payload=NULL;size_t n=0u;
    if(!load(path,&raw,&n))return 0;
    backup=(uint8_t*)malloc(n);
    if(!backup){free(raw);return 0;}
    memcpy(backup,raw,n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(raw,n,&scene);
    MusaDwgAc1032LayerFlagsEvidence f={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
            scene,UINT64_C(0x10),&f)&&f.source_crc_verified&&
            f.raw_values_70_290_370==UINT16_C(0xe0);
    size_t needed=0u,got=0u;
    MusaDwgAc1032PayloadInfo meta={0},again={0};
    MusaDwgAc1032ObjectStreams layout={0};
    if(ok)ok=musa_dwg_ac1032_object_payload_file(
               raw,n,UINT64_C(0x10),NULL,0u,&needed,&meta)&&
             needed>0u&&needed<65536u&&
             musa_dwg_ac1032_stream_layout_file(
               raw,n,UINT64_C(0x10),&layout);
    if(ok){
        payload=(uint8_t*)malloc(needed);
        ok=payload&&musa_dwg_ac1032_object_payload_file(
           raw,n,UINT64_C(0x10),payload,needed,&got,&again)&&
           got==needed;
    }
    MusaDwgAc1032LayerBaseBitPreflight b={0};
    if(ok){
        ok=musa_dwg_ac1032_layer_ot51_base_bits_at(
              payload,needed,&layout,&b)&&
           b.source_crc_verified&&b.layer_handle==UINT64_C(0x10)&&
           b.raw_values==f.raw_values_70_290_370&&
           b.lineweight_field==7u&&
           b.on_bit==0u&&b.frozen_bit==0u&&
           b.plotting_bit==0u&&b.unmapped_bits==0u&&
           b.display_accepted==0u;
    }
    if(ok){
        MusaDwgAc1032LayerBaseBitPreflight unchanged=b;
        MusaDwgAc1032ObjectStreams bad=layout;
        bad.raw_object_type=19u;
        ok=!musa_dwg_ac1032_layer_ot51_base_bits_at(
             payload,needed,&bad,&b)&&
           memcmp(&b,&unchanged,sizeof(b))==0&&
           !musa_dwg_ac1032_layer_ot51_base_bits_at(
             payload,needed-1u,&layout,&b)&&
           memcmp(&b,&unchanged,sizeof(b))==0&&
           !musa_dwg_ac1032_layer_ot51_base_bits_at(
             payload,needed,&layout,NULL);
    }
    ok=ok&&!memcmp(raw,backup,n);
    fprintf(stderr,"%s original R2018 LAYER 0 raw=0xE0 on-bit=0 "
       "frozen-bit=0 plot-bit=0 lw-field=7; display=NOT ACCEPTED\n",
       ok?"PASS":"FAIL");
    free(payload);free(raw);free(backup);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(!pure())return 1;
    if(argc==1)return 0;
    if(argc!=6)return 2;
    int all=1;
    for(int i=1;i<argc;i++)all=original(argv[i])&&all;
    return all?0:1;
}
