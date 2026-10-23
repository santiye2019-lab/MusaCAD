/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original AutoCAD R2018 PolyLine3D.dwg H=0x1C1: independently
 * authenticated ordered parent/children H/CRC -> 2D v6 POLY only if
 * every original XYZ vertex is genuinely planar, no lossy flattening. */
#include "musacad_dwg_ac1032_preview_v6.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int near(double a,double b){return isfinite(a)&&fabs(a-b)<1e-4;}
static int load(const char *name,uint8_t **out,size_t *len){
    FILE *f=fopen(name,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    const long n=ftell(f);
    if(n<256||n>64L*1024*1024||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *buf=(uint8_t*)malloc((size_t)n);
    if(!buf){fclose(f);return 0;}
    const int ok=fread(buf,1u,(size_t)n,f)==(size_t)n&&
                 fclose(f)==0;
    if(!ok){free(buf);return 0;}
    *out=buf;*len=(size_t)n;return 1;
}
int main(int argc,char **argv){
    if(argc==1){
        MusaAc1032PreviewV6Stats unchanged={0};
        unchanged.total_indexed_records=102u;
        return (!musa_dwg_ac1032_preview_v6_planar_polylines(
             NULL,NULL,0u,&unchanged)&&
             unchanged.total_indexed_records==102u)?0:1;
    }
    if(argc!=2)return 2;
    uint8_t *raw=NULL,*backup=NULL;size_t n=0;
    if(!load(argv[1],&raw,&n))return 2;
    backup=(uint8_t*)malloc(n);
    if(!backup){free(raw);return 2;}
    memcpy(backup,raw,n);
    MusaDwgAc1032SceneIndex *index=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(raw,n,&index);
    MusaDwgAc1032Polyline3DOrderedVertex children[128]={0};
    MusaDwgAc1032Polyline3DOrdered original={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_polyline3d_vertices(
             index,UINT64_C(0x1c1),children,128u,&original)&&
             original.hard_owner_links_verified&&
             original.reverse_vertex_owners_verified&&
             original.seqend_type_verified&&
             original.header.spline_flags==0u&&
             original.vertex_count>=2u&&original.vertex_count<=128u&&
             near(children[0].point[0],21.4409)&&
             near(children[0].point[1],14.6498)&&
             near(children[0].point[2],0.0)&&
             near(children[1].point[0],20.8962)&&
             near(children[1].point[1],5.65378)&&
             near(children[1].point[2],0.0)&&
             !memcmp(raw,backup,n);
    for(size_t i=0;i<(size_t)original.vertex_count&&ok;i++)
        if(children[i].point[2]!=0.0)ok=0;
    free(raw);free(backup);
    raw=NULL;
    MusaAc1032PreviewV6Stats size={0},got={0};
    if(ok)ok=musa_dwg_ac1032_preview_v6_planar_polylines(
       index,NULL,0u,&size)&&
       size.required_float_words>=7u&&
       size.rendered_primitives>0u&&
       size.omitted_records>0u&&size.partial_geometry;
    float *words=NULL,*previous=NULL;
    if(ok){
        words=(float*)malloc(size.required_float_words*sizeof(float));
        previous=(float*)malloc(size.required_float_words*sizeof(float));
        if(!words||!previous)ok=0;
    }
    if(ok){
        for(size_t i=0;i<size.required_float_words;i++)words[i]=-813.0f;
        memcpy(previous,words,size.required_float_words*sizeof(float));
        MusaAc1032PreviewV6Stats untouched={0};
        untouched.total_indexed_records=25u;
        ok=!musa_dwg_ac1032_preview_v6_planar_polylines(
              index,words,size.required_float_words-1u,&untouched)&&
           untouched.total_indexed_records==25u&&
           !memcmp(words,previous,size.required_float_words*sizeof(float));
    }
    if(ok)ok=musa_dwg_ac1032_preview_v6_planar_polylines(
        index,words,size.required_float_words,&got)&&
        !memcmp(&size,&got,sizeof(got))&&
        words[0]==6.0f&&words[6]==1.0f;
    size_t found=0;
    for(size_t at=7u;ok&&at<got.required_float_words;){
        const int type=(int)words[at++];
        if(type==263&&at+4u<=got.required_float_words)at+=4u;
        else if(type==775&&at+2u<=got.required_float_words)at+=2u;
        else if(type==1031&&at+8u<=got.required_float_words)at+=8u;
        else if(type==519&&at<got.required_float_words){
            const int signed_count=(int)words[at++];
            const size_t count=(size_t)abs(signed_count);
            if(count<2u||count>100000u||
               count>(got.required_float_words-at)/2u){ok=0;break;}
            if(count==original.vertex_count&&
               near(words[at],children[0].point[0])&&
               near(words[at+1u],children[0].point[1])&&
               near(words[at+2u],children[1].point[0])&&
               near(words[at+3u],children[1].point[1])&&
               (signed_count<0)==(original.header.closed!=0u))
                found++;
            at+=count*2u;
        }else ok=0;
    }
    ok=ok&&found>0u;
    fprintf(stderr,"%s original source R2018 PolyLine3D to bounded "
        "planar v6 POLY, matched=%zu source_owned_vertices=%u "
        "partial=%u\n",ok?"PASS":"FAIL",found,
        original.vertex_count,(unsigned)size.partial_geometry);
    free(words);free(previous);
    musa_dwg_ac1032_scene_index_close(index);
    return ok?0:1;
}
