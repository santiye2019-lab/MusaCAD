/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine untouched R2018 sample_2018.dwg OT1 H8E joined evidence.
 * Separate from OT44 MTEXT; no assertion of glyph/viewport rendering. */
#include "musacad_dwg_ac1032_text_bundle.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int near(double a,double b){
    return isfinite(a)&&fabs(a-b)<1e-8;
}
static int original(const char *path){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    const long n=ftell(f);
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
    MusaDwgAc1032TextBundle bundle={0};
    ok=musa_dwg_ac1032_scene_text_bundle(
           scene,UINT64_C(0x8e),&bundle)&&
       bundle.source_join_verified&&
       !bundle.font_glyph_layout_accepted&&
       !bundle.model_viewport_visibility_accepted&&
       !bundle.final_display_color_accepted&&
       !strcmp(bundle.unicode.utf8,"Jen teksto simpla, cxu ne?")&&
       bundle.unicode.utf16_code_units==26u&&
       bundle.world.horizontal_alignment==4u&&
       near(bundle.world.insertion[0],-50.31873725478131)&&
       near(bundle.world.insertion[1],134.695759123913)&&
       near(bundle.world.alignment[0],9.264596078552017)&&
       near(bundle.world.alignment[1],136.3624257905797)&&
       near(bundle.world.height,5.0)&&
       bundle.graph.layer_object_type==51u&&
       bundle.graph.style_object_type==53u&&
       bundle.graph.layer_handle==bundle.color.layer_handle&&
       !strcmp(bundle.layer.utf8,"Tavolo 1")&&
       bundle.color.entity_and_layer_crc_verified&&
       !bundle.color.rendering_accepted;
    if(ok){
        MusaDwgAc1032TextBundle bad;
        memset(&bad,0xa5,sizeof(bad));
        const MusaDwgAc1032TextBundle saved_bad=bad;
        ok=!musa_dwg_ac1032_scene_text_bundle(
              scene,UINT64_MAX,&bad)&&
           !memcmp(&bad,&saved_bad,sizeof(bad))&&
           !musa_dwg_ac1032_scene_text_bundle(
              scene,0u,&bad)&&
           !memcmp(&bad,&saved_bad,sizeof(bad));
    }
    fprintf(stderr,"%s genuine OT1 H8E Unicode/WCS/OT51/ENC bundle; "
        "glyph/font/owner/viewport/final color UNVERIFIED\n",
        ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0; /* host compile smoke */
    if(argc!=2)return 2;
    return original(argv[1])?0:1;
}
