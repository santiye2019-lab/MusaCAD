/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_handles.h"
#include "musacad_crc16.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define MUSA_R2004_HMAP_PAGE_MAX 2040u

/* Reject source/output aliasing before any validation or write. Never
 * calculate pointer + length: uintptr_t subtraction avoids wraparound. */
static int overlaps(const void *a,size_t na,const void *b,size_t nb) {
    if(!na||!nb)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y ? y-x<na : x-y<nb;
}

/* Modular unsigned, little 7-bit groups, high-bit continuation.
 * Explicit overflow + noncanonical + zero-delta refusal. */
static int modular_unsigned(const uint8_t *p,size_t end,size_t *pos,
                            uint64_t *out) {
    uint64_t v=0u;
    if(!p||!pos||!out)return 0;
    for(unsigned i=0u;i<10u;i++){
        if(*pos>=end)return 0;
        const uint8_t b=p[(*pos)++];
        const uint64_t bits=(uint64_t)(b&0x7fu);
        const unsigned shift=i*7u;
        if(shift>=64u||bits>(UINT64_MAX>>shift))return 0;
        v|=bits<<shift;
        if(!(b&0x80u)){
            if(!v||(i && !bits))return 0;
            *out=v;
            return 1;
        }
    }
    return 0;
}
/* Modular signed: all non-final bytes have seven magnitude bits;
 * final byte stores six magnitude bits and sign 0x40. */
static int modular_signed(const uint8_t *p,size_t end,size_t *pos,
                          int64_t *out) {
    uint64_t mag=0u;
    if(!p||!pos||!out)return 0;
    for(unsigned i=0u;i<10u;i++){
        if(*pos>=end)return 0;
        const uint8_t b=p[(*pos)++];
        const int last=(b&0x80u)==0u;
        const uint64_t bits=(uint64_t)(b&(last?0x3fu:0x7fu));
        const unsigned shift=i*7u;
        if(shift>=64u||bits>(UINT64_MAX>>shift))return 0;
        mag|=bits<<shift;
        if(last){
            const int negative=(b&0x40u)!=0u;
            if(i>0u && mag<(UINT64_C(1)<<((i-1u)*7u+6u)))
                return 0;
            if((negative && (mag==0u || mag>UINT64_C(0x8000000000000000))) ||
               (!negative && mag>(uint64_t)INT64_MAX))
                return 0;
            *out=negative?
                (mag==UINT64_C(0x8000000000000000)?
                    INT64_MIN:-(int64_t)mag):
                (int64_t)mag;
            return 1;
        }
    }
    return 0;
}
static int scan(const uint8_t *section,size_t section_bytes,
                size_t object_bytes,MusaDwgAc1018HandleEntry *dst,
                size_t *count_out) {
    if(!section||!count_out||section_bytes<8u||!object_bytes)return 0;
    size_t cursor=0u,count=0u;
    uint64_t handle=0u;
    int terminal=0;
    while(cursor<section_bytes){
        if(section_bytes-cursor<4u)return 0;
        const size_t start=cursor;
        const size_t size=((size_t)section[cursor]<<8u)|
                           (size_t)section[cursor+1u];
        if(size<2u||size>MUSA_R2004_HMAP_PAGE_MAX||
           size>section_bytes-cursor-2u)return 0;
        const size_t page_end=start+size;
        const uint16_t stored=((uint16_t)section[page_end]<<8u)|
                              (uint16_t)section[page_end+1u];
        uint16_t computed=0u;
        if(!musa_core_crc16_arc(0xc0c1u,section+start,size,&computed)||
           computed!=stored)return 0;
        cursor=start+2u;
        /* IMPORTANT: reset OFFSET at every 2004 handle-map page. */
        uint64_t object_offset=0u;
        if(size==2u){
            if(page_end+2u!=section_bytes||count==0u)return 0;
            terminal=1;cursor=section_bytes;break;
        }
        if(cursor==page_end)return 0;
        while(cursor<page_end){
            uint64_t handle_delta=0u;
            int64_t offset_delta=0;
            if(!modular_unsigned(section,page_end,&cursor,&handle_delta)||
               !modular_signed(section,page_end,&cursor,&offset_delta)||
               handle_delta>UINT64_MAX-handle)return 0;
            const uint64_t next_handle=handle+handle_delta;
            uint64_t next_offset=object_offset;
            if(offset_delta>=0){
                if((uint64_t)offset_delta>UINT64_MAX-next_offset)
                    return 0;
                next_offset+=(uint64_t)offset_delta;
            }else{
                const uint64_t abs_delta=offset_delta==INT64_MIN?
                    UINT64_C(0x8000000000000000):
                    (uint64_t)(-offset_delta);
                if(abs_delta>next_offset)return 0;
                next_offset-=abs_delta;
            }
            if(next_offset>=(uint64_t)object_bytes||count==SIZE_MAX)
                return 0;
            if(dst){
                dst[count].handle=next_handle;
                dst[count].object_offset=next_offset;
            }
            ++count;
            handle=next_handle;
            object_offset=next_offset;
        }
        cursor=page_end+2u;
    }
    if(!terminal || !count)return 0;
    *count_out=count;return 1;
}
int musa_dwg_ac1018_handles_index(const uint8_t *handle_section,
    size_t handle_bytes,size_t object_section_bytes,
    MusaDwgAc1018HandleEntry *entries,size_t capacity,
    size_t *out_count) {
    if(!handle_section||!out_count||(!entries&&capacity)||
       overlaps(handle_section,handle_bytes,out_count,sizeof(*out_count)))
        return 0;
    if(entries){
        /* Alias, even within unused capacity, could corrupt validated bytes.
         * A byte count overflow must be refused before computing any range. */
        if(capacity>SIZE_MAX/sizeof(*entries))return 0;
        const size_t output_bytes=capacity*sizeof(*entries);
        if(overlaps(handle_section,handle_bytes,entries,output_bytes)||
           overlaps(entries,output_bytes,out_count,sizeof(*out_count)))
            return 0;
    }
    size_t needed=0u;
    if(!scan(handle_section,handle_bytes,object_section_bytes,NULL,&needed))
        return 0;
    if(entries){
        if(needed>capacity)return 0;
        /* Immutable input guarantee makes this second validated scan
         * transactional; never expose half an index for corrupt sources. */
        size_t verified=0u;
        if(!scan(handle_section,handle_bytes,object_section_bytes,
                 entries,&verified)||verified!=needed)return 0;
    }
    *out_count=needed;
    return 1;
}
