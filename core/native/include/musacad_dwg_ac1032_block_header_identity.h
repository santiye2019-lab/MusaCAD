/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_BLOCK_HEADER_IDENTITY_H
#define MUSACAD_DWG_AC1032_BLOCK_HEADER_IDENTITY_H
#include "musacad_dwg_ac1032_object_streams.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_dwg_ac1015_handle_refs.h"
#include <stdint.h>
#include <stddef.h>
/* Strict original OT49 object identity, preceding the as-yet undecoded
 * BLOCK_HEADER base-point. A verified OT49 is NOT a decoded block base. */
typedef struct MusaDwgAc1032BlockHeaderIdentity {
    uint64_t indexed_handle;
    size_t fields_start_bit;
    uint8_t original_record_crc_verified;
    uint8_t embedded_self_handle_verified;
    uint8_t empty_eed_verified;
    uint8_t no_reactors_verified;
    uint8_t xdictionary_missing;
    uint8_t base_point_decoded; /* ALWAYS zero */
} MusaDwgAc1032BlockHeaderIdentity;
static inline int musa_dwg_ac1032_block_header_ot49_identity(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032BlockHeaderIdentity *out){
    if(!raw||!layout||!out||!n||n>64u*1024u*1024u||
       n>SIZE_MAX/8u||!layout->original_record_crc_verified||
       !layout->indexed_handle||layout->raw_object_type!=49u||
       layout->object_type_field_bits!=10u||
       layout->total_payload_bits!=(uint64_t)n*8u||
       layout->data_stream_bits>SIZE_MAX||
       layout->data_stream_bits<=10u||
       layout->handle_stream_start_bit!=layout->data_stream_bits||
       !layout->handle_stream_bits||
       layout->handle_stream_bits!=
          layout->total_payload_bits-layout->data_stream_bits)return 0;
    MusaDwgAc1015Bits bits={0};
    MusaDwgAc1015HandleRef self={0};
    uint32_t kind=0u,type=0u,reactors=0u,xdic=0u;
    uint16_t eed=0u;
    if(!musa_dwg_ac1015_bits_init(
           &bits,raw,n,0u,(size_t)layout->data_stream_bits)||
       !musa_dwg_ac1015_bits_read_u(&bits,2u,&kind)||kind!=0u||
       !musa_dwg_ac1015_bits_read_u(&bits,8u,&type)||type!=49u||
       !musa_dwg_ac1015_read_handle_ref(
           &bits,layout->indexed_handle,&self)||
       self.code!=0u||self.relative||
       self.target!=layout->indexed_handle||
       !musa_dwg_ac1015_bits_read_bs(&bits,&eed)||eed!=0u||
       !musa_dwg_ac1015_bits_read_bl(&bits,&reactors)||reactors!=0u||
       !musa_dwg_ac1015_bits_read_u(&bits,1u,&xdic)||
       bits.bit_pos>=bits.bit_end)return 0;
    MusaDwgAc1032BlockHeaderIdentity v={0};
    v.indexed_handle=layout->indexed_handle;
    v.fields_start_bit=bits.bit_pos;
    v.original_record_crc_verified=1u;
    v.embedded_self_handle_verified=1u;
    v.empty_eed_verified=1u;
    v.no_reactors_verified=1u;
    v.xdictionary_missing=(uint8_t)xdic;
    *out=v;return 1;
}
#endif
