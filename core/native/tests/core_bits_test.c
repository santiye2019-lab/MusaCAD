/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_bits.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void){
    uint8_t data[1024];
    memset(data,0xaau,sizeof(data));
    MusaBits w={0},r={0};
    assert(musa_bits_writer(&w,data,sizeof(data)));
    assert(musa_bits_write(&w,3u,5u));
    assert(musa_bits_write(&w,2u,1u));
    assert((data[0]&31u)==13u); /* 101 + 01 in LSB-first bit order */
    assert(musa_bits_align_byte(&w));
    assert(w.bit_position==8u);
    assert(musa_bits_write(&w,64u,UINT64_MAX));
    assert(musa_bits_writer(&w,data,sizeof(data))); /* overwrite later */
    assert(musa_bits_write(&w,3u,5u));
    assert(musa_bits_write(&w,2u,1u));
    assert(musa_bits_align_byte(&w));
    assert(musa_bits_write(&w,64u,UINT64_MAX));
    const uint64_t varints[]={0u,1u,127u,128u,16384u,UINT64_MAX};
    for(size_t i=0;i<sizeof(varints)/sizeof(varints[0]);i++)
        assert(musa_bits_write_uleb128(&w,varints[i]));
    const size_t written=w.bit_position;
    assert(musa_bits_reader(&r,data,sizeof(data)));
    uint64_t out=99u;
    assert(musa_bits_read(&r,3u,&out)&&out==5u);
    assert(musa_bits_read(&r,2u,&out)&&out==1u);
    assert(musa_bits_align_byte(&r));
    assert(musa_bits_read(&r,64u,&out)&&out==UINT64_MAX);
    for(size_t i=0;i<sizeof(varints)/sizeof(varints[0]);i++)
        assert(musa_bits_read_uleb128(&r,&out)&&out==varints[i]);
    assert(r.bit_position==written);
    /* Every bit width up to 64: roundtrip deterministic values and offsets. */
    uint8_t random_bytes[1024]={0};
    assert(musa_bits_writer(&w,random_bytes,sizeof(random_bytes)));
    uint64_t expect[128]={0};
    for(unsigned bits=1u;bits<=64u;bits++){
        uint64_t v=UINT64_C(0x9e3779b97f4a7c15)^(UINT64_C(0xd6e8feb86659fd93)*(uint64_t)bits);
        if(bits<64u)v&=UINT64_MAX>>(64u-bits);
        expect[bits]=v;
        assert(musa_bits_write(&w,bits,v));
    }
    size_t end=w.bit_position;
    assert(musa_bits_reader(&r,random_bytes,sizeof(random_bytes)));
    for(unsigned bits=1u;bits<=64u;bits++){
        assert(musa_bits_read(&r,bits,&out));
        assert(out==expect[bits]);
    }
    assert(r.bit_position==end);
    /* Invalid/truncated reads and writes preserve cursor and the buffer. */
    uint8_t small[]={0x55u,0xaau};
    uint8_t before[2];memcpy(before,small,sizeof(small));
    assert(musa_bits_writer(&w,small,2u));
    assert(musa_bits_seek(&w,15u));
    size_t pos=w.bit_position;
    assert(!musa_bits_write(&w,2u,3u)&&w.bit_position==pos);
    assert(!musa_bits_write(&w,1u,2u)&&w.bit_position==pos);
    assert(!musa_bits_write(&w,65u,0u)&&w.bit_position==pos);
    assert(!musa_bits_seek(&w,17u)&&w.bit_position==pos);
    assert(!musa_bits_write_uleb128(&w,128u)&&w.bit_position==pos);
    assert(memcmp(small,before,sizeof(small))==0);
    assert(musa_bits_reader(&r,small,sizeof(small)));
    assert(musa_bits_seek(&r,15u));
    out=777u;pos=r.bit_position;
    assert(!musa_bits_read(&r,2u,&out)&&out==777u&&r.bit_position==pos);
    assert(!musa_bits_read_uleb128(&r,&out)&&out==777u&&r.bit_position==pos);
    uint8_t overlong[]={0x80u,0x00u};
    assert(musa_bits_reader(&r,overlong,sizeof(overlong)));
    assert(!musa_bits_read_uleb128(&r,&out)&&r.bit_position==0u);
    uint8_t too_long[10];memset(too_long,0xff,sizeof(too_long));
    assert(musa_bits_reader(&r,too_long,sizeof(too_long)));
    assert(!musa_bits_read_uleb128(&r,&out)&&r.bit_position==0u);
    uint8_t overflow[10];memset(overflow,0x80,sizeof(overflow));overflow[9]=2u;
    assert(musa_bits_reader(&r,overflow,sizeof(overflow)));
    assert(!musa_bits_read_uleb128(&r,&out)&&r.bit_position==0u);
    assert(!musa_bits_reader(&r,NULL,1u));
    assert(!musa_bits_writer(&w,NULL,1u));
    assert(!musa_bits_reader(&r,small,SIZE_MAX));
    assert(!musa_bits_writer(&w,small,SIZE_MAX));
    assert(musa_bits_reader(&r,NULL,0u));
    assert(musa_bits_remaining(&r)==0u);
    puts("PASS: portable first-party bit cursor, 1..64 bits, canonical varints, bounds and rollback");
    return 0;
}
