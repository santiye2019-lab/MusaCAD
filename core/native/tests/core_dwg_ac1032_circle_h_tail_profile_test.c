/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real original R2018 OT18 H1BB source H-tail profile.  Independent DXF
 * identifies CIRCLE handle and radius. This is a reproducible BLOCKER
 * diagnostic, not a guessed owner/layer or display authorization. */
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
static int independent_circle(const char *path){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    char code[128],value[1024];
    int in_circle=0,has_handle=0,has_radius=0,found=0;
    while(fgets(code,sizeof(code),f)&&fgets(value,sizeof(value),f)){
        trim(code);trim(value);
        if(!strcmp(code,"0")){
            if(in_circle&&has_handle&&has_radius)found++;
            in_circle=!strcmp(value,"CIRCLE");
            has_handle=0;has_radius=0;
        }else if(in_circle&&!strcmp(code,"5")){
            char *end=NULL;
            unsigned long long h=strtoull(value,&end,16);
            has_handle=end!=value&&*end=='\0'&&h==0x1bbULL;
        }else if(in_circle&&!strcmp(code,"40")){
            char *end=NULL;
            double r=strtod(value,&end);
            has_radius=end!=value&&*end=='\0'&&isfinite(r)&&
                fabs(r-130.0490046078758)<1e-6;
        }
    }
    if(in_circle&&has_handle&&has_radius)found++;
    int ok=ferror(f)==0&&found==1;
    if(fclose(f)!=0)ok=0;
    return ok;
}
static int original(const char *dwg,const char *dxf){
    if(!independent_circle(dxf))return 0;
    FILE *f=fopen(dwg,"rb");
    if(!f)return 0;
    int ok=fseek(f,0,SEEK_END)==0;
    long n=ok?ftell(f):-1L;
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)!=0)ok=0;
    uint8_t *bytes=ok?(uint8_t*)malloc((size_t)n):NULL;
    uint8_t *saved=bytes?(uint8_t*)malloc((size_t)n):NULL;
    if(!bytes||!saved)ok=0;
    if(ok)ok=fread(bytes,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(ok)memcpy(saved,bytes,(size_t)n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(bytes,"AC1032",6u)&&
        musa_dwg_ac1032_scene_index_open(bytes,(size_t)n,&scene)&&
        !memcmp(bytes,saved,(size_t)n);
    free(bytes);free(saved);
    if(!ok){musa_dwg_ac1032_scene_index_close(scene);return 0;}
    uint16_t type=0u;
    MusaDwgAc1032NativeCurve curve={0};
    MusaDwgAc1032CircleHTailProfile profile={0};
    ok=musa_dwg_ac1032_scene_index_object_type(
           scene,UINT64_C(0x1bb),&type)&&type==18u&&
       musa_dwg_ac1032_scene_index_read_curve(
           scene,UINT64_C(0x1bb),&curve)&&curve.geometry_decoded&&
       fabs(curve.radius-130.0490046078758)<1e-6&&
       musa_dwg_ac1032_scene_index_circle_h_tail_profile(
           scene,UINT64_C(0x1bb),&profile)&&
       profile.original_record_crc_verified&&
       profile.indexed_handle==UINT64_C(0x1bb)&&
       profile.raw_object_type==18u&&
       profile.handle_end_bit>profile.handle_start_bit&&
       profile.prefix_bit_count>0u&&profile.prefix_bit_count<=64u&&
       profile.profile_status>=MUSACAD_AC1032_CIRCLE_H_COMMON_UNSUPPORTED&&
       profile.profile_status==
           MUSACAD_AC1032_CIRCLE_H_EXACT_RELATIVE_H12_EDGE&&
       profile.first_ref_code==12u&&
       profile.first_raw_target==UINT64_C(0x1b8)&&
       profile.second_ref_code==5u&&
       profile.second_raw_target==UINT64_C(0x8a)&&
       profile.verified_relative_h12_owner_h5_layer_source&&
       !profile.exact_h4_h5_source_verified&&
       !profile.final_visibility_and_paint_accepted;
    if(ok){
        MusaDwgAc1032EntityBlockOwnerLayerSource edge={0};
        MusaDwgAc1032EntityLayerRef layer={0};
        ok=musa_dwg_ac1032_scene_index_entity_block_owner_layer_source(
               scene,UINT64_C(0x1bb),&edge)&&
           edge.child_handle==UINT64_C(0x1bb)&&
           edge.owner_block_header_handle==UINT64_C(0x1b8)&&
           edge.layer_handle==UINT64_C(0x8a)&&
           edge.original_owner_wire_code==12u&&
           edge.original_relative_h12_owner_verified&&
           edge.exact_h4_owner_and_h5_layer_source_verified&&
           !edge.visible_scene_and_paint_accepted&&
           musa_dwg_ac1032_scene_index_entity_layer_ref(
               scene,UINT64_C(0x1bb),&layer)&&
           layer.layer_handle==UINT64_C(0x8a)&&
           layer.raw_layer_type==51u&&
           layer.original_entity_crc_verified&&
           layer.original_layer_crc_verified;
    }
    if(ok){
        MusaDwgAc1032CircleHTailProfile repeat={0};
        ok=musa_dwg_ac1032_scene_index_circle_h_tail_profile(
             scene,UINT64_C(0x1bb),&repeat)&&
           !memcmp(&profile,&repeat,sizeof(profile));
    }
    if(ok){
        MusaDwgAc1032CircleHTailProfile before=profile;
        ok=!musa_dwg_ac1032_scene_index_circle_h_tail_profile(
              scene,UINT64_C(0x1c8),&profile)&&
           !memcmp(&before,&profile,sizeof(profile))&&
           !musa_dwg_ac1032_scene_index_circle_h_tail_profile(
              scene,UINT64_MAX,&profile)&&
           !memcmp(&before,&profile,sizeof(profile))&&
           !musa_dwg_ac1032_scene_index_circle_h_tail_profile(
              scene,UINT64_C(0x1bb),NULL);
    }
    fprintf(stderr,
      "%s genuine R2018 OT18 H1BB raw H profile status=%u "
      "mode=%u first=(%u,H%llX) second=(%u,H%llX) "
      "bits=%llu residual=%llu exact=%u paint=0\n",
      ok?"PASS":"FAIL",(unsigned)profile.profile_status,
      (unsigned)profile.entity_mode,(unsigned)profile.first_ref_code,
      (unsigned long long)profile.first_raw_target,
      (unsigned)profile.second_ref_code,
      (unsigned long long)profile.second_raw_target,
      (unsigned long long)(profile.handle_end_bit-profile.handle_start_bit),
      (unsigned long long)profile.residual_bits_after_two_refs,
      (unsigned)profile.exact_h4_h5_source_verified);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1)return 0; /* CTest compile/ABI smoke; real oracle in CI */
    if(argc!=3)return 2;
    return original(argv[1],argv[2])?0:1;
}
