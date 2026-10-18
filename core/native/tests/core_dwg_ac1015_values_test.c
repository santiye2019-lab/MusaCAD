/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_values.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s line %d\n",#x,__LINE__);exit(1);}}while(0)
typedef struct Writer{uint8_t data[256];size_t pos;} Writer;
static void put_u(Writer *w,uint32_t v,unsigned width){
    CHECK(width>0u&&width<=32u&&w->pos+width<=sizeof(w->data)*8u);
    for(unsigned i=0u;i<width;i++){
        const size_t p=w->pos++;
        if((v>>(width-1u-i))&1u)w->data[p/8u]|=(uint8_t)(1u<<(7u-(p%8u)));
    }
}
static void put_raw(Writer *w,uint64_t v,unsigned bytes){
    for(unsigned i=0;i<bytes;i++)put_u(w,(uint8_t)(v>>(8u*i)),8u);
}
static uint64_t double_bits(double x){uint64_t v=0u;memcpy(&v,&x,8u);return v;}
static void put_rd(Writer *w,double d){put_raw(w,double_bits(d),8u);}
static MusaDwgAc1015Bits read_from(const Writer *w,size_t start,size_t end){
    MusaDwgAc1015Bits c={0};
    CHECK(musa_dwg_ac1015_bits_init(&c,w->data,sizeof(w->data),start,end));
    return c;
}
static void integer_cases(void){
    Writer w={0};put_u(&w,5u,3u);
    put_u(&w,0u,2u);put_raw(&w,0xffffu,2u); /* BS raw negative */
    put_u(&w,1u,2u);put_raw(&w,255u,1u);
    put_u(&w,2u,2u);put_u(&w,3u,2u);
    put_u(&w,0u,2u);put_raw(&w,0xfffffffeu,4u); /* BL raw negative */
    put_u(&w,1u,2u);put_raw(&w,17u,1u);
    put_u(&w,2u,2u);
    const size_t invalid_bl=w.pos;put_u(&w,3u,2u);
    put_raw(&w,0x89abcdefu,4u);/* following RL */
    MusaDwgAc1015Bits c=read_from(&w,3u,w.pos);
    uint16_t s=0u;uint32_t l=0u;
    CHECK(musa_dwg_ac1015_bits_read_bs(&c,&s)&&s==0xffffu);
    CHECK(musa_dwg_ac1015_bits_read_bs(&c,&s)&&s==255u);
    CHECK(musa_dwg_ac1015_bits_read_bs(&c,&s)&&s==0u);
    CHECK(musa_dwg_ac1015_bits_read_bs(&c,&s)&&s==256u);
    CHECK(musa_dwg_ac1015_bits_read_bl(&c,&l)&&l==0xfffffffeu);
    CHECK(musa_dwg_ac1015_bits_read_bl(&c,&l)&&l==17u);
    CHECK(musa_dwg_ac1015_bits_read_bl(&c,&l)&&l==0u);
    CHECK(c.bit_pos==invalid_bl);
    const size_t old=c.bit_pos;l=0x13579bdfu;
    CHECK(!musa_dwg_ac1015_bits_read_bl(&c,&l));
    CHECK(c.bit_pos==old&&l==0x13579bdfu);
    uint32_t bits=0xaaaa5555u;
    CHECK(!musa_dwg_ac1015_bits_read_u(&c,33u,&bits));
    CHECK(c.bit_pos==old&&bits==0xaaaa5555u);
    c.bit_pos+=2u;
    CHECK(musa_dwg_ac1015_bits_read_rl(&c,&l)&&l==0x89abcdefu);
    CHECK(c.bit_pos==w.pos);
    CHECK(!musa_dwg_ac1015_bits_read_rl(&c,&l));
    CHECK(l==0x89abcdefu&&c.bit_pos==w.pos);
}
static void coordinate_cases(void){
    Writer w={0};put_u(&w,7u,3u);
    /* R2000 LINE-specific coordinate fields following a hypothetical,
     * independently verified entity-common-data cursor. These are NOT
     * full DWG LINE records and do not establish geometry acceptance. */
    put_u(&w,1u,1u); /* both Z coordinates zero */
    put_rd(&w,101.25);
    put_u(&w,3u,2u);put_rd(&w,302.75); /* DD full */
    put_rd(&w,-23.5);
    put_u(&w,0u,2u); /* DD default = starting Y */
    put_u(&w,1u,1u); /* BT thickness default 0 */
    put_u(&w,1u,1u); /* BE extrusion default 0,0,1 */
    /* BD: zero, one, raw, invalid */
    put_u(&w,2u,2u);put_u(&w,1u,2u);
    put_u(&w,0u,2u);put_rd(&w,-19.125);
    const size_t invalid_bd=w.pos;put_u(&w,3u,2u);
    /* DD mode 01 changes four low bytes, default upper bytes */
    const double def=11.25;
    const uint64_t baseline=double_bits(def);
    put_u(&w,1u,2u);put_raw(&w,0x01020304u,4u);
    const uint64_t expected1=(baseline&UINT64_C(0xffffffff00000000))|0x01020304u;
    /* DD mode 10: first 2 bytes patch LE positions 4,5 then 4 bytes 0..3 */
    put_u(&w,2u,2u);put_raw(&w,0x0040u,2u);put_raw(&w,0x12345678u,4u);
    const uint64_t expected2=(baseline&UINT64_C(0xffff000000000000))|
                             UINT64_C(0x0000004000000000)|UINT64_C(0x12345678);
    MusaDwgAc1015Bits c=read_from(&w,3u,w.pos);
    uint32_t b=0u;double v=-999.0;double xyz[3]={7,8,9};
    CHECK(musa_dwg_ac1015_bits_read_u(&c,1u,&b)&&b==1u);
    CHECK(musa_dwg_ac1015_bits_read_rd(&c,&v)&&v==101.25);
    CHECK(musa_dwg_ac1015_bits_read_dd(&c,v,&v)&&v==302.75);
    CHECK(musa_dwg_ac1015_bits_read_rd(&c,&v)&&v==-23.5);
    CHECK(musa_dwg_ac1015_bits_read_dd(&c,v,&v)&&v==-23.5);
    CHECK(musa_dwg_ac1015_bits_read_bt(&c,&v)&&v==0.0);
    CHECK(musa_dwg_ac1015_bits_read_be(&c,xyz));
    CHECK(xyz[0]==0.0&&xyz[1]==0.0&&xyz[2]==1.0);
    CHECK(musa_dwg_ac1015_bits_read_bd(&c,&v)&&v==0.0);
    CHECK(musa_dwg_ac1015_bits_read_bd(&c,&v)&&v==1.0);
    CHECK(musa_dwg_ac1015_bits_read_bd(&c,&v)&&v==-19.125);
    CHECK(c.bit_pos==invalid_bd);
    v=123.0;CHECK(!musa_dwg_ac1015_bits_read_bd(&c,&v));
    CHECK(c.bit_pos==invalid_bd&&v==123.0);
    c.bit_pos+=2u;
    CHECK(musa_dwg_ac1015_bits_read_dd(&c,def,&v));
    CHECK(double_bits(v)==expected1);
    CHECK(musa_dwg_ac1015_bits_read_dd(&c,def,&v));
    CHECK(double_bits(v)==expected2);
    CHECK(c.bit_pos==w.pos);
}
static void rejection_cases(void){
    Writer w={0};
    put_u(&w,0u,2u);put_raw(&w,UINT64_C(0x7ff0000000000000),8u);
    MusaDwgAc1015Bits c=read_from(&w,0u,w.pos);
    const size_t pos=c.bit_pos;double v=42.0;
    CHECK(!musa_dwg_ac1015_bits_read_bd(&c,&v));
    CHECK(v==42.0&&c.bit_pos==pos);
    MusaDwgAc1015Bits sentinel={0};
    CHECK(!musa_dwg_ac1015_bits_init(&sentinel,w.data,SIZE_MAX,0u,0u));
    CHECK(sentinel.data==NULL&&sentinel.bit_pos==0u);
    CHECK(!musa_dwg_ac1015_bits_init(&sentinel,w.data,sizeof(w.data),11u,10u));
    CHECK(sentinel.data==NULL);
    CHECK(!musa_dwg_ac1015_bits_init(&sentinel,w.data,sizeof(w.data),0u,sizeof(w.data)*8u+1u));
    CHECK(sentinel.data==NULL);
    CHECK(!musa_dwg_ac1015_bits_init(&sentinel,NULL,1u,0u,1u));
    Writer e={0};
    put_u(&e,0u,1u); /* BE explicit: 0, 0, 0 => rejected */
    put_u(&e,2u,2u);put_u(&e,2u,2u);put_u(&e,2u,2u);
    c=read_from(&e,0u,e.pos);
    double xyz[3]={3.0,4.0,5.0};
    CHECK(!musa_dwg_ac1015_bits_read_be(&c,xyz));
    CHECK(c.bit_pos==0u&&xyz[0]==3.0&&xyz[1]==4.0&&xyz[2]==5.0);
    w=(Writer){0};put_u(&w,0u,2u);put_raw(&w,double_bits(7.5),8u);
    c=read_from(&w,0u,w.pos-1u);
    CHECK(!musa_dwg_ac1015_bits_read_bd(&c,&v));
    CHECK(c.bit_pos==0u&&v==42.0);
    /* Caller limits prevent a valid trailing record from being consumed. */
    CHECK(!musa_dwg_ac1015_bits_read_u(&c,32u,NULL));
}
int main(void){
    integer_cases();coordinate_cases();rejection_cases();
    puts("PASS: first-party R2000 BS/BL/RL/RD/BD/DD/BT/BE field decoder, unaligned and rollback");
    return 0;
}
