/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
static void sample(uint8_t raw[32]){
    memset(raw,0xEE,32u);
    raw[4]=3u;raw[5]=0u; /* MS: three payload bytes */
    raw[6]=8u; /* MC: 8 handle-stream bits */
    raw[7]=0x0au;raw[8]=0x80u;raw[9]=0x42u; /* OT 42 */
    uint16_t crc=0;
    assert(musa_core_crc16_arc(0xc0c1u,raw+4u,6u,&crc));
    raw[10]=(uint8_t)crc;raw[11]=(uint8_t)(crc>>8u);
}
int main(void){
    uint8_t source[32],dest[8];
    sample(source);memset(dest,0xBD,sizeof(dest));
    MusaDwgAc1032PayloadInfo info={0};
    size_t need=0u;
    assert(musa_dwg_ac1032_object_payload_bytes(
        source,sizeof(source),7u,4u,NULL,0u,&need,&info));
    assert(need==3u&&info.payload_bytes==3u&&info.handle_stream_bits==8u);
    assert(info.raw_object_type==42u&&info.object_type_field_bits==10u);
    assert(info.original_record_crc_verified==1u);
    assert(info.indexed_handle==7u&&info.object_section_offset==4u);
    assert(musa_dwg_ac1032_object_payload_bytes(
        source,sizeof(source),7u,4u,dest,sizeof(dest),&need,&info));
    assert(dest[0]==0x0au&&dest[1]==0x80u&&dest[2]==0x42u);
    for(size_t i=3u;i<sizeof(dest);i++)assert(dest[i]==0xBDu);
    const MusaDwgAc1032PayloadInfo saved=info;
    const size_t saved_need=need;
    uint8_t snapshot[8];memcpy(snapshot,dest,sizeof(snapshot));
    assert(!musa_dwg_ac1032_object_payload_bytes(
        source,sizeof(source),7u,4u,dest,2u,&need,&info));
    source[11]^=1u;
    assert(!musa_dwg_ac1032_object_payload_bytes(
        source,sizeof(source),7u,4u,dest,sizeof(dest),&need,&info));
    source[11]^=1u;
    assert(!musa_dwg_ac1032_object_payload_bytes(
        source,11u,7u,4u,dest,sizeof(dest),&need,&info));
    assert(!musa_dwg_ac1032_object_payload_bytes(
        source,sizeof(source),0u,4u,dest,sizeof(dest),&need,&info));
    assert(!musa_dwg_ac1032_object_payload_bytes(
        source,sizeof(source),7u,4u,source+13u,3u,&need,&info));
    assert(!musa_dwg_ac1032_object_payload_bytes(
        source,sizeof(source),7u,4u,dest,sizeof(dest),
        (size_t *)(void*)dest,&info));
    assert(!musa_dwg_ac1032_object_payload_bytes(
        source,sizeof(source),7u,4u,dest,sizeof(dest),&need,
        (MusaDwgAc1032PayloadInfo *)(void*)dest));
    assert(!memcmp(dest,snapshot,sizeof(dest)));
    assert(!memcmp(&info,&saved,sizeof(saved))&&need==saved_need);
    return 0;
}
