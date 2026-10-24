/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_layer.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_dwg_ac1015_handle_refs.h"
#include <stdint.h>
#include <stddef.h>
#define LIMIT (64u*1024u*1024u)
static int overlap(const void *a,size_t al,const void *b,size_t bl){
    if(!al||!bl)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<al:x-y<bl;
}
int musa_dwg_ac1032_layer_ot51_identity_at(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *l,
    MusaDwgAc1032LayerIdentity *out){
    if(!raw||!l||!out||!n||n>LIMIT||n>SIZE_MAX/8u||
       overlap(raw,n,l,sizeof(*l))||
       overlap(raw,n,out,sizeof(*out))||
       overlap(l,sizeof(*l),out,sizeof(*out))||
       !l->original_record_crc_verified||!l->indexed_handle||
       l->raw_object_type!=51u||l->object_type_field_bits!=10u||
       l->total_payload_bits!=(uint64_t)n*8u||
       l->data_stream_bits>SIZE_MAX||
       l->data_stream_bits<=l->object_type_field_bits||
       l->handle_stream_start_bit!=l->data_stream_bits||
       !l->handle_stream_bits||
       l->handle_stream_bits!=l->total_payload_bits-l->data_stream_bits)
        return 0;
    MusaDwgAc1015Bits b={0};
    uint32_t kind=0u,type=0u,xdic=0u;
    uint16_t eed=0u;
    uint32_t reactors=0u;
    MusaDwgAc1015HandleRef self={0};
    if(!musa_dwg_ac1015_bits_init(&b,raw,n,0u,
                (size_t)l->data_stream_bits)||
       !musa_dwg_ac1015_bits_read_u(&b,2u,&kind)||kind!=0u||
       !musa_dwg_ac1015_bits_read_u(&b,8u,&type)||type!=51u||
       !musa_dwg_ac1015_read_handle_ref(
             &b,l->indexed_handle,&self)||
       self.code!=0u||self.relative||self.target!=l->indexed_handle||
       !musa_dwg_ac1015_bits_read_bs(&b,&eed)||eed!=0u||
       !musa_dwg_ac1015_bits_read_bl(&b,&reactors)||reactors!=0u||
       !musa_dwg_ac1015_bits_read_u(&b,1u,&xdic)||xdic!=1u||
       b.bit_pos>=b.bit_end)
        return 0;
    MusaDwgAc1032LayerIdentity value={0};
    value.indexed_handle=l->indexed_handle;
    value.data_fields_start_bit=b.bit_pos;
    value.object_type=51u;
    value.original_record_crc_verified=1u;
    value.embedded_self_handle_verified=1u;
    value.empty_eed_verified=1u;
    value.no_reactors_verified=1u;
    value.xdictionary_missing=1u;
    *out=value;
    return 1;
}
