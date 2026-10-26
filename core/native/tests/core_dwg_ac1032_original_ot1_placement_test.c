/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent original R2018 sample_2018.dwg H8E AcDbText OCS oracle. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int near(double a,double b){return isfinite(a)&&fabs(a-b)<1e-8;}
static int check(const char *filename){
    FILE *f=fopen(filename,"rb");
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
    MusaDwgAc1032TextPlacement p={0};
    MusaDwgAc1032TextSource txt={0};
    const uint64_t h=UINT64_C(0x8e);
    ok=musa_dwg_ac1032_scene_index_read_text_source(scene,h,&txt)&&
       !strcmp(txt.utf8,"Jen teksto simpla, cxu ne?")&&
       musa_dwg_ac1032_scene_index_read_text_placement(scene,h,&p)&&
       p.indexed_handle==h&&p.original_record_crc_verified&&
       p.numeric_fields_decoded&&!p.wcs_font_and_display_accepted&&
       near(p.insertion[0],-50.31873725478131)&&
       near(p.insertion[1],134.695759123913)&&
       near(p.insertion[2],0.0)&&
       near(p.alignment[0],9.264596078552017)&&
       near(p.alignment[1],136.3624257905797)&&
       near(p.alignment[2],0.0)&&
       near(p.height,5.0)&&
       near(p.rotation,0.0)&&
       near(p.width_factor,1.0)&&
       p.horizontal_alignment==4u&&p.vertical_alignment==0u;
    if(ok){
        MusaDwgAc1032TextPlacement bad;
        memset(&bad,0xa5,sizeof(bad));
        const MusaDwgAc1032TextPlacement before=bad;
        ok=!musa_dwg_ac1032_scene_index_read_text_placement(
             scene,UINT64_MAX,&bad)&&
           !memcmp(&bad,&before,sizeof(bad));
    }
    fprintf(stderr,"%s genuine OT1 H8E dataflags=%02X OCS=(%.12g,%.12g,%.12g) align=(%.12g,%.12g) height=%.12g halign=%u geom_end=%zu string_start=%zu; WCS/font/display UNVERIFIED\\n",
       ok?"PASS":"FAIL",(unsigned)p.data_flags,
       p.insertion[0],p.insertion[1],p.insertion[2],
       p.alignment[0],p.alignment[1],p.height,
       (unsigned)p.horizontal_alignment,p.geometry_end_bit,
       p.original_string_start_bit);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return check(argv[1])?0:1;
}
