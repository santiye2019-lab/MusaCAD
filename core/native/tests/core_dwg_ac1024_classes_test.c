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
int main(void){
    if(!test_layout(0))return 11;
    if(!test_layout(1))return 12;
    puts("AC1024 Classes numeric prefix only; no CRC, names or source acceptance");
    return 0;
}
