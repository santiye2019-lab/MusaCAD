/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine R2018 example_2018.dwg H1BB circle: relative H code C ->
 * original BLOCK_HEADER H1B8, second H5 -> LAYER H8A.
 * Independent companion DXF confirms H1BB CIRCLE and R130.0490046.
 * This is an authenticated source edge, not a visible rendered entity. */
#include "musacad_dwg_ac1032_scene_index.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void trim(char *s){
    size_t n=strlen(s);
    while(n&&(s[n-1]=='\n'||s[n-1]=='\r'||s[n-1]==' '||
              s[n-1]=='\t'))s[--n]='\0';
    size_t i=0u;
    while(s[i]==' '||s[i]=='\t')i++;
    if(i)memmove(s,s+i,strlen(s+i)+1u);
}
static int oracle(const char *path){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    char code[128],value[1024];
    int circle=0,handle=0,radius=0,found=0;
    while(fgets(code,sizeof(code),f)&&fgets(value,sizeof(value),f)){
        trim(code);trim(value);
        if(!strcmp(code,"0")){
            if(circle&&handle&&radius)found++;
            circle=!strcmp(value,"CIRCLE");
            handle=radius=0;
        }else if(circle&&!strcmp(code,"5")){
            char *end=NULL;
            unsigned long long h=strtoull(value,&end,16);
            handle=end!=value&&*end=='\0'&&h==0x1bbULL;
        }else if(circle&&!strcmp(code,"40")){
            char *end=NULL;
            double r=strtod(value,&end);
            radius=end!=value&&*end=='\0'&&isfinite(r)&&
                fabs(r-130.0490046078758)<1e-6;
        }
    }
    if(circle&&handle&&radius)found++;
    int ok=ferror(f)==0&&found==1;
    if(fclose(f)!=0)ok=0;
    return ok;
}
static int genuine(const char *dwg,const char *dxf){
    if(!oracle(dxf))return 0;
    FILE *f=fopen(dwg,"rb");if(!f)return 0;
    int ok=fseek(f,0,SEEK_END)==0;
    long n=ok?ftell(f):-1L;
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)!=0)ok=0;
    uint8_t *src=ok?(uint8_t*)malloc((size_t)n):NULL;
    uint8_t *saved=src?(uint8_t*)malloc((size_t)n):NULL;
    if(!src||!saved)ok=0;
    if(ok)ok=fread(src,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(ok)memcpy(saved,src,(size_t)n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(src,"AC1032",6u)&&
        musa_dwg_ac1032_scene_index_open(src,(size_t)n,&scene)&&
        !memcmp(src,saved,(size_t)n);
    free(src);free(saved);
    if(!ok){musa_dwg_ac1032_scene_index_close(scene);return 0;}
    MusaDwgAc1032CircleHTailProfile raw={0};
    MusaDwgAc1032CircleRelativeOwnerLayerSource edge={0};
    /* CI-only forensic output: preserve strict rejection while identifying
     * whether original target types or the BLOCK_HEADER forward H list
     * are the source of the current authenticated-edge mismatch. */
    {
        uint16_t ot_owner=0u,ot_layer=0u;
        MusaDwgAc1032BlockHeaderTypedChildren children={0};
        const int owner_type_ok=musa_dwg_ac1032_scene_index_object_type(
            scene,UINT64_C(0x1b8),&ot_owner);
        const int layer_type_ok=musa_dwg_ac1032_scene_index_object_type(
            scene,UINT64_C(0x8a),&ot_layer);
        const int children_ok=musa_dwg_ac1032_scene_index_block_header_typed_children(
            scene,UINT64_C(0x1b8),&children);
        unsigned hits=0u;
        if(children_ok){
            for(uint32_t i=0u;i<children.prefix.child_count;i++){
                if(children.prefix.child_handles[i]==UINT64_C(0x1bb))hits++;
            }
        }
        fprintf(stderr,"DIAG CIRCLE H1BB targets owner_type=%d/%u layer_type=%d/%u "
            "children=%d count=%u H1BB_membership=%u canonical=%u typed=%u\\n",
            owner_type_ok,(unsigned)ot_owner,layer_type_ok,(unsigned)ot_layer,
            children_ok,children_ok?children.prefix.child_count:0u,hits,
            children_ok?children.prefix.canonical_h_roles_verified:0u,
            children_ok?children.all_child_record_types_crc_verified:0u);
    }
    {
        MusaDwgAc1032CircleHTailProfile trace={0};
        const int got=musa_dwg_ac1032_scene_index_circle_h_tail_profile(
            scene,UINT64_C(0x1bb),&trace);
        fprintf(stderr,"DIAG Htail got=%d status=%u refs=%u/%u targets=%llu/%llu "
            "bits=%u residual=%llu byte4=0x%02x last3=0x%02x\\n",
            got,(unsigned)trace.profile_status,
            (unsigned)trace.first_ref_code,(unsigned)trace.second_ref_code,
            (unsigned long long)trace.first_raw_target,
            (unsigned long long)trace.second_raw_target,
            (unsigned)trace.prefix_bit_count,
            (unsigned long long)trace.residual_bits_after_two_refs,
            (unsigned)trace.prefix_msb[4],
            (unsigned)(trace.prefix_msb[4]&0xe0u));
    }
    ok=musa_dwg_ac1032_scene_index_circle_h_tail_profile(
           scene,UINT64_C(0x1bb),&raw)&&
       raw.profile_status==MUSACAD_AC1032_CIRCLE_H_FIRST_NOT_H4&&
       raw.first_ref_code==12u&&raw.second_ref_code==5u&&
       raw.first_raw_target==UINT64_C(0x1b8)&&
       raw.second_raw_target==UINT64_C(0x8a)&&
       raw.residual_bits_after_two_refs==3u&&
       musa_dwg_ac1032_scene_index_circle_relative_owner_layer_source(
           scene,UINT64_C(0x1bb),&edge)&&
       edge.circle_handle==UINT64_C(0x1bb)&&
       edge.owner_block_header_handle==UINT64_C(0x1b8)&&
       edge.layer_handle==UINT64_C(0x8a)&&
       edge.raw_circle_object_type==18u&&
       edge.first_source_handle_code==12u&&
       edge.second_source_handle_code==5u&&
       edge.original_circle_and_targets_crc_verified&&
       edge.original_forward_block_child_membership_verified&&
       edge.exact_relative_owner_and_layer_source_verified&&
       !edge.model_space_viewport_and_paint_accepted;
    if(ok){
        MusaDwgAc1032CircleRelativeOwnerLayerSource repeat={0};
        ok=musa_dwg_ac1032_scene_index_circle_relative_owner_layer_source(
              scene,UINT64_C(0x1bb),&repeat)&&
           !memcmp(&repeat,&edge,sizeof(edge));
    }
    if(ok){
        MusaDwgAc1032CircleRelativeOwnerLayerSource before=edge;
        ok=!musa_dwg_ac1032_scene_index_circle_relative_owner_layer_source(
               scene,UINT64_C(0x1c8),&edge)&&
           !memcmp(&before,&edge,sizeof(edge))&&
           !musa_dwg_ac1032_scene_index_circle_relative_owner_layer_source(
               scene,UINT64_C(0x1bd),&edge)&&
           !memcmp(&before,&edge,sizeof(edge))&&
           !musa_dwg_ac1032_scene_index_circle_relative_owner_layer_source(
               scene,UINT64_MAX,&edge)&&
           !memcmp(&before,&edge,sizeof(edge))&&
           !musa_dwg_ac1032_scene_index_circle_relative_owner_layer_source(
               scene,UINT64_C(0x1bb),NULL);
    }
    fprintf(stderr,"%s original R2018 CIRCLE H1BB owner H1B8 (code C), "
      "layer H8A (code 5), forward CRC/OT membership; paint=0\n",
      ok?"PASS":"FAIL");
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0; /* ABI smoke; real source oracle in CI */
    if(argc!=3)return 2;
    return genuine(argv[1],argv[2])?0:1;
}
