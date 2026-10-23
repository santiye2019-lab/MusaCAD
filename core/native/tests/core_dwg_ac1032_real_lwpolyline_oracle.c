/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original AutoCAD 2018/Polyline.dwg (LWPOLYLINE OT 77, H=0x1BD).
 * Original independent AutoCAD export 2018/Polyline.txt contains OCS
 * first-three vertices. Require full bounded geometry decoding too.
 * Public upstream DWG is only an acceptance fixture, not a CAD decoder.
 */
#include "musacad_dwg_ac1032_lwpolyline.h"
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_streams.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_FILE (64u*1024u*1024u)
#define VERTEX_CAP 128u
static int load(const char *p,uint8_t **f,size_t *bytes){
    FILE *fp=fopen(p,"rb");if(!fp)return 0;
    if(fseek(fp,0,SEEK_END)!=0){fclose(fp);return 0;}
    const long size=ftell(fp);
    if(size<256||size>(long)MAX_FILE||fseek(fp,0,SEEK_SET)!=0){
       fclose(fp);return 0;
    }
    uint8_t *tmp=(uint8_t*)malloc((size_t)size);
    if(!tmp){fclose(fp);return 0;}
    int ok=fread(tmp,1u,(size_t)size,fp)==(size_t)size;
    if(fclose(fp)!=0)ok=0;
    if(!ok){free(tmp);return 0;}
    *f=tmp;*bytes=(size_t)size;return 1;
}
static int near(double a,double b){
    return isfinite(a)&&fabs(a-b)<=0.0001;
}
static void diagnose(const uint8_t *file,size_t n,uint64_t h){
    MusaDwgAc1032ObjectType ot={0};
    MusaDwgAc1032ObjectStreams l={0};
    MusaDwgAc1032PayloadInfo meta={0};
    size_t needed=0u;
    int typeok=musa_dwg_ac1032_object_type_file(file,n,h,&ot);
    int payload=musa_dwg_ac1032_object_payload_file(
        file,n,h,NULL,0u,&needed,&meta);
    int layout=musa_dwg_ac1032_stream_layout_file(file,n,h,&l);
    fprintf(stderr,"Polyline original source: OT_ok=%d type=%u"
       " payload_ok=%d size=%zu layout_ok=%d"
       " DATA=%llu HANDLES=%llu CRC=%u\n",
       typeok,ot.type_code,payload,needed,layout,
       (unsigned long long)l.data_stream_bits,
       (unsigned long long)l.handle_stream_bits,
       (unsigned)meta.original_record_crc_verified);
    if(!payload||!layout||!needed||needed>2048u)return;
    uint8_t *raw=(uint8_t*)malloc(needed);
    if(!raw)return;
    size_t got=0u;MusaDwgAc1032PayloadInfo p={0};
    int copy=musa_dwg_ac1032_object_payload_file(
        file,n,h,raw,needed,&got,&p);
    MusaDwgAc1032EntityCommon common={0};
    int prefix=copy&&musa_dwg_ac1032_entity_common_bytes(
        raw,got,&l,77u,&common);
    MusaDwgAc1032LWVertex vertices[VERTEX_CAP]={0};
    MusaDwgAc1032NativeLWPolyline poly={0};
    int fields=prefix&&musa_dwg_ac1032_lwpolyline_fields_at(
        raw,got,&l,common.geometry_start_bit,
        vertices,VERTEX_CAP,&poly);
    fprintf(stderr,"Polyline original parsing: copy=%d prefix=%d"
       " geometry_bit=%zu fields=%d count=%u flags=%X\n",
       copy,prefix,common.geometry_start_bit,fields,
       poly.vertex_count,poly.flags);
    fprintf(stderr,"Polyline original payload first 64:");
    for(size_t i=0u;i<got&&i<64u;i++)fprintf(stderr," %02X",raw[i]);
    fputc('\n',stderr);
    free(raw);
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    uint8_t *file=NULL;size_t n=0u;
    if(!load(argv[1],&file,&n))return 2;
    const uint64_t h=UINT64_C(0x1bd);
    MusaDwgAc1032LWVertex vertices[VERTEX_CAP]={0};
    MusaDwgAc1032NativeLWPolyline poly={0};
    int ok=musa_dwg_ac1032_read_lwpolyline_file(
        file,n,h,vertices,VERTEX_CAP,&poly);
    if(ok){
       static const double expected[3][2]={
          {9.00535,12.2913},{17.1819,14.7267},{20.7826,8.24476}
       };
       ok=poly.geometry_decoded&&poly.common.common_decoded&&
          poly.common.prefix.raw_object_type==77u&&
          poly.indexed_handle==h&&poly.vertex_count>=3u&&
          poly.vertex_count<=VERTEX_CAP&&poly.closed==0u&&
          near(poly.elevation,0.0)&&
          near(poly.thickness,0.0)&&
          near(poly.extrusion[2],1.0);
       for(unsigned i=0u;ok&&i<3u;i++)
          ok=near(vertices[i].x,expected[i][0])&&
             near(vertices[i].y,expected[i][1]);
    }
    fprintf(stderr,"Original R2018 LWPOLYLINE read=%d"
       " vertex_count=%u flags=%u"
       " xyz_ocs_first=(%.9g,%.9g)"
       " second=(%.9g,%.9g)"
       " third=(%.9g,%.9g)\n",
       ok,poly.vertex_count,poly.flags,
       vertices[0].x,vertices[0].y,
       vertices[1].x,vertices[1].y,
       vertices[2].x,vertices[2].y);
    if(ok){
       const MusaDwgAc1032NativeLWPolyline old=poly;
       const MusaDwgAc1032LWVertex old_vertices[VERTEX_CAP]={
          {vertices[0].x,vertices[0].y,vertices[0].bulge,
            vertices[0].start_width,vertices[0].end_width}
       };
       ok=!musa_dwg_ac1032_read_lwpolyline_file(
          file,n,h,vertices,2u,&poly)&&
          !memcmp(&old,&poly,sizeof(poly))&&
          !memcmp(old_vertices,vertices,sizeof(*vertices));
       const uint8_t first=file[0];
       file[0]^=1u;
       ok=ok&&!musa_dwg_ac1032_read_lwpolyline_file(
          file,n,h,vertices,VERTEX_CAP,&poly)&&
          !memcmp(&old,&poly,sizeof(poly));
       file[0]=first;
       ok=ok&&!musa_dwg_ac1032_read_lwpolyline_file(
          file,n,UINT64_MAX,vertices,VERTEX_CAP,&poly);
    }
    if(!ok)diagnose(file,n,h);
    fprintf(stderr,"%s genuine 2018 LWPOLYLINE verified OCS"
       " coordinates and strict source\n",ok?"PASS":"FAIL");
    free(file);return ok?0:1;
}
