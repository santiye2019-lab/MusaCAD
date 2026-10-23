/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic R2018 OT16 polyline header and OT11 VERTEX3D 3BD XYZ.
 * A header count does not by itself prove owned child HANDLE references.
 */
#include "musacad_dwg_ac1032_polyline3d.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
typedef struct Writer {uint8_t b[128];size_t at;} Writer;
static void b(Writer *w,unsigned count,uint32_t value){
   assert(w&&count&&count<=32u&&w->at+count<=sizeof(w->b)*8u);
   for(unsigned i=0u;i<count;i++){
      size_t pos=w->at++;
      if((value>>(count-1u-i))&1u)
        w->b[pos/8u]|=(uint8_t)(1u<<(7u-pos%8u));
   }
}
static void rd(Writer *w,double d){
   uint64_t u=0u;memcpy(&u,&d,8u);
   for(unsigned i=0u;i<8u;i++)
       b(w,8u,(uint32_t)(u>>(8u*i))&255u);
}
static void bd(Writer *w,double d){
   if(d==0.0){b(w,2u,2u);return;}
   if(d==1.0){b(w,2u,1u);return;}
   b(w,2u,0u);rd(w,d);
}
static void make(Writer *w,uint16_t ot,uint64_t handle){
   memset(w,0,sizeof(*w));
   assert(handle<=0xffffu&&handle>0xffu);
   b(w,2u,0u);b(w,8u,ot);
   b(w,8u,2u);b(w,8u,(uint32_t)handle>>8u);
   b(w,8u,(uint32_t)handle&255u);
   b(w,2u,2u);b(w,1u,0u);b(w,2u,2u);
   b(w,2u,2u);b(w,1u,1u);b(w,1u,0u);
   assert(w->at==43u);
   b(w,2u,3u);b(w,2u,1u);
   b(w,2u,0u);b(w,2u,0u);b(w,2u,0u);
   b(w,8u,0u);b(w,1u,0u);b(w,1u,0u);b(w,1u,0u);
   b(w,2u,2u);b(w,8u,29u);
   assert(w->at==74u);
}
static void layout(MusaDwgAc1032ObjectStreams *l,
                   size_t n,uint64_t h,uint16_t ot){
   memset(l,0,sizeof(*l));
   l->indexed_handle=h;l->raw_object_type=ot;
   l->object_type_field_bits=10u;
   l->original_record_crc_verified=1u;
   l->total_payload_bits=n*8u;
   l->data_stream_bits=(n-1u)*8u;
   l->handle_stream_start_bit=l->data_stream_bits;
   l->handle_stream_bits=8u;
}
static void header(void){
   Writer w;make(&w,16u,0x1c1u);
   b(&w,8u,0u);b(&w,8u,1u);
   b(&w,2u,1u);b(&w,8u,5u);
   size_t end=w.at,n=(end+7u)/8u+1u;
   MusaDwgAc1032ObjectStreams l;layout(&l,n,0x1c1u,16u);
   MusaDwgAc1032EntityCommon common={0};
   assert(musa_dwg_ac1032_entity_common_bytes(
     w.b,n,&l,16u,&common));
   assert(common.geometry_start_bit==74u);
   MusaDwgAc1032Polyline3DHeader got={0};
   assert(musa_dwg_ac1032_polyline3d_header_fields_at(
      w.b,n,&l,common.geometry_start_bit,&got));
   assert(got.indexed_handle==0x1c1u&&got.owned_object_count==5u);
   assert(got.closed==1u&&got.spline_flags==0u);
   assert(got.geometry_end_bit==end);
   const MusaDwgAc1032Polyline3DHeader saved=got;
   l.data_stream_bits=end-1u;
   l.handle_stream_start_bit=l.data_stream_bits;
   l.handle_stream_bits=l.total_payload_bits-l.data_stream_bits;
   assert(!musa_dwg_ac1032_polyline3d_header_fields_at(
      w.b,n,&l,common.geometry_start_bit,&got));
   assert(!memcmp(&saved,&got,sizeof(got)));
}
static void vertex(void){
   Writer w;make(&w,11u,0x1c2u);
   b(&w,8u,0x20u);
   bd(&w,21.4409);bd(&w,14.6498);bd(&w,-4.5);
   size_t end=w.at,n=(end+7u)/8u+1u;
   MusaDwgAc1032ObjectStreams l;layout(&l,n,0x1c2u,11u);
   MusaDwgAc1032EntityCommon common={0};
   assert(musa_dwg_ac1032_entity_common_bytes(
     w.b,n,&l,11u,&common));
   assert(common.geometry_start_bit==74u);
   MusaDwgAc1032Vertex3D got={0};
   assert(musa_dwg_ac1032_vertex3d_fields_at(
      w.b,n,&l,common.geometry_start_bit,&got));
   assert(got.coordinates_decoded&&got.indexed_handle==0x1c2u);
   assert(got.flags==0x20u);
   assert(fabs(got.point[0]-21.4409)<1e-12);
   assert(fabs(got.point[1]-14.6498)<1e-12);
   assert(got.point[2]==-4.5);
   const MusaDwgAc1032Vertex3D saved=got;
   l.data_stream_bits=end-1u;
   l.handle_stream_start_bit=l.data_stream_bits;
   l.handle_stream_bits=l.total_payload_bits-l.data_stream_bits;
   assert(!musa_dwg_ac1032_vertex3d_fields_at(
      w.b,n,&l,common.geometry_start_bit,&got));
   assert(!memcmp(&saved,&got,sizeof(got)));
   l.raw_object_type=16u;
   assert(!musa_dwg_ac1032_entity_common_bytes(
      w.b,n,&l,11u,&common));
}
int main(void){header();vertex();return 0;}
