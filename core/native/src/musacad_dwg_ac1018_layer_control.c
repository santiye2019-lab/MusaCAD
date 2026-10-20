/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_layer_control.h"
#include "musacad_dwg_ac1018_nonentity.h"
#include "musacad_dwg_ac1018_eed.h"
#include "musacad_dwg_ac1015_handle_refs.h"
#include "musacad_dwg_ac1015_values.h"
#include <stddef.h>
#include <stdint.h>

static int overlap(const void *a,size_t na,const void *b,size_t nb){
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return na&&nb&&((x<=y&&y-x<na)||(y<x&&x-y<nb));
}
static int framed(const uint8_t *p,size_t n,size_t *head,size_t *body){
    if(n<4u)return 0;
    const uint16_t lo=(uint16_t)p[0]|((uint16_t)p[1]<<8u);
    if(!(lo&UINT16_C(0x8000))){*head=2u;*body=lo;return lo>=8u;}
    if(n<6u)return 0;
    const uint16_t hi=(uint16_t)p[2]|((uint16_t)p[3]<<8u);
    if(!hi||(hi&UINT16_C(0x8000)))return 0;
    *head=4u;*body=(size_t)(lo&UINT16_C(0x7fff))|((size_t)hi<<15u);
    return *body>=8u;
}
static int decode(const uint8_t *objects,size_t object_bytes,
                  size_t object_offset,uint64_t self,
                  const MusaDwgAc1018NonEntityIdentity *identity,
                  uint64_t *handles,uint32_t *entries){
    const uint8_t *record=objects+object_offset;
    size_t head=0u,body=0u;
    if(!framed(record,object_bytes-object_offset,&head,&body)||
       body>SIZE_MAX/8u)return 0;
    MusaDwgAc1015Bits bits={0};
    if(!musa_dwg_ac1015_bits_init(&bits,record+head,body,0u,body*8u))
        return 0;
    uint16_t typ=0u;
    uint32_t endbit=0u,reactors=0u,eed=0u,xdict=0u,count=0u;
    MusaDwgAc1015HandleRef embedded={0};
    if(!musa_dwg_ac1015_bits_read_bs(&bits,&typ)||typ!=50u||
       !musa_dwg_ac1015_bits_read_rl(&bits,&endbit)||
       endbit!=identity->pre_handle_bits||
       !musa_dwg_ac1015_read_handle_ref(&bits,self,&embedded)||
       embedded.code!=0u||embedded.target!=self||
       !musa_dwg_ac1018_eed_skip(&bits,&eed)||
       eed!=identity->eed_skipped_bytes||
       !musa_dwg_ac1015_bits_read_bl(&bits,&reactors)||
       reactors!=0u||reactors!=identity->reactor_count||
       !musa_dwg_ac1015_bits_read_u(&bits,1u,&xdict)||
       xdict!=1u||xdict!=identity->xdictionary_missing||
       !musa_dwg_ac1015_bits_read_bl(&bits,&count)||
       !count||count>4096u||bits.bit_pos!=(size_t)endbit)return 0;
    /* Root-owner NULL soft pointer, DWG H code=4, count=0. This special
     * sentinel is not a nonzero indexed handle and must not be resolved.
     */
    uint32_t code=0u,width=0u;
    if(!musa_dwg_ac1015_bits_read_u(&bits,4u,&code)||
       !musa_dwg_ac1015_bits_read_u(&bits,4u,&width)||
       code!=4u||width!=0u)return 0;
    for(uint32_t i=0u;i<count;i++){
        MusaDwgAc1015HandleRef ref={0};
        if(!musa_dwg_ac1015_read_handle_ref(&bits,self,&ref)||
           ref.code!=2u||ref.relative||!ref.target||
           ref.target==self)return 0;
        if(handles)handles[i]=ref.target;
    }
    if(bits.bit_end-bits.bit_pos>7u)return 0;
    while(bits.bit_pos<bits.bit_end){
        uint32_t pad=0u;
        if(!musa_dwg_ac1015_bits_read_u(&bits,1u,&pad)||pad)return 0;
    }
    *entries=count;
    return 1;
}
int musa_dwg_ac1018_layer_control_refs_record(
    const uint8_t *objects,size_t object_bytes,size_t object_offset,
    uint64_t indexed_handle,uint64_t *entry_handles,size_t capacity,
    MusaDwgAc1018LayerControlRefs *out){
    if(!objects||!out||!indexed_handle||object_offset>=object_bytes||
       (entry_handles==NULL && capacity!=0u)||
       overlap(objects,object_bytes,out,sizeof(*out)))return 0;
    MusaDwgAc1018NonEntityIdentity identity={0};
    if(!musa_dwg_ac1018_nonentity_identity_record(
           objects,object_bytes,object_offset,indexed_handle,&identity)||
       identity.declared_type!=50u)return 0;
    uint32_t count=0u;
    if(!decode(objects,object_bytes,object_offset,
               indexed_handle,&identity,NULL,&count))return 0;
    if(entry_handles){
        if(capacity<count||capacity>SIZE_MAX/sizeof(*entry_handles)||
           overlap(objects,object_bytes,entry_handles,
                   capacity*sizeof(*entry_handles))||
           overlap(out,sizeof(*out),entry_handles,
                   capacity*sizeof(*entry_handles)))return 0;
        uint32_t verified=0u;
        if(!decode(objects,object_bytes,object_offset,indexed_handle,
                   &identity,entry_handles,&verified)||verified!=count)
            return 0;
    }
    MusaDwgAc1018LayerControlRefs checked={0};
    checked.indexed_handle=indexed_handle;
    checked.entry_count=count;
    checked.crc_verified=identity.record_crc_verified;
    checked.embedded_self_verified=identity.embedded_self_verified;
    checked.supported_handle_roles_verified=1u;
    *out=checked;
    return 1;
}
