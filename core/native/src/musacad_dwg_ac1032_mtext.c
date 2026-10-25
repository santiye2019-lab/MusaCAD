/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_mtext.h"
#include "musacad_dwg_ac1015_values.h"
#include <stdint.h>
#include <string.h>
#define MAX_SOURCE_BYTES (64u*1024u*1024u)
static int overlap(const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
static int append_utf8(char *dst,size_t cap,size_t *used,uint32_t cp){
    if(cp<0x20u||cp>0x10ffffu||
       (cp>=0xd800u&&cp<=0xdfffu))return 0;
    const unsigned n=cp<0x80u?1u:cp<0x800u?2u:cp<0x10000u?3u:4u;
    if(*used>=cap||(size_t)n>cap-1u-*used)return 0;
    if(n==1u)dst[(*used)++]=(char)cp;
    else if(n==2u){
        dst[(*used)++]=(char)(0xc0u|(cp>>6u));
        dst[(*used)++]=(char)(0x80u|(cp&0x3fu));
    }else if(n==3u){
        dst[(*used)++]=(char)(0xe0u|(cp>>12u));
        dst[(*used)++]=(char)(0x80u|((cp>>6u)&0x3fu));
        dst[(*used)++]=(char)(0x80u|(cp&0x3fu));
    }else{
        dst[(*used)++]=(char)(0xf0u|(cp>>18u));
        dst[(*used)++]=(char)(0x80u|((cp>>12u)&0x3fu));
        dst[(*used)++]=(char)(0x80u|((cp>>6u)&0x3fu));
        dst[(*used)++]=(char)(0x80u|(cp&0x3fu));
    }
    return 1;
}
int musa_dwg_ac1032_mtext_single_tu_at(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *l,
    size_t geometry_start_bit,MusaDwgAc1032MTextSource *out){
    if(!raw||!l||!out||!n||n>MAX_SOURCE_BYTES||
       n>SIZE_MAX/8u||
       overlap(raw,n,l,sizeof(*l))||
       overlap(raw,n,out,sizeof(*out))||
       overlap(l,sizeof(*l),out,sizeof(*out))||
       !l->original_record_crc_verified||!l->indexed_handle||
       l->raw_object_type!=44u||l->object_type_field_bits!=10u||
       l->total_payload_bits!=(uint64_t)n*8u||
       l->data_stream_bits>SIZE_MAX||
       l->handle_stream_start_bit!=l->data_stream_bits||
       !l->handle_stream_bits||
       l->handle_stream_bits!=l->total_payload_bits-l->data_stream_bits)
        return 0;
    const size_t end=(size_t)l->data_stream_bits;
    if(end<18u||geometry_start_bit>end-18u)return 0;
    MusaDwgAc1015Bits b={0};
    uint32_t present=0u,low=0u,high=0u;
    if(!musa_dwg_ac1015_bits_init(&b,raw,n,end-1u,end)||
       !musa_dwg_ac1015_bits_read_u(&b,1u,&present)||present!=1u||
       !musa_dwg_ac1015_bits_init(&b,raw,n,end-17u,end-1u)||
       !musa_dwg_ac1015_bits_read_u(&b,8u,&low)||
       !musa_dwg_ac1015_bits_read_u(&b,8u,&high))return 0;
    const uint32_t string_bits=low|(high<<8u);
    if(!string_bits||(string_bits&0x8000u)||
       (size_t)string_bits>end-17u-geometry_start_bit)return 0;
    const size_t start=end-17u-(size_t)string_bits;
    uint16_t units=0u;
    if(!musa_dwg_ac1015_bits_init(&b,raw,n,start,end-17u)||
       !musa_dwg_ac1015_bits_read_bs(&b,&units)||
       !units||units>512u||
       b.bit_end-b.bit_pos!=(size_t)units*16u)return 0;
    MusaDwgAc1032MTextSource result={0};
    uint32_t high_surrogate=0u;size_t used=0u;
    for(uint16_t i=0u;i<units;i++){
        uint32_t lo=0u,hi=0u,cp=0u;
        if(!musa_dwg_ac1015_bits_read_u(&b,8u,&lo)||
           !musa_dwg_ac1015_bits_read_u(&b,8u,&hi))return 0;
        const uint32_t unit=lo|(hi<<8u);
        if(high_surrogate){
            if(unit<0xdc00u||unit>0xdfffu)return 0;
            cp=0x10000u+((high_surrogate-0xd800u)<<10u)+(unit-0xdc00u);
            high_surrogate=0u;
        }else if(unit>=0xd800u&&unit<=0xdbffu){
            high_surrogate=unit;continue;
        }else if(unit>=0xdc00u&&unit<=0xdfffu)return 0;
        else cp=unit;
        if(!append_utf8(result.utf8,sizeof(result.utf8),&used,cp))
            return 0;
    }
    if(high_surrogate||b.bit_pos!=b.bit_end||
       used>=sizeof(result.utf8))return 0;
    result.utf8[used]='\0';
    result.indexed_handle=l->indexed_handle;
    result.original_string_start_bit=start;
    result.utf16_code_units=units;
    result.utf8_bytes=(uint32_t)used;
    result.original_record_crc_verified=1u;
    result.geometry_style_and_glyphs_accepted=0u;
    *out=result;
    return 1;
}

/* Read actual MTEXT geometric DATA prefix, bounded before the independent
 * STRING frame. No scan for plausible coordinates and no glyph acceptance.
 * All numeric fields use first-party bounded C11 bit grammar. */
#include <math.h>
int musa_dwg_ac1032_mtext_placement_at(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *l,
    size_t geom,MusaDwgAc1032MTextPlacement *out){
    if(!raw||!l||!out||!n||n>MAX_SOURCE_BYTES||
       overlap(raw,n,l,sizeof(*l))||
       overlap(raw,n,out,sizeof(*out))||
       overlap(l,sizeof(*l),out,sizeof(*out))||
       !l->original_record_crc_verified||!l->indexed_handle||
       l->raw_object_type!=44u||l->object_type_field_bits!=10u||
       geom<l->object_type_field_bits||
       l->total_payload_bits!=(uint64_t)n*8u||
       l->handle_stream_start_bit!=l->data_stream_bits||
       !l->handle_stream_bits||
       l->handle_stream_bits!=l->total_payload_bits-l->data_stream_bits)
        return 0;
    MusaDwgAc1032MTextSource source={0};
    if(!musa_dwg_ac1032_mtext_single_tu_at(
          raw,n,l,geom,&source)||
       !source.original_record_crc_verified||
       source.indexed_handle!=l->indexed_handle||
       source.original_string_start_bit<=geom)return 0;
    MusaDwgAc1015Bits bits={0};
    if(!musa_dwg_ac1015_bits_init(&bits,raw,n,geom,
          source.original_string_start_bit))return 0;
    MusaDwgAc1032MTextPlacement v={0};
    v.indexed_handle=l->indexed_handle;
    v.original_string_start_bit=source.original_string_start_bit;
    for(unsigned i=0u;i<3u;i++)
        if(!musa_dwg_ac1015_bits_read_bd(&bits,&v.insertion[i])||
           !isfinite(v.insertion[i]))return 0;
    for(unsigned i=0u;i<3u;i++)
        if(!musa_dwg_ac1015_bits_read_bd(&bits,&v.extrusion[i])||
           !isfinite(v.extrusion[i]))return 0;
    for(unsigned i=0u;i<3u;i++)
        if(!musa_dwg_ac1015_bits_read_bd(&bits,&v.x_axis[i])||
           !isfinite(v.x_axis[i]))return 0;
    if(!musa_dwg_ac1015_bits_read_bd(&bits,&v.rect_width)||
       !musa_dwg_ac1015_bits_read_bd(&bits,&v.rect_height)||
       !musa_dwg_ac1015_bits_read_bd(&bits,&v.text_height)||
       !musa_dwg_ac1015_bits_read_bs(&bits,&v.attachment)||
       !musa_dwg_ac1015_bits_read_bs(&bits,&v.drawing_direction)||
       !isfinite(v.rect_width)||v.rect_width<=0.0||
       !isfinite(v.rect_height)||v.rect_height<0.0||
       !isfinite(v.text_height)||v.text_height<=0.0||
       v.attachment<1u||v.attachment>9u||
       (v.drawing_direction!=1u&&v.drawing_direction!=3u&&
        v.drawing_direction!=5u)||
       hypot(hypot(v.extrusion[0],v.extrusion[1]),v.extrusion[2])<0.5||
       hypot(hypot(v.x_axis[0],v.x_axis[1]),v.x_axis[2])<0.5||
       bits.bit_pos>source.original_string_start_bit)return 0;
    v.geometry_end_bit=bits.bit_pos;
    v.original_record_crc_verified=1u;
    v.geometry_fields_decoded=1u;
    v.glyph_layout_and_display_accepted=0u;
    *out=v;
    return 1;
}
