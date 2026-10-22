/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_line_fields.h"
#include "musacad_dwg_ac1015_values.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
#define LINE_PAYLOAD_MAX (64u*1024u*1024u)
static int overlap(const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
int musa_dwg_ac1032_line_fields_at(
    const uint8_t *data,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geom_at,
    MusaDwgAc1032LineFields *out){
    if(!data||!layout||!out||!bytes||bytes>LINE_PAYLOAD_MAX||
       overlap(data,bytes,layout,sizeof(*layout))||
       overlap(data,bytes,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),out,sizeof(*out))||
       !layout->original_record_crc_verified||!layout->indexed_handle||
       layout->raw_object_type!=19u||layout->object_type_field_bits!=10u||
       layout->total_payload_bits!=(uint64_t)bytes*8u||
       layout->handle_stream_start_bit!=layout->data_stream_bits||
       !layout->handle_stream_bits||
       layout->handle_stream_bits!=layout->total_payload_bits-layout->data_stream_bits||
       layout->data_stream_bits<layout->object_type_field_bits||
       layout->data_stream_bits>SIZE_MAX||
       geom_at<layout->object_type_field_bits||
       (uint64_t)geom_at>=layout->data_stream_bits)return 0;
    /* R2018 and R2000 share the documented RD/DD/BT/BE field coding.
     * Reuse the existing first-party bounded bit numeric grammar ONLY.
     * Do NOT use the R2000 object/ENTITY-prefix grammar on modern data. */
    MusaDwgAc1015Bits b={0};
    if(!musa_dwg_ac1015_bits_init(
        &b,data,bytes,geom_at,(size_t)layout->data_stream_bits))return 0;
    MusaDwgAc1032LineFields candidate={0};
    candidate.indexed_handle=layout->indexed_handle;
    candidate.geometry_start_bit=(uint64_t)geom_at;
    uint32_t z_zero=0u;
    if(!musa_dwg_ac1015_bits_read_u(&b,1u,&z_zero)||
       !musa_dwg_ac1015_bits_read_rd(&b,&candidate.start[0])||
       !musa_dwg_ac1015_bits_read_dd(&b,candidate.start[0],&candidate.end[0])||
       !musa_dwg_ac1015_bits_read_rd(&b,&candidate.start[1])||
       !musa_dwg_ac1015_bits_read_dd(&b,candidate.start[1],&candidate.end[1]))
        return 0;
    if(!z_zero&&
       (!musa_dwg_ac1015_bits_read_rd(&b,&candidate.start[2])||
        !musa_dwg_ac1015_bits_read_dd(&b,candidate.start[2],&candidate.end[2])))
        return 0;
    if(!musa_dwg_ac1015_bits_read_bt(&b,&candidate.thickness)||
       !musa_dwg_ac1015_bits_read_be(&b,candidate.extrusion))return 0;
    for(unsigned k=0;k<3u;k++){
        if(!isfinite(candidate.start[k])||
           !isfinite(candidate.end[k])||
           !isfinite(candidate.extrusion[k]))return 0;
    }
    if(!isfinite(candidate.thickness)||
       (candidate.extrusion[0]==0.0&&candidate.extrusion[1]==0.0&&
        candidate.extrusion[2]==0.0))return 0;
    candidate.geometry_end_bit=(uint64_t)b.bit_pos;
    candidate.coordinates_decoded=1u;
    *out=candidate;return 1;
}
