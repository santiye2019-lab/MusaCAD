/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent unchanged R2018 example_2018.dwg, DXF AcDbAttribute
 * group-5 H18F. Admit ONLY source CRC/H authenticated OT3 entity
 * common, never string content, INSERT ownership, model-space or display. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int genuine(const char *file){
    FILE *f=fopen(file,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *src=(uint8_t*)malloc((size_t)n);
    uint8_t *copy=(uint8_t*)malloc((size_t)n);
    if(!src||!copy){free(src);free(copy);fclose(f);return 0;}
    int ok=fread(src,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(ok)memcpy(copy,src,(size_t)n);
    ok=ok&&!memcmp(src,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(src,(size_t)n,&scene)&&
             !memcmp(src,copy,(size_t)n);
    free(src);free(copy);
    if(!ok)return 0;
    const uint64_t hs[]={UINT64_C(0x18f)};
    for(size_t i=0u;i<1u&&ok;i++){
        MusaDwgAc1032EntityCommon common={0};
        uint16_t type=0u;
        ok=musa_dwg_ac1032_scene_index_object_type(scene,hs[i],&type)&&
           type==3u&&
           musa_dwg_ac1032_scene_index_read_attdef_common(
               scene,hs[i],&common)&&
           common.common_decoded&&common.prefix.prefix_decoded&&
           common.prefix.indexed_handle==hs[i]&&
           common.prefix.raw_object_type==3u&&
           common.geometry_start_bit>common.prefix.next_color_bit;
    }
    if(ok){
        MusaDwgAc1032EntityCommon untouched;
        memset(&untouched,0x5a,sizeof(untouched));
        const MusaDwgAc1032EntityCommon before=untouched;
        ok=!musa_dwg_ac1032_scene_index_read_attdef_common(
                 scene,UINT64_MAX,&untouched)&&
           !memcmp(&before,&untouched,sizeof(before))&&
           !musa_dwg_ac1032_scene_index_read_attdef_common(
                 scene,UINT64_C(0x190),&untouched)&&
           !memcmp(&before,&untouched,sizeof(before))&&
           !musa_dwg_ac1032_scene_index_read_attdef_common(
                 scene,UINT64_C(0x18f),NULL);
    }
    fprintf(stderr,"%s original R2018 ATTDEF OT3 H18F "
        "CRC/H/common ENC; text/owner/placement/rendering UNVERIFIED\n",
        ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1){
        MusaDwgAc1032EntityCommon untouched;
        memset(&untouched,0x5a,sizeof(untouched));
        const MusaDwgAc1032EntityCommon before=untouched;
        return !musa_dwg_ac1032_scene_index_read_attdef_common(
            NULL,UINT64_C(0x18f),&untouched)&&
            !memcmp(&before,&untouched,sizeof(before))?0:1;
    }
    if(argc!=2)return 2;
    return genuine(argv[1])?0:1;
}
