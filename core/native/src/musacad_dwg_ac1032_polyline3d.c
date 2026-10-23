/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_polyline3d.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1015_values.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MAX_DWG_BYTES (64u*1024u*1024u)
#define MAX_OBJECT_BYTES (64u*1024u*1024u)
#define MAX_OWNED_VERTICES (100000u)
static int overlap(const void *p,size_t pn,const void *q,size_t qn){
    if(!pn||!qn)return 0;
    const uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
    return a<=b?b-a<pn:a-b<qn;
}
static int check_fields(const uint8_t *raw,size_t bytes,
     const MusaDwgAc1032ObjectStreams *layout,size_t geom,
     uint16_t type,const void *out,size_t out_bytes){
    return raw&&layout&&out&&bytes&&bytes<=MAX_OBJECT_BYTES&&
       !overlap(raw,bytes,layout,sizeof(*layout))&&
       !overlap(raw,bytes,out,out_bytes)&&
       !overlap(layout,sizeof(*layout),out,out_bytes)&&
       layout->original_record_crc_verified&&
       layout->indexed_handle&&layout->raw_object_type==type&&
       layout->object_type_field_bits==10u&&
       layout->total_payload_bits==(uint64_t)bytes*8u&&
       layout->data_stream_bits<=SIZE_MAX&&
       layout->handle_stream_start_bit==layout->data_stream_bits&&
       layout->handle_stream_bits&&
       layout->handle_stream_bits==
         layout->total_payload_bits-layout->data_stream_bits&&
       geom>=10u&&(uint64_t)geom<layout->data_stream_bits;
}
int musa_dwg_ac1032_polyline3d_header_fields_at(
    const uint8_t *raw,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geom,
    MusaDwgAc1032Polyline3DHeader *out){
    if(!check_fields(raw,bytes,layout,geom,16u,out,sizeof(*out)))
        return 0;
    MusaDwgAc1015Bits b={0};
    if(!musa_dwg_ac1015_bits_init(&b,raw,bytes,geom,
           (size_t)layout->data_stream_bits))return 0;
    MusaDwgAc1032Polyline3DHeader result={0};
    uint32_t f1=0u,f2=0u;
    if(!musa_dwg_ac1015_bits_read_u(&b,8u,&f1)||
       !musa_dwg_ac1015_bits_read_u(&b,8u,&f2)||
       (f1&~3u)!=0u||(f2&~1u)!=0u||
       !musa_dwg_ac1015_bits_read_bl(
          &b,&result.owned_object_count)||
       result.owned_object_count<2u||
       result.owned_object_count>MAX_OWNED_VERTICES)return 0;
    result.spline_flags=(uint8_t)f1;
    result.polyline_flags=(uint8_t)f2;
    result.closed=(uint8_t)(f2&1u);
    result.indexed_handle=layout->indexed_handle;
    result.geometry_end_bit=(uint64_t)b.bit_pos;
    result.header_decoded=1u;
    *out=result;
    return 1;
}
int musa_dwg_ac1032_vertex3d_fields_at(
    const uint8_t *raw,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geom,
    MusaDwgAc1032Vertex3D *out){
    if(!check_fields(raw,bytes,layout,geom,11u,out,sizeof(*out)))
        return 0;
    MusaDwgAc1015Bits b={0};
    if(!musa_dwg_ac1015_bits_init(&b,raw,bytes,geom,
           (size_t)layout->data_stream_bits))return 0;
    MusaDwgAc1032Vertex3D result={0};
    uint32_t flags=0u;
    if(!musa_dwg_ac1015_bits_read_u(&b,8u,&flags))return 0;
    result.flags=(uint8_t)flags;
    for(unsigned i=0u;i<3u;i++)
       if(!musa_dwg_ac1015_bits_read_bd(&b,&result.point[i])||
          !isfinite(result.point[i]))return 0;
    result.indexed_handle=layout->indexed_handle;
    result.geometry_end_bit=(uint64_t)b.bit_pos;
    result.coordinates_decoded=1u;
    *out=result;
    return 1;
}
/* Authenticated source extraction is shared by both R2018 record types.
 * Original H, OT, CRC, DATA/HANDLES layout and actual source-derived
 * entity attributes all must agree. No source mutation or guessed offsets.
 */
static int read_common(const uint8_t *file,size_t n,uint64_t h,
    uint16_t type,uint8_t **payload,size_t *bytes,
    MusaDwgAc1032ObjectStreams *layout,
    MusaDwgAc1032EntityCommon *common){
    if(!file||!payload||!bytes||!layout||!common||
       !h||n<256u||n>MAX_DWG_BYTES||
       memcmp(file,"AC1032",6u)!=0)return 0;
    size_t needed=0u;
    MusaDwgAc1032PayloadInfo meta={0};
    if(!musa_dwg_ac1032_object_payload_file(
       file,n,h,NULL,0u,&needed,&meta)||
       !meta.original_record_crc_verified||
       meta.indexed_handle!=h||meta.raw_object_type!=type||
       meta.object_type_field_bits!=10u||
       !needed||needed>MAX_OBJECT_BYTES||
       needed!=meta.payload_bytes)return 0;
    MusaDwgAc1032ObjectStreams l={0};
    if(!musa_dwg_ac1032_stream_layout_file(
          file,n,h,&l)||
       !l.original_record_crc_verified||
       l.indexed_handle!=h||
       l.object_section_offset!=meta.object_section_offset||
       l.raw_object_type!=type||
       l.object_type_field_bits!=meta.object_type_field_bits||
       l.total_payload_bits!=(uint64_t)needed*8u||
       l.handle_stream_bits!=meta.handle_stream_bits)return 0;
    uint8_t *raw=(uint8_t*)malloc(needed);
    if(!raw)return 0;
    size_t copied=0u;
    MusaDwgAc1032PayloadInfo cp={0};
    MusaDwgAc1032EntityCommon co={0};
    int ok=musa_dwg_ac1032_object_payload_file(
          file,n,h,raw,needed,&copied,&cp)&&
       copied==needed&&cp.original_record_crc_verified&&
       cp.indexed_handle==h&&
       cp.object_section_offset==meta.object_section_offset&&
       cp.raw_object_type==type&&
       cp.handle_stream_bits==meta.handle_stream_bits&&
       musa_dwg_ac1032_entity_common_bytes(
           raw,needed,&l,type,&co)&&
       co.common_decoded&&
       co.prefix.indexed_handle==h&&
       co.prefix.raw_object_type==type;
    if(!ok){free(raw);return 0;}
    *payload=raw;*bytes=needed;*layout=l;*common=co;
    return 1;
}
int musa_dwg_ac1032_read_polyline3d_header_file(
     const uint8_t *file,size_t n,uint64_t h,
     MusaDwgAc1032Polyline3DHeader *out){
    if(!out||!file||overlap(file,n,out,sizeof(*out)))return 0;
    uint8_t *raw=NULL;size_t len=0u;
    MusaDwgAc1032ObjectStreams l={0};
    MusaDwgAc1032EntityCommon co={0};
    if(!read_common(file,n,h,16u,&raw,&len,&l,&co))return 0;
    MusaDwgAc1032Polyline3DHeader result={0};
    int ok=musa_dwg_ac1032_polyline3d_header_fields_at(
           raw,len,&l,co.geometry_start_bit,&result);
    free(raw);
    if(!ok||!result.header_decoded||
       result.indexed_handle!=h||
       result.geometry_end_bit>l.data_stream_bits)return 0;
    result.common=co;
    *out=result;
    return 1;
}
int musa_dwg_ac1032_read_vertex3d_file(
     const uint8_t *file,size_t n,uint64_t h,
     MusaDwgAc1032Vertex3D *out){
    if(!out||!file||overlap(file,n,out,sizeof(*out)))return 0;
    uint8_t *raw=NULL;size_t len=0u;
    MusaDwgAc1032ObjectStreams l={0};
    MusaDwgAc1032EntityCommon co={0};
    if(!read_common(file,n,h,11u,&raw,&len,&l,&co))return 0;
    MusaDwgAc1032Vertex3D result={0};
    int ok=musa_dwg_ac1032_vertex3d_fields_at(
           raw,len,&l,co.geometry_start_bit,&result);
    free(raw);
    if(!ok||!result.coordinates_decoded||
       result.indexed_handle!=h||
       result.geometry_end_bit>l.data_stream_bits)return 0;
    result.common=co;
    *out=result;
    return 1;
}
