/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_values.h"
#include <float.h>
#include <stdint.h>
#include <string.h>
/* DWG encoded integers are little-endian BYTES, but each byte is traversed
 * most-significant BIT first. This distinction matters for unaligned fields.
 * IEEE-754 is required by the published R2000 RD representation. */
_Static_assert(sizeof(double)==8 && DBL_MANT_DIG==53, "DWG requires binary64");
int musa_dwg_ac1015_bits_init(MusaDwgAc1015Bits *out,
    const uint8_t *data,size_t bytes,size_t start_bit,size_t end_bit){
    if(!out||!data||bytes>SIZE_MAX/8u||
       start_bit>end_bit||end_bit>bytes*8u)return 0;
    const MusaDwgAc1015Bits next={data,bytes,start_bit,end_bit};
    *out=next;return 1;
}
int musa_dwg_ac1015_bits_read_u(MusaDwgAc1015Bits *c,
                                 unsigned bits,uint32_t *out){
    if(!c||!out||!c->data||bits==0u||bits>32u||
       c->size_bytes>SIZE_MAX/8u||
       c->bit_end>c->size_bytes*8u||c->bit_pos>c->bit_end||
       (size_t)bits>c->bit_end-c->bit_pos)return 0;
    uint32_t value=0u;
    for(unsigned i=0u;i<bits;i++){
        const size_t at=c->bit_pos+(size_t)i;
        value=(value<<1u)|((uint32_t)
            ((c->data[at/8u]>>(7u-(at%8u)))&1u));
    }
    *out=value;c->bit_pos+=(size_t)bits;return 1;
}
static int raw_bytes(MusaDwgAc1015Bits *c,unsigned count,uint64_t *out){
    if(!c||!out||count>8u)return 0;
    MusaDwgAc1015Bits next=*c;
    uint64_t value=0u;
    for(unsigned i=0u;i<count;i++){
        uint32_t byte=0u;
        if(!musa_dwg_ac1015_bits_read_u(&next,8u,&byte))return 0;
        value|=((uint64_t)byte)<<(8u*i);
    }
    *out=value;*c=next;return 1;
}
int musa_dwg_ac1015_bits_read_rl(MusaDwgAc1015Bits *c,uint32_t *out){
    uint64_t v=0u;
    if(!out||!raw_bytes(c,4u,&v))return 0;
    *out=(uint32_t)v;return 1;
}
int musa_dwg_ac1015_bits_read_bs(MusaDwgAc1015Bits *c,uint16_t *out){
    if(!c||!out)return 0;
    MusaDwgAc1015Bits next=*c;uint32_t op=0u;uint64_t v=0u;
    if(!musa_dwg_ac1015_bits_read_u(&next,2u,&op))return 0;
    if(op==0u){if(!raw_bytes(&next,2u,&v))return 0;}
    else if(op==1u){if(!raw_bytes(&next,1u,&v))return 0;}
    else v=op==2u?0u:256u;
    *out=(uint16_t)v;*c=next;return 1;
}
int musa_dwg_ac1015_bits_read_bl(MusaDwgAc1015Bits *c,uint32_t *out){
    if(!c||!out)return 0;
    MusaDwgAc1015Bits next=*c;uint32_t op=0u;uint64_t v=0u;
    if(!musa_dwg_ac1015_bits_read_u(&next,2u,&op)||op==3u)return 0;
    if(op==0u){if(!raw_bytes(&next,4u,&v))return 0;}
    else if(op==1u){if(!raw_bytes(&next,1u,&v))return 0;}
    *out=(uint32_t)v;*c=next;return 1;
}
static int from_binary64(uint64_t bits,double *out){
    if(!out||(bits&UINT64_C(0x7ff0000000000000))==
                UINT64_C(0x7ff0000000000000))return 0;
    memcpy(out,&bits,sizeof(bits));return 1;
}
int musa_dwg_ac1015_bits_read_rd(MusaDwgAc1015Bits *c,double *out){
    if(!c||!out)return 0;
    MusaDwgAc1015Bits next=*c;uint64_t bits=0u;double value=0.0;
    if(!raw_bytes(&next,8u,&bits)||!from_binary64(bits,&value))return 0;
    *out=value;*c=next;return 1;
}
int musa_dwg_ac1015_bits_read_bd(MusaDwgAc1015Bits *c,double *out){
    if(!c||!out)return 0;
    MusaDwgAc1015Bits next=*c;uint32_t op=0u;double value=0.0;
    if(!musa_dwg_ac1015_bits_read_u(&next,2u,&op)||op==3u)return 0;
    if(op==0u){if(!musa_dwg_ac1015_bits_read_rd(&next,&value))return 0;}
    else if(op==1u)value=1.0;
    *out=value;*c=next;return 1;
}
int musa_dwg_ac1015_bits_read_dd(MusaDwgAc1015Bits *c,
                                  double base,double *out){
    if(!c||!out)return 0;
    MusaDwgAc1015Bits next=*c;uint32_t op=0u;double value=0.0;
    uint64_t bits=0u;
    memcpy(&bits,&base,sizeof(bits));
    if(!from_binary64(bits,&value)||
       !musa_dwg_ac1015_bits_read_u(&next,2u,&op))return 0;
    if(op==3u){
        if(!musa_dwg_ac1015_bits_read_rd(&next,&value))return 0;
    }else if(op!=0u){
        uint8_t b[8];
        for(unsigned i=0u;i<8u;i++)b[i]=(uint8_t)(bits>>(8u*i));
        if(op==2u){
            uint64_t pair=0u;
            if(!raw_bytes(&next,2u,&pair))return 0;
            b[4]=(uint8_t)pair;b[5]=(uint8_t)(pair>>8u);
        }
        uint64_t first=0u;
        if(!raw_bytes(&next,4u,&first))return 0;
        for(unsigned i=0u;i<4u;i++)b[i]=(uint8_t)(first>>(8u*i));
        bits=0u;
        for(unsigned i=0u;i<8u;i++)bits|=(uint64_t)b[i]<<(8u*i);
        if(!from_binary64(bits,&value))return 0;
    }
    *out=value;*c=next;return 1;
}
int musa_dwg_ac1015_bits_read_bt(MusaDwgAc1015Bits *c,double *out){
    if(!c||!out)return 0;
    MusaDwgAc1015Bits next=*c;uint32_t is_zero=0u;double value=0.0;
    if(!musa_dwg_ac1015_bits_read_u(&next,1u,&is_zero))return 0;
    if(!is_zero&&!musa_dwg_ac1015_bits_read_bd(&next,&value))return 0;
    *out=value;*c=next;return 1;
}
int musa_dwg_ac1015_bits_read_be(MusaDwgAc1015Bits *c,double out_xyz[3]){
    if(!c||!out_xyz)return 0;
    MusaDwgAc1015Bits next=*c;uint32_t is_default=0u;
    double xyz[3]={0.0,0.0,1.0};
    if(!musa_dwg_ac1015_bits_read_u(&next,1u,&is_default))return 0;
    if(!is_default){
        for(unsigned i=0u;i<3u;i++){
            if(!musa_dwg_ac1015_bits_read_bd(&next,&xyz[i]))return 0;
        }
        if(xyz[0]==0.0&&xyz[1]==0.0&&xyz[2]==0.0)return 0;
    }
    memcpy(out_xyz,xyz,sizeof(xyz));*c=next;return 1;
}
