/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_insert_circle_numeric_wireframe_v6.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int genuine(const char *path){
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    int ok=fseek(f,0,SEEK_END)==0;
    long n=ok?ftell(f):-1L;
    if(n<256L||n>64L*1024L*1024L||fseek(f,0,SEEK_SET)!=0)ok=0;
    uint8_t *buffer=ok?(uint8_t*)malloc((size_t)n):NULL;
    if(!buffer)ok=0;
    if(ok)ok=fread(buffer,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    MusaDwgAc1032SceneIndex *scene=NULL;
    if(ok)ok=!memcmp(buffer,"AC1032",6u)&&
          musa_dwg_ac1032_scene_index_open(buffer,(size_t)n,&scene);
    free(buffer);
    if(!ok){musa_dwg_ac1032_scene_index_close(scene);return 0;}
    MusaDwgAc1032CircleWireframeEvidence a={0},b={0};
    if(ok)ok=musa_dwg_ac1032_circle_wireframe_v6_diagnostic(
            scene,NULL,0u,&a)&&
          a.source_only_partial&&a.paint_is_diagnostic_not_source&&
          !a.final_color_visibility_and_ownership_accepted&&
          a.insert_candidates>=3u&&
          a.source_numeric_circles>=1u&&
          a.unresolved_h4_h5_circles>=1u&&
          a.emitted_numeric_wireframes==a.source_numeric_circles&&
          a.required_float_words==7u+
              a.emitted_numeric_wireframes*(2u+2u*65u)&&
          a.required_float_words<=MUSACAD_AC1032_CIRCLE_WIRE_WORDS;
    float *words=NULL,*again=NULL;
    if(ok){
        words=(float*)malloc(a.required_float_words*sizeof(float));
        again=(float*)malloc(a.required_float_words*sizeof(float));
        if(!words||!again)ok=0;
    }
    if(ok)ok=musa_dwg_ac1032_circle_wireframe_v6_diagnostic(
            scene,words,a.required_float_words,&b)&&
          !memcmp(&a,&b,sizeof(a))&&
          musa_dwg_ac1032_circle_wireframe_v6_diagnostic(
            scene,again,a.required_float_words,&b)&&
          !memcmp(&a,&b,sizeof(a))&&
          !memcmp(words,again,a.required_float_words*sizeof(float))&&
          words[0]==6.0f&&
          words[1]==(float)a.emitted_numeric_wireframes&&
          words[6]==1.0f;
    size_t at=7u;
    for(size_t k=0u;ok&&k<a.emitted_numeric_wireframes;k++){
        if(at+132u>a.required_float_words){ok=0;break;}
        ok=words[at]==518.0f&&words[at+1u]==65.0f;
        const float first_x=words[at+2u],first_y=words[at+3u];
        const float last_x=words[at+130u],last_y=words[at+131u];
        ok=ok&&first_x==last_x&&first_y==last_y;
        float min_x=INFINITY,max_x=-INFINITY,min_y=INFINITY,max_y=-INFINITY;
        for(size_t j=0;ok&&j<65u;j++){
            const float x=words[at+2u+2u*j];
            const float y=words[at+3u+2u*j];
            if(!isfinite(x)||!isfinite(y)){ok=0;break;}
            if(x<min_x)min_x=x;
            if(x>max_x)max_x=x;
            if(y<min_y)min_y=y;
            if(y>max_y)max_y=y;
        }
        ok=ok&&min_x>=words[2]&&max_x<=words[4]&&
           min_y>=words[3]&&max_y<=words[5];
        at+=132u;
    }
    ok=ok&&at==a.required_float_words;
    if(ok){
        const MusaDwgAc1032CircleWireframeEvidence before=a;
        float sentinel[7]={11.f,12.f,13.f,14.f,15.f,16.f,17.f};
        float short_buffer[7]={11.f,12.f,13.f,14.f,15.f,16.f,17.f};
        ok=!musa_dwg_ac1032_circle_wireframe_v6_diagnostic(
                scene,short_buffer,7u,&a)&&
           !memcmp(short_buffer,sentinel,sizeof(sentinel))&&
           !memcmp(&before,&a,sizeof(a))&&
           !musa_dwg_ac1032_circle_wireframe_v6_diagnostic(
                NULL,words,before.required_float_words,&a)&&
           !memcmp(&before,&a,sizeof(a))&&
           !musa_dwg_ac1032_circle_wireframe_v6_diagnostic(
                scene,words,before.required_float_words,NULL);
    }
    fprintf(stderr,"%s real original R2018 CIRCLE numeric wireframe: %zu ellipses; "
                   "H4/H5 unresolved %zu; magenta NON-SOURCE, final paint ZERO\n",
            ok?"PASS":"FAIL",a.emitted_numeric_wireframes,
            a.unresolved_h4_h5_circles);
    free(words);free(again);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    return genuine(argv[1])?0:1;
}
