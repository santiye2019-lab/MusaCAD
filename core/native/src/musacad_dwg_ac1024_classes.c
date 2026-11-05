/* SPDX-License-Identifier: MIT */
#include "musacad_dwg_ac1024_classes.h"
#include <stdint.h>
#include <string.h>

#define MUSA_R2010_CLASSES_MAX (8u * 1024u * 1024u)
static const uint8_t start_sentinel[16] = {
    0x8d,0xa1,0xc4,0xb8,0xc4,0xa9,0xf8,0xc5,
    0xc0,0xdc,0xf4,0x5f,0xe7,0xcf,0xb6,0x8a
};
static const uint8_t end_sentinel[16] = {
    0x72,0x5e,0x3b,0x47,0x3b,0x56,0x07,0x3a,
    0x3f,0x23,0x0b,0xa0,0x18,0x30,0x49,0x75
};
static uint32_t read_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8u) |
           ((uint32_t)p[2]<<16u) | ((uint32_t)p[3]<<24u);
}
static int overlaps(const void *a,size_t an,const void *b,size_t bn) {
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}
static int bit_read(const uint8_t *p,size_t bytes,
                    size_t *cursor,unsigned width,uint32_t *value) {
    if(!p||!cursor||!value||!width||width>8u||
       bytes>SIZE_MAX/8u||*cursor>bytes*8u||
       width>bytes*8u-*cursor)return 0;
    size_t at=*cursor;
    uint32_t v=0u;
    for(unsigned n=0u;n<width;++n){
        const size_t pos=at+n;
        v=(v<<1u)|((p[pos/8u]>>(7u-(pos%8u)))&1u);
    }
    *cursor=at+width;
    *value=v;
    return 1;
}
static int bit_short(const uint8_t *p,size_t bytes,
                     size_t *cursor,uint16_t *out) {
    size_t at=*cursor;
    uint32_t mode=0u,first=0u,second=0u;
    if(!bit_read(p,bytes,&at,2u,&mode))return 0;
    uint32_t value=0u;
    switch(mode){
    case 0u:
        if(!bit_read(p,bytes,&at,8u,&first)||
           !bit_read(p,bytes,&at,8u,&second))return 0;
        value=first|(second<<8u);break;
    case 1u:
        if(!bit_read(p,bytes,&at,8u,&first))return 0;
        value=first;break;
    case 2u: value=0u;break;
    case 3u: value=256u;break;
    default:return 0;
    }
    *cursor=at;
    *out=(uint16_t)value;
    return 1;
}
int musa_dwg_ac1024_classes_prefix_shape_probe(
    const uint8_t *section,size_t len,uint8_t extra,
    MusaDwgAc1024ClassesPrefix *out) {
    if(!section||!out||extra>1u||
       len<16u+4u+5u+2u+16u||
       len>MUSA_R2010_CLASSES_MAX||
       overlaps(section,len,out,sizeof(*out))||
       memcmp(section,start_sentinel,16u)!=0)
        return 0;
    const size_t start=16u+4u+(extra?4u:0u);
    if(len<start+5u+18u)return 0;
    const uint32_t area=read_le32(section+16u);
    if(extra&&read_le32(section+20u)!=0u)return 0;
    if(area<5u||area>MUSA_R2010_CLASSES_MAX||
       (size_t)area>len-start-18u||
       start+(size_t)area+18u!=len||
       memcmp(section+len-16u,end_sentinel,16u)!=0)
        return 0;

    /* Bounded preamble only. Section CRC bytes at -18,-17 are present
     * but NOT verified. There is no source provenance in this API. */
    size_t cursor=0u;
    uint16_t maximum=0u;
    uint32_t reserved1=0u,reserved2=0u,flag=0u;
    if(!bit_short(section+start,area,&cursor,&maximum)||
       !bit_read(section+start,area,&cursor,8u,&reserved1)||
       !bit_read(section+start,area,&cursor,8u,&reserved2)||
       !bit_read(section+start,area,&cursor,1u,&flag)||
       maximum<500u||!flag||reserved1||reserved2||
       cursor>=((size_t)area)*8u)
        return 0;
    MusaDwgAc1024ClassesPrefix next={0};
    next.declared_data_area_bytes=area;
    next.maximum_class_number=maximum;
    next.data_start_byte=start;
    next.records_start_bit=cursor;
    next.extra_rl_present=extra;
    next.prefix_shape_verified=1u;
    /* Never set original_section_authenticated, class_name_resolved,
     * or display_accepted in a shape-only diagnostic. */
    *out=next;
    return 1;
}
