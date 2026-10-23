/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 2018+ ARC/CIRCLE native FIELD grammar from an authentic-prefix position.
 * Synthetic only; original AutoCAD DWG/oracle separately required in CI.
 */
#include "musacad_dwg_ac1032_curve.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
typedef struct Writer {uint8_t b[128];size_t at;} Writer;
static void bits(Writer *w,unsigned n,uint32_t v){
    assert(w&&n>0u&&n<=32u&&w->at+n<=sizeof(w->b)*8u);
    for(unsigned i=0;i<n;i++){
       size_t pos=w->at++;
       if((v>>(n-1u-i))&1u)
           w->b[pos/8u]|=(uint8_t)(1u<<(7u-(pos%8u)));
    }
}
static void rd(Writer *w,double x){
    uint64_t raw=0;memcpy(&raw,&x,8);
    for(unsigned i=0;i<8;i++)bits(w,8u,(uint32_t)(raw>>(8u*i))&255u);
}
static void bd(Writer *w,double x){
    if(x==1.0){bits(w,2u,1u);return;}
    if(x==0.0){bits(w,2u,2u);return;}
    bits(w,2u,0u);rd(w,x);
}
static void make(Writer *w,uint16_t type,double radius){
    memset(w,0,sizeof(*w));
    bits(w,2u,0u);bits(w,8u,type);
    bits(w,8u,2u);bits(w,8u,1u);bits(w,8u,0xcau);
    bits(w,2u,2u);bits(w,1u,0u);bits(w,2u,2u);
    bits(w,2u,2u);bits(w,1u,1u);bits(w,1u,0u);
    assert(w->at==43u);
    bits(w,2u,3u); /* color BYLAYER */
    bits(w,2u,1u); /* scale BD 1.0 */
    bits(w,2u,0u);bits(w,2u,0u);bits(w,2u,0u);
    bits(w,8u,0u);
    bits(w,1u,0u);bits(w,1u,0u);bits(w,1u,0u);
    bits(w,2u,2u);bits(w,8u,29u);
    assert(w->at==74u);
    bd(w,-18.1933);bd(w,6.68652);bd(w,0.0);
    bd(w,radius);
    bits(w,1u,1u); /* default thickness zero */
    bits(w,1u,1u); /* default extrusion +Z */
    if(type==17u){bd(w,2.00242);bd(w,1.51876);}
}
static void verify(uint16_t type){
    Writer w;make(&w,type,8.2931);
    const size_t data_bytes=(w.at+7u)/8u;
    assert(data_bytes+1u<=sizeof(w.b));
    w.b[data_bytes]=0xa6u;
    MusaDwgAc1032ObjectStreams l={0};
    l.indexed_handle=0x1cau;
    l.raw_object_type=type;
    l.object_type_field_bits=10u;
    l.original_record_crc_verified=1u;
    l.total_payload_bits=(data_bytes+1u)*8u;
    l.data_stream_bits=data_bytes*8u;
    l.handle_stream_start_bit=l.data_stream_bits;
    l.handle_stream_bits=8u;
    MusaDwgAc1032EntityCommon common={0};
    assert(musa_dwg_ac1032_entity_common_bytes(w.b,data_bytes+1u,
            &l,type,&common));
    assert(common.geometry_start_bit==74u&&
           common.prefix.raw_object_type==type);
    MusaDwgAc1032NativeCurve got={0};
    assert(musa_dwg_ac1032_curve_fields_at(w.b,data_bytes+1u,
           &l,common.geometry_start_bit,&got));
    assert(got.geometry_decoded&&got.object_type==type);
    assert(fabs(got.center[0]+18.1933)<1e-9);
    assert(fabs(got.center[1]-6.68652)<1e-9);
    assert(got.center[2]==0.0&&fabs(got.radius-8.2931)<1e-9);
    assert(got.thickness==0.0&&got.extrusion[2]==1.0);
    if(type==17u){
       assert(fabs(got.start_angle-2.00242)<1e-9);
       assert(fabs(got.end_angle-1.51876)<1e-9);
    }
    const MusaDwgAc1032NativeCurve save=got;
    l.data_stream_bits=got.geometry_end_bit-1u;
    l.handle_stream_start_bit=l.data_stream_bits;
    l.handle_stream_bits=l.total_payload_bits-l.data_stream_bits;
    assert(!musa_dwg_ac1032_curve_fields_at(
       w.b,data_bytes+1u,&l,common.geometry_start_bit,&got));
    assert(!memcmp(&save,&got,sizeof(got)));
    l.raw_object_type=(uint16_t)(type==17u?19u:17u);
    assert(!musa_dwg_ac1032_entity_common_bytes(
       w.b,data_bytes+1u,&l,type,&common));
}
int main(void){verify(17u);verify(18u);return 0;}
