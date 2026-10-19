/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_nonentity.h"
#include "musacad_dwg_ac1015_handle_refs.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_dwg_ac1018_eed.h"
#include "musacad_crc16.h"
#include <stdint.h>
#include <stddef.h>

/* Pointer-range collision check without overflowing uintptr_t additions. */
static int overlap(const void *a,size_t na,const void *b,size_t nb){
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return na&&nb&&((x<=y&&y-x<na)||(y<x&&x-y<nb));
}
static int ms(const uint8_t *p,size_t n,size_t *head,size_t *body){
    if(n<4u)return 0;
    const uint16_t lo=(uint16_t)p[0]|(uint16_t)((uint16_t)p[1]<<8u);
    if(!(lo&UINT16_C(0x8000))){
        *head=2u;*body=lo;return lo>=8u;
    }
    if(n<6u)return 0;
    const uint16_t hi=(uint16_t)p[2]|(uint16_t)((uint16_t)p[3]<<8u);
    if(!hi||(hi&UINT16_C(0x8000)))return 0;
    *head=4u;
    *body=(size_t)(lo&UINT16_C(0x7fff))|((size_t)hi<<15u);
    return *body>=8u;
}
int musa_dwg_ac1018_nonentity_identity_record(
    const uint8_t *objects,size_t object_bytes,size_t object_offset,
    uint64_t indexed_handle,MusaDwgAc1018NonEntityIdentity *out){
    if(!objects||!out||!indexed_handle||object_offset>=object_bytes||
       overlap(objects,object_bytes,out,sizeof(*out)))return 0;
    const uint8_t *record=objects+object_offset;
    const size_t available=object_bytes-object_offset;
    size_t head=0u,body=0u;
    if(!ms(record,available,&head,&body)||available-head<2u||
       body>available-head-2u||body>SIZE_MAX/8u)return 0;
    uint16_t crc=0u;
    const size_t crc_at=head+body;
    if(!musa_core_crc16_arc(0xc0c1u,record,crc_at,&crc)||
       crc!=((uint16_t)record[crc_at]|
            (uint16_t)((uint16_t)record[crc_at+1u]<<8u)))return 0;
    MusaDwgAc1015Bits bits={0};
    uint16_t typ=0u;
    uint32_t endbit=0u,reactors=0u,eed=0u,xdict_missing=0u;
    MusaDwgAc1015HandleRef self={0};
    if(!musa_dwg_ac1015_bits_init(&bits,record+head,body,0u,body*8u)||
       !musa_dwg_ac1015_bits_read_bs(&bits,&typ)||
       (typ!=42u&&typ!=49u&&typ!=51u)||
       !musa_dwg_ac1015_bits_read_rl(&bits,&endbit)||
       (size_t)endbit>body*8u||
       !musa_dwg_ac1015_read_handle_ref(&bits,indexed_handle,&self)||
       self.code!=0u||self.target!=indexed_handle||
       !musa_dwg_ac1018_eed_skip(&bits,&eed)||
       !musa_dwg_ac1015_bits_read_bl(&bits,&reactors)||
       reactors>64u||
       !musa_dwg_ac1015_bits_read_u(&bits,1u,&xdict_missing)||
       bits.bit_pos>(size_t)endbit)return 0;
    MusaDwgAc1018NonEntityIdentity verified={0};
    verified.indexed_handle=indexed_handle;
    verified.record_bytes=crc_at+2u;
    verified.pre_handle_bits=endbit;
    verified.reactor_count=reactors;
    verified.eed_skipped_bytes=eed;
    verified.declared_type=typ;
    verified.xdictionary_missing=(uint8_t)xdict_missing;
    verified.record_crc_verified=1u;
    verified.embedded_self_verified=1u;
    *out=verified;
    return 1;
}

/* In the supported AC1018 layer-object prefix, text TV uses a
 * bit-short byte count followed by codepage bytes (pre-2007).
 * Decode only printable ASCII; non-ASCII requires an explicit future
 * codepage converter rather than mangling an engineering layer name.
 * All writes are deferred until *every* accepted prefix field is valid.
 * The remaining CMC and H tails are intentionally NOT claimed decoded. */
int musa_dwg_ac1018_layer_prefix_record(
    const uint8_t *objects,size_t object_bytes,size_t object_offset,
    uint64_t indexed_handle,MusaDwgAc1018LayerPrefix *out){
    if(!objects||!out||object_offset>=object_bytes||
       overlap(objects,object_bytes,out,sizeof(*out)))return 0;
    MusaDwgAc1018NonEntityIdentity identity={0};
    if(!musa_dwg_ac1018_nonentity_identity_record(
           objects,object_bytes,object_offset,indexed_handle,&identity)||
       identity.declared_type!=51u)return 0;
    const uint8_t *p=objects+object_offset;
    size_t head=0u,body=0u;
    if(!ms(p,object_bytes-object_offset,&head,&body)||
       body>SIZE_MAX/8u)return 0;
    MusaDwgAc1015Bits b={0};
    if(!musa_dwg_ac1015_bits_init(&b,p+head,body,0u,body*8u))
        return 0;
    uint16_t type=0u,name_len=0u,xref=0u,flags=0u;
    uint32_t endbit=0u,eed=0u,reactors=0u,xdict=0u,bit=0u;
    MusaDwgAc1015HandleRef self={0};
    if(!musa_dwg_ac1015_bits_read_bs(&b,&type)||type!=51u||
       !musa_dwg_ac1015_bits_read_rl(&b,&endbit)||
       endbit!=identity.pre_handle_bits||
       !musa_dwg_ac1015_read_handle_ref(&b,indexed_handle,&self)||
       self.code!=0u||self.target!=indexed_handle||
       !musa_dwg_ac1018_eed_skip(&b,&eed)||
       !musa_dwg_ac1015_bits_read_bl(&b,&reactors)||
       reactors>64u||reactors!=identity.reactor_count||
       !musa_dwg_ac1015_bits_read_u(&b,1u,&xdict)||
       xdict!=identity.xdictionary_missing||
       b.bit_pos>(size_t)endbit||
       !musa_dwg_ac1015_bits_read_bs(&b,&name_len)||
       name_len==0u||name_len>255u||
       b.bit_pos>(size_t)endbit||
       (size_t)name_len>((size_t)endbit-b.bit_pos)/8u)return 0;
    MusaDwgAc1018LayerPrefix checked={0};
    for(size_t i=0u;i<name_len;i++){
        if(!musa_dwg_ac1015_bits_read_u(&b,8u,&bit)||
           bit<0x20u||bit>0x7eu)return 0;
        checked.name[i]=(char)bit;
    }
    if(b.bit_pos>(size_t)endbit||
       !musa_dwg_ac1015_bits_read_u(&b,1u,&bit))return 0;
    checked.ever_referenced=(uint8_t)bit;
    if(!musa_dwg_ac1015_bits_read_bs(&b,&xref)||
       !musa_dwg_ac1015_bits_read_u(&b,1u,&bit))return 0;
    checked.xref_dependent=(uint8_t)bit;
    if(!musa_dwg_ac1015_bits_read_bs(&b,&flags)||
       b.bit_pos>(size_t)endbit||
       (size_t)endbit-b.bit_pos<2u)return 0;
    /* An actual layer still has its CMC color and H fields.
     * Merely consuming the accepted prefix is not whole-record proof. */
    checked.indexed_handle=indexed_handle;
    checked.name_bytes=name_len;
    checked.xref_index_plus_one=xref;
    checked.raw_layer_flags=flags;
    checked.record_crc_verified=identity.record_crc_verified;
    checked.embedded_self_verified=identity.embedded_self_verified;
    *out=checked;
    return 1;
}
