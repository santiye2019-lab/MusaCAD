/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Genuine original AutoCAD 2018 files -> source-free MusaCAD C11 renderer
 * v6 word stream; independently compare decoded coordinates by source H. */
#include "musacad_dwg_ac1032_preview_v6.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_DWG (64u*1024u*1024u)
static int near(double x,double y){
   return isfinite(x)&&fabs(x-y)<0.0001;
}
static int load(const char *path,uint8_t **out,size_t *length){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256||n>(long)MAX_DWG||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *p=(uint8_t*)malloc((size_t)n);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f))ok=0;
    if(!ok){free(p);return 0;}
    *out=p;*length=(size_t)n;return 1;
}
static int prove(const char *path,uint64_t handle,int type){
    uint8_t *file=NULL,*copy=NULL;
    size_t n=0u;
    if(!load(path,&file,&n))return 0;
    copy=(uint8_t*)malloc(n);
    if(!copy){free(file);return 0;}
    memcpy(copy,file,n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(file,n,&scene);
    MusaDwgAc1032NativeLine ref_line={0};
    MusaDwgAc1032NativePoint ref_point={0};
    if(ok&&type==1)
        ok=musa_dwg_ac1032_scene_index_read_line(
            scene,handle,&ref_line);
    else if(ok&&type==3)
        ok=musa_dwg_ac1032_scene_index_read_point(
            scene,handle,&ref_point);
    if(ok)ok=!memcmp(file,copy,n);
    free(file);file=NULL;free(copy);copy=NULL;
    MusaAc1032PreviewV6Stats stats={0},rendered={0};
    if(ok)ok=musa_dwg_ac1032_preview_v6_line_point(
         scene,NULL,0u,&stats)&&
         stats.total_indexed_records==
             musa_dwg_ac1032_scene_index_count(scene)&&
         stats.rendered_primitives>0u&&
         stats.omitted_records>0u&&
         stats.required_float_words>=10u&&
         stats.partial_geometry==1u&&
         stats.provisional_aci_style==1u;
    float *words=NULL,*saved=NULL;
    if(ok){
        words=(float*)malloc(stats.required_float_words*sizeof(float));
        saved=(float*)malloc(stats.required_float_words*sizeof(float));
        if(!words||!saved)ok=0;
    }
    if(ok)for(size_t i=0u;i<stats.required_float_words;i++)
       words[i]=-12345.0f;
    if(ok){
       memcpy(saved,words,stats.required_float_words*sizeof(float));
       MusaAc1032PreviewV6Stats untouched=stats;
       ok=!musa_dwg_ac1032_preview_v6_line_point(
                scene,words,stats.required_float_words-1u,&untouched)&&
          !memcmp(words,saved,stats.required_float_words*sizeof(float))&&
          !memcmp(&stats,&untouched,sizeof(stats));
    }
    if(ok)ok=musa_dwg_ac1032_preview_v6_line_point(
       scene,words,stats.required_float_words,&rendered)&&
       !memcmp(&stats,&rendered,sizeof(stats))&&
       words[0]==6.0f&&words[1]==(float)stats.rendered_primitives&&
       words[6]==1.0f;
    int found=0;
    for(size_t i=7u;ok&&i<stats.required_float_words;){
       if(words[i]==263.0f&&i+5u<=stats.required_float_words){
           if(type==1&&near(words[i+1u],ref_line.geometry.start[0])&&
              near(words[i+2u],ref_line.geometry.start[1])&&
              near(words[i+3u],ref_line.geometry.end[0])&&
              near(words[i+4u],ref_line.geometry.end[1]))found++;
           i+=5u;
       }else if(words[i]==775.0f&&i+3u<=stats.required_float_words){
           if(type==3&&near(words[i+1u],ref_point.point[0])&&
              near(words[i+2u],ref_point.point[1]))found++;
           i+=3u;
       }else ok=0;
    }
    if(ok)ok=found>=1;
    if(ok){
       memcpy(saved,words,stats.required_float_words*sizeof(float));
       ok=!musa_dwg_ac1032_preview_v6_line_point(
          scene,words,stats.required_float_words,
          (MusaAc1032PreviewV6Stats*)words)&&
          !memcmp(saved,words,stats.required_float_words*sizeof(float));
    }
    fprintf(stderr,"%s original AC1032 source-free v6 preview"
            " type=%d indexed=%zu emitted=%zu omitted=%zu\n",
            ok?"PASS":"FAIL",type,stats.total_indexed_records,
            stats.rendered_primitives,stats.omitted_records);
    free(words);free(saved);musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc==1){
       MusaAc1032PreviewV6Stats stats={0};
       stats.total_indexed_records=99u;
       int ok=!musa_dwg_ac1032_preview_v6_line_point(
             NULL,NULL,0u,&stats)&&stats.total_indexed_records==99u;
       return ok?0:1;
    }
    if(argc!=3)return 2;
    return prove(argv[1],UINT64_C(0x1ca),1)&&
           prove(argv[2],UINT64_C(0x1c0),3)?0:1;
}
