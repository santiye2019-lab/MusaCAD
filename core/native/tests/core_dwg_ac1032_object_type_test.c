/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

/* The object type field is bit-packed: 2-bit OT mode then 8 or
 * 16 source bytes (unaligned) on the authenticated logical object. */
static void prepare(uint8_t buf[32],uint8_t b0,uint8_t b1,uint8_t b2){
    memset(buf,0,32u);
    buf[4]=3u;buf[5]=0u; /* canonical modular short payload size */
    buf[6]=8u; /* unsigned MC handle bit stream size */
    buf[7]=b0;buf[8]=b1;buf[9]=b2;
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,buf+4u,6u,&crc));
    buf[10]=(uint8_t)crc;buf[11]=(uint8_t)(crc>>8u);
}
int main(void){
    uint8_t buf[32];
    MusaDwgAc1032ObjectType ot={0};
    prepare(buf,0x0au,0x80u,0u);  /* 00 + 0x2a => 42 */
    assert(musa_dwg_ac1032_object_type_bytes(buf,sizeof(buf),7u,4u,&ot));
    assert(ot.type_code==42u&&ot.ot_opcode==0u&&ot.type_field_bits==10u);
    assert(ot.indexed_handle==7u&&ot.object_section_offset==4u);
    assert(ot.record_crc_verified==1u);
    prepare(buf,0x42u,0x80u,0u); /* 01 + 0x0a => 0x1fa */
    assert(musa_dwg_ac1032_object_type_bytes(buf,sizeof(buf),7u,4u,&ot));
    assert(ot.type_code==506u&&ot.ot_opcode==1u);
    prepare(buf,0x8du,0x04u,0x80u); /* 10 + raw ushort 0x1234 */
    assert(musa_dwg_ac1032_object_type_bytes(buf,sizeof(buf),7u,4u,&ot));
    assert(ot.type_code==0x1234u&&ot.ot_opcode==2u&&ot.type_field_bits==18u);
    const MusaDwgAc1032ObjectType saved=ot;
    prepare(buf,0xc0u,0u,0u); /* OT 11 is reserved */
    assert(!musa_dwg_ac1032_object_type_bytes(buf,sizeof(buf),7u,4u,&ot));
    prepare(buf,0x0au,0x80u,0u);
    buf[10]^=1u; /* source CRC corruption */
    assert(!musa_dwg_ac1032_object_type_bytes(buf,sizeof(buf),7u,4u,&ot));
    buf[10]^=1u;
    assert(!musa_dwg_ac1032_object_type_bytes(buf,11u,7u,4u,&ot));
    assert(!musa_dwg_ac1032_object_type_bytes(buf,sizeof(buf),0u,4u,&ot));
    assert(!musa_dwg_ac1032_object_type_bytes(buf,sizeof(buf),7u,31u,&ot));
    assert(!musa_dwg_ac1032_object_type_bytes(buf,sizeof(buf),7u,4u,
         (MusaDwgAc1032ObjectType*)(void*)(buf+16u)));
    assert(!memcmp(&ot,&saved,sizeof(saved)));
    return 0;
}
