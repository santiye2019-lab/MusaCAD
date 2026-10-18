/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_object.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static MusaDwgAc1015HandleEntry entry={12u,128u};
/* Independently encode DWG MSB-first bits; RL bytes are LE. */
static void put_bit(uint8_t *p,size_t bit,unsigned value){
    if(value)p[bit/8u]|=(uint8_t)(1u<<(7u-(bit&7u)));
}
static size_t put_bits(uint8_t *p,size_t bit,uint32_t value,unsigned width){
    for(unsigned k=0u;k<width;k++)
        put_bit(p,bit++, (value>>(width-1u-k))&1u);
    return bit;
}
static void put_record(uint8_t *file,size_t capacity,uint32_t length,int extended){
    memset(file,0,capacity);
    memcpy(file,"AC1015",6u);
    uint8_t *p=file+128u;
    const size_t size_bytes=extended?4u:2u;
    p[0]=(uint8_t)length;
    p[1]=(uint8_t)(((length>>8)&0x7fu)|(extended?0x80u:0u));
    if(extended){
        p[2]=(uint8_t)(length>>15);
        p[3]=(uint8_t)(length>>23);
    }
    size_t bit=0u;
    bit=put_bits(p+size_bytes,bit,1u,2u); /* code 01, 8-bit BS */
    bit=put_bits(p+size_bytes,bit,1u,8u); /* type raw byte */
    /* Pre-handle field RL=42, little-endian in successive raw bytes. */
    bit=put_bits(p+size_bytes,bit,42u,8u);
    for(unsigned k=0u;k<3u;k++)bit=put_bits(p+size_bytes,bit,0u,8u);
    assert(bit==42u);
    uint16_t crc=0;
    assert(musa_core_crc16_arc(0xc0c1u,p,size_bytes+length,&crc));
    p[size_bytes+length]=(uint8_t)crc;
    p[size_bytes+length+1u]=(uint8_t)(crc>>8);
}
static void fails_without_touch(uint8_t *file,size_t n){
    MusaDwgAc1015ObjectRecord out={0};
    out.indexed_handle=123u;
    out.absolute_offset=555u;
    out.data=file+1u;
    out.data_bytes=999u;
    out.size_field_bytes=55u;
    out.total_record_bytes=44u;
    out.type_code=987u;
    out.pre_handle_bits=333u;
    out.record_crc_verified=23;
    out.geometry_decoded=42;
    const MusaDwgAc1015ObjectRecord original=out;
    assert(!musa_dwg_ac1015_read_object_record(file,n,&entry,&out));
    assert(memcmp(&original,&out,sizeof(out))==0);
}
int main(void){
    uint8_t file[40000],baseline[40000];
    put_record(file,sizeof(file),12u,0);
    memcpy(baseline,file,sizeof(file));
    MusaDwgAc1015ObjectRecord result={0};
    assert(musa_dwg_ac1015_read_object_record(file,sizeof(file),&entry,&result));
    assert(result.indexed_handle==12u&&result.absolute_offset==128u);
    assert(result.type_code==1u&&result.pre_handle_bits==42u);
    assert(result.data_bytes==12u&&result.size_field_bytes==2u);
    assert(result.total_record_bytes==16u);
    assert(result.data==file+130u&&result.record_crc_verified==1);
    assert(result.geometry_decoded==0);
    assert(memcmp(baseline,file,sizeof(file))==0);
    file[140u]^=1u;fails_without_touch(file,sizeof(file));
    file[140u]^=1u;
    file[128u]=0u;fails_without_touch(file,sizeof(file));
    file[128u]=12u;
    file[129u]=0x80u;fails_without_touch(file,sizeof(file));file[129u]=0u;
    fails_without_touch(file,130u);
    file[0]='X';fails_without_touch(file,sizeof(file));file[0]='A';
    MusaDwgAc1015HandleEntry bad={0u,128u};
    assert(!musa_dwg_ac1015_read_object_record(file,sizeof(file),&bad,&result));
    bad.handle=3u;bad.file_offset=UINT64_MAX;
    assert(!musa_dwg_ac1015_read_object_record(file,sizeof(file),&bad,&result));
    assert(!musa_dwg_ac1015_read_object_record(file,sizeof(file),&entry,NULL));
    /* Two 15-bit modular-short length blocks; 32780 bytes of payload. */
    put_record(file,sizeof(file),32780u,1);
    assert(musa_dwg_ac1015_read_object_record(file,sizeof(file),&entry,&result));
    assert(result.size_field_bytes==4u&&result.data_bytes==32780u);
    file[131u]=0x80u; /* third modular-short continuation: invalid */
    fails_without_touch(file,sizeof(file));
    puts("PASS: AC1015 object MS/BS/RL header, CRC, 2+4 byte length and failure rollback");
    return 0;
}
