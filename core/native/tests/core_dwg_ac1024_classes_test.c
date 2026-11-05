/* SPDX-License-Identifier: MIT */
#include "musacad_dwg_ac1024_classes.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const uint8_t opening[16]={
    0x8d,0xa1,0xc4,0xb8,0xc4,0xa9,0xf8,0xc5,
    0xc0,0xdc,0xf4,0x5f,0xe7,0xcf,0xb6,0x8a
};
static const uint8_t closing[16]={
    0x72,0x5e,0x3b,0x47,0x3b,0x56,0x07,0x3a,
    0x3f,0x23,0x0b,0xa0,0x18,0x30,0x49,0x75
};
static void bit_set(uint8_t *p,size_t at,unsigned width,uint32_t value){
    for(unsigned j=0u;j<width;j++){
        const size_t k=at+j;
        const uint8_t bit=(uint8_t)(1u<<(7u-k%8u));
        if((value>>(width-1u-j))&1u)p[k/8u]|=bit;
        else p[k/8u]&=(uint8_t)~bit;
    }
}
static size_t fixture(uint8_t *bytes,int extra){
    /* 8-byte class area plus 2-byte CRC placeholder and end sentinel.
     * The CRC is intentionally NOT trusted by the prefix probe. */
    const size_t start=20u+(extra?4u:0u);
    const size_t size=start+8u+18u;
    memset(bytes,0,80u);
    memcpy(bytes,opening,16u);
    bytes[16]=8u;
    /* BS 510 stored as 00 + raw LE short (0x01FE). */
    bit_set(bytes+start,0u,2u,0u);
    bit_set(bytes+start,2u,8u,0xfeu);
    bit_set(bytes+start,10u,8u,1u);
    bit_set(bytes+start,18u,8u,0u); /* RC reserved */
    bit_set(bytes+start,26u,8u,0u); /* RC reserved */
    bit_set(bytes+start,34u,1u,1u); /* required B */
    memcpy(bytes+size-16u,closing,16u);
    return size;
}
static int test_layout(int extra){
    _Alignas(MusaDwgAc1024ClassesPrefix) uint8_t bytes[80];
    const size_t n=fixture(bytes,extra);
    uint8_t untouched[80];
    memcpy(untouched,bytes,sizeof(bytes));
    MusaDwgAc1024ClassesPrefix info={0};
    if(!musa_dwg_ac1024_classes_prefix_shape_probe(
         bytes,n,(uint8_t)extra,&info)||
       info.declared_data_area_bytes!=8u||
       info.maximum_class_number!=510u||
       info.data_start_byte!=20u+(extra?4u:0u)||
       info.records_start_bit!=35u||
       info.extra_rl_present!=(uint8_t)extra||
       info.prefix_shape_verified!=1u||
       info.original_section_authenticated||
       info.class_name_resolved||info.display_accepted||
       memcmp(bytes,untouched,sizeof(bytes)))return 0;
    const MusaDwgAc1024ClassesPrefix saved=info;
    const size_t start=20u+(extra?4u:0u);
#define REFUSED() do {if(musa_dwg_ac1024_classes_prefix_shape_probe(\
    bytes,n,(uint8_t)extra,&info)||\
    memcmp(&saved,&info,sizeof(saved)))return 0;} while(0)
    bytes[0]^=1u;REFUSED();bytes[0]^=1u;
    bytes[n-1u]^=1u;REFUSED();bytes[n-1u]^=1u;
    bytes[16]=0u;REFUSED();bytes[16]=8u;
    bit_set(bytes+start,34u,1u,0u);REFUSED();
    bit_set(bytes+start,34u,1u,1u);
    bit_set(bytes+start,18u,8u,1u);REFUSED();
    bit_set(bytes+start,18u,8u,0u);
    bit_set(bytes+start,2u,8u,0xf3u); /* declared max 499 */
    REFUSED();bit_set(bytes+start,2u,8u,0xfeu);
    if(extra){bytes[20]=1u;REFUSED();bytes[20]=0u;}
    if(musa_dwg_ac1024_classes_prefix_shape_probe(bytes,n,
            (uint8_t)(1-extra),&info)||
       musa_dwg_ac1024_classes_prefix_shape_probe(bytes,n+1u,
            (uint8_t)extra,&info)||
       musa_dwg_ac1024_classes_prefix_shape_probe(bytes,n-1u,
            (uint8_t)extra,&info)||
       musa_dwg_ac1024_classes_prefix_shape_probe(bytes,n,
            2u,&info)||
       musa_dwg_ac1024_classes_prefix_shape_probe(bytes,n,
            (uint8_t)extra,NULL)||
       musa_dwg_ac1024_classes_prefix_shape_probe(bytes,n,
            (uint8_t)extra,
            (MusaDwgAc1024ClassesPrefix *)(void *)bytes)||
       memcmp(&saved,&info,sizeof(saved))||
       memcmp(bytes,untouched,sizeof(bytes)))return 0;
#undef REFUSED
    return 1;
}

static size_t numeric_fixture(uint8_t *bytes,int extra) {
    const size_t start=20u+(extra?4u:0u);
    const size_t size=start+16u+18u;
    memset(bytes,0,80u);
    memcpy(bytes,opening,16u);
    bytes[16]=16u;
    /* Class maximum 505. Pre-class preamble starts at area bit 0. */
    bit_set(bytes+start,0u,2u,0u);
    bit_set(bytes+start,2u,8u,0xf9u);
    bit_set(bytes+start,10u,8u,1u);
    bit_set(bytes+start,34u,1u,1u);
    /* First and only numeric class record (no Unicode names). */
    bit_set(bytes+start,35u,2u,0u);   /* BS class 505 */
    bit_set(bytes+start,37u,8u,0xf9u);
    bit_set(bytes+start,45u,8u,1u);
    bit_set(bytes+start,53u,2u,2u);   /* BS proxy flags=0 */
    bit_set(bytes+start,55u,1u,0u);   /* not zombie */
    bit_set(bytes+start,56u,2u,0u);   /* BS itemclassid=0x1f3 */
    bit_set(bytes+start,58u,8u,0xf3u);
    bit_set(bytes+start,66u,8u,1u);
    bit_set(bytes+start,74u,2u,2u);   /* BL instances=0 */
    bit_set(bytes+start,76u,2u,1u);   /* BS version=24 */
    bit_set(bytes+start,78u,8u,24u);
    bit_set(bytes+start,86u,2u,2u);   /* BS maint=0 */
    bit_set(bytes+start,88u,2u,2u);   /* BL unknown1=0 */
    bit_set(bytes+start,90u,2u,2u);   /* BL unknown2=0 */
    memcpy(bytes+size-16u,closing,16u);
    return size;
}
static int test_numeric(int extra) {
    _Alignas(MusaDwgAc1024ClassNumeric) uint8_t bytes[80];
    const size_t len=numeric_fixture(bytes,extra);
    uint8_t saved_source[80];
    memcpy(saved_source,bytes,sizeof(bytes));
    size_t count=97u;
    if(!musa_dwg_ac1024_classes_numeric_records(
            bytes,len,(uint8_t)extra,NULL,0u,&count)||count!=1u)
        return 0;
    MusaDwgAc1024ClassNumeric records[2]={{0}};
    if(!musa_dwg_ac1024_classes_numeric_records(
            bytes,len,(uint8_t)extra,records,2u,&count)||
       count!=1u||records[0].class_number!=505u||
       records[0].proxy_flags!=0u||
       records[0].item_class_id!=0x1f3u||
       records[0].object_instance_count!=0u||
       records[0].dwg_version!=24u||
       records[0].maintenance_version!=0u||
       records[0].unknown1||records[0].unknown2||
       records[0].was_proxy||
       records[0].original_section_authenticated||
       records[0].class_name_resolved||
       records[0].display_accepted||
       memcmp(saved_source,bytes,sizeof(bytes)))
        return 0;
    const MusaDwgAc1024ClassNumeric saved=records[0];
    const size_t start=20u+(extra?4u:0u);
#define REJECT() do{\
    if(musa_dwg_ac1024_classes_numeric_records(\
       bytes,len,(uint8_t)extra,records,2u,&count)||\
       count!=1u||memcmp(&records[0],&saved,sizeof(saved)))return 0;\
    }while(0)
    bit_set(bytes+start,58u,8u,0xf1u); /* invalid itemclassid 497 */
    REJECT();bit_set(bytes+start,58u,8u,0xf3u);
    bit_set(bytes+start,37u,8u,0xf8u); /* class 504, below max 505 */
    REJECT();bit_set(bytes+start,37u,8u,0xf9u);
    bit_set(bytes+start,88u,2u,3u); /* reserved BL 11 */
    REJECT();bit_set(bytes+start,88u,2u,2u);
    bytes[16]=8u;REJECT();bytes[16]=16u;
    if(musa_dwg_ac1024_classes_numeric_records(
          bytes,len,(uint8_t)extra,records,0u,&count)||
       musa_dwg_ac1024_classes_numeric_records(
          bytes,len,(uint8_t)extra,NULL,1u,&count)||
       musa_dwg_ac1024_classes_numeric_records(
          bytes,len,(uint8_t)extra,records,2u,NULL)||
       musa_dwg_ac1024_classes_numeric_records(
          bytes,len,(uint8_t)extra,
          (MusaDwgAc1024ClassNumeric *)(void *)bytes,1u,&count)||
       count!=1u||memcmp(&records[0],&saved,sizeof(saved))||
       memcmp(saved_source,bytes,sizeof(bytes)))
        return 0;
#undef REJECT
    return 1;
}
int main(void){
    if(!test_layout(0))return 11;
    if(!test_layout(1))return 12;
    if(!test_numeric(0))return 13;
    if(!test_numeric(1))return 14;
    puts("AC1024 Classes numeric prefix only; no CRC, names or source acceptance");
    return 0;
}
