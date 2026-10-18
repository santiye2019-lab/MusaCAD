/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_object.h"
#include "musacad_dwg_signature.h"
#include "musacad_crc16.h"
#include "musacad_dwg_ac1015_values.h"
#include <stddef.h>
#include <stdint.h>

/* AC1015 (R2000) raw object envelope, independently based on published
 * ODA DWG specification: MS object length, BS type, RL pre-handle bit
 * length, trailing RS CRC. No third-party decoder implementation.
 *
 * MS uses little-endian 16-bit units with bit 15 as continuation.
 * At most 2 units (30 payload bits) are valid, no overlong encodings. */
static int read_ms(const uint8_t *src,size_t available,
                   size_t *nbytes,size_t *length){
    if(!src||!nbytes||!length||available<2u)return 0;
    const uint16_t lo=(uint16_t)src[0]|((uint16_t)src[1]<<8);
    size_t n=2u;
    uint32_t value=(uint32_t)(lo&0x7fffu);
    if((lo&0x8000u)!=0u){
        if(available<4u)return 0;
        const uint16_t hi=(uint16_t)src[2]|((uint16_t)src[3]<<8);
        if((hi&0x8000u)!=0u||hi==0u)return 0;
        value|=((uint32_t)hi<<15);
        n=4u;
    }
    if(value<7u)return 0; /* 2-bit type+raw 32-bit pre-handle length. */
    *nbytes=n;*length=(size_t)value;return 1;
}
/* The same public read-only compressed DWG cursor is shared by the object
 * envelope and future geometric field decoders. Do not decode geometry from
 * an unverified record or assume the common entity prefix was consumed. */
static int read_type_and_endbit(const uint8_t *data,size_t n,
                                uint16_t *type,uint32_t *endbit){
    MusaDwgAc1015Bits bits={0};
    uint16_t raw_type=0u;
    uint32_t raw_endbit=0u;
    if(!data||!type||!endbit||n>SIZE_MAX/8u||
       !musa_dwg_ac1015_bits_init(&bits,data,n,0u,n*8u)||
       !musa_dwg_ac1015_bits_read_bs(&bits,&raw_type)||
       !musa_dwg_ac1015_bits_read_rl(&bits,&raw_endbit)||
       (uint64_t)raw_endbit>(uint64_t)n*8u)return 0;
    *type=raw_type;*endbit=raw_endbit;return 1;
}
int musa_dwg_ac1015_read_object_record(
    const uint8_t *file,size_t file_bytes,
    const MusaDwgAc1015HandleEntry *entry,
    MusaDwgAc1015ObjectRecord *out){
    if(!file||!entry||!out||entry->handle==0u||
       file_bytes<8u ||
       musa_core_dwg_identify_signature(file,file_bytes)!=MUSA_DWG_2000||
       entry->file_offset>=(uint64_t)file_bytes)return 0;
    const size_t offset=(size_t)entry->file_offset;
    const size_t available=file_bytes-offset;
    size_t size_bytes=0u,data_bytes=0u;
    if(!read_ms(file+offset,available,&size_bytes,&data_bytes)||
       available<size_bytes+2u||
       data_bytes>available-size_bytes-2u)return 0;
    const size_t crc_pos=offset+size_bytes+data_bytes;
    uint16_t crc=0u;
    if(!musa_core_crc16_arc(0xc0c1u,file+offset,
                            size_bytes+data_bytes,&crc)||
       crc!=((uint16_t)file[crc_pos]|
             ((uint16_t)file[crc_pos+1u]<<8)))return 0;
    uint16_t type=0u;
    uint32_t endbit=0u;
    if(!read_type_and_endbit(file+offset+size_bytes,data_bytes,
                             &type,&endbit))return 0;
    MusaDwgAc1015ObjectRecord candidate={0};
    candidate.indexed_handle=entry->handle;
    candidate.absolute_offset=entry->file_offset;
    candidate.data=file+offset+size_bytes;
    candidate.data_bytes=data_bytes;
    candidate.size_field_bytes=size_bytes;
    candidate.total_record_bytes=size_bytes+data_bytes+2u;
    candidate.type_code=type;
    candidate.pre_handle_bits=endbit;
    candidate.record_crc_verified=1;
    candidate.geometry_decoded=0;
    *out=candidate;
    return 1;
}
