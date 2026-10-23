/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Immutable original AutoCAD R2018 POINT, external matching Point.txt:
 * handle 0x1C0, coordinates 27.928 8.65308 0, default normal/thickness.
 * This binary probe links ONLY the MusaCAD-owned shared C11 engine.
 */
#include "musacad_dwg_ac1032_point.h"
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_streams.h"
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
static void stage_diagnostics(const uint8_t *file,size_t n,
                              uint64_t handle){
    MusaDwgAc1032ObjectType ot={0};
    MusaDwgAc1032PayloadInfo info={0};
    MusaDwgAc1032ObjectStreams layout={0};
    size_t wanted=0u;
    int o=musa_dwg_ac1032_object_type_file(file,n,handle,&ot);
    int p=musa_dwg_ac1032_object_payload_file(
          file,n,handle,NULL,0u,&wanted,&info);
    int l=musa_dwg_ac1032_stream_layout_file(file,n,handle,&layout);
    fprintf(stderr,"Point original stages: OT_ok=%d OT=%u"
      " CRC=%u payload_ok=%d wanted=%zu"
      " metaOT=%u indexed=%llX layout_ok=%d"
      " payload_bits=%llu DATA_bits=%llu HANDLES_bits=%llu\n",
      o,ot.type_code,(unsigned)ot.record_crc_verified,p,wanted,
      info.raw_object_type,(unsigned long long)info.indexed_handle,l,
      (unsigned long long)layout.total_payload_bits,
      (unsigned long long)layout.data_stream_bits,
      (unsigned long long)layout.handle_stream_bits);
    if(!p||!l||wanted==0u||wanted>2048u)return;
    uint8_t *raw=(uint8_t*)malloc(wanted);
    if(!raw)return;
    size_t got=0u;
    MusaDwgAc1032PayloadInfo copied={0};
    int copy=musa_dwg_ac1032_object_payload_file(
        file,n,handle,raw,wanted,&got,&copied);
    MusaDwgAc1032EntityCommon common={0};
    int pref=copy&&musa_dwg_ac1032_entity_common_bytes(
           raw,got,&layout,27u,&common);
    MusaDwgAc1032NativePoint point={0};
    int field=pref&&musa_dwg_ac1032_point_fields_at(
           raw,got,&layout,common.geometry_start_bit,&point);
    fprintf(stderr,"Point original parser: copied=%d prefix=%d"
       " geometry_start=%zu field=%d XYZ=(%.9g,%.9g,%.9g)"
       " endbit=%llu\n",copy,pref,common.geometry_start_bit,field,
       point.point[0],point.point[1],point.point[2],
       (unsigned long long)point.geometry_end_bit);
    fprintf(stderr,"Point original payload first 64 byte hex:");
    for(size_t i=0u;i<got&&i<64u;i++)
        fprintf(stderr," %02X",raw[i]);
    fputc('\n',stderr);
    free(raw);
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
    if(!ok)stage_diagnostics(file,n,h);
    fprintf(stderr,"%s authentic original R2018 POINT XYZ"
       " with full source / CRC / handle proof\n",ok?"PASS":"FAIL");
    free(file);
    return ok?0:1;
}
