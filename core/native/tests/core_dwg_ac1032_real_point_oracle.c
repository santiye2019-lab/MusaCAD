/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Immutable original AutoCAD R2018 POINT, external matching Point.txt:
 * handle 0x1C0, coordinates 27.928 8.65308 0, default normal/thickness.
 * This binary probe links ONLY the MusaCAD-owned shared C11 engine.
 */
#include "musacad_dwg_ac1032_point.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_DWG (64u*1024u*1024u)
static int load(const char *path,uint8_t **out,size_t *n){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
    const long length=ftell(f);
    if(length<256||length>(long)MAX_DWG||
       fseek(f,0,SEEK_SET)!=0){fclose(f);return 0;}
    uint8_t *p=(uint8_t*)malloc((size_t)length);
    if(!p){fclose(f);return 0;}
    int ok=fread(p,1u,(size_t)length,f)==(size_t)length;
    if(fclose(f)!=0)ok=0;
    if(!ok){free(p);return 0;}
    *out=p;*n=(size_t)length;return 1;
}
static int near(double x,double y){
    return isfinite(x)&&fabs(x-y)<=0.0001;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    uint8_t *file=NULL;size_t n=0u;
    if(!load(argv[1],&file,&n))return 2;
    const uint64_t h=UINT64_C(0x1c0);
    MusaDwgAc1032NativePoint point={0};
    int ok=musa_dwg_ac1032_read_point_file(file,n,h,&point);
    fprintf(stderr,"Original R2018 POINT read=%d H=%llX"
      " XYZ=(%.9g,%.9g,%.9g) thickness=%.9g normalZ=%.9g"
      " x_angle=%.9g geometry_at=%zu\n",
      ok,(unsigned long long)point.indexed_handle,
      point.point[0],point.point[1],point.point[2],
      point.thickness,point.extrusion[2],point.x_axis_angle,
      point.common.geometry_start_bit);
    ok=ok&&point.geometry_decoded&&point.common.common_decoded&&
       point.indexed_handle==h&&
       point.common.prefix.raw_object_type==27u&&
       point.common.prefix.indexed_handle==h&&
       near(point.point[0],27.928)&&
       near(point.point[1],8.65308)&&
       near(point.point[2],0.0)&&
       near(point.thickness,0.0)&&
       near(point.extrusion[0],0.0)&&
       near(point.extrusion[1],0.0)&&
       near(point.extrusion[2],1.0);
    if(ok){
        const MusaDwgAc1032NativePoint saved=point;
        const uint8_t first=file[0],last=file[n-1u];
        file[0]^=1u;
        ok=!musa_dwg_ac1032_read_point_file(file,n,h,&point)&&
           !memcmp(&point,&saved,sizeof(point));
        file[0]=first;
        if(ok){
           file[n-1u]^=1u;
           ok=!musa_dwg_ac1032_read_point_file(file,n,h,&point)&&
              !memcmp(&point,&saved,sizeof(point));
           file[n-1u]=last;
        }
        if(ok){
           ok=!musa_dwg_ac1032_read_point_file(
               file,n,UINT64_MAX,&point)&&
              !memcmp(&point,&saved,sizeof(point));
        }
        if(ok)ok=!musa_dwg_ac1032_read_point_file(
             file,n,h,(MusaDwgAc1032NativePoint*)(void*)file);
    }
    fprintf(stderr,"%s authentic original R2018 POINT XYZ"
       " with full source / CRC / handle proof\n",ok?"PASS":"FAIL");
    free(file);
    return ok?0:1;
}
