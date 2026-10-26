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
int musa_dwg_ac1032_entity_prefix_bytes(
    const uint8_t *raw,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,uint16_t expected_type,
    MusaDwgAc1032EntityPrefix *out){
    if(!raw||!layout||!out||!bytes||bytes>MAX_BYTES||
       overlap(raw,bytes,layout,sizeof(*layout))||
       overlap(raw,bytes,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),out,sizeof(*out))||
       !layout->original_record_crc_verified||
       !layout->indexed_handle||(expected_type!=1u&&expected_type!=2u&&expected_type!=7u&&expected_type!=17u&&expected_type!=18u&&
        expected_type!=19u&&expected_type!=27u&&
        expected_type!=77u&&expected_type!=35u&&expected_type!=16u&&
        expected_type!=11u&&expected_type!=44u)||
       layout->raw_object_type!=expected_type||
       layout->object_type_field_bits!=10u||
       layout->total_payload_bits!=(uint64_t)bytes*8u||
       layout->handle_stream_start_bit!=layout->data_stream_bits||
       !layout->handle_stream_bits||
       layout->handle_stream_bits!=layout->total_payload_bits-layout->data_stream_bits||
       layout->data_stream_bits>SIZE_MAX||
       layout->data_stream_bits<=layout->object_type_field_bits)
        return 0;
    /* Confirm the 10 source OT bits actually encode the expected type. */
    MusaDwgAc1015Bits ot={0};uint32_t code=0u,kind=0u;
    if(!musa_dwg_ac1015_bits_init(&ot,raw,bytes,0u,10u)||
       !musa_dwg_ac1015_bits_read_u(&ot,2u,&kind)||kind!=0u||
       !musa_dwg_ac1015_bits_read_u(&ot,8u,&code)||
       code!=expected_type)return 0;
    /* Full source bounds: DWG numeric readers cannot consume HANDLES. */
    MusaDwgAc1015Bits c={0};
    if(!musa_dwg_ac1015_bits_init(&c,raw,bytes,
         layout->object_type_field_bits,(size_t)layout->data_stream_bits))
        return 0;
    uint32_t h_header=0u,preview=0u,mode=0u;
    uint16_t edata=0u;
    uint32_t reactors=0u,xdic=0u,ds=0u;
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
       !xdic||ds)return 0;
    MusaDwgAc1032LinePrefix result={0};
    result.indexed_handle=self;
    result.raw_object_type=expected_type;
    result.entity_mode=(uint8_t)mode;
    result.reactor_count=reactors;
    result.xdictionary_missing=(uint8_t)xdic;
    result.has_binary_data_store=(uint8_t)ds;
    /* R2018 does not store the obsolete R2000 previous/next linker bit.
     * Those links are absent by format invariant (R2004+), not a B in DATA.
     * Original AutoCAD LINE 0x1CA verifies next ENC starts at bit 43. */
    result.no_prev_next_links=1u;
    result.next_color_bit=c.bit_pos;
    result.prefix_decoded=1u;
    *out=result;
    return 1;
}

int musa_dwg_ac1032_line_prefix_bytes(
    const uint8_t *raw,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LinePrefix *out){
    return musa_dwg_ac1032_entity_prefix_bytes(
       raw,bytes,layout,19u,out);
}

/* Specification-derived AC1032 ENTITY common tail, after the independently
 * CRC/handle-authenticated OT/H/EED/flags early prefix. No LibreDWG runtime.
 * The common C11 source supports Android, Windows and iOS.
 */
#include <math.h>
int musa_dwg_ac1032_entity_common_bytes(
    const uint8_t *raw,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,uint16_t expected_type,
    MusaDwgAc1032EntityCommon *out){
    if(!raw||!layout||!out||!bytes||bytes>MAX_BYTES||
       overlap(raw,bytes,layout,sizeof(*layout))||
       overlap(raw,bytes,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),out,sizeof(*out)))
        return 0;
    MusaDwgAc1032LinePrefix early={0};
    if(!musa_dwg_ac1032_entity_prefix_bytes(
         raw,bytes,layout,expected_type,&early)||
       !early.prefix_decoded||
       (uint64_t)early.next_color_bit>=layout->data_stream_bits||
       layout->data_stream_bits>SIZE_MAX)return 0;
    MusaDwgAc1015Bits c={0};
    if(!musa_dwg_ac1015_bits_init(&c,raw,bytes,
        early.next_color_bit,(size_t)layout->data_stream_bits))return 0;
    MusaDwgAc1032LineCommon candidate={0};
    candidate.prefix=early;
    if(!musa_dwg_ac1015_bits_read_bs(&c,&candidate.encoded_color))
        return 0;
    const uint16_t flags=(uint16_t)(candidate.encoded_color&0xe000u);
    candidate.color_index=(uint16_t)(candidate.encoded_color&0x1fffu);
    /* Unknown/reserved combinations must not be reinterpreted as
     * legitimate geometry; the color reference is in the handle stream. */
    if((flags&0x4000u)&&!(flags&0x8000u))return 0;
    if(flags&0x8000u){
        if(!musa_dwg_ac1015_bits_read_bl(&c,&candidate.rgb)||
           candidate.rgb>0x00ffffffu)return 0;
        candidate.has_rgb=1u;
        candidate.has_color_reference=(flags&0x4000u)!=0u;
    }else if(candidate.color_index>256u)return 0;
    if(flags&0x2000u){
        if(!musa_dwg_ac1015_bits_read_bl(&c,&candidate.transparency))
            return 0;
        const uint32_t mode=(candidate.transparency>>24u)&0xffu;
        if(mode!=0u&&mode!=1u&&mode!=3u)return 0;
        candidate.has_transparency=1u;
    }
    uint32_t ltype=0u,plot=0u,material=0u;
    uint32_t shadow=0u,full=0u,face=0u,edge=0u,weight=0u;
    uint16_t invisible=0u;
    if(!musa_dwg_ac1015_bits_read_bd(&c,&candidate.linetype_scale)||
       !isfinite(candidate.linetype_scale)||
       candidate.linetype_scale<=0.0||
       !musa_dwg_ac1015_bits_read_u(&c,2u,&ltype)||
       !musa_dwg_ac1015_bits_read_u(&c,2u,&plot)||
       !musa_dwg_ac1015_bits_read_u(&c,2u,&material)||
       !musa_dwg_ac1015_bits_read_u(&c,8u,&shadow)||
       !musa_dwg_ac1015_bits_read_u(&c,1u,&full)||
       !musa_dwg_ac1015_bits_read_u(&c,1u,&face)||
       !musa_dwg_ac1015_bits_read_u(&c,1u,&edge)||
       !musa_dwg_ac1015_bits_read_bs(&c,&invisible)||
       !musa_dwg_ac1015_bits_read_u(&c,8u,&weight)||
       c.bit_pos>=c.bit_end)return 0;
    candidate.linetype_flags=(uint8_t)ltype;
    candidate.plotstyle_flags=(uint8_t)plot;
    candidate.material_flags=(uint8_t)material;
    candidate.shadow_flags=(uint8_t)shadow;
    candidate.visualstyle_flags=(uint8_t)(full|(face<<1u)|(edge<<2u));
    candidate.invisibility=invisible;
    candidate.lineweight=(uint8_t)weight;
    candidate.geometry_start_bit=c.bit_pos;
    candidate.common_decoded=1u;
    *out=candidate;
    return 1;
}

int musa_dwg_ac1032_line_common_bytes(
    const uint8_t *raw,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LineCommon *out){
    return musa_dwg_ac1032_entity_common_bytes(
       raw,bytes,layout,19u,out);
}
