/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_point.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1015_values.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MUSA_POINT_OBJECT_MAX (64u*1024u*1024u)
#define MUSA_POINT_FILE_MAX (256u*1024u*1024u)
static int overlap(const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
int musa_dwg_ac1032_point_fields_at(
    const uint8_t *raw,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geom,
    MusaDwgAc1032NativePoint *out){
    if(!raw||!layout||!out||!bytes||bytes>MUSA_POINT_OBJECT_MAX||
       overlap(raw,bytes,layout,sizeof(*layout))||
       overlap(raw,bytes,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),out,sizeof(*out))||
       !layout->original_record_crc_verified||
       !layout->indexed_handle||layout->raw_object_type!=27u||
       layout->object_type_field_bits!=10u||
       layout->total_payload_bits!=(uint64_t)bytes*8u||
       layout->handle_stream_start_bit!=layout->data_stream_bits||
       layout->data_stream_bits>SIZE_MAX||
       !layout->handle_stream_bits||
       layout->handle_stream_bits!=layout->total_payload_bits-layout->data_stream_bits||
       geom<layout->object_type_field_bits||
       (uint64_t)geom>=layout->data_stream_bits)return 0;
    MusaDwgAc1015Bits bits={0};
    if(!musa_dwg_ac1015_bits_init(&bits,raw,bytes,geom,
       (size_t)layout->data_stream_bits))return 0;
    MusaDwgAc1032NativePoint candidate={0};
    candidate.indexed_handle=layout->indexed_handle;
    for(unsigned i=0u;i<3u;i++){
        if(!musa_dwg_ac1015_bits_read_bd(&bits,&candidate.point[i])||
           !isfinite(candidate.point[i]))return 0;
    }
    if(!musa_dwg_ac1015_bits_read_bt(&bits,&candidate.thickness)||
       !musa_dwg_ac1015_bits_read_be(&bits,candidate.extrusion)||
       !musa_dwg_ac1015_bits_read_bd(&bits,&candidate.x_axis_angle)||
       !isfinite(candidate.thickness)||
       !isfinite(candidate.x_axis_angle))return 0;
    for(unsigned i=0u;i<3u;i++)
        if(!isfinite(candidate.extrusion[i]))return 0;
    candidate.geometry_end_bit=(uint64_t)bits.bit_pos;
    candidate.geometry_decoded=1u;
    *out=candidate;return 1;
}
int musa_dwg_ac1032_read_point_file(
    const uint8_t *file,size_t n,uint64_t handle,
    MusaDwgAc1032NativePoint *out){
    if(!file||!out||!handle||n<256u||n>MUSA_POINT_FILE_MAX||
       overlap(file,n,out,sizeof(*out))||
       memcmp(file,"AC1032",6u)!=0)return 0;
    size_t needed=0u;
    MusaDwgAc1032PayloadInfo meta={0};
    if(!musa_dwg_ac1032_object_payload_file(
       file,n,handle,NULL,0u,&needed,&meta)||
       !meta.original_record_crc_verified||
       meta.indexed_handle!=handle||
       meta.raw_object_type!=27u||meta.object_type_field_bits!=10u||
       needed==0u||needed>MUSA_POINT_OBJECT_MAX||
       needed!=meta.payload_bytes)return 0;
    MusaDwgAc1032ObjectStreams layout={0};
    if(!musa_dwg_ac1032_stream_layout_file(
       file,n,handle,&layout)||
       !layout.original_record_crc_verified||
       layout.indexed_handle!=handle||
       layout.object_section_offset!=meta.object_section_offset||
       layout.raw_object_type!=meta.raw_object_type||
       layout.object_type_field_bits!=meta.object_type_field_bits||
       layout.total_payload_bits!=(uint64_t)needed*8u||
       layout.handle_stream_bits!=meta.handle_stream_bits)return 0;
    uint8_t *raw=(uint8_t*)malloc(needed);
    if(!raw)return 0;
    size_t copied=0u;
    MusaDwgAc1032PayloadInfo proof={0};
    MusaDwgAc1032NativePoint candidate={0};
    int ok=musa_dwg_ac1032_object_payload_file(
       file,n,handle,raw,needed,&copied,&proof)&&
       copied==needed&&
       proof.original_record_crc_verified&&
       proof.indexed_handle==meta.indexed_handle&&
       proof.object_section_offset==meta.object_section_offset&&
       proof.payload_bytes==meta.payload_bytes&&
       proof.handle_stream_bits==meta.handle_stream_bits&&
       proof.raw_object_type==27u&&
       musa_dwg_ac1032_entity_common_bytes(
          raw,needed,&layout,27u,&candidate.common)&&
       candidate.common.common_decoded&&
       candidate.common.prefix.indexed_handle==handle;
    if(ok){
        MusaDwgAc1032NativePoint fields={0};
        ok=musa_dwg_ac1032_point_fields_at(
           raw,needed,&layout,candidate.common.geometry_start_bit,&fields);
        if(ok){
            fields.common=candidate.common;
            candidate=fields;
        }
    }
    free(raw);
    if(!ok||!candidate.geometry_decoded||
       candidate.indexed_handle!=handle||
       candidate.geometry_end_bit>layout.data_stream_bits)return 0;
    *out=candidate;
    return 1;
}
