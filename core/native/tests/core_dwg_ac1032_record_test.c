/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_record.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void put_crc(uint8_t *p,size_t n){
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,p,n,&crc));
    p[n]=(uint8_t)crc;
    p[n+1u]=(uint8_t)(crc>>8u);
}
int main(void){
    /* Original R2018 layout MS(payload_bytes), unsigned MC(handle_bits),
     * remaining data, CRC16. A source-indexed start lies after initial
     * 4-byte logical Objects section marker. */
    uint8_t data[64]={0};
    data[0]=0xcau;data[1]=0x0du;
    data[4]=3u;data[5]=0u; /* 3 payload bytes */
    data[6]=8u;data[7]=0x51u;data[8]=0xa1u; /* 8-bit handle stream */
    put_crc(data+4u,5u);
    MusaDwgAc1032RecordEnvelope out={0};
    assert(musa_dwg_ac1032_record_envelope_bytes(data,sizeof(data),42u,4u,&out));
    assert(out.indexed_handle==42u&&out.object_section_offset==4u);
    assert(out.payload_bytes==3u&&out.handle_stream_bits==8u);
    assert(out.ms_field_bytes==2u&&out.mc_field_bytes==1u);
    assert(out.total_record_bytes==7u&&out.record_crc_verified==1u);
    const MusaDwgAc1032RecordEnvelope saved=out;
    data[9]^=1u;
    assert(!musa_dwg_ac1032_record_envelope_bytes(data,sizeof(data),42u,4u,&out));
    data[9]^=1u;
    data[5]=0x80u;
    assert(!musa_dwg_ac1032_record_envelope_bytes(data,sizeof(data),42u,4u,&out));
    data[5]=0u;
    assert(!musa_dwg_ac1032_record_envelope_bytes(data,10u,42u,4u,&out));
    assert(!musa_dwg_ac1032_record_envelope_bytes(data,sizeof(data),0u,4u,&out));
    assert(!musa_dwg_ac1032_record_envelope_bytes(data,sizeof(data),42u,63u,&out));
    assert(!musa_dwg_ac1032_record_envelope_bytes(data,sizeof(data),42u,4u,
      (MusaDwgAc1032RecordEnvelope*)(void*)(data+16u)));
    assert(!memcmp(&out,&saved,sizeof(out)));
    /* Canonical two-word MS (32769 bytes) proves 15+15 bits, not
     * two generic little-endian uint16 fields. */
    const size_t biglen=32769u;
    uint8_t *big=calloc(biglen+6u,1u);
    assert(big);
    big[0]=0x01u;big[1]=0x80u;big[2]=0x01u;big[3]=0x00u;
    big[4]=0x08u;
    put_crc(big,4u+biglen);
    assert(musa_dwg_ac1032_record_envelope_bytes(big,biglen+6u,
                                                 123u,0u,&out));
    assert(out.payload_bytes==biglen&&out.ms_field_bytes==4u);
    big[3]=0x80u;
    assert(!musa_dwg_ac1032_record_envelope_bytes(big,biglen+6u,
                                                  123u,0u,&out));
    free(big);
    return 0;
}
