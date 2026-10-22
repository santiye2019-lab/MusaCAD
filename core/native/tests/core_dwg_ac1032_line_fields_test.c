/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_line_fields.h"
#include "musacad_dwg_ac1032_object_streams.h"
#include "musacad_crc16.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
/* Independent FIELD fixture. 10-bit R2018 raw LINE type, 16-bit
 * placeholder common header (real prefix is NOT claimed), followed by
 * genuine R2000+ LINE RD/DD/BT/BE grammar and one handle-stream byte.
 */
typedef struct Writer { uint8_t b[120];size_t bits; } Writer;
static void bits(Writer *w,unsigned width,uint32_t value){
    assert(width>0u&&width<=32u&&w->bits+width<sizeof(w->b)*8u);
    for(unsigned i=0;i<width;i++){
        unsigned n=width-i-1u;
        size_t at=w->bits++;
        if((value>>n)&1u)w->b[at/8u]|=(uint8_t)(1u<<(7u-(at%8u)));
    }
}
static void rd(Writer *w,double value){
    uint64_t raw=0u;memcpy(&raw,&value,sizeof(raw));
    for(unsigned i=0;i<8u;i++)bits(w,8u,(uint32_t)(raw>>(i*8u))&255u);
}
static size_t object(uint8_t raw[160],int three_d){
    Writer w={0};bits(&w,2u,0u);bits(&w,8u,19u);
    bits(&w,16u,0xAC42u); /* stand-in unparsed entity prefix */
    bits(&w,1u,(uint32_t)(three_d?0:1));
    rd(&w,4.0);          /* start X */
    bits(&w,2u,3u);rd(&w,9.0); /* DD end X full */
    rd(&w,6.0);          /* start Y */
    bits(&w,2u,0u);      /* end Y defaults to 6 */
    if(three_d){
        rd(&w,2.0);bits(&w,2u,3u);rd(&w,5.0);
    }
    bits(&w,1u,1u); /* default thickness zero */
    bits(&w,1u,1u); /* default OCS normal (0,0,1) */
    const size_t data_bytes=(w.bits+7u)/8u;
    w.b[data_bytes]=0xA6u; /* handle tail */
    const size_t payload=data_bytes+1u;
    assert(payload<120u);
    memset(raw,0,160u);
    raw[0]=(uint8_t)payload;raw[1]=(uint8_t)(payload>>8u);
    raw[2]=8u;
    memcpy(raw+3u,w.b,payload);
    uint16_t crc=0u;
    assert(musa_core_crc16_arc(0xc0c1u,raw,payload+3u,&crc));
    raw[payload+3u]=(uint8_t)crc;
    raw[payload+4u]=(uint8_t)(crc>>8u);
    return payload+5u;
}
int main(void){
    uint8_t raw[160];
    size_t n=object(raw,0);
    MusaDwgAc1032ObjectStreams layout={0};
    assert(musa_dwg_ac1032_stream_layout_bytes(raw,n,7u,0u,&layout));
    assert(layout.raw_object_type==19u);
    MusaDwgAc1032LineFields line={0};
    assert(musa_dwg_ac1032_line_fields_at(
        raw+3u,layout.total_payload_bits/8u,&layout,26u,&line));
    assert(line.indexed_handle==7u&&line.start[0]==4.0&&line.end[0]==9.0);
    assert(line.start[1]==6.0&&line.end[1]==6.0);
    assert(line.start[2]==0.0&&line.end[2]==0.0);
    assert(line.extrusion[2]==1.0&&line.thickness==0.0);
    const MusaDwgAc1032LineFields saved=line;
    assert(!musa_dwg_ac1032_line_fields_at(
        raw+3u,layout.total_payload_bits/8u,&layout,layout.data_stream_bits-8u,&line));
    assert(!memcmp(&saved,&line,sizeof(line)));
    n=object(raw,1);
    assert(musa_dwg_ac1032_stream_layout_bytes(raw,n,7u,0u,&layout));
    assert(musa_dwg_ac1032_line_fields_at(
        raw+3u,layout.total_payload_bits/8u,&layout,26u,&line));
    assert(line.start[2]==2.0&&line.end[2]==5.0);
    const MusaDwgAc1032LineFields saved_3d=line;
    layout.raw_object_type=18u;
    assert(!musa_dwg_ac1032_line_fields_at(
        raw+3u,layout.total_payload_bits/8u,&layout,26u,&line));
    layout.raw_object_type=19u;
    assert(!musa_dwg_ac1032_line_fields_at(
        raw+3u,layout.total_payload_bits/8u-1u,&layout,26u,&line));
    assert(!musa_dwg_ac1032_line_fields_at(
        raw+3u,layout.total_payload_bits/8u,&layout,26u,
        (MusaDwgAc1032LineFields *)(void*)(raw+3u)));
    assert(!memcmp(&saved_3d,&line,sizeof(line)));
    return 0;
}
