/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef MUSACAD_DWG_AC1032_BLOCK_HEADER_IDENTITY_H
#define MUSACAD_DWG_AC1032_BLOCK_HEADER_IDENTITY_H
#include "musacad_dwg_ac1032_object_streams.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_dwg_ac1015_handle_refs.h"
#include <stdint.h>
#include <stddef.h>
/* Reject overlapping buffers: the parser must never overwrite its own
 * authenticated input, metadata or output while decoding. */
static inline int musa_dwg_ac1032_ot49_overlap(
    const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
/* A bounded source fingerprint of the OT49 identity prefix, intended
 * for regression comparison only. It does not attest to base-point data
 * or to complete BLOCK_HEADER grammar. */
typedef struct MusaDwgAc1032BlockHeaderPrefixEvidence {
    uint64_t indexed_handle;
    size_t fields_start_bit;
    uint64_t prefix_fnv1a64;
    uint8_t original_record_crc_verified;
    uint8_t base_point_decoded; /* ALWAYS zero */
} MusaDwgAc1032BlockHeaderPrefixEvidence;
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
       n>SIZE_MAX/8u||
       musa_dwg_ac1032_ot49_overlap(raw,n,layout,sizeof(*layout))||
       musa_dwg_ac1032_ot49_overlap(raw,n,out,sizeof(*out))||
       musa_dwg_ac1032_ot49_overlap(layout,sizeof(*layout),
                                    out,sizeof(*out))||
       !layout->original_record_crc_verified||
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
/* Fingerprint only authenticated prefix bits, masking unused bits of
 * the final byte. This is an observation, never a DWG semantic decoder. */
static inline int musa_dwg_ac1032_block_header_ot49_prefix_evidence(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032BlockHeaderPrefixEvidence *out){
    if(!out||!raw||!layout||
       musa_dwg_ac1032_ot49_overlap(raw,n,out,sizeof(*out))||
       musa_dwg_ac1032_ot49_overlap(layout,sizeof(*layout),
                                    out,sizeof(*out)))return 0;
    MusaDwgAc1032BlockHeaderIdentity id={0};
    if(!musa_dwg_ac1032_block_header_ot49_identity(
           raw,n,layout,&id))return 0;
    const size_t whole=id.fields_start_bit/8u;
    const unsigned rem=(unsigned)(id.fields_start_bit%8u);
    if(whole>n||(whole==n&&rem))return 0;
    uint64_t hash=UINT64_C(14695981039346656037);
    for(size_t i=0;i<whole;i++){
        hash^=(uint64_t)raw[i];
        hash*=UINT64_C(1099511628211);
    }
    if(rem){
        const uint8_t mask=(uint8_t)(0xffu<<(8u-rem));
        hash^=(uint64_t)(raw[whole]&mask);
        hash*=UINT64_C(1099511628211);
    }
    MusaDwgAc1032BlockHeaderPrefixEvidence v={0};
    v.indexed_handle=id.indexed_handle;
    v.fields_start_bit=id.fields_start_bit;
    v.prefix_fnv1a64=hash;
    v.original_record_crc_verified=1u;
    *out=v;return 1;
}

/* Precisely bound the original OT49 field region after the verified
 * identity prefix and before the independent handle stream. This does
 * not interpret field bits or infer a BLOCK_HEADER base point. */
typedef struct MusaDwgAc1032BlockHeaderFieldSpan {
    uint64_t indexed_handle;
    size_t first_bit;
    size_t end_bit; /* exclusive */
    uint8_t original_record_crc_verified;
    uint8_t base_point_decoded; /* ALWAYS zero */
} MusaDwgAc1032BlockHeaderFieldSpan;
static inline int musa_dwg_ac1032_block_header_ot49_field_span(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032BlockHeaderFieldSpan *out){
    if(!out||!raw||!layout||
       musa_dwg_ac1032_ot49_overlap(raw,n,out,sizeof(*out))||
       musa_dwg_ac1032_ot49_overlap(layout,sizeof(*layout),
                                    out,sizeof(*out)))return 0;
    MusaDwgAc1032BlockHeaderIdentity id={0};
    if(!musa_dwg_ac1032_block_header_ot49_identity(
           raw,n,layout,&id)||
       layout->data_stream_bits>SIZE_MAX||
       id.fields_start_bit>=(size_t)layout->data_stream_bits)return 0;
    MusaDwgAc1032BlockHeaderFieldSpan v={0};
    v.indexed_handle=id.indexed_handle;
    v.first_bit=id.fields_start_bit;
    v.end_bit=(size_t)layout->data_stream_bits;
    v.original_record_crc_verified=1u;
    *out=v;return 1;
}
/* Bounded raw bit copier; DWG bit order is MSB-first within each byte.
 * This primitive does NOT authenticate source bytes: only the OT49
 * wrapper below may use it for verified field windows. */
static inline int musa_dwg_ac1032_ot49_copy_msb_bits(
    const uint8_t *raw,size_t n,size_t start_bit,size_t bit_count,
    uint8_t *out,size_t out_bytes){
    if(!raw||!out||n>SIZE_MAX/8u||!bit_count||
       bit_count>SIZE_MAX-7u||
       out_bytes<(bit_count+7u)/8u||
       start_bit>n*8u||bit_count>n*8u-start_bit||
       musa_dwg_ac1032_ot49_overlap(raw,n,out,out_bytes))return 0;
    const size_t bytes=(bit_count+7u)/8u;
    for(size_t i=0;i<bytes;i++)out[i]=0u;
    for(size_t i=0;i<bit_count;i++){
        const size_t src=start_bit+i;
        const uint8_t bit=(uint8_t)((raw[src/8u]>>(7u-(src%8u)))&1u);
        out[i/8u]|=(uint8_t)(bit<<(7u-(i%8u)));
    }
    return 1;
}
/* Copy a caller-selected bit window from the authenticated OT49 field
 * region. This is raw evidence, NOT interpretation of base coordinates.
 * Output is unchanged on any invalid request. Bits are packed MSB-first, matching the DWG bit cursor. */
static inline int musa_dwg_ac1032_block_header_ot49_copy_field_bits(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *layout,
    size_t start_bit,size_t bit_count,
    uint8_t *out,size_t out_bytes){
    if(!raw||!layout||!out||!bit_count||
       bit_count>SIZE_MAX-7u||
       out_bytes<(bit_count+7u)/8u||
       musa_dwg_ac1032_ot49_overlap(raw,n,out,out_bytes)||
       musa_dwg_ac1032_ot49_overlap(layout,sizeof(*layout),
                                    out,out_bytes))return 0;
    MusaDwgAc1032BlockHeaderFieldSpan span={0};
    if(!musa_dwg_ac1032_block_header_ot49_field_span(
           raw,n,layout,&span)||
       start_bit<span.first_bit||start_bit>span.end_bit||
       bit_count>span.end_bit-start_bit)return 0;
    return musa_dwg_ac1032_ot49_copy_msb_bits(
        raw,n,start_bit,bit_count,out,out_bytes);
}
/* Original R2018 BLOCK_HEADER (OT49) base coordinates, decoded from
 * published ODA on-disk field order: 64 B, xrefindex+1 BS, Xdep B,
 * anonymous/hasattrs/isxref/overlaid B, local non-XREF owned BL,\n * base 3BD. XREF/dependent profiles are explicitly unsupported.
 * A separate string stream holds names; it must not be consumed as DATA.
 * Source H/CRC is mandatory. Raw has-ATTDEF bit is not yet a verified\n * semantic attribute inventory. This is not child expansion or paint. */
typedef struct MusaDwgAc1032BlockHeaderBase {
    uint64_t indexed_handle;
    size_t base_end_bit;
    double base_xyz[3];
    uint32_t owned_object_count;
    uint16_t xref_index_plus_one;
    uint8_t flag64,xref_dependent,anonymous,raw_has_attdefs_bit;
    uint8_t is_xref,xref_overlaid,xref_loaded;
    uint8_t original_record_crc_verified;
    uint8_t base_point_decoded;
    uint8_t block_children_and_display_accepted; /* ALWAYS ZERO */
} MusaDwgAc1032BlockHeaderBase;
static inline int musa_dwg_ac1032_block_header_ot49_base_at(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032BlockHeaderBase *out){
    if(!raw||!layout||!out||
       musa_dwg_ac1032_ot49_overlap(raw,n,out,sizeof(*out))||
       musa_dwg_ac1032_ot49_overlap(layout,sizeof(*layout),
                                    out,sizeof(*out)))return 0;
    MusaDwgAc1032BlockHeaderFieldSpan span={0};
    if(!musa_dwg_ac1032_block_header_ot49_field_span(
           raw,n,layout,&span))return 0;
    MusaDwgAc1015Bits bits={0};
    uint32_t f64=0u,dep=0u,anon=0u,atts=0u,isxref=0u;
    uint32_t overlay=0u,loaded=0u,owned=0u;
    uint16_t xrefindex=0u;
    double xyz[3]={0.0,0.0,0.0};
    if(!musa_dwg_ac1015_bits_init(
           &bits,raw,n,span.first_bit,span.end_bit)||
       !musa_dwg_ac1015_bits_read_u(&bits,1u,&f64)||
       !musa_dwg_ac1015_bits_read_bs(&bits,&xrefindex)||
       !musa_dwg_ac1015_bits_read_u(&bits,1u,&dep)||
       !musa_dwg_ac1015_bits_read_u(&bits,1u,&anon)||
       !musa_dwg_ac1015_bits_read_u(&bits,1u,&atts)||
       !musa_dwg_ac1015_bits_read_u(&bits,1u,&isxref)||
       !musa_dwg_ac1015_bits_read_u(&bits,1u,&overlay)||
       /* Only original local/non-XREF block records are proven.
        * XREF-loaded and dependent variants must fail closed. */
       isxref||overlay||dep||xrefindex||
       !musa_dwg_ac1015_bits_read_bl(&bits,&owned)||
       owned>1000000u||
       !musa_dwg_ac1015_bits_read_bd(&bits,&xyz[0])||
       !musa_dwg_ac1015_bits_read_bd(&bits,&xyz[1])||
       !musa_dwg_ac1015_bits_read_bd(&bits,&xyz[2])||
       bits.bit_pos>=span.end_bit)return 0;
    MusaDwgAc1032BlockHeaderBase value={0};
    value.indexed_handle=span.indexed_handle;
    value.base_end_bit=bits.bit_pos;
    value.base_xyz[0]=xyz[0];
    value.base_xyz[1]=xyz[1];
    value.base_xyz[2]=xyz[2];
    value.owned_object_count=owned;
    value.xref_index_plus_one=xrefindex;
    value.flag64=(uint8_t)f64;
    value.xref_dependent=(uint8_t)dep;
    value.anonymous=(uint8_t)anon;
    value.raw_has_attdefs_bit=(uint8_t)atts;
    value.is_xref=(uint8_t)isxref;
    value.xref_overlaid=(uint8_t)overlay;
    value.xref_loaded=(uint8_t)loaded;
    value.original_record_crc_verified=1u;
    value.base_point_decoded=1u;
    *out=value;return 1;
}

#endif
