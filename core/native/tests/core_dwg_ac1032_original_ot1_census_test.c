/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original R2018 sample_2018.dwg genuine OT1 H8E now accounted as
 * source-authenticated 2D TEXT, not as opaque or MTEXT. No font/viewport
 * acceptance and no guess about other unsupported indexed records. */
#include "musacad_dwg_ac1032_display_readiness.h"
#include "musacad_dwg_ac1032_text_bundle.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int original(const char *path){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *raw=(uint8_t*)malloc((size_t)n);
    uint8_t *saved=(uint8_t*)malloc((size_t)n);
    if(!raw||!saved){free(raw);free(saved);fclose(f);return 0;}
    int ok=fread(raw,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(ok)memcpy(saved,raw,(size_t)n);
    ok=ok&&!memcmp(raw,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
        raw,(size_t)n,&scene)&&!memcmp(raw,saved,(size_t)n);
    free(raw);free(saved);
    if(!ok)return 0;
    MusaAc1032DisplayReadiness report={0};
    MusaDwgAc1032TextBundle h8e={0};
    size_t observed_ot1=0u;
    if(ok)ok=musa_dwg_ac1032_scene_text_bundle(
        scene,UINT64_C(0x8e),&h8e)&&
        h8e.source_join_verified&&
        !strcmp(h8e.unicode.utf8,"Jen teksto simpla, cxu ne?");
    const size_t count=musa_dwg_ac1032_scene_index_count(scene);
    for(size_t i=0u;ok&&i<count;i++){
        MusaDwgAc1032HandleEntry e={0};uint16_t type=0u;
        ok=musa_dwg_ac1032_scene_index_entry(scene,i,&e)&&
           musa_dwg_ac1032_scene_index_object_type(
               scene,e.handle,&type);
        if(ok&&type==1u)observed_ot1++;
    }
    if(ok)ok=musa_dwg_ac1032_scene_display_readiness(
        scene,&report)&&
        report.indexed_records==count&&
        report.original_ot1_text_records==observed_ot1&&
        report.original_ot1_text_records>=1u&&
        report.original_ot1_text_joined>=1u&&
        report.original_ot1_text_joined+
           report.original_ot1_text_rejected==observed_ot1&&
        report.source_style_joined+
           report.known_style_rejected==report.known_planar_records&&
        report.known_planar_records+
           report.other_indexed_records==count&&
        !report.display_accepted;
    if(ok){
        MusaAc1032DisplayReadiness bad;
        memset(&bad,0xa5,sizeof(bad));
        const MusaAc1032DisplayReadiness before=bad;
        ok=!musa_dwg_ac1032_scene_display_readiness(
            NULL,&bad)&&!memcmp(&bad,&before,sizeof(bad));
    }
    fprintf(stderr,"%s R2018 OT1 H8E counted: original=%zu "
        "joined=%zu rejected=%zu; glyph/viewport UNVERIFIED\n",
        ok?"PASS":"FAIL",report.original_ot1_text_records,
        report.original_ot1_text_joined,
        report.original_ot1_text_rejected);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return original(argv[1])?0:1;
}
