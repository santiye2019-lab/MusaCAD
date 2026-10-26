/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_all_insert_line_v6_diagnostic.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int genuine(const char *path){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    int ok=fseek(f,0,SEEK_END)==0;
    const long length=ok?ftell(f):-1;
    if(length<256L||length>64L*1024L*1024L||
       fseek(f,0,SEEK_SET)!=0)ok=0;
    uint8_t *bytes=NULL;
    if(ok)bytes=(uint8_t*)malloc((size_t)length);
    if(!bytes)ok=0;
    if(ok)ok=fread(bytes,1u,(size_t)length,f)==(size_t)length;
    if(fclose(f)!=0)ok=0;
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(bytes,"AC1032",6u)&&
             musa_dwg_ac1032_scene_index_open(
                 bytes,(size_t)length,&scene);
    free(bytes);
    if(!ok){
        musa_dwg_ac1032_scene_index_close(scene);
        return 0;
    }
    const uint64_t oracle[]={UINT64_C(0x190),
                              UINT64_C(0x1c8),UINT64_C(0x756)};
    size_t verified_oracle=0u;
    for(unsigned i=0u;ok&&i<3u;i++){
        MusaDwgAc1032InsertLineV6Diagnostic d={0};
        ok=musa_dwg_ac1032_insert_line_v6_diagnostic(
            scene,oracle[i],NULL,0u,&d)&&
           d.insert_handle==oracle[i]&&
           d.original_h4_h5_wcs_source_verified&&
           d.packet_partial_by_design&&!d.final_visibility_and_paint_accepted;
        if(ok)verified_oracle++;
    }
    MusaDwgAc1032AllInsertLineV6Diagnostic a={0},repeat={0};
    if(ok)ok=verified_oracle==3u&&
        musa_dwg_ac1032_all_insert_line_v6_diagnostic(
            scene,NULL,0u,&a)&&
        musa_dwg_ac1032_all_insert_line_v6_diagnostic(
            scene,NULL,0u,&repeat)&&
        !memcmp(&a,&repeat,sizeof(a))&&
        a.candidate_insert_records>=3u&&
        a.verified_insert_records>=3u&&
        a.candidate_insert_records==
            a.verified_insert_records+a.unsupported_insert_records&&
        a.verified_source_line_count==
            a.admitted_line_count+a.omitted_verified_line_count&&
        a.required_float_words>=7u&&
        a.required_float_words<=
            MUSACAD_AC1032_ALL_INSERT_LINE_MAX_FLOAT_WORDS&&
        a.source_bounds_present==(a.admitted_line_count>0u)&&
        a.source_packet_is_partial&&!a.final_visibility_and_paint_accepted;
    float *packet=NULL,*again=NULL;
    if(ok){
        packet=(float*)malloc(a.required_float_words*sizeof(float));
        again=(float*)malloc(a.required_float_words*sizeof(float));
        if(!packet||!again)ok=0;
    }
    if(ok)ok=musa_dwg_ac1032_all_insert_line_v6_diagnostic(
                 scene,packet,a.required_float_words,&repeat)&&
              !memcmp(&a,&repeat,sizeof(a))&&
              musa_dwg_ac1032_all_insert_line_v6_diagnostic(
                 scene,again,a.required_float_words,&repeat)&&
              !memcmp(packet,again,a.required_float_words*sizeof(float))&&
              packet[0]==6.0f&&
              packet[1]==(float)a.admitted_line_count&&
              packet[6]==1.0f;
    /* Independently concatenate every original instance's source packet.
     * Same BLOCK_HEADER in distinct INSERTs is NOT deduplicated. */
    size_t cursor=7u,seen=0u,source_count=0u;
    for(size_t i=0u;ok&&i<musa_dwg_ac1032_scene_index_count(scene);i++){
        MusaDwgAc1032HandleEntry e={0};
        uint16_t t=0u;
        ok=musa_dwg_ac1032_scene_index_entry(scene,i,&e)&&
           musa_dwg_ac1032_scene_index_object_type(scene,e.handle,&t);
        if(!ok)break;
        if(t!=7u)continue;
        float one[7u+6u*MUSACAD_AC1032_OT49_CHILD_LIMIT]={0};
        MusaDwgAc1032InsertLineV6Diagnostic d={0};
        if(!musa_dwg_ac1032_insert_line_v6_diagnostic(
            scene,e.handle,one,sizeof(one)/sizeof(one[0]),&d))
            continue;
        const size_t extra=d.required_float_words-7u;
        ok=cursor+extra<=a.required_float_words&&
           !memcmp(packet+cursor,one+7u,extra*sizeof(float));
        cursor+=extra;
        source_count+=d.original_source_line_count;
        seen++;
    }
    if(ok)ok=cursor==a.required_float_words&&
             seen==a.verified_insert_records&&
             source_count==a.verified_source_line_count;
    if(ok){
        const MusaDwgAc1032AllInsertLineV6Diagnostic previous=a;
        const float sentinel[6]={11.f,12.f,13.f,14.f,15.f,16.f};
        float out[6]={11.f,12.f,13.f,14.f,15.f,16.f};
        ok=!musa_dwg_ac1032_all_insert_line_v6_diagnostic(
               scene,out,6u,&a)&&
           !memcmp(out,sentinel,sizeof(out))&&
           !memcmp(&a,&previous,sizeof(a))&&
           !musa_dwg_ac1032_all_insert_line_v6_diagnostic(
               NULL,packet,previous.required_float_words,&a)&&
           !memcmp(&a,&previous,sizeof(a))&&
           !musa_dwg_ac1032_all_insert_line_v6_diagnostic(
               scene,NULL,1u,&a)&&
           !memcmp(&a,&previous,sizeof(a))&&
           !musa_dwg_ac1032_all_insert_line_v6_diagnostic(
               scene,packet,previous.required_float_words,NULL)&&
           !memcmp(&a,&previous,sizeof(a));
    }
    fprintf(stderr,"%s real R2018 all authenticated INSERT LINE source packet: %zu instances, %zu lines admitted, source-only, no final paint\n",
            ok?"PASS":"FAIL",a.verified_insert_records,a.admitted_line_count);
    free(packet);free(again);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    return genuine(argv[1])?0:1;
}
