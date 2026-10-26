/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine R2018 OT1 H8E OCS -> WCS source anchor oracle.
 * Synthetic negative-Z and rotation verify arbitrary-axis transform.
 * NO glyph baseline/font or resolved model-space visibility. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int near(double a,double b){return isfinite(a)&&fabs(a-b)<1e-8;}
static int synthetic(void){
    MusaDwgAc1032TextPlacement p={0};
    p.indexed_handle=UINT64_C(0x8e);
    p.original_record_crc_verified=1u;
    p.numeric_fields_decoded=1u;
    p.extrusion[2]=-1.0;
    p.insertion[0]=2.0;p.insertion[1]=3.0;p.insertion[2]=4.0;
    p.alignment[0]=5.0;p.alignment[1]=6.0;p.alignment[2]=7.0;
    p.height=2.0;p.width_factor=1.0;
    p.rotation=acos(-1.0)/2.0;
    MusaDwgAc1032TextWorldPlacement w={0};
    if(!musa_dwg_ac1032_text_placement_world(&p,&w)||
       !near(w.insertion[0],-2.0)||
       !near(w.insertion[1],3.0)||
       !near(w.insertion[2],-4.0)||
       !near(w.alignment[0],-5.0)||
       !near(w.alignment[1],6.0)||
       !near(w.alignment[2],-7.0)||
       !near(w.baseline_unit[0],0.0)||
       !near(w.baseline_unit[1],1.0)||
       !near(w.baseline_unit[2],0.0)||
       !near(w.up_unit[0],1.0)||
       !near(w.up_unit[1],0.0)||
       !near(w.up_unit[2],0.0)||
       !w.wcs_math_decoded||w.font_and_display_accepted)return 0;
    MusaDwgAc1032TextWorldPlacement bad;
    memset(&bad,0xa5,sizeof(bad));
    const MusaDwgAc1032TextWorldPlacement saved=bad;
    p.extrusion[2]=0.0;
    return !musa_dwg_ac1032_text_placement_world(&p,&bad)&&
           !memcmp(&bad,&saved,sizeof(bad));
}
static int original(const char *filename){
    FILE *f=fopen(filename,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    const long n=ftell(f);
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
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
        src,(size_t)n,&scene)&&!memcmp(src,copy,(size_t)n);
    free(src);free(copy);
    if(!ok)return 0;
    const uint64_t h=UINT64_C(0x8e);
    MusaDwgAc1032TextSource text={0};
    MusaDwgAc1032TextPlacement p={0};
    MusaDwgAc1032TextWorldPlacement w={0};
    ok=musa_dwg_ac1032_scene_index_read_text_source(scene,h,&text)&&
       !strcmp(text.utf8,"Jen teksto simpla, cxu ne?")&&
       musa_dwg_ac1032_scene_index_read_text_placement(scene,h,&p)&&
       musa_dwg_ac1032_text_placement_world(&p,&w)&&
       w.indexed_handle==h&&w.original_record_crc_verified&&
       w.wcs_math_decoded&&!w.font_and_display_accepted&&
       near(p.extrusion[0],0.0)&&near(p.extrusion[1],0.0)&&
       near(p.extrusion[2],1.0)&&
       near(w.insertion[0],-50.31873725478131)&&
       near(w.insertion[1],134.695759123913)&&
       near(w.insertion[2],0.0)&&
       near(w.alignment[0],9.264596078552017)&&
       near(w.alignment[1],136.3624257905797)&&
       near(w.alignment[2],0.0)&&
       near(w.baseline_unit[0],1.0)&&
       near(w.baseline_unit[1],0.0)&&
       near(w.up_unit[0],0.0)&&
       near(w.up_unit[1],1.0)&&
       near(w.height,5.0)&&
       w.horizontal_alignment==4u&&
       synthetic();
    fprintf(stderr,"%s genuine OT1 H8E WCS anchors "
        "insertion=(%.10g,%.10g) align=(%.10g,%.10g); "
        "font/glyph/model/viewport UNVERIFIED\\n",
        ok?"PASS":"FAIL",w.insertion[0],w.insertion[1],
        w.alignment[0],w.alignment[1]);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return synthetic()?0:1;
    if(argc!=2)return 2;
    return original(argv[1])?0:1;
}
