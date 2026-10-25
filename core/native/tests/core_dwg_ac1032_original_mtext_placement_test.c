/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original R2018/Text.dwg MTEXT H1C0 geometry oracle from AutoCAD Text.txt.
 * Source point/size/axes only. No font, glyph, paragraph, owner, viewport,
 * layer visibility, final color or drawn text acceptance. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int near(double a,double b,double tolerance){
    return isfinite(a)&&fabs(a-b)<tolerance;
}
int main(int argc,char **argv){
    if(argc==1)return 0; /* CTest portable API compile smoke */
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");
    if(!f)return 1;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 1;}
    const long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)!=0){
        fclose(f);return 1;
    }
    uint8_t *bytes=(uint8_t*)malloc((size_t)n);
    uint8_t *snapshot=(uint8_t*)malloc((size_t)n);
    if(!bytes||!snapshot){free(bytes);free(snapshot);fclose(f);return 1;}
    int ok=fread(bytes,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    memcpy(snapshot,bytes,(size_t)n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(bytes,"AC1032",6u)&&
        musa_dwg_ac1032_scene_index_open(bytes,(size_t)n,&scene)&&
        memcmp(bytes,snapshot,(size_t)n)==0;
    free(bytes);free(snapshot);
    MusaDwgAc1032MTextPlacement place={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_mtext_placement(
        scene,UINT64_C(0x1c0),&place)&&
        place.indexed_handle==UINT64_C(0x1c0)&&
        place.original_record_crc_verified&&place.geometry_fields_decoded&&
        !place.glyph_layout_and_display_accepted&&
        near(place.insertion[0],7.33298,0.0002)&&
        near(place.insertion[1],14.1158,0.0002)&&
        near(place.insertion[2],0.0,0.00001)&&
        near(place.extrusion[0],0.0,0.00001)&&
        near(place.extrusion[1],0.0,0.00001)&&
        near(place.extrusion[2],1.0,0.00001)&&
        near(place.x_axis[0],1.0,0.00001)&&
        near(place.x_axis[1],0.0,0.00001)&&
        near(place.x_axis[2],0.0,0.00001)&&
        near(place.rect_width,13.0319,0.0002)&&
        near(place.text_height,0.2,0.00001)&&
        place.attachment==1u&&place.drawing_direction==5u&&
        place.geometry_end_bit<place.original_string_start_bit;
    if(ok){
        const MusaDwgAc1032MTextPlacement saved=place;
        if(musa_dwg_ac1032_scene_index_read_mtext_placement(
               scene,UINT64_MAX,&place)||
           memcmp(&saved,&place,sizeof(place))!=0)ok=0;
    }
    fprintf(stderr,"%s original R2018 MTEXT H1C0 source placement/size/axis; glyphs and full drawing UNRESOLVED\n",
        ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok?0:1;
}
