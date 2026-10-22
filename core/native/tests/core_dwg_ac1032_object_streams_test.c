/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_object_streams.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
/* SOURCE: original R2018 MS/MC/OT envelope using three payload octets.
 * DATA 16 bits: [OT=00+0x30, remaining 6 bits]
 * HANDLES 8 bits: A6 (padding is not silently removed).
 */
static void sample(uint8_t raw[32]){
    memset(raw,0xEE,32u);
    raw[4]=3u;raw[5]=0u;raw[6]=8u;
    raw[7]=0x0cu;raw[8]=0x00u;raw[9]=0xA6u;
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,raw+4u,6u,&crc));
    raw[10]=(uint8_t)crc;raw[11]=(uint8_t)(crc>>8u);
}
int main(void){
    uint8_t src[32];sample(src);
    MusaDwgAc1032ObjectStreams layout={0};
    assert(musa_dwg_ac1032_stream_layout_bytes(
        src,sizeof(src),7u,4u,&layout));
    assert(layout.indexed_handle==7u&&layout.object_section_offset==4u);
    assert(layout.raw_object_type==48u&&layout.object_type_field_bits==10u);
    assert(layout.original_record_crc_verified&&layout.total_payload_bits==24u);
    assert(layout.data_stream_bits==16u&&layout.handle_stream_start_bit==16u);
    assert(layout.handle_stream_bits==8u);
    uint32_t value=0u;
    assert(musa_dwg_ac1032_stream_read_bits(src+7u,3u,
        &layout,MUSA_AC1032_STREAM_OBJECT_DATA,0u,10u,&value));
    assert(value==48u);
    assert(musa_dwg_ac1032_stream_read_bits(src+7u,3u,
        &layout,MUSA_AC1032_STREAM_HANDLES,0u,8u,&value));
    assert(value==0xA6u);
    assert(musa_dwg_ac1032_stream_read_bits(src+7u,3u,
        &layout,MUSA_AC1032_STREAM_HANDLES,4u,4u,&value));
    assert(value==6u);
    const uint32_t snapshot=value;
    const MusaDwgAc1032ObjectStreams layout_before=layout;
    assert(!musa_dwg_ac1032_stream_read_bits(src+7u,3u,
        &layout,MUSA_AC1032_STREAM_OBJECT_DATA,14u,3u,&value));
    assert(!musa_dwg_ac1032_stream_read_bits(src+7u,3u,
        &layout,MUSA_AC1032_STREAM_HANDLES,1u,8u,&value));
    assert(!musa_dwg_ac1032_stream_read_bits(src+7u,2u,
        &layout,MUSA_AC1032_STREAM_HANDLES,0u,8u,&value));
    assert(!musa_dwg_ac1032_stream_read_bits(src+7u,3u,
        &layout,3u,0u,8u,&value));
    assert(!musa_dwg_ac1032_stream_read_bits(src+7u,3u,
        &layout,MUSA_AC1032_STREAM_OBJECT_DATA,0u,33u,&value));
    assert(!musa_dwg_ac1032_stream_read_bits(src+7u,3u,
        &layout,MUSA_AC1032_STREAM_OBJECT_DATA,0u,1u,
        (uint32_t*)(void*)(src+8u)));
    src[11]^=1u;
    assert(!musa_dwg_ac1032_stream_layout_bytes(
        src,sizeof(src),7u,4u,&layout));
    src[11]^=1u;
    assert(!musa_dwg_ac1032_stream_layout_bytes(
        src,11u,7u,4u,&layout));
    assert(!musa_dwg_ac1032_stream_layout_bytes(
        src,sizeof(src),7u,4u,
        (MusaDwgAc1032ObjectStreams*)(void*)(src+12u)));
    assert(value==snapshot&&
       !memcmp(&layout,&layout_before,sizeof(layout)));
    return 0;
}
