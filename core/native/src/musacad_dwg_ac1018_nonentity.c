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
