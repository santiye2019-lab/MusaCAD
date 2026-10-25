/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Verified genuine AutoCAD R2018 Text.dwg MTEXT H1C0 -> true original
 * H5 reference -> OT51 Unicode LAYER0, source ENC and raw status evidence.
 * NEVER assumes viewport visibility, font or final rendered BYLAYER. */
#include "musacad_dwg_ac1032_style_evidence.h"
#include "musacad_dwg_ac1032_display_readiness.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static int sample(const char *filename){
    FILE *f=fopen(filename,"rb");
    if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256L||n>64L*1024L*1024L||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *bytes=(uint8_t*)malloc((size_t)n);
    uint8_t *copy=(uint8_t*)malloc((size_t)n);
    if(!bytes||!copy){free(bytes);free(copy);fclose(f);return 0;}
    int ok=fread(bytes,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    memcpy(copy,bytes,(size_t)n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=musa_dwg_ac1032_scene_index_open(
        bytes,(size_t)n,&scene)&&memcmp(copy,bytes,(size_t)n)==0;
    free(bytes);free(copy);
    if(!ok){fprintf(stderr,"FAIL MTEXT original scene CRC/H open\n");return 0;}
    const uint64_t h=UINT64_C(0x1c0);
    MusaDwgAc1032EntityCommon common={0};
    MusaDwgAc1032MTextSource content={0};
    MusaDwgAc1032MTextPlacement place={0};
    MusaDwgAc1032EntityLayerName linked={0};
    MusaAc1032ColorProvenance color={0};
    MusaAc1032PlanarStyleEvidence evidence={0};
    MusaAc1032DisplayReadiness readiness={0};
    ok=musa_dwg_ac1032_scene_index_read_mtext_common(
          scene,h,&common)&&common.common_decoded&&
       common.prefix.indexed_handle==h&&
       common.prefix.raw_object_type==44u&&
       musa_dwg_ac1032_scene_index_read_mtext_source(
          scene,h,&content)&&content.original_record_crc_verified&&
       content.indexed_handle==h&&content.utf8_bytes>0u&&
       strstr(content.utf8,"multiline text")!=NULL&&
       musa_dwg_ac1032_scene_index_read_mtext_placement(
          scene,h,&place)&&place.geometry_fields_decoded&&
       fabs(place.insertion[0]-7.33298)<0.0002&&
       fabs(place.insertion[1]-14.1158)<0.0002&&
       musa_dwg_ac1032_scene_index_entity_layer_name(
          scene,h,&linked)&&
       linked.source_graph_and_name_verified&&
       linked.edge.entity_handle==h&&
       linked.edge.raw_entity_type==44u&&
       linked.edge.original_entity_crc_verified&&
       linked.edge.original_layer_crc_verified&&
       linked.layer.original_record_crc_verified&&
       !strcmp(linked.layer.utf8,"0");
    if(!ok)fprintf(stderr,
       "FAIL genuine MTEXT H1C0 -> strict original Unicode LAYER0 H5 graph\n");
    if(ok)ok=musa_dwg_ac1032_scene_planar_color_provenance(
       scene,h,&color)&&color.entity_handle==h&&
       color.raw_entity_type==44u&&
       color.layer_handle==linked.edge.layer_handle&&
       color.entity_and_layer_crc_verified&&
       !color.rendering_accepted;
    if(!ok)fprintf(stderr,"FAIL original MTEXT ENC color provenance\n");
    if(ok){
       const unsigned categories=(unsigned)color.entity_has_direct_aci+
         (unsigned)color.entity_by_layer+(unsigned)color.entity_by_block+
         (unsigned)color.entity_complex_color_or_transparency;
       ok=categories==1u;
       if(color.entity_has_direct_rgb24)ok=ok&&
          color.entity_source_rgb24<=UINT32_C(0x00ffffff);
    }
    if(ok)ok=musa_dwg_ac1032_scene_planar_style_evidence(
        scene,h,&evidence)&&
       evidence.source_layer_join_verified&&
       !evidence.visibility_resolved&&
       !evidence.display_style_resolved&&
       evidence.color.entity_handle==h&&
       evidence.original_layer_name.indexed_handle==color.layer_handle&&
       evidence.raw_layer_status.indexed_handle==color.layer_handle&&
       !evidence.base_bit_preflight.display_accepted;
    if(ok)ok=musa_dwg_ac1032_scene_display_readiness(
        scene,&readiness)&&readiness.indexed_records>0u&&
       readiness.known_planar_records>=1u&&
       readiness.source_style_joined>=1u&&
       !readiness.display_accepted;
    if(ok){
        const MusaAc1032ColorProvenance saved=color;
        MusaAc1032ColorProvenance legacy={0};
        ok=!musa_dwg_ac1032_scene_line_point_color_provenance(
            scene,h,&legacy)&&
           !musa_dwg_ac1032_scene_planar_color_provenance(
               scene,UINT64_MAX,&color)&&
           !memcmp(&saved,&color,sizeof(color));
    }
    fprintf(stderr,"%s authentic R2018 MTEXT H1C0 LAYER=%llX "
       "ACI=%u RGB=%u BYLAYER=%u BYBLOCK=%u fullStyle=NO\n",
       ok?"PASS":"FAIL",(unsigned long long)color.layer_handle,
       color.entity_has_direct_aci,color.entity_has_direct_rgb24,
       color.entity_by_layer,color.entity_by_block);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0;
    if(argc!=2)return 2;
    return sample(argv[1])?0:1;
}
