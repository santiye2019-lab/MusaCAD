/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Source-verified AC1032 original files, single extraction+index then
 * repeated indexed reads. NEVER link/use LibreDWG code; fixture-only.
 * Validate parity against previously certified FIRST-PARTY per-file readers.
 */
#include "musacad_dwg_ac1032_scene_index.h"
#include "musacad_dwg_ac1032_line_file.h"
#include "musacad_dwg_ac1032_curve.h"
#include "musacad_dwg_ac1032_point.h"
#include "musacad_dwg_ac1032_lwpolyline.h"
#include "musacad_dwg_ac1032_polyline3d.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MAX_FILE (64u*1024u*1024u)
#define MAX_V 128u
static int load(const char *path,uint8_t **out,size_t *bytes){
   FILE *f=fopen(path,"rb");if(!f)return 0;
   if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
   long length=ftell(f);
   if(length<256||length>(long)MAX_FILE||fseek(f,0,SEEK_SET)!=0){
       fclose(f);return 0;
   }
   uint8_t *p=(uint8_t*)malloc((size_t)length);
   if(!p){fclose(f);return 0;}
   int ok=fread(p,1u,(size_t)length,f)==(size_t)length;
   if(fclose(f)!=0)ok=0;
   if(!ok){free(p);return 0;}
   *out=p;*bytes=(size_t)length;return 1;
}
static int eq(double a,double b){
  return isfinite(a)&&isfinite(b)&&fabs(a-b)<0.000001;
}
int main(int argc,char **argv){
   if(argc!=4)return 2;
   unsigned type=(unsigned)strtoul(argv[1],NULL,10);
   uint64_t handle=(uint64_t)strtoull(argv[2],NULL,16);
   if(!handle)return 2;
   uint8_t *file=NULL;size_t file_n=0u;
   if(!load(argv[3],&file,&file_n))return 2;
   MusaDwgAc1032SceneIndex *s=NULL;
   int ok=musa_dwg_ac1032_scene_index_open(file,file_n,&s);
   if(!ok){free(file);return 1;}
   const size_t count=musa_dwg_ac1032_scene_index_count(s);
   ok=count>0u&&count<=1000000u;
   size_t lo=0u,hi=count;
   while(ok&&lo<hi){
       size_t mid=lo+(hi-lo)/2u;
       MusaDwgAc1032HandleEntry e={0};
       ok=musa_dwg_ac1032_scene_index_entry(s,mid,&e);
       if(!ok)break;
       if(e.handle<handle)lo=mid+1u;else hi=mid;
   }
   MusaDwgAc1032HandleEntry match={0};
   ok=ok&&lo<count&&
      musa_dwg_ac1032_scene_index_entry(s,lo,&match)&&
      match.handle==handle;
   uint16_t found_type=0u;
   ok=ok&&musa_dwg_ac1032_scene_index_object_type(
       s,handle,&found_type)&&found_type==type;
   MusaDwgAc1032NativeLine line={0};
   MusaDwgAc1032NativeCurve curve={0};
   MusaDwgAc1032NativePoint point={0};
   MusaDwgAc1032NativeLWPolyline poly={0};
   MusaDwgAc1032LWVertex vertices[MAX_V]={0};
   MusaDwgAc1032Vertex3D vertex3d={0};
   /* Whole-file reader provides independent first-party reference.
    * Compare before releasing original bytes, then demonstrate the cached
    * scene owns everything needed for a second read AFTER source free. */
   if(ok&&type==19u){
       MusaDwgAc1032NativeLine before={0};
       ok=musa_dwg_ac1032_read_line_file(
          file,file_n,handle,&before)&&
          musa_dwg_ac1032_scene_index_read_line(s,handle,&line)&&
          eq(before.geometry.start[0],line.geometry.start[0])&&
          eq(before.geometry.start[1],line.geometry.start[1])&&
          eq(before.geometry.start[2],line.geometry.start[2])&&
          eq(before.geometry.end[0],line.geometry.end[0])&&
          eq(before.geometry.end[1],line.geometry.end[1])&&
          eq(before.geometry.end[2],line.geometry.end[2]);
   }else if(ok&&(type==17u||type==18u)){
       MusaDwgAc1032NativeCurve before={0};
       ok=musa_dwg_ac1032_read_curve_file(
          file,file_n,handle,&before)&&
          musa_dwg_ac1032_scene_index_read_curve(s,handle,&curve)&&
          eq(before.center[0],curve.center[0])&&
          eq(before.center[1],curve.center[1])&&
          eq(before.center[2],curve.center[2])&&
          eq(before.radius,curve.radius)&&
          eq(before.start_angle,curve.start_angle)&&
          eq(before.end_angle,curve.end_angle);
   }else if(ok&&type==27u){
       MusaDwgAc1032NativePoint before={0};
       ok=musa_dwg_ac1032_read_point_file(
          file,file_n,handle,&before)&&
          musa_dwg_ac1032_scene_index_read_point(s,handle,&point)&&
          eq(before.point[0],point.point[0])&&
          eq(before.point[1],point.point[1])&&
          eq(before.point[2],point.point[2]);
   }else if(ok&&type==77u){
       MusaDwgAc1032NativeLWPolyline before={0};
       MusaDwgAc1032LWVertex saved[MAX_V]={0};
       ok=musa_dwg_ac1032_read_lwpolyline_file(
          file,file_n,handle,saved,MAX_V,&before)&&
          musa_dwg_ac1032_scene_index_read_lwpolyline(
          s,handle,vertices,MAX_V,&poly)&&
          before.vertex_count==poly.vertex_count;
       for(size_t i=0u;ok&&i<poly.vertex_count;i++)
          ok=eq(saved[i].x,vertices[i].x)&&
             eq(saved[i].y,vertices[i].y)&&
             eq(saved[i].bulge,vertices[i].bulge);
   }else if(ok&&type==11u){
       MusaDwgAc1032Vertex3D before={0};
       ok=musa_dwg_ac1032_read_vertex3d_file(
          file,file_n,handle,&before)&&
          musa_dwg_ac1032_scene_index_read_vertex3d(s,handle,&vertex3d)&&
          eq(before.point[0],vertex3d.point[0])&&
          eq(before.point[1],vertex3d.point[1])&&
          eq(before.point[2],vertex3d.point[2]);
   }else ok=0;
   free(file);file=NULL;
   if(ok&&type==19u){
       MusaDwgAc1032NativeLine again={0};
       ok=musa_dwg_ac1032_scene_index_read_line(s,handle,&again)&&
          eq(again.geometry.start[0],line.geometry.start[0])&&
          eq(again.geometry.end[1],line.geometry.end[1]);
   }else if(ok&&(type==17u||type==18u)){
       MusaDwgAc1032NativeCurve again={0};
       ok=musa_dwg_ac1032_scene_index_read_curve(s,handle,&again)&&
          eq(again.radius,curve.radius);
   }else if(ok&&type==27u){
       MusaDwgAc1032NativePoint again={0};
       ok=musa_dwg_ac1032_scene_index_read_point(s,handle,&again)&&
          eq(again.point[0],point.point[0]);
   }else if(ok&&type==77u){
       MusaDwgAc1032LWVertex again[MAX_V]={0};
       MusaDwgAc1032NativeLWPolyline p2={0};
       ok=musa_dwg_ac1032_scene_index_read_lwpolyline(
           s,handle,again,MAX_V,&p2)&&
          p2.vertex_count==poly.vertex_count&&
          eq(again[0].x,vertices[0].x);
   }else if(ok&&type==11u){
       MusaDwgAc1032Vertex3D again={0};
       ok=musa_dwg_ac1032_scene_index_read_vertex3d(s,handle,&again)&&
          eq(again.point[0],vertex3d.point[0]);
   }
   uint16_t unchanged=777u;
   if(ok)ok=!musa_dwg_ac1032_scene_index_object_type(
       s,UINT64_MAX,&unchanged)&&unchanged==777u;
   fprintf(stderr,"%s original R2018 cached scene: OT=%u H=%llX"
       " indexed_count=%zu; independent per-file parity and source-free"
       " repeat lookup\n",ok?"PASS":"FAIL",type,
       (unsigned long long)handle,count);
   musa_dwg_ac1032_scene_index_close(s);
   return ok?0:1;
}
