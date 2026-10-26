/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original first-party OT1 TEXT single-TU source, never MTEXT grammar.
 * Source record CRC/H/OT1/layout is a mandatory caller precondition. */
#include "musacad_dwg_ac1032_text.h"
#include "musacad_dwg_ac1015_values.h"
#include <stdint.h>
#include <string.h>
#define LIMIT (64u*1024u*1024u)
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
int musa_dwg_ac1032_text_single_tu_at(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *l,
    size_t geom,MusaDwgAc1032TextSource *out){
    if(!raw||!l||!out||!n||n>LIMIT||n>SIZE_MAX/8u||
       overlap(raw,n,l,sizeof(*l))||
       overlap(raw,n,out,sizeof(*out))||
       overlap(l,sizeof(*l),out,sizeof(*out))||
       !l->original_record_crc_verified||!l->indexed_handle||
       l->raw_object_type!=1u||l->object_type_field_bits!=10u||
       l->total_payload_bits!=(uint64_t)n*8u||
       l->data_stream_bits>SIZE_MAX||
       l->handle_stream_start_bit!=l->data_stream_bits||
       !l->handle_stream_bits||
       l->handle_stream_bits!=l->total_payload_bits-l->data_stream_bits)
       return 0;
    const size_t end=(size_t)l->data_stream_bits;
    if(end<18u||geom<l->object_type_field_bits||geom>end-18u)
        return 0;
    MusaDwgAc1015Bits b={0};
    uint32_t present=0u,low=0u,high=0u;
    if(!musa_dwg_ac1015_bits_init(&b,raw,n,end-1u,end)||
       !musa_dwg_ac1015_bits_read_u(&b,1u,&present)||present!=1u||
       !musa_dwg_ac1015_bits_init(&b,raw,n,end-17u,end-1u)||
       !musa_dwg_ac1015_bits_read_u(&b,8u,&low)||
       !musa_dwg_ac1015_bits_read_u(&b,8u,&high))return 0;
    const uint32_t string_bits=low|(high<<8u);
    if(!string_bits||(string_bits&0x8000u)||
       (size_t)string_bits>end-17u-geom)return 0;
    const size_t start=end-17u-(size_t)string_bits;
    if(start<=geom)return 0;
    uint16_t units=0u;
    if(!musa_dwg_ac1015_bits_init(&b,raw,n,start,end-17u)||
       !musa_dwg_ac1015_bits_read_bs(&b,&units)||
       !units||units>512u||
       b.bit_end-b.bit_pos!=(size_t)units*16u)return 0;
    MusaDwgAc1032TextSource value={0};
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
        if(!append_utf8(value.utf8,sizeof(value.utf8),&used,cp))
            return 0;
    }
    if(high_surrogate||b.bit_pos!=b.bit_end||
       used>=sizeof(value.utf8))return 0;
    value.utf8[used]='\0';
    value.indexed_handle=l->indexed_handle;
    value.original_string_start_bit=start;
    value.utf16_code_units=units;
    value.utf8_bytes=(uint32_t)used;
    value.original_record_crc_verified=1u;
    value.geometry_font_and_display_accepted=0u;
    *out=value;
    return 1;
}

/* First-party R2000+ TEXT numeric grammar (ODA on-disk field order,
 * not LibreDWG source). Reject all unconsumed DATA bits before TU STRING. */
#include <math.h>
int musa_dwg_ac1032_text_placement_at(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *l,
    size_t geom,MusaDwgAc1032TextPlacement *out){
    if(!raw||!l||!out||!n||n>LIMIT||n>SIZE_MAX/8u||
       overlap(raw,n,l,sizeof(*l))||
       overlap(raw,n,out,sizeof(*out))||
       overlap(l,sizeof(*l),out,sizeof(*out))||
       !l->original_record_crc_verified||l->raw_object_type!=1u||
       !l->indexed_handle||l->object_type_field_bits!=10u||
       l->total_payload_bits!=(uint64_t)n*8u||
       l->data_stream_bits>SIZE_MAX||
       l->handle_stream_start_bit!=l->data_stream_bits||
       !l->handle_stream_bits||
       l->handle_stream_bits!=l->total_payload_bits-l->data_stream_bits)
        return 0;
    MusaDwgAc1032TextSource source={0};
    if(!musa_dwg_ac1032_text_single_tu_at(raw,n,l,geom,&source)||
       !source.original_record_crc_verified||
       source.indexed_handle!=l->indexed_handle||
       source.original_string_start_bit<=geom)return 0;
    MusaDwgAc1015Bits b={0};
    if(!musa_dwg_ac1015_bits_init(
          &b,raw,n,geom,source.original_string_start_bit))return 0;
    MusaDwgAc1032TextPlacement v={0};
    v.indexed_handle=l->indexed_handle;
    v.original_string_start_bit=source.original_string_start_bit;
    uint32_t flags=0u;
    if(!musa_dwg_ac1015_bits_read_u(&b,8u,&flags))return 0;
    v.data_flags=(uint8_t)flags;
    double elevation=0.0;
    if(!(flags&1u)&&!musa_dwg_ac1015_bits_read_rd(&b,&elevation))
        return 0;
    if(!musa_dwg_ac1015_bits_read_rd(&b,&v.insertion[0])||
       !musa_dwg_ac1015_bits_read_rd(&b,&v.insertion[1]))
        return 0;
    v.insertion[2]=elevation;
    v.alignment[0]=v.insertion[0];
    v.alignment[1]=v.insertion[1];
    v.alignment[2]=elevation;
    if(!(flags&2u)){
        if(!musa_dwg_ac1015_bits_read_dd(
             &b,v.insertion[0],&v.alignment[0])||
           !musa_dwg_ac1015_bits_read_dd(
             &b,v.insertion[1],&v.alignment[1]))return 0;
    }
    if(!musa_dwg_ac1015_bits_read_be(&b,v.extrusion)||
       !musa_dwg_ac1015_bits_read_bt(&b,&v.thickness))
        return 0;
    if(!(flags&4u)&&!musa_dwg_ac1015_bits_read_rd(&b,&v.oblique))
        return 0;
    if(!(flags&8u)&&!musa_dwg_ac1015_bits_read_rd(&b,&v.rotation))
        return 0;
    if(!musa_dwg_ac1015_bits_read_rd(&b,&v.height))return 0;
    v.width_factor=1.0;
    if(!(flags&16u)&&!musa_dwg_ac1015_bits_read_rd(&b,&v.width_factor))
        return 0;
    if(!(flags&32u)&&!musa_dwg_ac1015_bits_read_bs(&b,&v.generation))
        return 0;
    if(!(flags&64u)&&!musa_dwg_ac1015_bits_read_bs(
         &b,&v.horizontal_alignment))return 0;
    if(!(flags&128u)&&!musa_dwg_ac1015_bits_read_bs(
         &b,&v.vertical_alignment))return 0;
    if(b.bit_pos!=source.original_string_start_bit||
       !isfinite(elevation)||!isfinite(v.insertion[0])||
       !isfinite(v.insertion[1])||
       !isfinite(v.alignment[0])||!isfinite(v.alignment[1])||
       !isfinite(v.thickness)||!isfinite(v.oblique)||
       !isfinite(v.rotation)||!isfinite(v.height)||v.height<=0.0||
       !isfinite(v.width_factor)||v.width_factor<=0.0||
       v.horizontal_alignment>5u||v.vertical_alignment>3u||
       hypot(hypot(v.extrusion[0],v.extrusion[1]),v.extrusion[2])<0.5)
        return 0;
    v.geometry_end_bit=b.bit_pos;
    v.original_record_crc_verified=1u;
    v.numeric_fields_decoded=1u;
    v.wcs_font_and_display_accepted=0u;
    *out=v;
    return 1;
}

/* Original OT1 TEXT OCS anchors and rotation axes in WCS. This only
 * composes the existing independent C11 arbitrary-axis math with the
 * authenticated source numeric placement. Never treats alignment anchor
 * as a glyph baseline without the original text style/font metrics. */
#include "musacad_ocs.h"
int musa_dwg_ac1032_text_placement_world(
    const MusaDwgAc1032TextPlacement *source,
    MusaDwgAc1032TextWorldPlacement *out){
    if(!source||!out||(const void*)source==(const void*)out||
       !source->indexed_handle||!source->original_record_crc_verified||
       !source->numeric_fields_decoded||
       source->wcs_font_and_display_accepted||
       !isfinite(source->rotation)||!isfinite(source->height)||
       source->height<=0.0||!isfinite(source->width_factor)||
       source->width_factor<=0.0||
       source->horizontal_alignment>5u||
       source->vertical_alignment>3u)return 0;
    MusaCoreOcsBasis basis={0};
    if(!musa_core_ocs_basis(source->extrusion,&basis))return 0;
    MusaDwgAc1032TextWorldPlacement value={0};
    const double ins[2]={source->insertion[0],source->insertion[1]};
    const double aln[2]={source->alignment[0],source->alignment[1]};
    if(!musa_core_ocs_to_world(
           &basis,ins,source->insertion[2],value.insertion)||
       !musa_core_ocs_to_world(
           &basis,aln,source->alignment[2],value.alignment))
        return 0;
    const double c=cos(source->rotation),s=sin(source->rotation);
    if(!isfinite(c)||!isfinite(s))return 0;
    for(size_t i=0u;i<3u;i++){
        value.baseline_unit[i]=c*basis.x[i]+s*basis.y[i];
        value.up_unit[i]=-s*basis.x[i]+c*basis.y[i];
        if(!isfinite(value.baseline_unit[i])||
           !isfinite(value.up_unit[i]))return 0;
    }
    value.indexed_handle=source->indexed_handle;
    value.height=source->height;
    value.width_factor=source->width_factor;
    value.horizontal_alignment=source->horizontal_alignment;
    value.vertical_alignment=source->vertical_alignment;
    value.original_record_crc_verified=1u;
    value.wcs_math_decoded=1u;
    value.font_and_display_accepted=0u;
    *out=value;
    return 1;
}
