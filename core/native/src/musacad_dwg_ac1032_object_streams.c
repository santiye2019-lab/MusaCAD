/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_object_streams.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include <stdint.h>
#include <string.h>

#define MAX_ORIGINAL_OBJECT_BITS (64u*1024u*1024u)
static int overlap(const void *p,size_t pn,const void *q,size_t qn){
    if(!pn||!qn)return 0;
    const uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
    return a<=b?b-a<pn:a-b<qn;
}
static int partition(const MusaDwgAc1032PayloadInfo *verified,
                     size_t bytes,MusaDwgAc1032ObjectStreams *out){
    if(!verified||!out||!verified->original_record_crc_verified||
       !verified->indexed_handle||!verified->raw_object_type||
       !bytes||bytes>MAX_ORIGINAL_OBJECT_BITS||
       verified->payload_bytes!=bytes||
       (verified->object_type_field_bits!=10u&&
        verified->object_type_field_bits!=18u))return 0;
    const uint64_t total=(uint64_t)bytes*8u;
    const uint64_t handles=verified->handle_stream_bits;
    if(!handles||handles>=total)return 0;
    const uint64_t data=total-handles;
    if(data<verified->object_type_field_bits)return 0;
    MusaDwgAc1032ObjectStreams candidate={0};
    candidate.indexed_handle=verified->indexed_handle;
    candidate.object_section_offset=verified->object_section_offset;
    candidate.total_payload_bits=total;
    candidate.data_stream_bits=data;
    candidate.handle_stream_start_bit=data;
    candidate.handle_stream_bits=handles;
    candidate.raw_object_type=verified->raw_object_type;
    candidate.object_type_field_bits=verified->object_type_field_bits;
    candidate.original_record_crc_verified=1u;
    *out=candidate;return 1;
}
int musa_dwg_ac1032_stream_layout_bytes(
    const uint8_t *objects,size_t objects_size,uint64_t handle,
    uint64_t offset,MusaDwgAc1032ObjectStreams *out){
    if(!objects||!out||!objects_size||
       overlap(objects,objects_size,out,sizeof(*out)))return 0;
    MusaDwgAc1032PayloadInfo payload={0};
    size_t needed=0u;
    if(!musa_dwg_ac1032_object_payload_bytes(
          objects,objects_size,handle,offset,NULL,0u,&needed,&payload))
        return 0;
    MusaDwgAc1032ObjectStreams candidate={0};
    if(!partition(&payload,needed,&candidate))return 0;
    *out=candidate;return 1;
}
int musa_dwg_ac1032_stream_layout_file(
    const uint8_t *file,size_t file_bytes,uint64_t handle,
    MusaDwgAc1032ObjectStreams *out){
    if(!file||!out||!file_bytes||
       overlap(file,file_bytes,out,sizeof(*out)))return 0;
    MusaDwgAc1032PayloadInfo payload={0};
    size_t needed=0u;
    if(!musa_dwg_ac1032_object_payload_file(
          file,file_bytes,handle,NULL,0u,&needed,&payload))
        return 0;
    MusaDwgAc1032ObjectStreams candidate={0};
    if(!partition(&payload,needed,&candidate))return 0;
    *out=candidate;return 1;
}
int musa_dwg_ac1032_stream_read_bits(
    const uint8_t *payload,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,
    uint8_t stream,size_t relative,unsigned width,uint32_t *out){
    if(!payload||!layout||!out||bytes==0u||
       bytes>MAX_ORIGINAL_OBJECT_BITS||width==0u||width>32u||
       stream>MUSA_AC1032_STREAM_HANDLES||
       overlap(payload,bytes,layout,sizeof(*layout))||
       overlap(payload,bytes,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),out,sizeof(*out))||
       !layout->original_record_crc_verified||
       !layout->raw_object_type||!layout->indexed_handle||
       (layout->object_type_field_bits!=10u&&
        layout->object_type_field_bits!=18u))return 0;
    const uint64_t total=(uint64_t)bytes*8u;
    if(layout->total_payload_bits!=total||
       !layout->handle_stream_bits||
       layout->data_stream_bits<layout->object_type_field_bits||
       layout->data_stream_bits>=total||
       layout->handle_stream_start_bit!=layout->data_stream_bits||
       layout->handle_stream_bits!=total-layout->data_stream_bits)
        return 0;
    const uint64_t span=(stream==MUSA_AC1032_STREAM_OBJECT_DATA)
       ?layout->data_stream_bits:layout->handle_stream_bits;
    const uint64_t start=(stream==MUSA_AC1032_STREAM_OBJECT_DATA)
       ?0u:layout->handle_stream_start_bit;
    if((uint64_t)relative>span||(uint64_t)width>span-(uint64_t)relative)
        return 0;
    const uint64_t at=start+(uint64_t)relative;
    uint32_t value=0u;
    for(unsigned i=0u;i<width;i++){
        const uint64_t pos=at+(uint64_t)i;
        const uint8_t bit=(payload[(size_t)(pos/8u)]>>
             (7u-(unsigned)(pos%8u)))&1u;
        value=(value<<1u)|(uint32_t)bit;
    }
    *out=value;return 1;
}
