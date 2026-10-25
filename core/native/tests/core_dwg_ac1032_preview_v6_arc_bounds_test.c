/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exact AC1032 native v6 fit-box tests: C11 math + real original R2018
 * CRC/H-verified ARC/CIRCLE. Never accepts layer visibility semantics. */
#include "musacad_dwg_ac1032_preview_v6.h"
#include <math.h>
#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const double pi=3.1415926535897932384626433832795;
static int near(double a,double b){return isfinite(a)&&fabs(a-b)<1e-8;}
static int check(double x,double y,double r,double s,double sweep,
                 const double *expected){
    double b[4]={0};
    if(!musa_dwg_ac1032_preview_v6_arc_bounds(
         x,y,r,s,sweep,b))return 0;
    if(b[0]>b[2]||b[1]>b[3])return 0;
    for(unsigned i=0;i<4u;i++)
        if(expected&&!near(b[i],expected[i]))return 0;
    for(unsigned j=0;j<=720u;j++){
        const double a=s+sweep*(double)j/720.0;
        const double sx=x+r*cos(a),sy=y+r*sin(a);
        if(sx<b[0]-1e-8||sx>b[2]+1e-8||
           sy<b[1]-1e-8||sy>b[3]+1e-8)return 0;
    }
    return 1;
}
static int pure(void){
    const double q1[]={0,0,2,2};
    const double narrow[]={cos(20*pi/180),sin(10*pi/180),
                            cos(10*pi/180),sin(20*pi/180)};
    const double wrap[]={cos(10*pi/180),-sin(10*pi/180),
                           1,sin(10*pi/180)};
    const double top[]={-cos(40*pi/180),sin(40*pi/180),
                          cos(40*pi/180),1};
    const double circle[]={35,-35,45,-25};
    if(!check(0,0,2,0,pi/2,q1)||
       !check(0,0,1,10*pi/180,10*pi/180,narrow)||
       !check(0,0,1,350*pi/180,20*pi/180,wrap)||
       !check(0,0,1,10*pi/180,-20*pi/180,wrap)||
       !check(0,0,1,40*pi/180,100*pi/180,top)||
       !check(40,-30,5,0,2*pi,circle)||
       !check(40,-30,5,pi/2,-2*pi,circle)||
       !check(0,0,3,2.00242,1.51876-2.00242+2*pi,NULL))
        return 0;
    double b[]={7,8,9,10},saved[4];memcpy(saved,b,sizeof(b));
    if(musa_dwg_ac1032_preview_v6_arc_bounds(0,0,0,0,1,b)||
       musa_dwg_ac1032_preview_v6_arc_bounds(0,0,1,0,0,b)||
       musa_dwg_ac1032_preview_v6_arc_bounds(0,0,1,0,2*pi+0.01,b)||
       musa_dwg_ac1032_preview_v6_arc_bounds(0,0,1,HUGE_VAL,1,b)||
       musa_dwg_ac1032_preview_v6_arc_bounds(HUGE_VAL,0,1,0,1,b)||
       musa_dwg_ac1032_preview_v6_arc_bounds(
           0,0,2.0*(double)FLT_MAX,0,1,b)||
       musa_dwg_ac1032_preview_v6_arc_bounds(0,0,1,0,1,NULL)||
       memcmp(b,saved,sizeof(b)))return 0;
    return 1;
}
static int load(const char *p,uint8_t **out,size_t *n){
    FILE *f=fopen(p,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long m=ftell(f);
    if(m<256||m>64L*1024L*1024L||fseek(f,0,SEEK_SET)){
        fclose(f);return 0;
    }
    uint8_t *buf=(uint8_t*)malloc((size_t)m);
    if(!buf){fclose(f);return 0;}
    int ok=fread(buf,1,(size_t)m,f)==(size_t)m;
    if(fclose(f))ok=0;
    if(!ok){free(buf);return 0;}
    *out=buf;*n=(size_t)m;return 1;
}
static int real(const char *path,uint16_t type){
    uint8_t *raw=NULL,*copy=NULL;size_t n=0u;
    if(!load(path,&raw,&n))return 0;
    copy=(uint8_t*)malloc(n);
    if(!copy){free(raw);return 0;}
    memcpy(copy,raw,n);
    MusaDwgAc1032SceneIndex *scene=NULL;
    int ok=musa_dwg_ac1032_scene_index_open(raw,n,&scene);
    MusaDwgAc1032NativeCurve curve={0};
    if(ok)ok=musa_dwg_ac1032_scene_index_read_curve(
        scene,UINT64_C(0x1bd),&curve)&&
        curve.object_type==type&&curve.geometry_decoded;
    double s=0,sweep=2*pi;
    if(ok&&type==17u){
        s=curve.start_angle;sweep=curve.end_angle-s;
        if(sweep<=0.0)sweep+=2*pi;
    }
    if(ok)ok=check(curve.center[0],curve.center[1],
                   curve.radius,s,sweep,NULL)&&
              memcmp(raw,copy,n)==0;
    MusaAc1032PreviewV6Stats size={0},stats={0};
    if(ok)ok=musa_dwg_ac1032_preview_v6_planar_curves(
          scene,NULL,0u,&size)&&size.rendered_primitives>0u;
    float *v=NULL;
    if(ok){
        v=(float*)malloc(size.required_float_words*sizeof(float));
        ok=v&&musa_dwg_ac1032_preview_v6_planar_curves(
          scene,v,size.required_float_words,&stats)&&
          stats.rendered_primitives==size.rendered_primitives&&
          v[2]<=v[4]&&v[3]<=v[5];
    }
    fprintf(stderr,"%s original AC1032 %s arc bounds and v6\n",
         ok?"PASS":"FAIL",type==17u?"ARC":"CIRCLE");
    free(v);free(copy);free(raw);
    musa_dwg_ac1032_scene_index_close(scene);
    return ok;
}
int main(int argc,char **argv){
    if(!pure())return 1;
    if(argc==1)return 0;
    if(argc!=3)return 2;
    return real(argv[1],17u)&&real(argv[2],18u)?0:1;
}
