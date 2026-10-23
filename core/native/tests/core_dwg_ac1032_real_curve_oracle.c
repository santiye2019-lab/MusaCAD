/* SPDX-License-Identifier: GPL-3.0-or-later */
/* ORIGINAL 2018/Arc.dwg and 2018/circle.dwg, independently exported
 * AutoCAD property listings: 2018/Arc.txt and 2018/circle.txt.
 * CRC-verified immutable originals, indexed H=0x1BD, no LibreDWG parser.
 */
#include "musacad_dwg_ac1032_curve.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_BYTES (64u*1024u*1024u)
static int load(const char *path,uint8_t **out,size_t *len){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    long n=ftell(f);
    if(n<256||n>(long)MAX_BYTES||fseek(f,0,SEEK_SET)!=0){
        fclose(f);return 0;
    }
    uint8_t *p=(uint8_t*)malloc((size_t)n);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)n,f)==(size_t)n;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(p);return 0;}
    *out=p;*len=(size_t)n;return 1;
}
static int near(double x,double expected){
    return isfinite(x)&&fabs(x-expected)<=0.0001;
}
static int test_original(const char *path,uint16_t type){
    uint8_t *f=NULL;size_t n=0u;
    if(!load(path,&f,&n))return 0;
    MusaDwgAc1032NativeCurve value={0};
    const uint64_t handle=UINT64_C(0x1bd);
    const int read=musa_dwg_ac1032_read_curve_file(f,n,handle,&value);
    fprintf(stderr,"P0 original AC1032 %s: native_ok=%d"
      " type=%u indexed_H=%llX center=(%.9g,%.9g,%.9g)"
      " radius=%.9g angles=(%.9g,%.9g) geometry_start=%zu\n",
      type==17u?"ARC":"CIRCLE",read,value.object_type,
      (unsigned long long)value.indexed_handle,
      value.center[0],value.center[1],value.center[2],
      value.radius,value.start_angle,value.end_angle,
      value.common.geometry_start_bit);
    int good=read&&value.object_type==type&&
        value.indexed_handle==handle&&value.geometry_decoded&&
        value.common.common_decoded&&
        value.common.prefix.raw_object_type==type&&
        near(value.center[2],0.0)&&
        near(value.extrusion[0],0.0)&&
        near(value.extrusion[1],0.0)&&
        near(value.extrusion[2],1.0)&&
        near(value.thickness,0.0);
    if(type==17u){
        good=good&&near(value.center[0],-18.1933)&&
              near(value.center[1],6.68652)&&
              near(value.radius,8.2931)&&
              near(value.start_angle,2.00242)&&
              near(value.end_angle,1.51876);
    }else{
        good=good&&near(value.center[0],23.4493)&&
              near(value.center[1],-1.09239)&&
              near(value.radius,13.5304);
    }
    if(good){
        const MusaDwgAc1032NativeCurve saved=value;
        const uint8_t first=f[0],last=f[n-1u];
        f[0]^=1u;
        good=!musa_dwg_ac1032_read_curve_file(
                 f,n,handle,&value)&&
             memcmp(&value,&saved,sizeof(value))==0;
        f[0]=first;
        if(good){
          f[n-1u]^=1u;
          good=!musa_dwg_ac1032_read_curve_file(
                  f,n,handle,&value)&&
              memcmp(&value,&saved,sizeof(value))==0;
          f[n-1u]=last;
        }
        if(good){
          good=!musa_dwg_ac1032_read_curve_file(
                  f,n,UINT64_MAX,&value)&&
              memcmp(&value,&saved,sizeof(value))==0;
        }
        if(good)good=!musa_dwg_ac1032_read_curve_file(
                       f,n,handle,
                       (MusaDwgAc1032NativeCurve*)(void*)f);
    }
    fprintf(stderr,"%s authentic R2018 %s reference geometry\n",
       good?"PASS":"FAIL",type==17u?"ARC":"CIRCLE");
    free(f);return good;
}
int main(int argc,char **argv){
    if(argc!=3)return 2;
    if(!test_original(argv[1],17u))return 1;
    if(!test_original(argv[2],18u))return 1;
    return 0;
}
