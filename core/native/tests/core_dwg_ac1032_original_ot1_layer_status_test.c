/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine R2018 OT1 H8E -> Unicode OT51 H81 raw layer status evidence.
 * Independently verify source flags vs C11 bit decomposition, WITHOUT
 * promoting 'on' to model/viewport visibility. No LibreDWG runtime. */
#include "musacad_dwg_ac1032_text_bundle.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int original(const char *file){
    FILE *f=fopen(file,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *src=(uint8_t*)malloc((size_t)n);
    uint8_t *saved=(uint8_t*)malloc((size_t)n);
    if(!src||!saved){free(src);free(saved);fclose(f);return 0;}
    int ok=fread(src,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(ok)memcpy(saved,src,(size_t)n);
    ok=ok&&!memcmp(src,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
        src,(size_t)n,&scene)&&!memcmp(src,saved,(size_t)n);
    free(src);free(saved);
    if(!ok)return 0;
    MusaDwgAc1032TextBundle b={0};
    MusaDwgAc1032LayerFlagsEvidence raw={0};
    MusaDwgAc1032LayerBaseBitPreflight bits={0};
    ok=musa_dwg_ac1032_scene_text_bundle(
         scene,UINT64_C(0x8e),&b)&&
       b.source_join_verified&&
       b.graph.layer_handle==UINT64_C(0x81)&&
       !strcmp(b.layer.utf8,"Tavolo 1")&&
       musa_dwg_ac1032_scene_index_read_layer_flags_evidence(
           scene,b.graph.layer_handle,&raw)&&
       musa_dwg_ac1032_layer_base_bits_decode(
           raw.raw_values_70_290_370,&bits)&&
       !memcmp(&raw,&b.layer_flags,sizeof(raw))&&
       b.layer_bits.layer_handle==b.graph.layer_handle&&
       b.layer_bits.source_crc_verified&&
       !b.layer_bits.display_accepted&&
       b.layer_bits.raw_values==bits.raw_values&&
       b.layer_bits.unmapped_bits==bits.unmapped_bits&&
       b.layer_bits.frozen_bit==bits.frozen_bit&&
       b.layer_bits.on_bit==bits.on_bit&&
       b.layer_bits.frozen_in_new_viewports_bit==
           bits.frozen_in_new_viewports_bit&&
       b.layer_bits.locked_bit==bits.locked_bit&&
       b.layer_bits.plotting_bit==bits.plotting_bit&&
       b.layer_bits.lineweight_field==bits.lineweight_field&&
       !b.model_viewport_visibility_accepted&&
       !b.final_display_color_accepted;
    if(ok){
        MusaDwgAc1032TextBundle sentinel;
        memset(&sentinel,0x5a,sizeof(sentinel));
        const MusaDwgAc1032TextBundle before=sentinel;
        ok=!musa_dwg_ac1032_scene_text_bundle(
             scene,UINT64_MAX,&sentinel)&&
           !memcmp(&sentinel,&before,sizeof(sentinel));
    }
    fprintf(stderr,"%s genuine R2018 OT1 H8E -> LAYER H81 "
        "raw status 0x%X; viewport/owner/visibility UNVERIFIED\n",
        ok?"PASS":"FAIL",(unsigned)raw.raw_values_70_290_370);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return original(argv[1])?0:1;
}
