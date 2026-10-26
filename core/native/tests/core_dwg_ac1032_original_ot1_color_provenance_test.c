/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent original R2018 sample_2018.dwg H8E OT1 TEXT source
 * ENC and authenticated H5 -> OT51 Unicode LAYER. NOT OT44 MTEXT.
 * Source color evidence never establishes final viewport visibility. */
#include "musacad_dwg_ac1032_color_provenance.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    MusaDwgAc1032TextPlacement placement={0};
    MusaDwgAc1032EntityCommon common={0};
    MusaDwgAc1032TextGraph graph={0};
    MusaDwgAc1032EntityLayerName layer={0};
    MusaAc1032ColorProvenance color={0};
    ok=musa_dwg_ac1032_scene_index_read_text_source(scene,h,&text)&&
       text.original_record_crc_verified&&text.indexed_handle==h&&
       !strcmp(text.utf8,"Jen teksto simpla, cxu ne?")&&
       musa_dwg_ac1032_scene_index_read_text_placement(
           scene,h,&placement)&&
       placement.original_record_crc_verified&&
       placement.numeric_fields_decoded&&
       !placement.wcs_font_and_display_accepted&&
       placement.indexed_handle==h&&
       musa_dwg_ac1032_scene_index_read_text_common(
           scene,h,&common)&&common.common_decoded&&
       common.prefix.indexed_handle==h&&
       common.prefix.raw_object_type==1u&&
       musa_dwg_ac1032_scene_index_text_graph(scene,h,&graph)&&
       graph.graph_edges_verified&&graph.original_entity_crc_verified&&
       graph.original_layer_crc_verified&&
       graph.original_style_crc_verified&&
       graph.layer_object_type==51u&&graph.style_object_type==53u&&
       !graph.font_and_display_accepted&&
       musa_dwg_ac1032_scene_index_entity_layer_name(
           scene,h,&layer)&&layer.source_graph_and_name_verified&&
       layer.edge.entity_handle==h&&
       layer.edge.raw_entity_type==1u&&
       layer.edge.layer_handle==graph.layer_handle&&
       layer.edge.original_entity_crc_verified&&
       layer.edge.original_layer_crc_verified&&
       layer.edge.source_handle_ref_verified&&
       !strcmp(layer.layer.utf8,"Tavolo 1")&&
       musa_dwg_ac1032_scene_planar_color_provenance(
           scene,h,&color)&&
       color.entity_handle==h&&color.raw_entity_type==1u&&
       color.layer_handle==graph.layer_handle&&
       color.entity_and_layer_crc_verified&&
       color.entity_aci==common.color_index&&
       color.source_entity_invisible==(uint8_t)(common.invisibility!=0u)&&
       !color.rendering_accepted;
    if(ok){
        const unsigned classes=(unsigned)color.entity_has_direct_aci+
            (unsigned)color.entity_by_layer+
            (unsigned)color.entity_by_block+
            (unsigned)color.entity_complex_color_or_transparency;
        ok=classes==1u;
        if(color.entity_has_direct_rgb24)
            ok=ok&&color.entity_source_rgb24<=UINT32_C(0x00ffffff);
    }
    if(ok){
        MusaAc1032ColorProvenance bad;
        memset(&bad,0xa5,sizeof(bad));
        const MusaAc1032ColorProvenance before=bad;
        MusaAc1032ColorProvenance legacy={0};
        ok=!musa_dwg_ac1032_scene_line_point_color_provenance(
              scene,h,&legacy)&&
           !musa_dwg_ac1032_scene_planar_color_provenance(
              scene,UINT64_MAX,&bad)&&
           !memcmp(&bad,&before,sizeof(bad));
    }
    fprintf(stderr,"%s genuine R2018 OT1 H8E LAYER=%llX "
        "source ACI=%u directACI=%u RGB=%u BYLAYER=%u "
        "font/owner/viewport/final color UNVERIFIED\\n",
        ok?"PASS":"FAIL",
        (unsigned long long)graph.layer_handle,
        (unsigned)color.entity_aci,
        (unsigned)color.entity_has_direct_aci,
        (unsigned)color.entity_has_direct_rgb24,
        (unsigned)color.entity_by_layer);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0; /* cross-platform compile smoke */
    if(argc!=2)return 2;
    return original(argv[1])?0:1;
}
