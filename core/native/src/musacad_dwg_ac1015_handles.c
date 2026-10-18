/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1015_handles.h"
#include "musacad_dwg_ac1015.h"
#include "musacad_crc16.h"
#include <limits.h>
#include <stdint.h>
#include <stddef.h>

/* ODA Open Design Specification chapter 22.1 (R13-15 AcDb:Handles).
 * No LibreDWG parser code, tables or runtime are linked or copied. */
#define MUSA_OBJECT_MAP_PAGE_MAX 2032u

static int read_modular_unsigned(const uint8_t *p,size_t end,size_t *at,
                                 uint64_t *value){
    uint64_t v=0;
    for(unsigned i=0u;i<10u;i++){
        if(*at>=end)return 0;
        const uint8_t b=p[(*at)++];
        const uint64_t chunk=(uint64_t)(b&0x7fu);
        const unsigned shift=i*7u;
        if(shift>=64u || chunk>(UINT64_MAX>>shift))return 0;
        v|=chunk<<shift;
        if((b&0x80u)==0u){
            if((i>0u && chunk==0u) || v==0u)return 0;
            *value=v;return 1;
        }
    }
    return 0;
}
/* Signed DWG modular char: final byte's 0x40 bit is SIGN, not payload.
 * All prior bytes have 7 payload bits. Reject overlong and overflow. */
static int read_modular_signed(const uint8_t *p,size_t end,size_t *at,
                               int64_t *value){
    uint64_t mag=0;
    for(unsigned i=0u;i<10u;i++){
        if(*at>=end)return 0;
        const uint8_t b=p[(*at)++];
        const int final=(b&0x80u)==0u;
        const uint64_t bits=(uint64_t)(b&(final?0x3fu:0x7fu));
        const unsigned shift=i*7u;
        if(shift>=64u || bits>(UINT64_MAX>>shift))return 0;
        mag|=bits<<shift;
        if(final){
            const int negative=(b&0x40u)!=0;
            if(i>0u && mag<(UINT64_C(1)<<((i-1u)*7u+6u)))return 0;
            if((negative&&mag>UINT64_C(0x8000000000000000))||
               (!negative&&mag>(uint64_t)INT64_MAX)||
               (negative&&mag==0u))return 0;
            *value=negative?
                (mag==UINT64_C(0x8000000000000000)?INT64_MIN:-(int64_t)mag):
                (int64_t)mag;
            return 1;
        }
    }
    return 0;
}
static int scan(const uint8_t *map,size_t map_len,size_t file_size,
                MusaDwgAc1015HandleEntry *output,size_t *entry_count){
    if(!map||!entry_count||map_len<4u)return 0;
    size_t at=0u,count=0u;
    uint64_t last_handle=0u,last_offset=0u;
    int terminal=0;
    while(at<map_len){
        if(map_len-at<4u)return 0;
        const size_t page_start=at;
        const size_t page_len=((size_t)map[at]<<8)|(size_t)map[at+1u];
        if(page_len<2u||page_len>MUSA_OBJECT_MAP_PAGE_MAX||
           page_len>map_len-at-2u)return 0;
        const size_t data_start=page_start+2u;
        const size_t page_end=page_start+page_len;
        const uint16_t stored=(uint16_t)((uint16_t)map[page_end]<<8)|
                              (uint16_t)map[page_end+1u];
        uint16_t computed=0u;
        if(!musa_core_crc16_arc(0xc0c1u,map+page_start,page_len,
                                &computed)||stored!=computed)return 0;
        size_t cursor=data_start;
        while(cursor<page_end){
            uint64_t delta_handle=0u;
            int64_t delta_offset=0;
            if(!read_modular_unsigned(map,page_end,&cursor,&delta_handle)||
               !read_modular_signed(map,page_end,&cursor,&delta_offset)||
               delta_handle>UINT64_MAX-last_handle)return 0;
            const uint64_t handle=last_handle+delta_handle;
            uint64_t pos=last_offset;
            if(delta_offset>=0){
                if((uint64_t)delta_offset>UINT64_MAX-pos)return 0;
                pos+=(uint64_t)delta_offset;
            }else{
                const uint64_t magnitude=delta_offset==INT64_MIN?
                    UINT64_C(0x8000000000000000):
                    (uint64_t)(-delta_offset);
                if(magnitude>pos)return 0;
                pos-=magnitude;
            }
            if(pos>=(uint64_t)file_size || count==SIZE_MAX)return 0;
            if(output){
                output[count].handle=handle;
                output[count].file_offset=pos;
            }
            count++;last_handle=handle;last_offset=pos;
        }
        at=page_end+2u;
        if(page_len==2u){
            if(at!=map_len||count==0u)return 0;
            terminal=1;break;
        }
    }
    if(!terminal)return 0;
    *entry_count=count;return 1;
}
int musa_dwg_ac1015_handles_index(const uint8_t *file,size_t file_bytes,
                                  MusaDwgAc1015HandleEntry *entries,
                                  size_t capacity,size_t *out_count){
    if(!file||!out_count||(!entries&&capacity!=0u))return 0;
    const uint8_t *map=NULL;
    size_t n=0u;
    if(!musa_dwg_ac1015_readonly_section(file,file_bytes,2u,&map,&n))
        return 0;
    size_t needed=0u;
    if(!scan(map,n,file_bytes,NULL,&needed))return 0;
    if(entries){
        if(needed>capacity)return 0;
        /* A second checked pass avoids any partial caller output on
         * malformed maps, under the immutable-input API contract. */
        size_t written=0u;
        if(!scan(map,n,file_bytes,entries,&written)||written!=needed)return 0;
    }
    *out_count=needed;
    return 1;
}
