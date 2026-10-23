/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original AutoCAD 2018 Polyline.dwg, no third-party DWG decoding. */
#include "musacad_dwg_ac1032_preview_v6.h"
#include "musacad_bulge.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VERTEX_CAP 128u
static int near(double a,double b){return isfinite(a)&&fabs(a-b)<0.0001;}
static int load(const char *path,uint8_t **out,size_t *len){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256||n>64L*1024*1024||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *raw=(uint8_t*)malloc((size_t)n);
    if(!raw){fclose(f);return 0;}
    int ok=fread(raw,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(!ok){free(raw);return 0;}
    *out=raw;*len=(size_t)n;return 1;
}
int main(int argc,char **argv){
    if(argc==1){
        MusaAc1032PreviewV6Stats stats={0};
        stats.total_indexed_records=33u;
        return (!musa_dwg_ac1032_preview_v6_planar_polylines(
              NULL,NULL,0u,&stats)&&
              stats.total_indexed_records==33u)?0:1;
    }
    if(argc!=2)return 2;
    uint8_t *raw=NULL,*snapshot=NULL;size_t len=0u;
    if(!load(argv[1],&raw,&len))return 2;
    snapshot=(uint8_t*)malloc(len);
    if(!snapshot){free(raw);return 2;}
    memcpy(snapshot,raw,len);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(raw,len,&scene);
    MusaDwgAc1032NativeLWPolyline poly={0};
    MusaDwgAc1032LWVertex vertex[VERTEX_CAP]={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_lwpolyline(
        scene,UINT64_C(0x1bd),vertex,VERTEX_CAP,&poly)&&
        poly.geometry_decoded&&poly.vertex_count>=3u&&
        poly.vertex_count<=VERTEX_CAP&&
        near(vertex[0].x,9.00535)&&near(vertex[0].y,12.2913)&&
        near(vertex[1].x,17.1819)&&near(vertex[1].y,14.7267)&&
        near(vertex[2].x,20.7826)&&near(vertex[2].y,8.24476)&&
        !memcmp(raw,snapshot,len);
    free(raw);free(snapshot);raw=NULL;
    MusaAc1032PreviewV6Stats expected={0},actual={0};
    if(ok)ok=musa_dwg_ac1032_preview_v6_planar_polylines(
         scene,NULL,0u,&expected)&&
         expected.total_indexed_records==
            musa_dwg_ac1032_scene_index_count(scene)&&
         expected.partial_geometry&&expected.omitted_records>0u&&
         expected.rendered_primitives>0u;
    float *words=NULL,*saved=NULL;
    if(ok){
        words=(float*)malloc(expected.required_float_words*sizeof(float));
        saved=(float*)malloc(expected.required_float_words*sizeof(float));
        if(!words||!saved)ok=0;
    }
    if(ok){
        for(size_t i=0u;i<expected.required_float_words;i++)
            words[i]=-12345.0f;
        memcpy(saved,words,expected.required_float_words*sizeof(float));
        MusaAc1032PreviewV6Stats before=expected;
        ok=!musa_dwg_ac1032_preview_v6_planar_polylines(
               scene,words,expected.required_float_words-1u,&before)&&
           !memcmp(&before,&expected,sizeof(before))&&
           !memcmp(words,saved,expected.required_float_words*sizeof(float));
    }
    if(ok)ok=musa_dwg_ac1032_preview_v6_planar_polylines(
         scene,words,expected.required_float_words,&actual)&&
         !memcmp(&actual,&expected,sizeof(actual))&&
         words[0]==6.0f&&words[6]==1.0f;
    int found=0;
    for(size_t i=7u;ok&&i<actual.required_float_words;){
        if(words[i]==263.0f&&i+5u<=actual.required_float_words)
            i+=5u;
        else if(words[i]==775.0f&&i+3u<=actual.required_float_words)
            i+=3u;
        else if(words[i]==1031.0f&&i+9u<=actual.required_float_words)
            i+=9u;
        else if(words[i]==519.0f&&i+2u<=actual.required_float_words){
            long num=(long)words[i+1u];if(num<0)num=-num;
            if(num<2||num>100000||
               (size_t)num>(actual.required_float_words-i-2u)/2u){
               ok=0;break;
            }
            if((size_t)num==poly.vertex_count&&
               near(words[i+2u],vertex[0].x)&&
               near(words[i+3u],vertex[0].y)&&
               near(words[i+4u],vertex[1].x)&&
               near(words[i+5u],vertex[1].y))
               found++;
            i+=2u+2u*(size_t)num;
        }else ok=0;
    }
    if(ok)ok=found>0;
    fprintf(stderr,"%s original AC1032 LWPOLYLINE"
       " true closed/open straight poly v6 vertices=%u"
       " primitives=%zu omitted=%zu\n",ok?"PASS":"FAIL",
       poly.vertex_count,actual.rendered_primitives,actual.omitted_records);
    free(words);free(saved);musa_dwg_ac1032_scene_index_close(scene);
    return ok?0:1;
}
