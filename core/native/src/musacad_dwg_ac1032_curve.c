/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_curve.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1015_values.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MAX_OBJECT_BYTES (64u*1024u*1024u)
#define MAX_DWG_BYTES (256u*1024u*1024u)
static int overlap(const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
int musa_dwg_ac1032_curve_fields_at(
    const uint8_t *raw,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geom,
    MusaDwgAc1032NativeCurve *out){
    if(!raw||!layout||!out||!bytes||bytes>MAX_OBJECT_BYTES||
       overlap(raw,bytes,layout,sizeof(*layout))||
       overlap(raw,bytes,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),out,sizeof(*out))||
       !layout->original_record_crc_verified||
       !layout->indexed_handle||
       (layout->raw_object_type!=17u&&layout->raw_object_type!=18u)||
       layout->object_type_field_bits!=10u||
       layout->total_payload_bits!=(uint64_t)bytes*8u||
       layout->handle_stream_start_bit!=layout->data_stream_bits||
       layout->data_stream_bits>SIZE_MAX||
       !layout->handle_stream_bits||
       layout->handle_stream_bits!=
          layout->total_payload_bits-layout->data_stream_bits||
       geom<layout->object_type_field_bits||
       (uint64_t)geom>=layout->data_stream_bits)return 0;
    MusaDwgAc1015Bits b={0};
    if(!musa_dwg_ac1015_bits_init(
           &b,raw,bytes,geom,(size_t)layout->data_stream_bits))return 0;
    MusaDwgAc1032NativeCurve candidate={0};
    candidate.indexed_handle=layout->indexed_handle;
    candidate.object_type=layout->raw_object_type;
    for(unsigned i=0u;i<3u;i++)
        if(!musa_dwg_ac1015_bits_read_bd(&b,&candidate.center[i])||
           !isfinite(candidate.center[i]))return 0;
    if(!musa_dwg_ac1015_bits_read_bd(&b,&candidate.radius)||
       !isfinite(candidate.radius)||candidate.radius<=0.0||
       !musa_dwg_ac1015_bits_read_bt(&b,&candidate.thickness)||
       !musa_dwg_ac1015_bits_read_be(&b,candidate.extrusion)||
       !isfinite(candidate.thickness))return 0;
    for(unsigned i=0u;i<3u;i++)
        if(!isfinite(candidate.extrusion[i]))return 0;
    if(candidate.object_type==17u){
        if(!musa_dwg_ac1015_bits_read_bd(&b,&candidate.start_angle)||
           !musa_dwg_ac1015_bits_read_bd(&b,&candidate.end_angle)||
           !isfinite(candidate.start_angle)||
           !isfinite(candidate.end_angle))return 0;
    }
    candidate.geometry_end_bit=b.bit_pos;
    candidate.geometry_decoded=1u;
    *out=candidate;
    return 1;
}
int musa_dwg_ac1032_read_curve_file(
    const uint8_t *file,size_t n,uint64_t handle,
    MusaDwgAc1032NativeCurve *out){
    if(!file||!out||!handle||n<256u||n>MAX_DWG_BYTES||
       overlap(file,n,out,sizeof(*out))||
       memcmp(file,"AC1032",6u)!=0)return 0;
    MusaDwgAc1032PayloadInfo meta={0};
    size_t required=0u;
    if(!musa_dwg_ac1032_object_payload_file(
          file,n,handle,NULL,0u,&required,&meta)||
       !meta.original_record_crc_verified||
       meta.indexed_handle!=handle||
       (meta.raw_object_type!=17u&&meta.raw_object_type!=18u)||
       meta.object_type_field_bits!=10u||
       required==0u||required>MAX_OBJECT_BYTES||
       required!=meta.payload_bytes)return 0;
    MusaDwgAc1032ObjectStreams layout={0};
    if(!musa_dwg_ac1032_stream_layout_file(
          file,n,handle,&layout)||
       !layout.original_record_crc_verified||
       layout.indexed_handle!=meta.indexed_handle||
       layout.object_section_offset!=meta.object_section_offset||
       layout.raw_object_type!=meta.raw_object_type||
       layout.object_type_field_bits!=meta.object_type_field_bits||
       layout.total_payload_bits!=(uint64_t)required*8u||
       layout.handle_stream_bits!=meta.handle_stream_bits)return 0;
    uint8_t *raw=(uint8_t*)malloc(required);
    if(!raw)return 0;
    size_t copied=0u;
    MusaDwgAc1032PayloadInfo copied_info={0};
    MusaDwgAc1032NativeCurve candidate={0};
    int ok=musa_dwg_ac1032_object_payload_file(
          file,n,handle,raw,required,&copied,&copied_info)&&
       copied==required&&
       copied_info.indexed_handle==meta.indexed_handle&&
       copied_info.object_section_offset==meta.object_section_offset&&
       copied_info.raw_object_type==meta.raw_object_type&&
       copied_info.handle_stream_bits==meta.handle_stream_bits&&
       copied_info.original_record_crc_verified&&
       musa_dwg_ac1032_entity_common_bytes(
          raw,required,&layout,meta.raw_object_type,&candidate.common)&&
       candidate.common.common_decoded&&
       candidate.common.prefix.indexed_handle==handle&&
       candidate.common.prefix.raw_object_type==meta.raw_object_type;
    if(ok){
        /* Do not hand fields_at the candidate.common as the out buffer:
         * it is a larger object; snapshot the common into the final
         * candidate only after successful geometry. */
        MusaDwgAc1032NativeCurve fields={0};
        ok=musa_dwg_ac1032_curve_fields_at(raw,required,&layout,
                   candidate.common.geometry_start_bit,&fields);
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

/* Original AC1032 ELLIPSE OT35: six 3BD vectors' raw source components
 * (center, semi-major, extrusion) followed by BD ratio/start/end.
 * Common prefix, OT, H-index, CRC and DATA/H partitions prevalidated. */
int musa_dwg_ac1032_ellipse_fields_at(
    const uint8_t *raw,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geom,
    MusaDwgAc1032NativeEllipse *out){
    if(!raw||!layout||!out||!bytes||bytes>MAX_OBJECT_BYTES||
       overlap(raw,bytes,layout,sizeof(*layout))||
       overlap(raw,bytes,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),out,sizeof(*out))||
       !layout->original_record_crc_verified||!layout->indexed_handle||
       layout->raw_object_type!=35u||layout->object_type_field_bits!=10u||
       layout->total_payload_bits!=(uint64_t)bytes*8u||
       layout->handle_stream_start_bit!=layout->data_stream_bits||
       !layout->handle_stream_bits||
       layout->handle_stream_bits!=layout->total_payload_bits-layout->data_stream_bits||
       layout->data_stream_bits>SIZE_MAX||
       geom<layout->object_type_field_bits||
       (uint64_t)geom>=layout->data_stream_bits)return 0;
    MusaDwgAc1015Bits b={0};
    if(!musa_dwg_ac1015_bits_init(
        &b,raw,bytes,geom,(size_t)layout->data_stream_bits))return 0;
    MusaDwgAc1032NativeEllipse value={0};
    value.indexed_handle=layout->indexed_handle;
    for(unsigned k=0;k<3u;k++)
        if(!musa_dwg_ac1015_bits_read_bd(&b,&value.center[k])||
           !isfinite(value.center[k]))return 0;
    for(unsigned k=0;k<3u;k++)
        if(!musa_dwg_ac1015_bits_read_bd(&b,&value.semi_major[k])||
           !isfinite(value.semi_major[k]))return 0;
    for(unsigned k=0;k<3u;k++)
        if(!musa_dwg_ac1015_bits_read_bd(&b,&value.extrusion[k])||
           !isfinite(value.extrusion[k]))return 0;
    if(!musa_dwg_ac1015_bits_read_bd(&b,&value.ratio)||
       !musa_dwg_ac1015_bits_read_bd(&b,&value.start_angle)||
       !musa_dwg_ac1015_bits_read_bd(&b,&value.end_angle)||
       !isfinite(value.ratio)||value.ratio<=0.0||value.ratio>1.0||
       !isfinite(value.start_angle)||!isfinite(value.end_angle)||
       hypot(value.semi_major[0],value.semi_major[1])<=0.0||
       b.bit_pos>layout->data_stream_bits)return 0;
    value.geometry_end_bit=b.bit_pos;
    value.geometry_decoded=1u;
    *out=value;return 1;
}
