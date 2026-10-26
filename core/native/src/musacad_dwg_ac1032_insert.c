/* SPDX-License-Identifier: GPL-3.0-or-later */
/* First-party C11 OT7 INSERT R2000+ numeric field order per public DWG
 * specification; deliberately NO external decoder, block graph or paint. */
#include "musacad_dwg_ac1032_insert.h"
#include "musacad_dwg_ac1015_values.h"
#include <math.h>
#include <stdint.h>
#define LIMIT (64u*1024u*1024u)
static int overlaps(const void *a,size_t n,const void *b,size_t m){
    if(!n||!m)return 0;
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<n:x-y<m;
}
int musa_dwg_ac1032_insert_placement_at(
    const uint8_t *raw,size_t n,
    const MusaDwgAc1032ObjectStreams *l,size_t geom,
    MusaDwgAc1032InsertPlacement *out){
    if(!raw||!l||!out||!n||n>LIMIT||n>SIZE_MAX/8u||
       overlaps(raw,n,l,sizeof(*l))||
       overlaps(raw,n,out,sizeof(*out))||
       overlaps(l,sizeof(*l),out,sizeof(*out))||
       !l->original_record_crc_verified||!l->indexed_handle||
       l->raw_object_type!=7u||l->object_type_field_bits!=10u||
       l->total_payload_bits!=(uint64_t)n*8u||
       l->data_stream_bits>SIZE_MAX||
       l->handle_stream_start_bit!=l->data_stream_bits||
       !l->handle_stream_bits||
       l->handle_stream_bits!=l->total_payload_bits-l->data_stream_bits||
       geom<l->object_type_field_bits||
       geom>=(size_t)l->data_stream_bits)return 0;
    MusaDwgAc1015Bits b={0};
    if(!musa_dwg_ac1015_bits_init(
        &b,raw,n,geom,(size_t)l->data_stream_bits))return 0;
    MusaDwgAc1032InsertPlacement v={0};
    v.indexed_handle=l->indexed_handle;
    for(unsigned k=0u;k<3u;k++)
        if(!musa_dwg_ac1015_bits_read_bd(&b,&v.insertion[k]))return 0;
    uint32_t flags=0u;
    if(!musa_dwg_ac1015_bits_read_u(&b,2u,&flags))return 0;
    v.scale_data_flags=(uint8_t)flags;
    v.scale[0]=v.scale[1]=v.scale[2]=1.0;
    if(flags==0u){
        if(!musa_dwg_ac1015_bits_read_rd(&b,&v.scale[0])||
           !musa_dwg_ac1015_bits_read_dd(&b,v.scale[0],&v.scale[1])||
           !musa_dwg_ac1015_bits_read_dd(&b,v.scale[0],&v.scale[2]))
            return 0;
    }else if(flags==1u){
        if(!musa_dwg_ac1015_bits_read_dd(&b,1.0,&v.scale[1])||
           !musa_dwg_ac1015_bits_read_dd(&b,1.0,&v.scale[2]))
            return 0;
    }else if(flags==2u){
        if(!musa_dwg_ac1015_bits_read_rd(&b,&v.scale[0]))return 0;
        v.scale[1]=v.scale[2]=v.scale[0];
    }else if(flags!=3u)return 0;
    uint32_t attribs=0u;
    if(!musa_dwg_ac1015_bits_read_bd(&b,&v.rotation)||
       !musa_dwg_ac1015_bits_read_be(&b,v.extrusion)||
       !musa_dwg_ac1015_bits_read_u(&b,1u,&attribs)||
       !musa_dwg_ac1015_bits_read_bl(&b,&v.owned_attrib_count))
        return 0;
    v.has_attribs=(uint8_t)attribs;
    if(b.bit_pos!=b.bit_end||
       (!v.has_attribs&&v.owned_attrib_count!=0u)||
       v.owned_attrib_count>100000u||
       !isfinite(v.rotation))return 0;
    for(unsigned k=0u;k<3u;k++)
        if(!isfinite(v.insertion[k])||!isfinite(v.scale[k])||
           v.scale[k]==0.0||!isfinite(v.extrusion[k]))
            return 0;
    if(hypot(hypot(v.extrusion[0],v.extrusion[1]),
             v.extrusion[2])<0.5)return 0;
    v.original_record_crc_verified=1u;
    v.numeric_fields_decoded=1u;
    *out=v;return 1;
}
