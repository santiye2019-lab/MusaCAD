/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_entity_prefix.h"
#include "musacad_dwg_ac1015_values.h"
#include <stddef.h>
#include <stdint.h>
#define MAX_BYTES (64u*1024u*1024u)
static int overlap(const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
int musa_dwg_ac1032_line_prefix_bytes(
    const uint8_t *raw,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LinePrefix *out){
    if(!raw||!layout||!out||!bytes||bytes>MAX_BYTES||
       overlap(raw,bytes,layout,sizeof(*layout))||
       overlap(raw,bytes,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),out,sizeof(*out))||
       !layout->original_record_crc_verified||
       !layout->indexed_handle||layout->raw_object_type!=19u||
       layout->object_type_field_bits!=10u||
       layout->total_payload_bits!=(uint64_t)bytes*8u||
       layout->handle_stream_start_bit!=layout->data_stream_bits||
       !layout->handle_stream_bits||
       layout->handle_stream_bits!=layout->total_payload_bits-layout->data_stream_bits||
       layout->data_stream_bits>SIZE_MAX||
       layout->data_stream_bits<=layout->object_type_field_bits)
        return 0;
    /* Full source bounds: DWG numeric readers cannot consume HANDLES. */
    MusaDwgAc1015Bits c={0};
    if(!musa_dwg_ac1015_bits_init(&c,raw,bytes,
         layout->object_type_field_bits,(size_t)layout->data_stream_bits))
        return 0;
    uint32_t h_header=0u,preview=0u,mode=0u;
    uint16_t edata=0u;
    uint32_t reactors=0u,xdic=0u,ds=0u,nolinks=0u;
    uint64_t self=0u;
    if(!musa_dwg_ac1015_bits_read_u(&c,8u,&h_header)||
       h_header>>4u||!(h_header&15u)||(h_header&15u)>8u)
        return 0;
    for(unsigned i=0u;i<(h_header&15u);i++){
        uint32_t byte=0u;
        if(!musa_dwg_ac1015_bits_read_u(&c,8u,&byte))return 0;
        self=(self<<8u)|byte;
    }
    if(self!=layout->indexed_handle||
       !musa_dwg_ac1015_bits_read_bs(&c,&edata))
        return 0;
    /* Do not publish a partially parsed source if uncommon forms are
     * encountered; next increments need dedicated safe EED/preview. */
    if(edata!=0u||
       !musa_dwg_ac1015_bits_read_u(&c,1u,&preview)||preview!=0u||
       !musa_dwg_ac1015_bits_read_u(&c,2u,&mode)||mode==3u||
       !musa_dwg_ac1015_bits_read_bl(&c,&reactors)||reactors!=0u||
       !musa_dwg_ac1015_bits_read_u(&c,1u,&xdic)||
       !musa_dwg_ac1015_bits_read_u(&c,1u,&ds)||
       !musa_dwg_ac1015_bits_read_u(&c,1u,&nolinks)||
       !nolinks||!xdic||ds)return 0;
    MusaDwgAc1032LinePrefix result={0};
    result.indexed_handle=self;
    result.raw_object_type=19u;
    result.entity_mode=(uint8_t)mode;
    result.reactor_count=reactors;
    result.xdictionary_missing=(uint8_t)xdic;
    result.has_binary_data_store=(uint8_t)ds;
    result.no_prev_next_links=(uint8_t)nolinks;
    result.next_color_bit=c.bit_pos;
    result.prefix_decoded=1u;
    *out=result;
    return 1;
}
