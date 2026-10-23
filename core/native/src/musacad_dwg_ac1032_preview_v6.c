/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_preview_v6.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MAX_PREVIEW_RECORDS 1000000u
static int overlaps(const void *a,size_t na,const void *b,size_t nb){
    if(!na||!nb)return 0;
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<na:x-y<nb;
}
static int valid(double x){
    return isfinite(x)&&x<=FLT_MAX&&x>=-FLT_MAX;
}
static int planar3(const double *xyz,double thickness,
                   const double *extrusion){
    return valid(xyz[0])&&valid(xyz[1])&&
       isfinite(xyz[2])&&fabs(xyz[2])<1e-10&&
       isfinite(thickness)&&fabs(thickness)<1e-10&&
       isfinite(extrusion[0])&&isfinite(extrusion[1])&&
       isfinite(extrusion[2])&&
       fabs(extrusion[0])<1e-10&&
       fabs(extrusion[1])<1e-10&&
       fabs(extrusion[2]-1.0)<1e-10;
}
static void bounds(double x,double y,double b[4],int *has){
    if(!*has){b[0]=b[2]=x;b[1]=b[3]=y;*has=1;return;}
    if(x<b[0])b[0]=x;if(x>b[2])b[2]=x;
    if(y<b[1])b[1]=y;if(y>b[3])b[3]=y;
}
/* Probe/emit in the same immutable source-index order. All integrity
 * failures abort BEFORE touching output through a mandatory probe pass.
 * This transport deliberately leaves original color/layer unresolved.
 */
static int process(const MusaDwgAc1032SceneIndex *s,
                   float *dst,size_t cap,
                   MusaAc1032PreviewV6Stats *stats){
    size_t n=musa_dwg_ac1032_scene_index_count(s);
    if(!n||n>MAX_PREVIEW_RECORDS)return 0;
    MusaAc1032PreviewV6Stats v={0};
    v.total_indexed_records=n;
    v.required_float_words=7u;
    v.provisional_aci_style=1u;
    double ext[4]={0};int has=0;
    size_t w=7u;uint64_t prev=0u;
    for(size_t i=0;i<n;i++){
        MusaDwgAc1032HandleEntry e={0};
        uint16_t t=0u;
        if(!musa_dwg_ac1032_scene_index_entry(s,i,&e)||
           !e.handle||e.handle<=prev||
           !musa_dwg_ac1032_scene_index_object_type(s,e.handle,&t))
            return 0;
        prev=e.handle;
        if(t==19u){
            MusaDwgAc1032NativeLine line={0};
            if(musa_dwg_ac1032_scene_index_read_line(s,e.handle,&line)&&
               planar3(line.geometry.start,line.geometry.thickness,
                       line.geometry.extrusion)&&
               planar3(line.geometry.end,line.geometry.thickness,
                       line.geometry.extrusion)){
                if(w>SIZE_MAX-5u)return 0;
                if(dst){
                    if(w+5u>cap)return 0;
                    dst[w]=263.0f;
                    dst[w+1u]=(float)line.geometry.start[0];
                    dst[w+2u]=(float)line.geometry.start[1];
                    dst[w+3u]=(float)line.geometry.end[0];
                    dst[w+4u]=(float)line.geometry.end[1];
                }
                w+=5u;
                bounds(line.geometry.start[0],line.geometry.start[1],ext,&has);
                bounds(line.geometry.end[0],line.geometry.end[1],ext,&has);
                v.rendered_primitives++;
            }else v.omitted_records++;
        }else if(t==27u){
            MusaDwgAc1032NativePoint point={0};
            if(musa_dwg_ac1032_scene_index_read_point(s,e.handle,&point)&&
               planar3(point.point,point.thickness,point.extrusion)){
                if(w>SIZE_MAX-3u)return 0;
                if(dst){
                    if(w+3u>cap)return 0;
                    dst[w]=775.0f;
                    dst[w+1u]=(float)point.point[0];
                    dst[w+2u]=(float)point.point[1];
                }
                w+=3u;
                bounds(point.point[0],point.point[1],ext,&has);
                v.rendered_primitives++;
            }else v.omitted_records++;
        }else v.omitted_records++;
    }
    if(v.rendered_primitives>16777215u||v.required_float_words>SIZE_MAX-w)
        return 0;
    v.required_float_words=w;
    v.partial_geometry=(uint8_t)(v.omitted_records>0u);
    if(dst){
        if(w>cap)return 0;
        dst[0]=6.0f;dst[1]=(float)v.rendered_primitives;
        dst[2]=has?(float)ext[0]:0.0f;
        dst[3]=has?(float)ext[1]:0.0f;
        dst[4]=has?(float)ext[2]:0.0f;
        dst[5]=has?(float)ext[3]:0.0f;
        dst[6]=v.partial_geometry?1.0f:0.0f;
    }
    *stats=v;
    return 1;
}
int musa_dwg_ac1032_preview_v6_line_point(
    const MusaDwgAc1032SceneIndex *s,float *words,size_t capacity,
    MusaAc1032PreviewV6Stats *out){
    if(!s||!out||(!words&&capacity)||
       capacity>SIZE_MAX/sizeof(float)||
       (words&&overlaps(words,capacity*sizeof(float),out,sizeof(*out))))
        return 0;
    MusaAc1032PreviewV6Stats before={0};
    if(!process(s,NULL,0u,&before))return 0;
    if(!words){*out=before;return 1;}
    if(capacity<before.required_float_words)return 0;
    /* Build in scratch; malformed source or OOM cannot mutate the
     * caller's view buffer, even if record count changes unexpectedly. */
    float *scratch=(float*)malloc(before.required_float_words*sizeof(float));
    if(!scratch)return 0;
    MusaAc1032PreviewV6Stats after={0};
    int ok=process(s,scratch,before.required_float_words,&after)&&
           !memcmp(&before,&after,sizeof(before));
    if(ok){
        memcpy(words,scratch,before.required_float_words*sizeof(float));
        *out=after;
    }
    free(scratch);return ok;
}
