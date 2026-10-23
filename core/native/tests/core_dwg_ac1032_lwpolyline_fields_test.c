/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic R2018 common ENTITY plus LWPOLYLINE OCS XY and R2000+ DD.
 * Real original 2018/Polyline.dwg is a DIFFERENT hard CI gate.
 */
#include "musacad_dwg_ac1032_lwpolyline.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef struct Writer {uint8_t b[512];size_t at;} Writer;
static void bits(Writer *w,unsigned n,uint32_t v){
    assert(n&&n<=32u&&w->at+n<=sizeof(w->b)*8u);
    for(unsigned i=0u;i<n;i++){
        const size_t pos=w->at++;
        if((v>>(n-i-1u))&1u)w->b[pos/8u]|=(uint8_t)(1u<<(7u-pos%8u));
    }
}
static void rd(Writer *w,double d){
    uint64_t u=0u;memcpy(&u,&d,8u);
    for(unsigned i=0u;i<8u;i++)bits(w,8u,(uint32_t)(u>>(8u*i))&255u);
}
static void make(Writer *w,unsigned flags){
    memset(w,0,sizeof(*w));
    bits(w,2u,0u);bits(w,8u,77u);
    bits(w,8u,2u);bits(w,8u,1u);bits(w,8u,0xbdu);
    bits(w,2u,2u);bits(w,1u,0u);bits(w,2u,2u);
    bits(w,2u,2u);bits(w,1u,1u);bits(w,1u,0u);
    assert(w->at==43u);
    bits(w,2u,3u);bits(w,2u,1u);
    bits(w,2u,0u);bits(w,2u,0u);bits(w,2u,0u);
    bits(w,8u,0u);bits(w,1u,0u);bits(w,1u,0u);bits(w,1u,0u);
    bits(w,2u,2u);bits(w,8u,29u);
    assert(w->at==74u);
    if(flags>255u){
        /* BS opcode 00: original 16-bit little-endian integer.
         * A one-byte stand-in would silently lose closed flag 0x200. */
        bits(w,2u,0u);
        bits(w,8u,flags&255u);bits(w,8u,(flags>>8u)&255u);
    }else{
        bits(w,2u,1u);bits(w,8u,flags);
    }
    if(flags&4u){bits(w,2u,0u);rd(w,1.25);}
    if(flags&8u){bits(w,2u,0u);rd(w,2.5);}
    if(flags&2u){bits(w,2u,2u);}
    if(flags&1u){
        bits(w,2u,2u);bits(w,2u,2u);bits(w,2u,1u);
    }
    bits(w,2u,1u);bits(w,8u,3u); /* BL vertex_count 3 */
    if(flags&16u){bits(w,2u,1u);bits(w,8u,3u);}
    if(flags&32u){bits(w,2u,1u);bits(w,8u,3u);}
    rd(w,9.00535);rd(w,12.2913);
    bits(w,2u,3u);rd(w,17.1819);
    bits(w,2u,3u);rd(w,14.7267);
    bits(w,2u,3u);rd(w,20.7826);
    bits(w,2u,3u);rd(w,8.24476);
    if(flags&16u)
        for(unsigned i=0u;i<3u;i++)bits(w,2u,2u);
    if(flags&32u)
        for(unsigned i=0u;i<6u;i++)bits(w,2u,2u);
}
static void verify(unsigned flags){
    Writer w;make(&w,flags);
    const size_t data=(w.at+7u)/8u;
    assert(data+1u<=sizeof(w.b));
    w.b[data]=0xa6u;
    MusaDwgAc1032ObjectStreams l={0};
    l.indexed_handle=0x1bdu;l.raw_object_type=77u;
    l.object_type_field_bits=10u;
    l.original_record_crc_verified=1u;
    l.total_payload_bits=(data+1u)*8u;
    l.data_stream_bits=data*8u;
    l.handle_stream_start_bit=l.data_stream_bits;
    l.handle_stream_bits=8u;
    MusaDwgAc1032EntityCommon common={0};
    assert(musa_dwg_ac1032_entity_common_bytes(
         w.b,data+1u,&l,77u,&common));
    assert(common.geometry_start_bit==74u);
    MusaDwgAc1032LWVertex v[3]={{0}};
    MusaDwgAc1032NativeLWPolyline poly={0};
    assert(musa_dwg_ac1032_lwpolyline_fields_at(
         w.b,data+1u,&l,common.geometry_start_bit,v,3u,&poly));
    assert(poly.geometry_decoded&&poly.vertex_count==3u);
    assert(v[0].x==9.00535&&v[0].y==12.2913);
    assert(v[1].x==17.1819&&v[1].y==14.7267);
    assert(v[2].x==20.7826&&v[2].y==8.24476);
    assert(poly.closed==((flags&512u)!=0u));
    assert(poly.bulge_count==((flags&16u)!=0u?3u:0u));
    assert(poly.width_count==((flags&32u)!=0u?3u:0u));
    const MusaDwgAc1032NativeLWPolyline save=poly;
    MusaDwgAc1032LWVertex saved_vertices[3];
    memcpy(saved_vertices,v,sizeof(v));
    assert(!musa_dwg_ac1032_lwpolyline_fields_at(
         w.b,data+1u,&l,common.geometry_start_bit,v,2u,&poly));
    assert(!memcmp(&save,&poly,sizeof(poly)));
    assert(!memcmp(saved_vertices,v,sizeof(v)));
    l.data_stream_bits=poly.geometry_end_bit-1u;
    l.handle_stream_start_bit=l.data_stream_bits;
    l.handle_stream_bits=l.total_payload_bits-l.data_stream_bits;
    assert(!musa_dwg_ac1032_lwpolyline_fields_at(
         w.b,data+1u,&l,common.geometry_start_bit,v,3u,&poly));
    assert(!memcmp(saved_vertices,v,sizeof(v)));
    assert(!memcmp(&save,&poly,sizeof(poly)));
}
int main(void){verify(0u);verify(4u|8u|16u|32u|512u);return 0;}
