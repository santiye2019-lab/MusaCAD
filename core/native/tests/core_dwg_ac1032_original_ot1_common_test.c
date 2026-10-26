/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Immutable genuine R2018 sample_2018.dwg OT1 TEXT H=8E:
 * only strict first-party source CRC/H and ENTITY common ENC evidence.
 * Companion sample_2018.dxf supplies AcDbText and original text oracle.
 * No claim of TEXT Unicode TU, placement, font, owner, or display.
 * Distinct from original 2018/Text.dwg OT44 MTEXT H1C0. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int sample(const char *filename){
    FILE *f=fopen(filename,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *src=(uint8_t*)malloc((size_t)n);
    uint8_t *saved=(uint8_t*)malloc((size_t)n);
    if(!src||!saved){free(src);free(saved);fclose(f);return 0;}
    int ok=fread(src,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(ok)memcpy(saved,src,(size_t)n);
    ok=ok&&memcmp(src,"AC1032",6u)==0;
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
        src,(size_t)n,&scene)&&!memcmp(src,saved,(size_t)n);
    free(src);free(saved);
    if(!ok){fprintf(stderr,"FAIL original OT1 physical scene\n");return 0;}
    const uint64_t h=UINT64_C(0x8e);
    uint16_t type=0u;
    MusaDwgAc1032EntityCommon common={0};
    ok=musa_dwg_ac1032_scene_index_object_type(scene,h,&type)&&
       type==1u&&
       musa_dwg_ac1032_scene_index_read_text_common(scene,h,&common)&&
       common.common_decoded&&common.prefix.prefix_decoded&&
       common.prefix.indexed_handle==h&&
       common.prefix.raw_object_type==1u&&
       common.prefix.reactor_count==0u&&
       common.prefix.xdictionary_missing==1u&&
       common.geometry_start_bit>common.prefix.next_color_bit&&
       common.linetype_scale>0.0;
    if(ok){
        MusaDwgAc1032EntityCommon sentinel;
        memset(&sentinel,0xa5,sizeof(sentinel));
        MusaDwgAc1032EntityCommon before=sentinel;
        ok=!musa_dwg_ac1032_scene_index_read_text_common(
                scene,UINT64_MAX,&sentinel)&&
           !memcmp(&sentinel,&before,sizeof(sentinel))&&
           !musa_dwg_ac1032_scene_index_read_mtext_common(
                scene,h,&sentinel)&&
           !memcmp(&sentinel,&before,sizeof(sentinel));
    }
    fprintf(stderr,"%s genuine R2018 OT1 TEXT H=8E common: "
        "ENC=%u mode=%u invisible=%u; TU/WCS/font/display UNVERIFIED\n",
        ok?"PASS":"FAIL",(unsigned)common.encoded_color,
        (unsigned)common.prefix.entity_mode,
        (unsigned)common.invisibility);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0; /* host ABI smoke without external fixture */
    if(argc!=2)return 2;
    return sample(argv[1])?0:1;
}
