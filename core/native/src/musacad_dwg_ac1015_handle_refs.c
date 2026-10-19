/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_handle_refs.h"
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

/* The Open Design Specification chapter 'HANDLE REFERENCES' describes
 * code:4, count:4 followed by count high-order-first bytes. Do not
 * reuse the little-endian raw integer decoder: H uses big-endian
 * handle bytes, unlike RD/RL.
 */
int musa_dwg_ac1015_read_handle_ref(
    MusaDwgAc1015Bits *c,uint64_t base,MusaDwgAc1015HandleRef *out){
    if(!c||!out||!c->data||base==0u)return 0;
    MusaDwgAc1015Bits next=*c;
    uint32_t code=0u,count=0u,part=0u;
    if(!musa_dwg_ac1015_bits_read_u(&next,4u,&code)||
       !musa_dwg_ac1015_bits_read_u(&next,4u,&count))return 0;
    const int absolute=code==0u||code==2u||code==3u||code==4u||code==5u;
    const int plus=code==6u||code==10u;
    const int minus=code==8u||code==12u;
    if(!absolute&&!plus&&!minus)return 0;
    if((code==6u||code==8u) ? count!=0u :
       count==0u||count>8u)return 0;
    uint64_t value=0u;
    for(uint32_t i=0u;i<count;i++){
        if(!musa_dwg_ac1015_bits_read_u(&next,8u,&part))return 0;
        if(i==0u&&part==0u)return 0; /* noncanonical leading zero */
        value=(value<<8u)|(uint64_t)part;
    }
    uint64_t target=value;
    if(plus){
        const uint64_t amount=code==6u?1u:value;
        if(amount>UINT64_MAX-base)return 0;
        target=base+amount;
    }else if(minus){
        const uint64_t amount=code==8u?1u:value;
        if(amount>base)return 0;
        target=base-amount;
    }
    if(target==0u)return 0; /* no null/dangling semantic shortcut */
    MusaDwgAc1015HandleRef result={0};
    result.target=target;
    result.encoded_value=value;
    result.code=(uint8_t)code;
    result.count=(uint8_t)count;
    result.relative=(uint8_t)(!absolute);
    *out=result;
    *c=next;
    return 1;
}
static int tail_scan(const MusaDwgAc1015ObjectRecord *r,
                     size_t count,MusaDwgAc1015HandleRef *output){
    MusaDwgAc1015Bits cur={0};
    if(!musa_dwg_ac1015_bits_init(&cur,r->data,r->data_bytes,
              (size_t)r->pre_handle_bits,r->data_bytes*8u))return 0;
    for(size_t i=0u;i<count;i++){
        MusaDwgAc1015HandleRef item={0};
        if(!musa_dwg_ac1015_read_handle_ref(&cur,r->indexed_handle,&item))
            return 0;
        if(output)output[i]=item;
    }
    const size_t remainder=cur.bit_end-cur.bit_pos;
    if(remainder>7u)return 0;
    for(size_t i=0;i<remainder;i++){
        uint32_t pad=0u;
        if(!musa_dwg_ac1015_bits_read_u(&cur,1u,&pad)||pad!=0u)return 0;
    }
    return cur.bit_pos==cur.bit_end;
}
int musa_dwg_ac1015_read_exact_handle_tail(
    const MusaDwgAc1015ObjectRecord *r,
    size_t expected_count,MusaDwgAc1015HandleRef *refs,size_t capacity){
    if(!r||!r->data||!r->record_crc_verified||
       r->indexed_handle==0u||r->data_bytes==0u||
       r->data_bytes>SIZE_MAX/8u||
       (size_t)r->pre_handle_bits>r->data_bytes*8u||
       (expected_count&&!refs)||expected_count>capacity||
       expected_count>r->data_bytes)return 0;
    /* Verify *all* bytes before any caller-owned entry is changed. */
    if(!tail_scan(r,expected_count,NULL))return 0;
    if(refs&&!tail_scan(r,expected_count,refs))return 0;
    return 1;
}

/* Binary lookup is valid only after complete, strictly increasing map
 * verification. Do not infer link type from H.code or target membership.
 */
static int map_find(const MusaDwgAc1015HandleEntry *map,size_t count,
                    uint64_t handle,uint64_t *offset){
    size_t low=0u,high=count;
    while(low<high){
        const size_t mid=low+(high-low)/2u;
        if(map[mid].handle<handle)low=mid+1u;
        else high=mid;
    }
    if(low==count||map[low].handle!=handle)return 0;
    *offset=map[low].file_offset;
    return 1;
}
int musa_dwg_ac1015_resolve_refs_in_map(
    const MusaDwgAc1015ObjectRecord *record,
    const MusaDwgAc1015HandleRef *refs,size_t ref_count,
    const MusaDwgAc1015HandleEntry *map,size_t map_count,
    size_t file_bytes,MusaDwgAc1015ResolvedRef *out,size_t capacity){
    if(!record||!record->record_crc_verified||!map||map_count==0u||
       record->indexed_handle==0u||file_bytes==0u||
       (uint64_t)record->absolute_offset>=(uint64_t)file_bytes||
       (ref_count&&(!refs||!out))||ref_count>capacity||
       (!out&&capacity!=0u))return 0;
    /* Never trust arbitrary or unsorted arrays as a binary-search index. */
    for(size_t i=0u;i<map_count;i++){
        if(map[i].handle==0u||
           map[i].file_offset>=(uint64_t)file_bytes||
           (i&&map[i-1u].handle>=map[i].handle))return 0;
    }
    uint64_t origin=0u;
    if(!map_find(map,map_count,record->indexed_handle,&origin)||
       origin!=(uint64_t)record->absolute_offset)return 0;
    /* First pass: every target must exist, before touching caller output. */
    for(size_t i=0u;i<ref_count;i++){
        uint64_t offset=0u;
        if(refs[i].target==0u||
           !map_find(map,map_count,refs[i].target,&offset))return 0;
    }
    for(size_t i=0u;i<ref_count;i++){
        uint64_t offset=0u;
        if(!map_find(map,map_count,refs[i].target,&offset))return 0;
        MusaDwgAc1015ResolvedRef item={0};
        item.ref=refs[i];
        item.file_offset=offset;
        out[i]=item;
    }
    return 1;
}
