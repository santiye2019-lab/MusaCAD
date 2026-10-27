/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent original R2018 sample_2018.dwg OT1 AcDbText H8E:
 * exact source Unicode TU vs companion DXF oracle, NOT glyph layout. */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_source_utf8.h"
#include "musacad_dwg_ac1032_text_source_bundle.h"
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
    ok=ok&&!memcmp(src,"AC1032",6u);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
        src,(size_t)n,&scene)&&!memcmp(src,saved,(size_t)n);
    free(src);free(saved);
    if(!ok){fprintf(stderr,"FAIL original OT1 physical scene\n");return 0;}
    const uint64_t h=UINT64_C(0x8e);
    MusaDwgAc1032TextSource text={0};
    MusaDwgAc1032EntityCommon common={0};
    ok=musa_dwg_ac1032_scene_index_read_text_common(scene,h,&common)&&
       common.prefix.raw_object_type==1u&&
       musa_dwg_ac1032_scene_index_read_text_source(scene,h,&text)&&
       text.original_record_crc_verified&&
       !text.geometry_font_and_display_accepted&&
       text.indexed_handle==h&&
       text.utf16_code_units==26u&&
       text.utf8_bytes==26u&&
       musa_dwg_ac1032_source_utf8_valid(
           text.utf8,text.utf8_bytes,sizeof(text.utf8),1)&&
       !strcmp(text.utf8,"Jen teksto simpla, cxu ne?");
    if(ok){
        MusaDwgAc1032TextSource sentinel;
        memset(&sentinel,0x5a,sizeof(sentinel));
        MusaDwgAc1032TextSource before=sentinel;
        ok=!musa_dwg_ac1032_scene_index_read_text_source(
              scene,UINT64_MAX,&sentinel)&&
           !memcmp(&sentinel,&before,sizeof(sentinel));
    }
    if(ok){
        MusaDwgAc1032TextSourceBundle bundle={0};
        ok=musa_dwg_ac1032_text_source_bundle(scene,h,1u,&bundle)&&
           bundle.indexed_handle==h&&bundle.raw_object_type==1u&&
           bundle.original_unicode_numeric_h_graph_verified&&
           bundle.original_layer_name_verified&&
           !bundle.font_layout_visibility_color_and_paint_accepted&&
           bundle.layer_handle!=0u&&bundle.style_handle!=0u&&
           bundle.utf8_bytes==text.utf8_bytes&&
           !strcmp(bundle.utf8,text.utf8);
        MusaDwgAc1032LayerName original_layer={0};
        if(ok)ok=musa_dwg_ac1032_scene_index_read_layer_name(
                       scene,bundle.layer_handle,&original_layer)&&
                  original_layer.original_record_crc_verified&&
                  original_layer.indexed_handle==bundle.layer_handle&&
                  bundle.layer_utf8_bytes==original_layer.utf8_bytes&&
                  bundle.layer_utf8_bytes>0u&&
                  bundle.layer_utf8_bytes<sizeof(bundle.layer_utf8)&&
                  !strcmp(bundle.layer_utf8,original_layer.utf8);
        if(ok){
            MusaDwgAc1032TextSourceBundle rejected;
            memset(&rejected,0x5a,sizeof(rejected));
            const MusaDwgAc1032TextSourceBundle before=rejected;
            ok=!musa_dwg_ac1032_text_source_bundle(
                   scene,h,44u,&rejected)&&
               !musa_dwg_ac1032_text_source_bundle(
                   scene,UINT64_MAX,1u,&rejected)&&
               !memcmp(&rejected,&before,sizeof(rejected));
        }
    }
    fprintf(stderr,"%s genuine R2018 OT1 H8E Unicode='%s'; "
       "WCS/font/owner/display UNVERIFIED\n",ok?"PASS":"FAIL",
       text.utf8);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return sample(argv[1])?0:1;
}
