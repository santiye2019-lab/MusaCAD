/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_line.h"
#include "musacad_dwg_ac1015_object.h"
#include "musacad_crc16.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST(x) do {if(!(x)){fprintf(stderr,"FAIL %s @%d\n",#x,__LINE__);exit(1);}}while(0)
enum { FILE_SIZE=512, DATA_START=130 };
typedef struct Writer{uint8_t bytes[256];size_t bit;} Writer;
static void put_bit(Writer *w,unsigned b){
    const size_t at=w->bit++;TEST(at<sizeof(w->bytes)*8u);
    if(b)w->bytes[at/8u]|=(uint8_t)(1u<<(7u-at%8u));
}
static void put_u(Writer *w,uint32_t v,unsigned width){
    TEST(width>0u&&width<=32u);
    for(unsigned i=0u;i<width;i++)put_bit(w,(v>>(width-i-1u))&1u);
}
static void put_raw(Writer *w,uint64_t v,unsigned n){
    for(unsigned i=0u;i<n;i++)put_u(w,(uint8_t)(v>>(8u*i)),8u);
}
static void rewrite_bit(Writer *w,size_t p,unsigned b){
    TEST(p<sizeof(w->bytes)*8u);
    const uint8_t mask=(uint8_t)(1u<<(7u-p%8u));
    if(b)w->bytes[p/8u]|=mask;else w->bytes[p/8u]&=(uint8_t)~mask;
}
static void rewrite_rl(Writer *w,size_t at,uint32_t n){
    for(unsigned i=0u;i<4u;i++){
        const uint8_t v=(uint8_t)(n>>(i*8u));
        for(unsigned bit=0u;bit<8u;bit++)
            rewrite_bit(w,at+(size_t)i*8u+bit,(v>>(7u-bit))&1u);
    }
}
static uint64_t double_bits(double d){uint64_t v=0;memcpy(&v,&d,sizeof(v));return v;}
static void put_rd(Writer *w,double d){put_raw(w,double_bits(d),8u);}
static void put_bd(Writer *w,double d){
    if(d==0.0){put_u(w,2u,2u);return;}
    if(d==1.0){put_u(w,1u,2u);return;}
    put_u(w,0u,2u);put_rd(w,d);
}
static void put_dd(Writer *w,double d){put_u(w,3u,2u);put_rd(w,d);}
static void make_fixture(int nonzero_z,int bad_extrusion,
                         uint8_t file[FILE_SIZE],size_t *field_start){
    Writer w={0};
    /* Real object envelope: DWG BS type 19 + RL pre-handle bit boundary. */
    put_u(&w,1u,2u);put_raw(&w,19u,1u);
    const size_t rl_at=w.bit;put_raw(&w,0u,4u);
    /* Seven sentinel bits emulate independently consumed common metadata.
     * Their format is deliberately NOT asserted to be R2000 common data. */
    put_u(&w,0x53u,7u);
    *field_start=w.bit;
    put_u(&w,nonzero_z?0u:1u,1u);
    put_rd(&w,-101.25);
    if(nonzero_z)put_dd(&w,302.75);
    else put_u(&w,0u,2u); /* default end X = start X */
    put_rd(&w,55.125);
    put_dd(&w,-18.75);
    if(nonzero_z){
        put_rd(&w,7.5);
        put_u(&w,0u,2u);/* default end Z = start Z */
    }
    if(nonzero_z){
        put_u(&w,0u,1u);put_bd(&w,0.75); /* BT thickness */
        put_u(&w,0u,1u);
        put_bd(&w,0.0);put_bd(&w,0.0);
        put_bd(&w,bad_extrusion?0.0:1.0);
    }else {
        put_u(&w,1u,1u); /* zero thickness */
        put_u(&w,1u,1u); /* default extrusion */
    }
    const size_t end=w.bit;
    TEST(end<=UINT32_MAX);
    rewrite_rl(&w,rl_at,(uint32_t)end);
    /* Leave at least one handle-stream byte after the exact data end. */
    put_u(&w,0xa5u,8u);
    const size_t bytes=(w.bit+7u)/8u;
    TEST(bytes<0x8000u&&bytes+DATA_START+2u<FILE_SIZE);
    memset(file,0,FILE_SIZE);memcpy(file,"AC1015",6u);
    file[128]=(uint8_t)bytes;file[129]=(uint8_t)(bytes>>8u);
    memcpy(file+DATA_START,w.bytes,bytes);
    uint16_t crc=0u;
    TEST(musa_core_crc16_arc(0xc0c1u,file+128,bytes+2u,&crc));
    file[DATA_START+bytes]=(uint8_t)crc;
    file[DATA_START+bytes+1u]=(uint8_t)(crc>>8u);
}
static MusaDwgAc1015ObjectRecord read_record(uint8_t file[FILE_SIZE]){
    const MusaDwgAc1015HandleEntry entry={0x3au,128u};
    MusaDwgAc1015ObjectRecord r={0};
    TEST(musa_dwg_ac1015_read_object_record(file,FILE_SIZE,&entry,&r));
    TEST(r.record_crc_verified==1&&r.type_code==19u&&r.geometry_decoded==0);
    return r;
}
static void assert_reject(const MusaDwgAc1015ObjectRecord *r,size_t first){
    MusaDwgAc1015LineFields out={0};
    out.start[0]=998.0;out.end[1]=-444.0;out.indexed_handle=37u;
    const MusaDwgAc1015LineFields prior=out;
    TEST(!musa_dwg_ac1015_decode_line_fields(r,first,&out));
    TEST(memcmp(&prior,&out,sizeof(out))==0);
}
int main(void){
    uint8_t file[FILE_SIZE],untouched[FILE_SIZE];
    size_t offset=0u;
    make_fixture(0,0,file,&offset);memcpy(untouched,file,sizeof(file));
    MusaDwgAc1015ObjectRecord r=read_record(file);
    MusaDwgAc1015LineFields line={0};
    TEST(musa_dwg_ac1015_decode_line_fields(&r,offset,&line));
    TEST(line.start[0]==-101.25&&line.end[0]==-101.25);
    TEST(line.start[1]==55.125&&line.end[1]==-18.75);
    TEST(line.start[2]==0.0&&line.end[2]==0.0);
    TEST(line.thickness==0.0&&line.extrusion[0]==0.0);
    TEST(line.extrusion[1]==0.0&&line.extrusion[2]==1.0);
    TEST(line.field_layout_decoded==1&&line.first_field_bit==offset);
    TEST(line.end_field_bit==r.pre_handle_bits&&line.indexed_handle==r.indexed_handle);
    TEST(r.geometry_decoded==0&&memcmp(file,untouched,sizeof(file))==0);

    MusaDwgAc1015ObjectRecord copy=r;
    copy.type_code=18u;assert_reject(&copy,offset);
    copy=r;copy.record_crc_verified=0;assert_reject(&copy,offset);
    copy=r;copy.pre_handle_bits-=1u;assert_reject(&copy,offset);
    copy=r;copy.pre_handle_bits+=1u;assert_reject(&copy,offset);
    copy=r;copy.pre_handle_bits=UINT32_MAX;assert_reject(&copy,offset);
    copy=r;copy.indexed_handle=0u;assert_reject(&copy,offset);
    copy=r;copy.data=NULL;assert_reject(&copy,offset);
    assert_reject(&r,(size_t)r.pre_handle_bits+1u);
    assert_reject(&r,offset+1u);

    make_fixture(1,0,file,&offset);r=read_record(file);
    TEST(musa_dwg_ac1015_decode_line_fields(&r,offset,&line));
    TEST(line.start[0]==-101.25&&line.end[0]==302.75);
    TEST(line.start[1]==55.125&&line.end[1]==-18.75);
    TEST(line.start[2]==7.5&&line.end[2]==7.5);
    TEST(line.thickness==0.75&&line.extrusion[2]==1.0);
    TEST(line.end_field_bit==r.pre_handle_bits);
    make_fixture(1,1,file,&offset);r=read_record(file);
    assert_reject(&r,offset); /* invalid zero extrusion */
    puts("PASS first-party R2000 LINE RD/DD XYZ, default-Z, BT/BE, CRC and fail-closed field boundary");
    return 0;
}
