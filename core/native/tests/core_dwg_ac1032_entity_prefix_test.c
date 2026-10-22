/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_entity_prefix.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
static void bits(uint8_t *p,size_t *pos,unsigned width,uint32_t value){
    for(unsigned k=0u;k<width;k++){
        unsigned shift=width-1u-k;size_t at=(*pos)++;
        if((value>>shift)&1u)p[at/8u]|=(uint8_t)(1u<<(7u-at%8u));
    }
}
int main(void){
    uint8_t payload[8]={0};size_t at=0u;
    bits(payload,&at,2u,0u);bits(payload,&at,8u,19u);
    bits(payload,&at,8u,2u); /* self handle header */
    bits(payload,&at,8u,1u);bits(payload,&at,8u,0xCAu);
    bits(payload,&at,2u,2u); /* EED BS 10 => empty */
    bits(payload,&at,1u,0u); /* preview absent */
    bits(payload,&at,2u,2u); /* MSPACE */
    bits(payload,&at,2u,2u); /* reactor BL 10 => zero */
    bits(payload,&at,1u,1u); /* XDic missing */
    bits(payload,&at,1u,0u); /* no datastore */
    bits(payload,&at,1u,1u); /* no links */
    assert(at==44u);
    MusaDwgAc1032ObjectStreams layout={0};
    layout.indexed_handle=0x1CAu;
    layout.raw_object_type=19u;
    layout.object_type_field_bits=10u;
    layout.total_payload_bits=64u;
    layout.data_stream_bits=56u;
    layout.handle_stream_start_bit=56u;
    layout.handle_stream_bits=8u;
    layout.original_record_crc_verified=1u;
    MusaDwgAc1032LinePrefix out={0};
    assert(musa_dwg_ac1032_line_prefix_bytes(payload,sizeof(payload),
        &layout,&out));
    assert(out.indexed_handle==0x1CAu&&out.next_color_bit==44u);
    assert(out.entity_mode==2u&&out.xdictionary_missing==1u);
    assert(out.prefix_decoded&&out.reactor_count==0u);
    const MusaDwgAc1032LinePrefix saved=out;
    layout.indexed_handle=0x1CBu;
    assert(!musa_dwg_ac1032_line_prefix_bytes(payload,sizeof(payload),
        &layout,&out));
    layout.indexed_handle=0x1CAu;
    payload[4u]^=1u;
    assert(!musa_dwg_ac1032_line_prefix_bytes(payload,sizeof(payload),
        &layout,&out));
    payload[4u]^=1u;
    layout.data_stream_bits=42u;
    layout.handle_stream_start_bit=42u;
    layout.handle_stream_bits=22u;
    assert(!musa_dwg_ac1032_line_prefix_bytes(payload,sizeof(payload),
        &layout,&out));
    layout.data_stream_bits=56u;
    layout.handle_stream_start_bit=56u;
    layout.handle_stream_bits=8u;
    assert(!musa_dwg_ac1032_line_prefix_bytes(payload,sizeof(payload),
        &layout,(MusaDwgAc1032LinePrefix*)(void*)payload));
    assert(!memcmp(&saved,&out,sizeof(saved)));
    return 0;
}
