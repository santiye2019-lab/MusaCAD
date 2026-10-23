/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original source-certified R2018 ARC/CIRCLE -> first-party v6 curve packet,
 * with independently checked indexed center/radius/angles, source-free cache. */
#include "musacad_dwg_ac1032_preview_v6.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int near(double a,double b){
    return isfinite(a)&&fabs(a-b)<0.0001;
}
static int load(const char *name,uint8_t **out,size_t *len){
    FILE *f=fopen(name,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256||n>64L*1024*1024||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *p=(uint8_t*)malloc((size_t)n);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(!ok){free(p);return 0;}
    *out=p;*len=(size_t)n;return 1;
}
static int verify(const char *path,uint16_t type){
    uint8_t *file=NULL,*snapshot=NULL;
    size_t n=0u;
    if(!load(path,&file,&n))return 0;
    snapshot=(uint8_t*)malloc(n);
    if(!snapshot){free(file);return 0;}
    memcpy(snapshot,file,n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(file,n,&scene);
    MusaDwgAc1032NativeCurve original={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_curve(
        scene,UINT64_C(0x1bd),&original)&&
        original.object_type==type&&
        original.geometry_decoded&&
        !memcmp(file,snapshot,n);
    free(file);free(snapshot);file=NULL;
    MusaAc1032PreviewV6Stats stats={0},out={0};
    if(ok)ok=musa_dwg_ac1032_preview_v6_planar_curves(
         scene,NULL,0u,&stats)&&
         stats.total_indexed_records==
           musa_dwg_ac1032_scene_index_count(scene)&&
         stats.rendered_primitives>0u&&
         stats.omitted_records>0u&&stats.partial_geometry==1u;
    float *words=NULL,*sentinel=NULL;
    if(ok){
        words=(float*)malloc(stats.required_float_words*sizeof(float));
        sentinel=(float*)malloc(stats.required_float_words*sizeof(float));
        if(!words||!sentinel)ok=0;
    }
    if(ok){
        for(size_t i=0u;i<stats.required_float_words;i++)
            words[i]=-123.0f;
        memcpy(sentinel,words,stats.required_float_words*sizeof(float));
        ok=!musa_dwg_ac1032_preview_v6_planar_curves(
           scene,words,stats.required_float_words-1u,&out)&&
           !memcmp(words,sentinel,stats.required_float_words*sizeof(float));
    }
    if(ok)ok=musa_dwg_ac1032_preview_v6_planar_curves(
         scene,words,stats.required_float_words,&out)&&
         !memcmp(&stats,&out,sizeof(stats))&&
         words[0]==6.0f&&words[6]==1.0f;
    int found=0;
    const double tau=6.2831853071795864769;
    const double start=type==17u?original.start_angle:0.0;
    double sweep=type==17u?original.end_angle-start:tau;
    if(sweep<=0.0)sweep+=tau;
    for(size_t i=7u;ok&&i<stats.required_float_words;){
        if(words[i]==263.0f&&i+5u<=stats.required_float_words)
            i+=5u;
        else if(words[i]==775.0f&&i+3u<=stats.required_float_words)
            i+=3u;
        else if(words[i]==1031.0f&&i+9u<=stats.required_float_words){
            if(near(words[i+1u],original.center[0])&&
               near(words[i+2u],original.center[1])&&
               near(words[i+3u],original.radius)&&
               words[i+4u]==0.0f&&words[i+5u]==0.0f&&
               near(words[i+6u],original.radius)&&
               near(words[i+7u],start)&&near(words[i+8u],sweep))
                  found++;
            i+=9u;
        }else ok=0;
    }
    ok=ok&&found>0;
    fprintf(stderr,"%s genuine AC1032 %s v6 CIRCLE/ARC exact curve"
            " mapped=%d partial=%u indexed=%zu\n",
            ok?"PASS":"FAIL",type==17u?"ARC":"CIRCLE",
            found,(unsigned)stats.partial_geometry,stats.total_indexed_records);
    free(words);free(sentinel);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1){
        MusaAc1032PreviewV6Stats stats={0};
        stats.total_indexed_records=77u;
        return !musa_dwg_ac1032_preview_v6_planar_curves(
             NULL,NULL,0u,&stats)&&
             stats.total_indexed_records==77u?0:1;
    }
    if(argc!=3)return 2;
    return verify(argv[1],17u)&&verify(argv[2],18u)?0:1;
}
