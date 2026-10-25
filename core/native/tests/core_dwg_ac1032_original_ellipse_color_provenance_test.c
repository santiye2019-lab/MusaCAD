/* SPDX-License-Identifier: GPL-3.0-or-later */
/* True original R2018 ELLIPSE H=0x1BE: source-derived ENC + entity H5
 * -> original OT51 LAYER Unicode name and source CMC.
 * Does not certify layer ON/FROZEN or final RGB/BYLAYER display. */
#include "musacad_dwg_ac1032_style_evidence.h"
#include "musacad_dwg_ac1032_display_readiness.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static int load(const char *name,uint8_t **buffer,size_t *size){
    FILE *file=fopen(name,"rb");
    if(!file)return 0;
    int ok=fseek(file,0,SEEK_END)==0;
    const long bytes=ok?ftell(file):0L;
    ok=ok&&bytes>=256L&&bytes<=64L*1024L*1024L&&
       fseek(file,0,SEEK_SET)==0;
    uint8_t *data=ok?(uint8_t*)malloc((size_t)bytes):NULL;
    if(!data)ok=0;
    if(ok)ok=fread(data,1u,(size_t)bytes,file)==(size_t)bytes;
    if(fclose(file)!=0)ok=0;
    if(!ok){free(data);return 0;}
    *buffer=data;*size=(size_t)bytes;return 1;
}
static int real_file(const char *path){
    uint8_t *bytes=NULL,*original=NULL;
    size_t size=0u;
    if(!load(path,&bytes,&size))return 0;
    original=(uint8_t*)malloc(size);
    if(!original){free(bytes);return 0;}
    memcpy(original,bytes,size);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(bytes,size,&scene)&&
           memcmp(bytes,original,size)==0;
    free(bytes);free(original);
    if(!ok)return 0;
    const uint64_t handle=UINT64_C(0x1be);
    MusaDwgAc1032NativeEllipse ellipse={0};
    MusaDwgAc1032EntityLayerName original_layer={0};
    MusaAc1032ColorProvenance color={0};
    MusaAc1032PlanarStyleEvidence style={0};
    MusaAc1032DisplayReadiness readiness={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_ellipse(
                scene,handle,&ellipse)&&ellipse.geometry_decoded&&
           ellipse.indexed_handle==handle&&
           fabs(ellipse.center[0]-11.1433)<0.003&&
           fabs(ellipse.center[1]-9.5374)<0.003;
    if(ok)ok=musa_dwg_ac1032_scene_index_entity_layer_name(
                scene,handle,&original_layer)&&
           original_layer.source_graph_and_name_verified==1u&&
           original_layer.edge.entity_handle==handle&&
           original_layer.edge.raw_entity_type==35u&&
           original_layer.layer.original_record_crc_verified==1u&&
           strcmp(original_layer.layer.utf8,"0")==0;
    if(ok)ok=musa_dwg_ac1032_scene_planar_color_provenance(
                scene,handle,&color)&&
           color.entity_handle==handle&&color.raw_entity_type==35u&&
           color.layer_handle==original_layer.edge.layer_handle&&
           color.entity_and_layer_crc_verified==1u&&
           color.rendering_accepted==0u;
    if(ok){
        unsigned categories=(unsigned)color.entity_has_direct_aci+
          (unsigned)color.entity_by_layer+
          (unsigned)color.entity_by_block+
          (unsigned)color.entity_complex_color_or_transparency;
        ok=categories==1u;
        if(color.entity_has_direct_rgb24)
            ok=ok&&color.entity_complex_color_or_transparency&&
                 color.entity_source_rgb24<=UINT32_C(0xffffff);
    }
    if(ok)ok=musa_dwg_ac1032_scene_planar_style_evidence(
                scene,handle,&style)&&
           style.source_layer_join_verified==1u&&
           style.color.entity_handle==handle&&
           style.original_layer_name.indexed_handle==color.layer_handle&&
           style.raw_layer_status.indexed_handle==color.layer_handle&&
           style.base_bit_preflight.layer_handle==color.layer_handle&&
           !style.visibility_resolved&&!style.display_style_resolved&&
           !style.base_bit_preflight.display_accepted;
    if(ok)ok=musa_dwg_ac1032_scene_display_readiness(
                scene,&readiness)&&
           readiness.indexed_records>0u&&
           readiness.known_planar_records>=1u&&
           readiness.source_style_joined>=1u&&
           !readiness.display_accepted;
    if(ok){
        MusaAc1032ColorProvenance snapshot=color,legacy={0};
        ok=!musa_dwg_ac1032_scene_line_point_color_provenance(
              scene,handle,&legacy)&&
           !musa_dwg_ac1032_scene_planar_color_provenance(
              scene,UINT64_MAX,&color)&&
           memcmp(&snapshot,&color,sizeof(color))==0&&
           !musa_dwg_ac1032_scene_planar_color_provenance(
              scene,handle,NULL);
    }
    fprintf(stderr,"%s source R2018 ELLIPSE H=%llX / LAYER=%llX "
      "directACI=%u BYLAYER=%u BYBLOCK=%u directRGB=%u "
      "final_visibility=UNVERIFIED\n",
      ok?"PASS":"FAIL",(unsigned long long)handle,
      (unsigned long long)color.layer_handle,
      color.entity_has_direct_aci,color.entity_by_layer,
      color.entity_by_block,color.entity_has_direct_rgb24);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0; /* CTest API smoke; CI provides original DWG */
    if(argc!=2)return 2;
    return real_file(argv[1])?0:1;
}
