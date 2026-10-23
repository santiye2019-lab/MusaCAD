/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independent synthetic authenticated-prefix POSITION field regression.
 * Original unmodified DWG + AutoCAD reference tested separately.
 */
#include "musacad_dwg_ac1032_point.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
typedef struct Writer { uint8_t b[128];size_t at; } Writer;
static void bits(Writer *w,unsigned n,uint32_t v){
   assert(w&&n>0u&&n<=32u&&w->at+n<=sizeof(w->b)*8u);
   for(unsigned i=0u;i<n;i++){
      const size_t pos=w->at++;
      if((v>>(n-1u-i))&1u)w->b[pos/8u]|=(uint8_t)(1u<<(7u-pos%8u));
   }
}
static void rd(Writer *w,double x){
    uint64_t u=0u;memcpy(&u,&x,8u);
    for(unsigned i=0u;i<8u;i++)bits(w,8u,(uint32_t)(u>>(8u*i))&255u);
}
static void bd(Writer *w,double x){
    if(x==0.0){bits(w,2u,2u);return;}
    if(x==1.0){bits(w,2u,1u);return;}
    bits(w,2u,0u);rd(w,x);
}
static void build(Writer *w,int znonzero){
    memset(w,0,sizeof(*w));
    bits(w,2u,0u);bits(w,8u,27u); /* OT 0x1B */
    bits(w,8u,2u);bits(w,8u,1u);bits(w,8u,0xc0u); /* H=0x1C0 */
    bits(w,2u,2u);bits(w,1u,0u);bits(w,2u,2u);
    bits(w,2u,2u);bits(w,1u,1u);bits(w,1u,0u);
    assert(w->at==43u);
    bits(w,2u,3u);bits(w,2u,1u);
    bits(w,2u,0u);bits(w,2u,0u);bits(w,2u,0u);
    bits(w,8u,0u);bits(w,1u,0u);bits(w,1u,0u);
    bits(w,1u,0u);bits(w,2u,2u);bits(w,8u,29u);
    assert(w->at==74u);
    bd(w,27.928);bd(w,8.65308);bd(w,znonzero?-14.5:0.0);
    bits(w,1u,1u); /* thickness default zero */
    bits(w,1u,1u); /* extrusion default */
    bd(w,znonzero?1.0:0.0);
}
static void verify(int znonzero){
    Writer w;build(&w,znonzero);
    const size_t end=w.at, data_bytes=(end+7u)/8u;
    assert(data_bytes+1u<=sizeof(w.b));
    w.b[data_bytes]=0xa6u;
    const size_t n=data_bytes+1u;
    uint8_t original[128];memcpy(original,w.b,sizeof(original));
    MusaDwgAc1032ObjectStreams l={0};
    l.indexed_handle=0x1c0u;
    l.raw_object_type=27u;
    l.object_type_field_bits=10u;
    l.total_payload_bits=(uint64_t)n*8u;
    l.data_stream_bits=(uint64_t)data_bytes*8u;
    l.handle_stream_start_bit=l.data_stream_bits;
    l.handle_stream_bits=8u;
    l.original_record_crc_verified=1u;
    MusaDwgAc1032EntityCommon common={0};
    assert(musa_dwg_ac1032_entity_common_bytes(
       w.b,n,&l,27u,&common));
    assert(common.geometry_start_bit==74u&&
           common.prefix.indexed_handle==0x1c0u);
    MusaDwgAc1032NativePoint point={0};
    assert(musa_dwg_ac1032_point_fields_at(
       w.b,n,&l,common.geometry_start_bit,&point));
    assert(point.geometry_decoded&&point.indexed_handle==0x1c0u);
    assert(point.geometry_end_bit==(uint64_t)end);
    assert(fabs(point.point[0]-27.928)<1e-10);
    assert(fabs(point.point[1]-8.65308)<1e-10);
    assert(point.point[2]==(znonzero?-14.5:0.0));
    assert(point.x_axis_angle==(znonzero?1.0:0.0));
    assert(point.thickness==0.0&&point.extrusion[2]==1.0);
    const MusaDwgAc1032NativePoint saved=point;
    l.data_stream_bits=(uint64_t)end-1u;
    l.handle_stream_start_bit=l.data_stream_bits;
    l.handle_stream_bits=l.total_payload_bits-l.data_stream_bits;
    assert(!musa_dwg_ac1032_point_fields_at(
        w.b,n,&l,common.geometry_start_bit,&point));
    assert(!memcmp(&saved,&point,sizeof(saved)));
    l.data_stream_bits=(uint64_t)data_bytes*8u;
    l.handle_stream_start_bit=l.data_stream_bits;
    l.handle_stream_bits=8u;
    l.indexed_handle=0x1c1u;
    assert(!musa_dwg_ac1032_entity_common_bytes(
        w.b,n,&l,27u,&common));
    assert(!memcmp(original,w.b,sizeof(original)));
}
int main(void){verify(0);verify(1);return 0;}
