/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic R2018 3D LINE complete ENTITY common-prefix -> real field
 * integration. This is NOT evidence for any independently made 3D DWG.
 * The genuine original file acceptance is a distinct production-API gate.
 */
#include "musacad_dwg_ac1032_entity_prefix.h"
#include "musacad_dwg_ac1032_line_fields.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef struct Writer { uint8_t bytes[128];size_t pos; } Writer;
static void bits(Writer *w,unsigned width,uint32_t value){
    assert(w&&width>0u&&width<=32u&&
           w->pos+width<=sizeof(w->bytes)*8u);
    for(unsigned i=0u;i<width;i++){
        const unsigned sh=width-1u-i;
        const size_t at=w->pos++;
        if((value>>sh)&1u)
            w->bytes[at/8u]|=(uint8_t)(1u<<(7u-at%8u));
    }
}
static uint64_t raw64(double value){
    uint64_t v=0u;memcpy(&v,&value,sizeof(v));return v;
}
static double float64(uint64_t value){
    double v=0.0;memcpy(&v,&value,sizeof(v));return v;
}
static void rd(Writer *w,double value){
    const uint64_t v=raw64(value);
    for(unsigned i=0u;i<8u;i++)
        bits(w,8u,(uint32_t)((v>>(i*8u))&255u));
}
static size_t build(Writer *w,int defaults, double start[3],
                    double end[3]){
    memset(w,0,sizeof(*w));
    bits(w,2u,0u);bits(w,8u,19u); /* original OT grammar */
    bits(w,8u,2u);bits(w,8u,1u);bits(w,8u,0xcau); /* H self */
    bits(w,2u,2u);bits(w,1u,0u);bits(w,2u,2u);
    bits(w,2u,2u);bits(w,1u,1u);bits(w,1u,0u);
    assert(w->pos==43u); /* no deprecated no-links B */
    bits(w,2u,3u); /* ENC: ACI=256 */
    bits(w,2u,1u); /* BD=1: linetype scale */
    bits(w,2u,0u);bits(w,2u,0u);bits(w,2u,0u);
    bits(w,8u,0u);bits(w,1u,0u);bits(w,1u,0u);
    bits(w,1u,0u);bits(w,2u,2u);bits(w,8u,29u);
    assert(w->pos==74u); /* source grammar-derived XYZ bit */
    start[0]=4.0;start[1]=-6.5;start[2]=2.0;
    end[0]=defaults?start[0]:float64(raw64(start[0])+1u);
    end[1]=defaults?start[1]:float64(raw64(start[1])^UINT64_C(0x0000010203040506));
    end[2]=defaults?start[2]:-3.0;
    bits(w,1u,0u); /* LINE z_zero=false: true 3D */
    rd(w,start[0]);
    if(defaults){bits(w,2u,0u);}
    else{
        bits(w,2u,1u); /* DD01: change least 4 bytes */
        const uint64_t v=raw64(end[0]);
        for(unsigned i=0;i<4u;i++)bits(w,8u,(uint32_t)(v>>(8u*i))&255u);
    }
    rd(w,start[1]);
    if(defaults){bits(w,2u,0u);}
    else{
        bits(w,2u,2u); /* DD10: change least 6 bytes */
        const uint64_t v=raw64(end[1]);
        bits(w,8u,(uint32_t)(v>>32u)&255u);
        bits(w,8u,(uint32_t)(v>>40u)&255u);
        for(unsigned i=0;i<4u;i++)bits(w,8u,(uint32_t)(v>>(8u*i))&255u);
    }
    rd(w,start[2]);
    if(defaults){bits(w,2u,0u);}
    else{bits(w,2u,3u);rd(w,end[2]);}
    bits(w,1u,1u); /* BT zero thickness */
    bits(w,1u,1u); /* BE default +Z normal */
    return w->pos;
}
static void verify(int defaults){
    Writer w;double expected_start[3],expected_end[3];
    const size_t meaningful=build(&w,defaults,expected_start,expected_end);
    const size_t data_bytes=(meaningful+7u)/8u;
    assert(data_bytes+1u<sizeof(w.bytes));
    w.bytes[data_bytes]=0xa6u; /* separate opaque handle stream */
    const size_t payload_bytes=data_bytes+1u;
    const uint8_t source_snapshot[128]={0}; /* overwritten below via copy */
    uint8_t snapshot[128];
    memcpy(snapshot,w.bytes,sizeof(snapshot));
    (void)source_snapshot;
    MusaDwgAc1032ObjectStreams layout={0};
    layout.indexed_handle=0x1cau;
    layout.raw_object_type=19u;
    layout.object_type_field_bits=10u;
    layout.original_record_crc_verified=1u; /* synthetic precondition */
    layout.total_payload_bits=(uint64_t)payload_bytes*8u;
    layout.data_stream_bits=(uint64_t)data_bytes*8u;
    layout.handle_stream_start_bit=layout.data_stream_bits;
    layout.handle_stream_bits=8u;
    MusaDwgAc1032LineCommon common={0};
    assert(musa_dwg_ac1032_line_common_bytes(
        w.bytes,payload_bytes,&layout,&common));
    assert(common.common_decoded&&common.geometry_start_bit==74u);
    MusaDwgAc1032LineFields g={0};
    assert(musa_dwg_ac1032_line_fields_at(
        w.bytes,payload_bytes,&layout,common.geometry_start_bit,&g));
    assert(g.coordinates_decoded&&g.indexed_handle==0x1cau);
    assert(g.geometry_end_bit==(uint64_t)meaningful);
    for(unsigned i=0u;i<3u;i++){
        assert(raw64(g.start[i])==raw64(expected_start[i]));
        assert(raw64(g.end[i])==raw64(expected_end[i]));
    }
    assert(g.thickness==0.0&&g.extrusion[2]==1.0);
    const MusaDwgAc1032LineFields preserved=g;
    layout.data_stream_bits=(uint64_t)meaningful-1u;
    layout.handle_stream_start_bit=layout.data_stream_bits;
    layout.handle_stream_bits=layout.total_payload_bits-layout.data_stream_bits;
    assert(!musa_dwg_ac1032_line_fields_at(
        w.bytes,payload_bytes,&layout,common.geometry_start_bit,&g));
    assert(!memcmp(&preserved,&g,sizeof(g)));
    assert(!memcmp(snapshot,w.bytes,sizeof(snapshot)));
}
int main(void){verify(0);verify(1);return 0;}
