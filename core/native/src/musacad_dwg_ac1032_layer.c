/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_layer.h"
#include "musacad_dwg_cmc.h"
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

/* Original R2007+ TU strings live in a separate object STRING stream,
 * not inline among the data fields. Its final one-bit presence marker
 * precedes a 16-bit little-endian bit length. This bounded v1 accepts
 * the ordinary single-short form and rejects extended-length strings.
 * Reference grammar: ODA .dwg object stream specification (not code). */
static int read_u8(MusaDwgAc1015Bits *b,uint32_t *out){
    return musa_dwg_ac1015_bits_read_u(b,8u,out);
}
static int utf8_append(char *dst,size_t *used,uint32_t cp){
    unsigned needed=(cp<0x80u)?1u:(cp<0x800u)?2u:
                    (cp<0x10000u)?3u:4u;
    if(cp<0x20u||cp>0x10ffffu||
       (cp>=0xd800u&&cp<=0xdfffu)||
       *used>511u-needed)return 0;
    if(needed==1u)dst[(*used)++]=(char)cp;
    else{
        for(unsigned n=needed;n>1u;n--){
            dst[*used+n-1u]=(char)(0x80u|(cp&0x3fu));
            cp>>=6u;
        }
        dst[*used]=(char)(((0xf0u<<(4u-needed))&0xffu)|cp);
        *used+=needed;
    }
    return 1;
}
int musa_dwg_ac1032_layer_ot51_name_at(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LayerName *out){
    if(!raw||!layout||!out||
       overlap(raw,n,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),out,sizeof(*out)))return 0;
    MusaDwgAc1032LayerIdentity id={0};
    if(!musa_dwg_ac1032_layer_ot51_identity_at(raw,n,layout,&id)||
       layout->data_stream_bits>SIZE_MAX||
       layout->data_stream_bits<id.data_fields_start_bit+18u)
       return 0;
    const size_t end=(size_t)layout->data_stream_bits;
    MusaDwgAc1015Bits present={0};
    uint32_t yes=0u,low=0u,high=0u;
    if(!musa_dwg_ac1015_bits_init(&present,raw,n,end-1u,end)||
       !musa_dwg_ac1015_bits_read_u(&present,1u,&yes)||
       yes!=1u)return 0;
    MusaDwgAc1015Bits size={0};
    if(!musa_dwg_ac1015_bits_init(&size,raw,n,end-17u,end-1u)||
       !read_u8(&size,&low)||!read_u8(&size,&high))return 0;
    const uint32_t count_bits=low|(high<<8u);
    if(!count_bits||(count_bits&0x8000u)||
       (size_t)count_bits>end-17u-id.data_fields_start_bit)
        return 0;
    const size_t start=end-17u-(size_t)count_bits;
    MusaDwgAc1015Bits text={0};
    uint16_t units=0u;
    if(!musa_dwg_ac1015_bits_init(&text,raw,n,start,end-17u)||
       !musa_dwg_ac1015_bits_read_bs(&text,&units)||
       !units||units>255u||
       text.bit_end-text.bit_pos!=(size_t)units*16u)
        return 0;
    MusaDwgAc1032LayerName result={0};
    size_t used=0u;
    uint32_t pending_high=0u;
    for(uint16_t i=0;i<units;i++){
        uint32_t lo=0u,hi=0u,cp=0u;
        if(!read_u8(&text,&lo)||!read_u8(&text,&hi))return 0;
        const uint32_t value=lo|(hi<<8u);
        if(pending_high){
            if(value<0xdc00u||value>0xdfffu)return 0;
            cp=0x10000u+((pending_high-0xd800u)<<10u)+
               (value-0xdc00u);
            pending_high=0u;
        }else if(value>=0xd800u&&value<=0xdbffu){
            pending_high=value;
            continue;
        }else if(value>=0xdc00u&&value<=0xdfffu)
            return 0;
        else cp=value;
        if(!utf8_append(result.utf8,&used,cp))return 0;
    }
    if(pending_high||text.bit_pos!=text.bit_end||
       used>=sizeof(result.utf8))return 0;
    result.utf8[used]='\0';
    result.indexed_handle=id.indexed_handle;
    result.original_string_start_bit=start;
    result.utf16_code_units=units;
    result.utf8_bytes=(uint32_t)used;
    result.original_record_crc_verified=1u;
    *out=result;
    return 1;
}

/* R2000+ original LAYER table numeric-field grammar. DWG R2018's TU
 * string lives separately and is verified before publishing the numeric
 * values. This is an evidence reader, not a visibility/style resolver. */
int musa_dwg_ac1032_layer_ot51_flags_evidence_at(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LayerFlagsEvidence *out){
    if(!raw||!layout||!out||
       overlap(raw,n,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),out,sizeof(*out)))
        return 0;
    MusaDwgAc1032LayerIdentity identity={0};
    MusaDwgAc1032LayerName name={0};
    if(!musa_dwg_ac1032_layer_ot51_identity_at(raw,n,layout,&identity)||
       !musa_dwg_ac1032_layer_ot51_name_at(raw,n,layout,&name)||
       identity.indexed_handle!=name.indexed_handle||
       identity.data_fields_start_bit>=name.original_string_start_bit||
       layout->data_stream_bits>SIZE_MAX)
        return 0;
    MusaDwgAc1015Bits bits={0};
    if(!musa_dwg_ac1015_bits_init(&bits,raw,n,
             identity.data_fields_start_bit,
             name.original_string_start_bit))
        return 0;
    const size_t starting=bits.bit_pos;
    uint32_t flag64=0u,dependent=0u;
    uint16_t xref=0u,raw_values=0u;
    if(!musa_dwg_ac1015_bits_read_u(&bits,1u,&flag64)||
       !musa_dwg_ac1015_bits_read_bs(&bits,&xref)||
       !musa_dwg_ac1015_bits_read_u(&bits,1u,&dependent)||
       !musa_dwg_ac1015_bits_read_bs(&bits,&raw_values)||
       bits.bit_pos>name.original_string_start_bit)
        return 0;
    MusaDwgAc1032LayerFlagsEvidence value={0};
    value.indexed_handle=identity.indexed_handle;
    value.numeric_flags_start_bit=starting;
    value.numeric_flags_end_bit=bits.bit_pos;
    value.xref_index_plus_one=xref;
    value.raw_values_70_290_370=raw_values;
    value.entry_64_flag=(uint8_t)flag64;
    value.xref_dependent=(uint8_t)dependent;
    value.source_crc_verified=1u;
    *out=value;
    return 1;
}

/* R2000+ LAYER numeric VALUES field: independent bit decomposition.
 * Always preliminary: actual layer visibility depends on model/viewport
 * and unvalidated flags; do not use this to hide or recolor geometry. */
int musa_dwg_ac1032_layer_base_bits_decode(
    uint16_t raw_values,MusaDwgAc1032LayerBaseBitPreflight *out){
    if(!out)return 0;
    MusaDwgAc1032LayerBaseBitPreflight v={0};
    v.raw_values=raw_values;
    v.unmapped_bits=(uint16_t)(raw_values & (uint16_t)~UINT16_C(0x03ff));
    v.frozen_bit=(uint8_t)((raw_values>>0u)&1u);
    v.on_bit=(uint8_t)((raw_values>>1u)&1u);
    v.frozen_in_new_viewports_bit=(uint8_t)((raw_values>>2u)&1u);
    v.locked_bit=(uint8_t)((raw_values>>3u)&1u);
    v.plotting_bit=(uint8_t)((raw_values>>4u)&1u);
    v.lineweight_field=(uint8_t)((raw_values>>5u)&31u);
    v.source_crc_verified=0u;  /* Numeric-only call has NO source proof. */
    v.display_accepted=0u; /* Per-viewport/full scene acceptance pending. */
    *out=v;return 1;
}
int musa_dwg_ac1032_layer_ot51_base_bits_at(
    const uint8_t *raw,size_t n,const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LayerBaseBitPreflight *out){
    if(!raw||!layout||!out||
       overlap(raw,n,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),out,sizeof(*out)))return 0;
    MusaDwgAc1032LayerFlagsEvidence evidence={0};
    if(!musa_dwg_ac1032_layer_ot51_flags_evidence_at(
          raw,n,layout,&evidence)||
       !evidence.indexed_handle||!evidence.source_crc_verified)return 0;
    MusaDwgAc1032LayerBaseBitPreflight v={0};
    if(!musa_dwg_ac1032_layer_base_bits_decode(
            evidence.raw_values_70_290_370,&v))return 0;
    v.layer_handle=evidence.indexed_handle;
    v.source_crc_verified=1u;
    *out=v;return 1;
}

/* This narrow original-source profile has one specifically *unexplained*
 * seven-bit segment. We verify it instead of assuming it is padding.
 * Full R2018 LAYER semantic parsing must replace this profile before
 * CMC information controls actual Android/Windows/iOS palette rendering. */
int musa_dwg_ac1032_layer_ot51_anchored_cmc_evidence_at(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032LayerCmcEvidence *out){
    if(!raw||!layout||!out||
       overlap(raw,n,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),out,sizeof(*out)))
        return 0;
    MusaDwgAc1032LayerIdentity identity={0};
    MusaDwgAc1032LayerName name={0};
    MusaDwgAc1032LayerFlagsEvidence flags={0};
    if(!musa_dwg_ac1032_layer_ot51_identity_at(
          raw,n,layout,&identity)||
       !musa_dwg_ac1032_layer_ot51_name_at(
          raw,n,layout,&name)||
       !musa_dwg_ac1032_layer_ot51_flags_evidence_at(
          raw,n,layout,&flags)||
       identity.indexed_handle!=name.indexed_handle||
       identity.indexed_handle!=flags.indexed_handle||
       !identity.original_record_crc_verified||
       !flags.source_crc_verified||!name.original_record_crc_verified||
       !name.utf8_bytes||!flags.numeric_flags_end_bit||
       flags.numeric_flags_end_bit>SIZE_MAX-7u||
       flags.numeric_flags_end_bit+7u>=name.original_string_start_bit)
        return 0;
    MusaDwgAc1015Bits prefix={0};
    uint32_t unexplained=0u;
    if(!musa_dwg_ac1015_bits_init(
             &prefix,raw,n,flags.numeric_flags_end_bit,
             flags.numeric_flags_end_bit+7u)||
       !musa_dwg_ac1015_bits_read_u(&prefix,7u,&unexplained)||
       unexplained!=3u)
        return 0;
    MusaDwgCmcField cmc={0};
    if(!musa_dwg_cmc_read_numeric_field(
          raw,n,flags.numeric_flags_end_bit+7u,
          name.original_string_start_bit,&cmc)||
       cmc.end_bit!=name.original_string_start_bit||
       cmc.start_bit!=flags.numeric_flags_end_bit+7u||
       !cmc.supported_numeric_only||
       cmc.indexed_color!=0u||
       (cmc.packed_rgb>>24u)!=0xc3u||
       (cmc.packed_rgb&0x00ffff00u)!=0u||
       (cmc.packed_rgb&0xffu)<1u||
       !flags.raw_values_70_290_370)
        return 0;
    MusaDwgAc1032LayerCmcEvidence result={0};
    result.indexed_handle=identity.indexed_handle;
    result.packed_color_method=cmc.packed_rgb;
    result.opaque_field_start_bit=flags.numeric_flags_end_bit;
    result.cmc_start_bit=cmc.start_bit;
    result.cmc_end_bit=cmc.end_bit;
    result.opaque_field_bits=7u;
    result.aci_index=(uint8_t)(cmc.packed_rgb&0xffu);
    result.original_record_crc_verified=1u;
    *out=result;
    return 1;
}
