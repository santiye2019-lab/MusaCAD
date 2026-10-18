/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_frame.h"
#include "musacad_dwg_ac1015.h"
#include "musacad_crc16.h"
#include <stdint.h>
#include <string.h>
/* Independently transcribed published R13-R15 DWG beginning sentinels.
 * Ending sentinels are their bytewise complements. These are format
 * constants and no external CAD decoder/runtime is used. */
static const uint8_t header_start[16]={
    0xcfu,0x7bu,0x1fu,0x23u,0xfdu,0xdeu,0x38u,0xa9u,
    0x5fu,0x7cu,0x68u,0xb8u,0x4eu,0x6du,0x33u,0x5fu
};
static const uint8_t class_start[16]={
    0x8du,0xa1u,0xc4u,0xb8u,0xc4u,0xa9u,0xf8u,0xc5u,
    0xc0u,0xdcu,0xf4u,0x5fu,0xe7u,0xcfu,0xb6u,0x8au
};
int musa_dwg_ac1015_parse_frame(const uint8_t *file,size_t file_bytes,
                                uint32_t section_kind,MusaDwgAc1015Frame *out){
    if(!file||!out||section_kind>1u)return 0;
    const uint8_t *begin=NULL;
    size_t total=0u;
    if(!musa_dwg_ac1015_readonly_section(file,file_bytes,section_kind,
                                           &begin,&total))return 0;
    /* 16 start + 4 size + 2 stored CRC + 16 end = 38 bytes. */
    if(!begin||total<38u)return 0;
    const uint8_t *sentinel=section_kind==0u?header_start:class_start;
    if(memcmp(begin,sentinel,16u)!=0)return 0;
    const uint32_t length=(uint32_t)begin[16]|
                           ((uint32_t)begin[17]<<8)|
                           ((uint32_t)begin[18]<<16)|
                           ((uint32_t)begin[19]<<24);
    /* Compare using subtraction to avoid overflow on 32-bit targets. */
    if((uint64_t)length!=(uint64_t)(total-38u))return 0;
    const size_t trailer=20u+(size_t)length;
    for(size_t i=0u;i<16u;i++){
        if(begin[trailer+2u+i]!=(uint8_t)(sentinel[i]^0xffu))
            return 0;
    }
    MusaDwgAc1015Frame parsed={0};
    parsed.payload=begin+20u;
    parsed.payload_bytes=(size_t)length;
    parsed.stored_payload_crc_unverified=(uint16_t)begin[trailer]|
                             (uint16_t)((uint16_t)begin[trailer+1u]<<8);
    parsed.section_kind=section_kind;
    parsed.envelope_verified=1;
    parsed.payload_crc_verified=0;
    *out=parsed;
    return 1;
}

/* The R13-R15 AcDb:Header and AcDb:Classes section CRC covers the
 * four-byte little-endian length and all following section data,
 * starting immediately after the 16-byte beginning sentinel.
 * Per the documented DWG format the initial CRC-16/ARC value is 0xC0C1.
 * The already-parsed envelope guarantees length+4 fits the original span.
 */
int musa_dwg_ac1015_parse_crc_verified_frame(const uint8_t *file,
                                               size_t file_bytes,
                                               uint32_t section_kind,
                                               MusaDwgAc1015Frame *out){
    if(!out)return 0;
    MusaDwgAc1015Frame parsed={0};
    if(!musa_dwg_ac1015_parse_frame(file,file_bytes,section_kind,&parsed)||
       parsed.payload_bytes>SIZE_MAX-4u)return 0;
    uint16_t computed=0u;
    if(!musa_core_crc16_arc(0xc0c1u,parsed.payload-4u,
                            parsed.payload_bytes+4u,&computed)||
       computed!=parsed.stored_payload_crc_unverified)return 0;
    parsed.payload_crc_verified=1;
    *out=parsed;
    return 1;
}
