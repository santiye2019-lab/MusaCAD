/* SPDX-License-Identifier: GPL-3.0-or-later */
/* First-party C11 OT7 INSERT R2000+ numeric field order per public DWG
 * specification; deliberately NO external decoder, block graph or paint. */
#include "musacad_dwg_ac1032_insert.h"
#include "musacad_dwg_ac1015_values.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#define LIMIT (64u*1024u*1024u)
/* Diagnostics are emitted only on rejection; no source bytes are modified. */
static int reject_field(const char *field,const MusaDwgAc1015Bits *b,
                        uint64_t handle){
    fprintf(stderr,"MC1032 INSERT reject H%llx field=%s bit=%zu end=%zu remaining=%zu\n",
            (unsigned long long)handle,field,b->bit_pos,b->bit_end,
            b->bit_pos<=b->bit_end?b->bit_end-b->bit_pos:0u);
    return 0;
}
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
        if(!musa_dwg_ac1015_bits_read_bd(&b,&v.insertion[k]))
            return reject_field("insertion",&b,v.indexed_handle);
    uint32_t flags=0u;
    if(!musa_dwg_ac1015_bits_read_u(&b,2u,&flags))
        return reject_field("scale_flags",&b,v.indexed_handle);
    v.scale_data_flags=(uint8_t)flags;
    v.scale[0]=v.scale[1]=v.scale[2]=1.0;
    if(flags==0u){
        if(!musa_dwg_ac1015_bits_read_rd(&b,&v.scale[0])||
           !musa_dwg_ac1015_bits_read_dd(&b,v.scale[0],&v.scale[1])||
           !musa_dwg_ac1015_bits_read_dd(&b,v.scale[0],&v.scale[2]))
            return reject_field("scale_xyz",&b,v.indexed_handle);
    }else if(flags==1u){
        if(!musa_dwg_ac1015_bits_read_dd(&b,1.0,&v.scale[1])||
           !musa_dwg_ac1015_bits_read_dd(&b,1.0,&v.scale[2]))
            return reject_field("scale_yz",&b,v.indexed_handle);
    }else if(flags==2u){
        if(!musa_dwg_ac1015_bits_read_rd(&b,&v.scale[0]))
            return reject_field("scale_x",&b,v.indexed_handle);
        v.scale[1]=v.scale[2]=v.scale[0];
    }else if(flags!=3u)return reject_field("scale_flags_invalid",&b,v.indexed_handle);
    uint32_t attribs=0u;
    if(!musa_dwg_ac1015_bits_read_bd(&b,&v.rotation))
        return reject_field("rotation",&b,v.indexed_handle);
    if(!musa_dwg_ac1015_bits_read_bd(&b,&v.extrusion[0])||
       !musa_dwg_ac1015_bits_read_bd(&b,&v.extrusion[1])||
       !musa_dwg_ac1015_bits_read_bd(&b,&v.extrusion[2]))
        return reject_field("extrusion",&b,v.indexed_handle);
    if(!musa_dwg_ac1015_bits_read_u(&b,1u,&attribs))
        return reject_field("has_attribs",&b,v.indexed_handle);
    v.has_attribs=(uint8_t)attribs;
    /* Attribute-free INSERT has no owned ATTRIB objects; its BL count
     * is present only for the attribute-bearing variant. */
    if(v.has_attribs &&
       !musa_dwg_ac1015_bits_read_bl(&b,&v.owned_attrib_count))
        return reject_field("owned_attrib_count",&b,v.indexed_handle);
    if(b.bit_pos!=b.bit_end){
        /* Preserve strict rejection while recording exact unparsed source bits.
         * This is evidence for the remaining INSERT grammar, not padding. */
        fprintf(stderr,"MC1032 INSERT trailing DATA H%llx bits=",
                (unsigned long long)v.indexed_handle);
        for(size_t bit=b.bit_pos;bit<b.bit_end;bit++){
            const uint8_t octet=raw[bit/8u];
            fputc((octet & (uint8_t)(1u << (bit%8u)))?'1':'0',stderr);
        }
        fputc('\n',stderr);
        fprintf(stderr,"MC1032 INSERT diagnostic handle=%llx geom=%zu data_end=%zu pos=%zu remaining=%zu flags=%u attrib=%u count=%u xyz=(%.12g,%.12g,%.12g)\n",
            (unsigned long long)v.indexed_handle,geom,(size_t)l->data_stream_bits,
            b.bit_pos,b.bit_end-b.bit_pos,(unsigned)v.scale_data_flags,
            (unsigned)v.has_attribs,(unsigned)v.owned_attrib_count,
            v.insertion[0],v.insertion[1],v.insertion[2]);
        return reject_field("trailing_data",&b,v.indexed_handle);
    }
    if((!v.has_attribs&&v.owned_attrib_count!=0u)||
       v.owned_attrib_count>100000u||
       !isfinite(v.rotation))return 0;
    for(unsigned k=0u;k<3u;k++)
        if(!isfinite(v.insertion[k])||!isfinite(v.scale[k])||
           v.scale[k]==0.0||!isfinite(v.extrusion[k]))
            return reject_field("coordinate_validation",&b,v.indexed_handle);
    if(hypot(hypot(v.extrusion[0],v.extrusion[1]),
             v.extrusion[2])<0.5)
        return reject_field("extrusion_norm",&b,v.indexed_handle);
    v.original_record_crc_verified=1u;
    v.numeric_fields_decoded=1u;
    *out=v;return 1;
}
