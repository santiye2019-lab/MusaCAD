/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void){
    const uint8_t sample[]="123456789";
    uint16_t result=0u;
    assert(musa_core_crc16_arc(0u,sample,9u,&result));
    assert(result==0xbb3du);
    /* Independent published R13-R15 section CRC seed on a known vector. */
    assert(musa_core_crc16_arc(0xc0c1u,sample,9u,&result));
    assert(result==0x7e6cu); /* Standard CRC-16/ARC reference vector */
    assert(musa_core_crc16_arc(0u,NULL,0u,&result)&&result==0u);
    assert(musa_core_crc16_arc(0x1234u,NULL,0u,&result)&&result==0x1234u);
    result=0x7777u;
    assert(!musa_core_crc16_arc(0u,NULL,1u,&result)&&result==0x7777u);
    assert(!musa_core_crc16_arc(0u,sample,9u,NULL));
    const uint8_t binary[]={0,255,1,2,128,0,25,99,42,0xfe,0xc3};
    uint16_t expected=0u;
    assert(musa_core_crc16_arc(0u,binary,sizeof(binary),&expected));
    for(size_t split=0u;split<=sizeof(binary);split++){
        uint16_t rolling=0u;
        assert(musa_core_crc16_arc(0u,binary,split,&rolling));
        assert(musa_core_crc16_arc(rolling,binary+split,
                                  sizeof(binary)-split,&rolling));
        assert(rolling==expected);
    }
    assert(binary[0]==0u&&binary[1]==255u&&binary[10]==0xc3u);
    puts("PASS: portable CRC-16/ARC standard vector, chunked values and guarded failure");
    return 0;
}
