/* SPDX-License-Identifier: MIT */
#include "musacad_dwg_ac1024_classes.h"
#include <stdint.h>
#include <stdlib.h>
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

static int bit_long(const uint8_t *p,size_t bytes,
                    size_t *cursor,uint32_t *out) {
    size_t at=*cursor;
    uint32_t mode=0u,v=0u,byte=0u;
    if(!bit_read(p,bytes,&at,2u,&mode))return 0;
    if(mode==0u){
        for(unsigned n=0u;n<4u;n++){
            if(!bit_read(p,bytes,&at,8u,&byte))return 0;
            v|=byte<<(n*8u);
        }
    } else if(mode==1u) {
        if(!bit_read(p,bytes,&at,8u,&v))return 0;
    } else if(mode!=2u) {
        return 0; /* BL 11 is reserved */
    }
    *cursor=at;
    *out=v;
    return 1;
}

int musa_dwg_ac1024_classes_numeric_records(
    const uint8_t *section,size_t len,uint8_t extra,
    MusaDwgAc1024ClassNumeric *records,size_t capacity,size_t *out_count) {
    if(!section||!out_count||(!records&&capacity)||
       (records&&capacity>SIZE_MAX/sizeof(*records))||
       overlaps(section,len,out_count,sizeof(*out_count))||
       (records&&(
          overlaps(section,len,records,capacity*sizeof(*records))||
          overlaps(records,capacity*sizeof(*records),
                   out_count,sizeof(*out_count)))))
        return 0;
    MusaDwgAc1024ClassesPrefix prefix={0};
    if(!musa_dwg_ac1024_classes_prefix_shape_probe(
            section,len,extra,&prefix)||
       prefix.original_section_authenticated||
       prefix.class_name_resolved||prefix.display_accepted||
       prefix.maximum_class_number<500u)
        return 0;
    const size_t limit=(size_t)prefix.maximum_class_number-499u;
    if(!limit||limit>4096u)return 0;
    MusaDwgAc1024ClassNumeric *tmp=
        (MusaDwgAc1024ClassNumeric *)calloc(limit,sizeof(*tmp));
    if(!tmp)return 0;
    const uint8_t *data=section+prefix.data_start_byte;
    const size_t bytes=prefix.declared_data_area_bytes;
    size_t at=prefix.records_start_bit,count=0u;
    uint16_t previous=499u;
    int ok=1;
    while(previous<prefix.maximum_class_number&&count<limit){
        uint16_t classnum=0u,proxy=0u,item=0u,dwg=0u,maint=0u;
        uint32_t zombie=0u,instances=0u,unk1=0u,unk2=0u;
        if(!bit_short(data,bytes,&at,&classnum)||
           !bit_short(data,bytes,&at,&proxy)||
           !bit_read(data,bytes,&at,1u,&zombie)||
           !bit_short(data,bytes,&at,&item)||
           !bit_long(data,bytes,&at,&instances)||
           !bit_short(data,bytes,&at,&dwg)||
           !bit_short(data,bytes,&at,&maint)||
           !bit_long(data,bytes,&at,&unk1)||
           !bit_long(data,bytes,&at,&unk2)||
           classnum<=previous||classnum>prefix.maximum_class_number||
           (item!=0x1f2u&&item!=0x1f3u)){
            ok=0;break;
        }
        MusaDwgAc1024ClassNumeric rec={0};
        rec.class_number=classnum;
        rec.proxy_flags=proxy;
        rec.item_class_id=item;
        rec.dwg_version=dwg;
        rec.maintenance_version=maint;
        rec.object_instance_count=instances;
        rec.unknown1=unk1;
        rec.unknown2=unk2;
        rec.was_proxy=(uint8_t)zombie;
        /* No original Classes CRC or TU string has been validated. */
        tmp[count++]=rec;
        previous=classnum;
    }
    if(previous!=prefix.maximum_class_number||!count||
       (records&&capacity<count))
        ok=0;
    if(ok){
        if(records)memcpy(records,tmp,count*sizeof(*tmp));
        *out_count=count;
    }
    free(tmp);
    return ok;
}

/* Source-agnostic R2010 RL bit-count diagnostic. It does NOT use the
 * CRC of the section, the physical page provenance, or TU names.
 * Unlike the older shape hypothesis, a mandatory RL bitcount is
 * consumed before the class BS prefix. Outputs remain fail-atomic. */
int musa_dwg_ac1024_classes_r2010_bitcount_prefix_probe(
    const uint8_t *section,size_t len,uint8_t extra,
    MusaDwgAc1024ClassesR2010BitcountPrefix *out){
    if(!section||!out||extra>1u||
       len<16u+4u+4u+5u+18u||
       len>MUSA_R2010_CLASSES_MAX||
       overlaps(section,len,out,sizeof(*out))||
       memcmp(section,start_sentinel,sizeof(start_sentinel))!=0)
        return 0;
    /* Original R2010 Classes pages may have exactly eight opaque bytes
     * AFTER the close sentinel. Never assume the sentinel ends at EOF
     * or allow arbitrary trailing bytes: use only two exact layouts. */
    size_t tail=0u;
    if(memcmp(section+len-16u,end_sentinel,16u)!=0){
        if(len<24u||memcmp(section+len-24u,end_sentinel,16u)!=0)
            return 0;
        tail=8u;
    }
    const size_t offset=24u+(extra?4u:0u);
    if(len<offset+5u+18u+tail)return 0;
    const uint32_t declared=read_le32(section+16u);
    const uint32_t maintenance=extra?read_le32(section+20u):0u;
    const uint32_t declared_bits=read_le32(section+20u+(extra?4u:0u));
    if(declared<5u||declared>MUSA_R2010_CLASSES_MAX||
       declared>len-offset-18u-tail||
       declared_bits<36u||
       (uint64_t)declared_bits>(uint64_t)declared*8u)
        return 0;
    size_t pos=0u;
    uint16_t maximum=0u;
    uint32_t zero1=0u,zero2=0u,flag=0u;
    if(!bit_short(section+offset,declared,&pos,&maximum)||
       !bit_read(section+offset,declared,&pos,8u,&zero1)||
       !bit_read(section+offset,declared,&pos,8u,&zero2)||
       !bit_read(section+offset,declared,&pos,1u,&flag)||
       maximum<500u||maximum>5000u||
       zero1||zero2||flag!=1u||pos>=declared_bits)
        return 0;
    MusaDwgAc1024ClassesR2010BitcountPrefix next={0};
    next.declared_data_area_bytes=declared;
    next.declared_data_bits=declared_bits;
    next.maintenance_word=maintenance;
    next.maximum_class_number=maximum;
    next.bitstream_start_byte=offset;
    next.numeric_records_start_bit=pos;
    next.extra_rl_present=extra;
    next.end_sentinel_followed_by_eight_bytes=(uint8_t)(tail==8u);
    next.byte_area_extent_exact=(uint8_t)(
        (size_t)declared==len-offset-18u-tail);
    next.structural_prefix_verified=1u;
    *out=next;
    return 1;
}

/* Do not conflate this diagnostics-only comparison with R2010
 * Classes parsing or with a CRC-verified original source section.
 * The caller independently authenticates the extracted section. */
static int class_preamble_at(const uint8_t *buf,size_t len,
                             size_t byte_offset){
    if(!buf||byte_offset>=len||len-byte_offset<5u)return 0;
    const uint8_t *area=buf+byte_offset;
    const size_t size=len-byte_offset;
    size_t at=0u;
    uint16_t maxnum=0u;
    uint32_t a=0u,b=0u,flag=0u;
    return bit_short(area,size,&at,&maxnum)&&
           bit_read(area,size,&at,8u,&a)&&
           bit_read(area,size,&at,8u,&b)&&
           bit_read(area,size,&at,1u,&flag)&&
           maxnum>=500u&&maxnum<=5000u&&
           a==0u&&b==0u&&flag==1u;
}
int musa_dwg_ac1024_classes_header_survey(
    const uint8_t *section,size_t len,
    MusaDwgAc1024ClassesHeaderSurvey *out){
    if(!section||!out||len<38u||
       len>MUSA_R2010_CLASSES_MAX||
       overlaps(section,len,out,sizeof(*out)))
        return 0;
    MusaDwgAc1024ClassesHeaderSurvey next={0};
    next.begin_at_zero=(uint8_t)(
        memcmp(section,start_sentinel,sizeof(start_sentinel))==0);
    next.end_at_eof=(uint8_t)(
        memcmp(section+len-sizeof(end_sentinel),end_sentinel,
               sizeof(end_sentinel))==0);
    /* A section under 64 bytes still has fully defined survey windows. */
    const size_t first_end=len<64u?len:64u;
    const size_t last_begin=len>64u?len-64u:0u;
    for(size_t at=0u;at<=first_end-16u;++at)
        if(!memcmp(section+at,start_sentinel,16u)){
            next.begin_in_first64=1u;break;
        }
    for(size_t at=last_begin;at<=len-16u;++at)
        if(!memcmp(section+at,end_sentinel,16u)){
            next.end_in_last64=1u;break;
        }
    if(next.begin_at_zero){
        const uint32_t size=read_le32(section+16u);
        next.declared_byte_area_fits=(uint8_t)(
            size>=5u && size<=len-38u);
        if(next.declared_byte_area_fits){
            next.area_exact_0extra=(uint8_t)(size==len-38u);
            next.area_exact_1extra=(uint8_t)(size==len-42u);
            next.area_exact_2extra=(uint8_t)(size==len-46u);
        }
        if(size>=5u&&size<=MUSA_R2010_CLASSES_MAX){
            const uint64_t bits=(uint64_t)size*8u;
            const uint32_t at20=read_le32(section+20u);
            const uint32_t at24=read_le32(section+24u);
            next.raw_bits20_in_range=(uint8_t)(
                at20>=36u&&(uint64_t)at20<=bits);
            next.raw_bits24_in_range=(uint8_t)(
                at24>=36u&&(uint64_t)at24<=bits);
        }
        next.preamble_at20=(uint8_t)class_preamble_at(section,len,20u);
        next.preamble_at24=(uint8_t)class_preamble_at(section,len,24u);
        next.preamble_at28=(uint8_t)class_preamble_at(section,len,28u);
    }
    *out=next;
    return 1;
}
