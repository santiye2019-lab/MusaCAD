/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_lwpolyline.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1015_values.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MAX_DWG_BYTES (256u*1024u*1024u)
#define MAX_PAYLOAD_BYTES (64u*1024u*1024u)
#define MAX_LW_VERTICES 100000u
static int overlap(const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
static int finite_bd(MusaDwgAc1015Bits *b,double *v){
    return musa_dwg_ac1015_bits_read_bd(b,v)&&isfinite(*v);
}
static int checks(const uint8_t *raw,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geom,
    MusaDwgAc1032LWVertex *vertices,size_t capacity,
    MusaDwgAc1032NativeLWPolyline *out){
    if(!raw||!layout||!vertices||!out||
       !bytes||bytes>MAX_PAYLOAD_BYTES||
       !capacity||capacity>MAX_LW_VERTICES||
       capacity>SIZE_MAX/sizeof(*vertices)||
       overlap(raw,bytes,layout,sizeof(*layout))||
       overlap(raw,bytes,vertices,capacity*sizeof(*vertices))||
       overlap(raw,bytes,out,sizeof(*out))||
       overlap(layout,sizeof(*layout),vertices,capacity*sizeof(*vertices))||
       overlap(layout,sizeof(*layout),out,sizeof(*out))||
       overlap(vertices,capacity*sizeof(*vertices),out,sizeof(*out))||
       !layout->original_record_crc_verified||
       !layout->indexed_handle||layout->raw_object_type!=77u||
       layout->object_type_field_bits!=10u||
       layout->total_payload_bits!=(uint64_t)bytes*8u||
       layout->handle_stream_start_bit!=layout->data_stream_bits||
       layout->data_stream_bits>SIZE_MAX||
       !layout->handle_stream_bits||
       layout->handle_stream_bits!=layout->total_payload_bits-layout->data_stream_bits||
       geom<10u||(uint64_t)geom>=layout->data_stream_bits)return 0;
    return 1;
}
int musa_dwg_ac1032_lwpolyline_fields_at(
    const uint8_t *raw,size_t bytes,
    const MusaDwgAc1032ObjectStreams *layout,size_t geom,
    MusaDwgAc1032LWVertex *vertices,size_t capacity,
    MusaDwgAc1032NativeLWPolyline *out){
    if(!checks(raw,bytes,layout,geom,vertices,capacity,out))return 0;
    MusaDwgAc1015Bits b={0};
    if(!musa_dwg_ac1015_bits_init(
         &b,raw,bytes,geom,(size_t)layout->data_stream_bits))return 0;
    MusaDwgAc1032NativeLWPolyline candidate={0};
    candidate.indexed_handle=layout->indexed_handle;
    candidate.extrusion[2]=1.0;
    uint32_t count=0u;
    if(!musa_dwg_ac1015_bits_read_bs(&b,&candidate.flags)||
       (candidate.flags & ~UINT16_C(0x033f))!=0u)return 0;
    candidate.closed=(uint8_t)((candidate.flags&512u)!=0u);
    if((candidate.flags&4u)!=0u&&
       !finite_bd(&b,&candidate.constant_width))return 0;
    if((candidate.flags&8u)!=0u&&
       !finite_bd(&b,&candidate.elevation))return 0;
    if((candidate.flags&2u)!=0u&&
       !finite_bd(&b,&candidate.thickness))return 0;
    if((candidate.flags&1u)!=0u){
        for(unsigned i=0u;i<3u;i++)
            if(!finite_bd(&b,&candidate.extrusion[i]))return 0;
        if(candidate.extrusion[0]==0.0&&
           candidate.extrusion[1]==0.0&&
           candidate.extrusion[2]==0.0)return 0;
    }
    if(!musa_dwg_ac1015_bits_read_bl(&b,&count)||
       !count||count>MAX_LW_VERTICES||count>capacity)return 0;
    candidate.vertex_count=count;
    if((candidate.flags&16u)!=0u&&
       !musa_dwg_ac1015_bits_read_bl(&b,&candidate.bulge_count))
        return 0;
    if((candidate.flags&32u)!=0u&&
       !musa_dwg_ac1015_bits_read_bl(&b,&candidate.width_count))
        return 0;
    if(candidate.bulge_count>count||candidate.width_count>count)return 0;
    MusaDwgAc1032LWVertex *scratch=
        (MusaDwgAc1032LWVertex*)calloc((size_t)count,sizeof(*scratch));
    if(!scratch)return 0;
    int ok=musa_dwg_ac1015_bits_read_rd(&b,&scratch[0].x)&&
           musa_dwg_ac1015_bits_read_rd(&b,&scratch[0].y);
    for(uint32_t i=1u;ok&&i<count;i++){
        ok=musa_dwg_ac1015_bits_read_dd(&b,scratch[i-1u].x,
                                         &scratch[i].x)&&
           musa_dwg_ac1015_bits_read_dd(&b,scratch[i-1u].y,
                                         &scratch[i].y);
    }
    for(uint32_t i=0u;ok&&i<candidate.bulge_count;i++)
       ok=finite_bd(&b,&scratch[i].bulge);
    for(uint32_t i=0u;ok&&i<candidate.width_count;i++)
       ok=finite_bd(&b,&scratch[i].start_width)&&
          finite_bd(&b,&scratch[i].end_width);
    for(uint32_t i=0u;ok&&i<count;i++)
       ok=isfinite(scratch[i].x)&&isfinite(scratch[i].y);
    if(ok){
        candidate.geometry_end_bit=(uint64_t)b.bit_pos;
        candidate.geometry_decoded=1u;
        memcpy(vertices,scratch,(size_t)count*sizeof(*scratch));
        *out=candidate;
    }
    free(scratch);
    return ok;
}
int musa_dwg_ac1032_read_lwpolyline_file(
    const uint8_t *file,size_t n,uint64_t handle,
    MusaDwgAc1032LWVertex *vertices,size_t capacity,
    MusaDwgAc1032NativeLWPolyline *out){
    if(!file||!vertices||!out||!handle||
       n<256u||n>MAX_DWG_BYTES||
       !capacity||capacity>MAX_LW_VERTICES||
       overlap(file,n,out,sizeof(*out))||
       overlap(file,n,vertices,capacity*sizeof(*vertices))||
       overlap(vertices,capacity*sizeof(*vertices),out,sizeof(*out))||
       memcmp(file,"AC1032",6u)!=0)return 0;
    size_t needed=0u;
    MusaDwgAc1032PayloadInfo meta={0};
    if(!musa_dwg_ac1032_object_payload_file(
          file,n,handle,NULL,0u,&needed,&meta)||
       !meta.original_record_crc_verified||
       meta.indexed_handle!=handle||
       meta.raw_object_type!=77u||
       meta.object_type_field_bits!=10u||
       !needed||needed>MAX_PAYLOAD_BYTES||
       needed!=meta.payload_bytes)return 0;
    MusaDwgAc1032ObjectStreams layout={0};
    if(!musa_dwg_ac1032_stream_layout_file(
          file,n,handle,&layout)||
       !layout.original_record_crc_verified||
       layout.indexed_handle!=handle||
       layout.object_section_offset!=meta.object_section_offset||
       layout.raw_object_type!=77u||
       layout.object_type_field_bits!=meta.object_type_field_bits||
       layout.total_payload_bits!=(uint64_t)needed*8u||
       layout.handle_stream_bits!=meta.handle_stream_bits)return 0;
    uint8_t *raw=(uint8_t*)malloc(needed);
    if(!raw)return 0;
    MusaDwgAc1032LWVertex *scratch=
       (MusaDwgAc1032LWVertex*)calloc(capacity,sizeof(*scratch));
    if(!scratch){free(raw);return 0;}
    size_t copied=0u;
    MusaDwgAc1032PayloadInfo proof={0};
    MusaDwgAc1032NativeLWPolyline candidate={0};
    int ok=musa_dwg_ac1032_object_payload_file(
          file,n,handle,raw,needed,&copied,&proof)&&
       copied==needed&&proof.original_record_crc_verified&&
       proof.indexed_handle==meta.indexed_handle&&
       proof.object_section_offset==meta.object_section_offset&&
       proof.raw_object_type==77u&&
       proof.payload_bytes==meta.payload_bytes&&
       proof.handle_stream_bits==meta.handle_stream_bits&&
       musa_dwg_ac1032_entity_common_bytes(
          raw,needed,&layout,77u,&candidate.common)&&
       candidate.common.prefix.indexed_handle==handle;
    if(ok){
       MusaDwgAc1032NativeLWPolyline fields={0};
       ok=musa_dwg_ac1032_lwpolyline_fields_at(
           raw,needed,&layout,candidate.common.geometry_start_bit,
           scratch,capacity,&fields);
       if(ok){
           fields.common=candidate.common;
           candidate=fields;
       }
    }
    if(ok&&candidate.geometry_decoded&&
       candidate.indexed_handle==handle&&
       candidate.geometry_end_bit<=layout.data_stream_bits){
       memcpy(vertices,scratch,
              (size_t)candidate.vertex_count*sizeof(*vertices));
       *out=candidate;
    }else ok=0;
    free(scratch);free(raw);
    return ok;
}
