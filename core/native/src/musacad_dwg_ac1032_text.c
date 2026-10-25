/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_text.h"
#include "musacad_dwg_ac1015_values.h"
#include <stdint.h>
#include <string.h>
#define MAX_TEXT_PAYLOAD (64u*1024u*1024u)
static int overlaps(const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
static int unit(MusaDwgAc1015Bits *bits,uint32_t *u){
    uint32_t a=0u,b=0u;
    if(!musa_dwg_ac1015_bits_read_u(bits,8u,&a)||
       !musa_dwg_ac1015_bits_read_u(bits,8u,&b))return 0;
    *u=a|(b<<8u);
    return 1;
}
static int append_codepoint(char *dst,size_t cap,size_t *used,uint32_t cp){
    unsigned count=cp<0x80u?1u:cp<0x800u?2u:cp<0x10000u?3u:4u;
    if(cp<0x20u||cp>0x10ffffu||
       (cp>=0xd800u&&cp<=0xdfffu)||
       *used>=cap||count>cap-1u-*used)return 0;
    if(count==1u)dst[(*used)++]=(char)cp;
    else{
        for(unsigned n=count;n>1u;n--){
            dst[*used+n-1u]=(char)(0x80u|(cp&0x3fu));
            cp>>=6u;
        }
        dst[*used]=(char)(((0xf0u<<(4u-count))&0xffu)|cp);
        *used+=count;
    }
    return 1;
}
int musa_dwg_ac1032_text_content_at(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *l,
    size_t geometry_start_bit,MusaDwgAc1032NativeTextContent *out){
    if(!raw||!l||!out||!n||n>MAX_TEXT_PAYLOAD||
       n>SIZE_MAX/8u||
       overlaps(raw,n,l,sizeof(*l))||
       overlaps(raw,n,out,sizeof(*out))||
       overlaps(l,sizeof(*l),out,sizeof(*out))||
       !l->original_record_crc_verified||!l->indexed_handle||
       l->raw_object_type!=1u||l->object_type_field_bits!=10u||
       l->total_payload_bits!=(uint64_t)n*8u||
       l->handle_stream_start_bit!=l->data_stream_bits||
       !l->handle_stream_bits||
       l->handle_stream_bits!=l->total_payload_bits-l->data_stream_bits||
       l->data_stream_bits>SIZE_MAX||
       geometry_start_bit<l->object_type_field_bits)
        return 0;
    const size_t end=(size_t)l->data_stream_bits;
    if(end<=geometry_start_bit+18u)return 0;
    uint32_t present=0u,lo=0u,hi=0u;
    MusaDwgAc1015Bits bits={0};
    if(!musa_dwg_ac1015_bits_init(&bits,raw,n,end-1u,end)||
       !musa_dwg_ac1015_bits_read_u(&bits,1u,&present)||
       present!=1u||
       !musa_dwg_ac1015_bits_init(&bits,raw,n,end-17u,end-1u)||
       !musa_dwg_ac1015_bits_read_u(&bits,8u,&lo)||
       !musa_dwg_ac1015_bits_read_u(&bits,8u,&hi))
        return 0;
    const uint32_t string_bits=lo|(hi<<8u);
    if(!string_bits||(string_bits&0x8000u)||
       (size_t)string_bits>end-17u-geometry_start_bit)
        return 0;
    const size_t start=end-17u-(size_t)string_bits;
    uint16_t units=0u;
    if(!musa_dwg_ac1015_bits_init(
          &bits,raw,n,start,end-17u)||
       !musa_dwg_ac1015_bits_read_bs(&bits,&units)||
       !units||units>512u||
       bits.bit_end-bits.bit_pos!=(size_t)units*16u)
        return 0;
    MusaDwgAc1032NativeTextContent result={0};
    uint32_t high=0u;
    size_t used=0u;
    for(uint16_t i=0;i<units;i++){
        uint32_t v=0u,cp=0u;
        if(!unit(&bits,&v))return 0;
        if(high){
            if(v<0xdc00u||v>0xdfffu)return 0;
            cp=0x10000u+((high-0xd800u)<<10u)+(v-0xdc00u);
            high=0u;
        }else if(v>=0xd800u&&v<=0xdbffu){
            high=v;continue;
        }else if(v>=0xdc00u&&v<=0xdfffu)return 0;
        else cp=v;
        if(!append_codepoint(result.utf8,sizeof(result.utf8),
                             &used,cp))return 0;
    }
    if(high||bits.bit_pos!=bits.bit_end||
       used>=sizeof(result.utf8))return 0;
    result.utf8[used]='\0';
    result.indexed_handle=l->indexed_handle;
    result.original_string_start_bit=start;
    result.utf16_code_units=(uint32_t)units;
    result.utf8_bytes=(uint32_t)used;
    result.original_record_crc_verified=1u;
    result.geometry_and_style_accepted=0u; /* no glyph rendering proof */
    *out=result;
    return 1;
}
