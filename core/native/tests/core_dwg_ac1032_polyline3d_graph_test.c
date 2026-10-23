/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_polyline3d.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef struct Writer {uint8_t data[36];size_t bit;} Writer;
static void write_bits(Writer *w,unsigned n,uint32_t v){
    assert(n>0u&&n<=32u&&w->bit+n<=sizeof(w->data)*8u);
    for(unsigned i=0u;i<n;i++){
       size_t at=w->bit++;
       if((v>>(n-1u-i))&1u)
           w->data[at/8u]|=(uint8_t)(1u<<(7u-at%8u));
    }
}
static void ref(Writer *w,uint8_t code,uint64_t target){
    unsigned count=0u;
    if(target<=255u)count=1u;else if(target<=65535u)count=2u;
    assert(count);
    write_bits(w,4u,code);write_bits(w,4u,count);
    if(count==2u)write_bits(w,8u,(uint32_t)(target>>8u));
    write_bits(w,8u,(uint32_t)target&255u);
}
static void build(Writer *w){
    memset(w,0,sizeof(*w));
    w->bit=101u;
    ref(w,5u,0x10u);
    for(uint64_t h=0x1c3u;h<=0x1c8u;h++)ref(w,3u,h);
    ref(w,3u,0x1c2u);
    assert(w->bit==285u);
}
int main(void){
    Writer w;build(&w);
    MusaDwgAc1032ObjectStreams l={0};
    l.indexed_handle=0x1c1u;l.raw_object_type=16u;
    l.object_type_field_bits=10u;
    l.original_record_crc_verified=1u;
    l.total_payload_bits=288u;l.data_stream_bits=101u;
    l.handle_stream_start_bit=101u;l.handle_stream_bits=187u;
    MusaDwgAc1032Polyline3DHeader h={0};
    h.indexed_handle=0x1c1u;
    h.header_decoded=1u;h.owned_object_count=6u;
    h.geometry_end_bit=100u;
    h.common.common_decoded=1u;
    h.common.prefix.indexed_handle=0x1c1u;
    h.common.prefix.raw_object_type=16u;
    uint64_t handles[6]={0},layer=0u,seqend=0u;
    assert(musa_dwg_ac1032_polyline3d_handle_fields_at(
       w.data,sizeof(w.data),&l,&h,handles,6u,&layer,&seqend));
    assert(layer==0x10u&&seqend==0x1c2u);
    for(unsigned i=0u;i<6u;i++)
       assert(handles[i]==0x1c3u+i);
    uint64_t saved[6];memcpy(saved,handles,sizeof(saved));
    l.data_stream_bits=102u;l.handle_stream_start_bit=102u;
    l.handle_stream_bits=186u;
    assert(!musa_dwg_ac1032_polyline3d_handle_fields_at(
       w.data,sizeof(w.data),&l,&h,handles,6u,&layer,&seqend));
    assert(!memcmp(saved,handles,sizeof(saved)));
    l.data_stream_bits=101u;l.handle_stream_start_bit=101u;
    l.handle_stream_bits=187u;
    assert(!musa_dwg_ac1032_polyline3d_handle_fields_at(
       w.data,sizeof(w.data),&l,&h,handles,5u,&layer,&seqend));
    assert(!memcmp(saved,handles,sizeof(saved)));
    assert(layer==0x10u&&seqend==0x1c2u);
    /* The last three handle padding bits are mandatory zero. */
    w.data[35u]^=1u;
    assert(!musa_dwg_ac1032_polyline3d_handle_fields_at(
       w.data,sizeof(w.data),&l,&h,handles,6u,&layer,&seqend));
    w.data[35u]^=1u;
    assert(!memcmp(saved,handles,sizeof(saved)));
    /* Exactly repeated ownership handles must never be published. */
    w.data[8u]^=1u;
    assert(!musa_dwg_ac1032_polyline3d_handle_fields_at(
       w.data,sizeof(w.data),&l,&h,handles,6u,&layer,&seqend));
    w.data[8u]^=1u;
    assert(!memcmp(saved,handles,sizeof(saved)));
    return 0;
}
