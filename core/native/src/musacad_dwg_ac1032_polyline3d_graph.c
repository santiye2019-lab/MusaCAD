/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1032_polyline3d.h"
#include "musacad_dwg_ac1032_object_payload.h"
#include "musacad_dwg_ac1032_object_type.h"
#include "musacad_dwg_ac1015_handle_refs.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MAX_FILE (64u*1024u*1024u)
#define MAX_VERTICES 100000u
#define MAX_OBJECT (64u*1024u*1024u)
static int overlap(const void *a,size_t an,const void *b,size_t bn){
    if(!an||!bn)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<an:x-y<bn;
}
static int cmp_u64(const void *a,const void *b){
    const uint64_t x=*(const uint64_t*)a,y=*(const uint64_t*)b;
    return (x>y)-(x<y);
}
static int zero_tail(MusaDwgAc1015Bits *c){
    if(!c||c->bit_pos>c->bit_end||
       c->bit_end-c->bit_pos>7u)return 0;
    while(c->bit_pos<c->bit_end){
        uint32_t pad=1u;
        if(!musa_dwg_ac1015_bits_read_u(c,1u,&pad)||pad)return 0;
    }
    return 1;
}
static int next_ref(MusaDwgAc1015Bits *b,uint64_t base,
                    uint8_t code,uint64_t *handle){
    MusaDwgAc1015HandleRef ref={0};
    if(!musa_dwg_ac1015_read_handle_ref(b,base,&ref)||
       ref.code!=code||!ref.target)return 0;
    *handle=ref.target;return 1;
}
static int copy_original_record(
    const uint8_t *file,size_t n,uint64_t h,uint16_t expected_type,
    uint8_t **data,size_t *bytes,
    MusaDwgAc1032ObjectStreams *layout){
    if(!file||!data||!bytes||!layout||!h||
       n<256u||n>MAX_FILE||memcmp(file,"AC1032",6u))return 0;
    MusaDwgAc1032PayloadInfo p={0},confirm={0};
    size_t required=0u,copied=0u;
    if(!musa_dwg_ac1032_object_payload_file(
          file,n,h,NULL,0u,&required,&p)||
       !p.original_record_crc_verified||
       p.indexed_handle!=h||p.raw_object_type!=expected_type||
       p.object_type_field_bits!=10u||
       !required||required>MAX_OBJECT||
       p.payload_bytes!=required)return 0;
    MusaDwgAc1032ObjectStreams l={0};
    if(!musa_dwg_ac1032_stream_layout_file(file,n,h,&l)||
       !l.original_record_crc_verified||
       l.indexed_handle!=h||
       l.raw_object_type!=expected_type||
       l.object_section_offset!=p.object_section_offset||
       l.object_type_field_bits!=p.object_type_field_bits||
       l.total_payload_bits!=(uint64_t)required*8u||
       l.handle_stream_bits!=p.handle_stream_bits||
       l.handle_stream_start_bit!=l.data_stream_bits||
       !l.handle_stream_bits||l.data_stream_bits>SIZE_MAX)return 0;
    uint8_t *raw=(uint8_t*)malloc(required);
    if(!raw)return 0;
    const int ok=musa_dwg_ac1032_object_payload_file(
        file,n,h,raw,required,&copied,&confirm)&&
        copied==required&&
        confirm.original_record_crc_verified&&
        confirm.indexed_handle==h&&
        confirm.object_section_offset==p.object_section_offset&&
        confirm.raw_object_type==expected_type&&
        confirm.handle_stream_bits==p.handle_stream_bits;
    if(!ok){free(raw);return 0;}
    *data=raw;*bytes=required;*layout=l;return 1;
}
int musa_dwg_ac1032_polyline3d_handle_fields_at(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *l,
    const MusaDwgAc1032Polyline3DHeader *header,
    uint64_t *handles,size_t capacity,
    uint64_t *layer,uint64_t *seqend){
    if(!raw||!l||!header||!handles||!layer||!seqend||
       !n||n>MAX_OBJECT||!capacity||capacity>MAX_VERTICES||
       capacity>SIZE_MAX/sizeof(*handles)||
       overlap(raw,n,l,sizeof(*l))||
       overlap(raw,n,header,sizeof(*header))||
       overlap(raw,n,handles,capacity*sizeof(*handles))||
       overlap(raw,n,layer,sizeof(*layer))||
       overlap(raw,n,seqend,sizeof(*seqend))||
       overlap(l,sizeof(*l),header,sizeof(*header))||
       overlap(l,sizeof(*l),handles,capacity*sizeof(*handles))||
       overlap(l,sizeof(*l),layer,sizeof(*layer))||
       overlap(l,sizeof(*l),seqend,sizeof(*seqend))||
       overlap(header,sizeof(*header),handles,capacity*sizeof(*handles))||
       overlap(header,sizeof(*header),layer,sizeof(*layer))||
       overlap(header,sizeof(*header),seqend,sizeof(*seqend))||
       overlap(handles,capacity*sizeof(*handles),layer,sizeof(*layer))||
       overlap(handles,capacity*sizeof(*handles),seqend,sizeof(*seqend))||
       overlap(layer,sizeof(*layer),seqend,sizeof(*seqend))||
       !l->original_record_crc_verified||
       !l->indexed_handle||l->raw_object_type!=16u||
       l->object_type_field_bits!=10u||
       l->total_payload_bits!=(uint64_t)n*8u||
       l->data_stream_bits>SIZE_MAX||
       l->handle_stream_start_bit!=l->data_stream_bits||
       !l->handle_stream_bits||
       l->handle_stream_bits!=
         l->total_payload_bits-l->data_stream_bits||
       !header->header_decoded||
       !header->common.common_decoded||
       header->common.prefix.raw_object_type!=16u||
       header->common.prefix.indexed_handle!=l->indexed_handle||
       header->indexed_handle!=l->indexed_handle||
       header->geometry_end_bit>l->data_stream_bits||
       header->owned_object_count<2u||
       header->owned_object_count>MAX_VERTICES||
       header->owned_object_count>capacity||
       header->common.linetype_flags||
       header->common.plotstyle_flags||
       header->common.material_flags||
       header->common.visualstyle_flags)return 0;
    MusaDwgAc1015Bits bits={0};
    if(!musa_dwg_ac1015_bits_init(&bits,raw,n,
           (size_t)l->data_stream_bits,
           (size_t)l->total_payload_bits))return 0;
    const size_t count=header->owned_object_count;
    uint64_t *temporary=(uint64_t*)calloc(count,sizeof(*temporary));
    uint64_t *sorted=(uint64_t*)calloc(count,sizeof(*sorted));
    if(!temporary||!sorted){
       free(sorted);free(temporary);return 0;
    }
    uint64_t lyr=0u,seq=0u;
    int good=next_ref(&bits,l->indexed_handle,5u,&lyr);
    for(size_t i=0u;good&&i<count;i++)
        good=next_ref(&bits,l->indexed_handle,3u,&temporary[i])&&
             temporary[i]!=lyr&&
             temporary[i]!=l->indexed_handle;
    if(good)good=next_ref(&bits,l->indexed_handle,3u,&seq)&&
                 seq!=lyr&&seq!=l->indexed_handle&&
                 zero_tail(&bits);
    if(good){
       memcpy(sorted,temporary,count*sizeof(*sorted));
       qsort(sorted,count,sizeof(*sorted),cmp_u64);
       for(size_t i=0u;i<count;i++)
          if(sorted[i]==seq||(i&&sorted[i]==sorted[i-1u]))
             good=0;
    }
    if(good){
       memcpy(handles,temporary,count*sizeof(*handles));
       *layer=lyr;*seqend=seq;
    }
    free(sorted);free(temporary);
    return good;
}
static int reverse_owner_and_layer(const uint8_t *file,size_t n,
    uint64_t child,uint64_t parent,uint64_t layer){
    uint8_t *raw=NULL;size_t bytes=0u;
    MusaDwgAc1032ObjectStreams l={0};
    if(!copy_original_record(file,n,child,11u,&raw,&bytes,&l))return 0;
    MusaDwgAc1015Bits b={0};
    int ok=musa_dwg_ac1015_bits_init(&b,raw,bytes,
             (size_t)l.data_stream_bits,(size_t)l.total_payload_bits);
    uint64_t actual_parent=0u,actual_layer=0u;
    if(ok)ok=next_ref(&b,child,4u,&actual_parent)&&
             actual_parent==parent&&
             next_ref(&b,child,5u,&actual_layer)&&
             actual_layer==layer&&zero_tail(&b);
    free(raw);return ok;
}
int musa_dwg_ac1032_read_polyline3d_vertices_file(
    const uint8_t *file,size_t n,uint64_t h,
    MusaDwgAc1032Polyline3DOrderedVertex *vertices,size_t capacity,
    MusaDwgAc1032Polyline3DOrdered *out){
    if(!file||!vertices||!out||!h||
       n<256u||n>MAX_FILE||
       capacity<2u||capacity>MAX_VERTICES||
       capacity>SIZE_MAX/sizeof(*vertices)||
       overlap(file,n,vertices,capacity*sizeof(*vertices))||
       overlap(file,n,out,sizeof(*out))||
       overlap(vertices,capacity*sizeof(*vertices),out,sizeof(*out))||
       memcmp(file,"AC1032",6u)!=0)return 0;
    MusaDwgAc1032Polyline3DHeader header={0};
    if(!musa_dwg_ac1032_read_polyline3d_header_file(
          file,n,h,&header)||!header.header_decoded||
       header.owned_object_count>capacity||
       header.owned_object_count>MAX_VERTICES||
       header.owned_object_count<2u)return 0;
    uint8_t *raw=NULL;size_t bytes=0u;
    MusaDwgAc1032ObjectStreams l={0};
    if(!copy_original_record(file,n,h,16u,&raw,&bytes,&l))return 0;
    const size_t count=header.owned_object_count;
    uint64_t *handles=(uint64_t*)calloc(count,sizeof(*handles));
    MusaDwgAc1032Polyline3DOrderedVertex *scratch=
        (MusaDwgAc1032Polyline3DOrderedVertex*)
         calloc(count,sizeof(*scratch));
    if(!handles||!scratch){
       free(scratch);free(handles);free(raw);return 0;
    }
    uint64_t layer=0u,seqend=0u;
    int ok=musa_dwg_ac1032_polyline3d_handle_fields_at(
           raw,bytes,&l,&header,handles,count,&layer,&seqend);
    free(raw);
    MusaDwgAc1032ObjectType seq_type={0};
    if(ok)ok=musa_dwg_ac1032_object_type_file(
           file,n,seqend,&seq_type)&&
           seq_type.record_crc_verified&&
           seq_type.indexed_handle==seqend&&
           seq_type.type_code==6u;
    for(size_t i=0u;ok&&i<count;i++){
       MusaDwgAc1032Vertex3D child={0};
       ok=musa_dwg_ac1032_read_vertex3d_file(
            file,n,handles[i],&child)&&
          child.coordinates_decoded&&
          child.indexed_handle==handles[i]&&
          child.common.prefix.indexed_handle==handles[i]&&
          child.common.prefix.raw_object_type==11u&&
          reverse_owner_and_layer(
            file,n,handles[i],h,layer);
       if(ok){
           scratch[i].indexed_handle=handles[i];
           scratch[i].flags=child.flags;
           memcpy(scratch[i].point,child.point,sizeof(child.point));
       }
    }
    if(ok){
       MusaDwgAc1032Polyline3DOrdered value={0};
       value.header=header;
       value.layer_handle=layer;
       value.seqend_handle=seqend;
       value.vertex_count=(uint32_t)count;
       value.hard_owner_links_verified=1u;
       value.reverse_vertex_owners_verified=1u;
       value.seqend_type_verified=1u;
       memcpy(vertices,scratch,count*sizeof(*scratch));
       *out=value;
    }
    free(scratch);free(handles);
    return ok;
}
