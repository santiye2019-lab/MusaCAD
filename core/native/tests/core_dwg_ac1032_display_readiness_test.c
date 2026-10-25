/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine untouched 2018 CAD source files: source layer and color census,
 * distinct from a complete visible drawing or exact BYLAYER rendering. */
#include "musacad_dwg_ac1032_display_readiness.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int load(const char *file,uint8_t **out,size_t *bytes){
    FILE *f=fopen(file,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long n=ftell(f);
    if(n<256||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)!=0){
        fclose(f);return 0;
    }
    uint8_t *raw=(uint8_t*)malloc((size_t)n);
    if(!raw){fclose(f);return 0;}
    int ok=fread(raw,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(raw);return 0;}
    *out=raw;*bytes=(size_t)n;return 1;
}
static int run(const char *file,uint64_t handle,uint16_t type){
    uint8_t *raw=NULL,*copy=NULL;size_t n=0u;
    if(!load(file,&raw,&n))return 0;
    copy=(uint8_t*)malloc(n);if(!copy){free(raw);return 0;}
    memcpy(copy,raw,n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(raw,n,&scene)&&
           memcmp(raw,copy,n)==0;
    free(copy);free(raw);
    MusaAc1032DisplayReadiness report={0};
    if(ok){
        MusaAc1032PlanarStyleEvidence entity={0};
        ok=musa_dwg_ac1032_scene_planar_style_evidence(
               scene,handle,&entity)&&
           entity.color.raw_entity_type==type&&
           musa_dwg_ac1032_scene_display_readiness(scene,&report)&&
           report.indexed_records==musa_dwg_ac1032_scene_index_count(scene)&&
           report.known_planar_records>0u&&
           report.source_style_joined>0u&&
           report.known_style_rejected<=report.known_planar_records&&
           report.source_style_joined+report.known_style_rejected==
               report.known_planar_records&&
           report.known_planar_records+report.other_indexed_records==
               report.indexed_records&&
           report.source_direct_aci+report.source_bylayer+
             report.source_byblock+report.source_complex_or_invisible==
               report.source_style_joined&&
           report.narrow_layer_aci_available>=1u&&
           report.narrow_layer_aci_available<=report.source_style_joined&&
           report.source_direct_rgb24<=report.source_complex_or_invisible&&
           report.display_accepted==0u;
    }
    if(ok){
        const MusaAc1032DisplayReadiness saved=report;
        MusaAc1032DisplayReadiness twice={0};
        ok=musa_dwg_ac1032_scene_display_readiness(scene,&twice)&&
           memcmp(&saved,&twice,sizeof(saved))==0&&
           !musa_dwg_ac1032_scene_display_readiness(NULL,&report)&&
           memcmp(&saved,&report,sizeof(saved))==0&&
           !musa_dwg_ac1032_scene_display_readiness(scene,NULL)&&
           memcmp(&saved,&report,sizeof(saved))==0;
    }
    fprintf(stderr,"%s original R2018 OT%u indexed=%zu planar=%zu "
         "linked=%zu rejected=%zu direct=%zu layer=%zu block=%zu "
         "complex=%zu other=%zu display=UNRESOLVED\n",
         ok?"PASS":"FAIL",(unsigned)type,report.indexed_records,
         report.known_planar_records,report.source_style_joined,
         report.known_style_rejected,report.source_direct_aci,
         report.source_bylayer,report.source_byblock,
         report.source_complex_or_invisible,report.other_indexed_records);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1){
        MusaAc1032DisplayReadiness r={0};
        r.indexed_records=77u;
        return !musa_dwg_ac1032_scene_display_readiness(NULL,&r)&&
               r.indexed_records==77u?0:1;
    }
    if(argc!=6)return 2;
    int pass=1;
    pass=run(argv[1],UINT64_C(0x1ca),19u)&&pass;
    pass=run(argv[2],UINT64_C(0x1c0),27u)&&pass;
    pass=run(argv[3],UINT64_C(0x1bd),17u)&&pass;
    pass=run(argv[4],UINT64_C(0x1bd),18u)&&pass;
    pass=run(argv[5],UINT64_C(0x1bd),77u)&&pass;
    return pass?0:1;
}
