/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_reactor_links.h"
#include "musacad_dwg_ac1018_eed.h"
#include "musacad_dwg_ac1015_handle_refs.h"
#include "musacad_dwg_ac1015_values.h"
#include "musacad_crc16.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* MS object payload byte length: little-endian short with continuation. */
static int read_ms(const uint8_t *p,size_t n,size_t *len_bytes,size_t *value){
    if(!p||!len_bytes||!value||n<2u)return 0;
    const uint16_t lo=(uint16_t)p[0]|((uint16_t)p[1]<<8u);
    if(!(lo&UINT16_C(0x8000))){
        if(lo<8u)return 0;
        *len_bytes=2u;*value=lo;return 1;
    }
    if(n<4u)return 0;
    const uint16_t hi=(uint16_t)p[2]|((uint16_t)p[3]<<8u);
    if(!hi||(hi&UINT16_C(0x8000)))return 0;
    *len_bytes=4u;*value=(size_t)(lo&UINT16_C(0x7fff))|
                             ((size_t)hi<<15u);
    return *value>=8u;
}
static int validate_map(const MusaDwgAc1015HandleEntry *map,
    size_t count,size_t object_bytes,uint64_t self,size_t off){
    if(!map||!count||!object_bytes||!self)return 0;
    size_t lo=0u,hi=count;
    for(size_t i=0u;i<count;i++){
        if(!map[i].handle||(i&&map[i-1u].handle>=map[i].handle)||
           map[i].file_offset>=(uint64_t)object_bytes)return 0;
    }
    while(lo<hi){
        const size_t mid=lo+(hi-lo)/2u;
        if(map[mid].handle<self)lo=mid+1u;else hi=mid;
    }
    return lo<count&&map[lo].handle==self&&
           map[lo].file_offset==(uint64_t)off;
}
static int member(const MusaDwgAc1015HandleEntry *map,size_t n,
    uint64_t target){
    size_t lo=0u,hi=n;
    if(!target)return 0;
    while(lo<hi){
        const size_t mid=lo+(hi-lo)/2u;
        if(map[mid].handle<target)lo=mid+1u;else hi=mid;
    }
    return lo<n&&map[lo].handle==target;
}
static int check_ref(MusaDwgAc1015Bits *bits,uint64_t self,
    uint8_t expected_code,const MusaDwgAc1015HandleEntry *map,
    size_t count,uint64_t *target){
    MusaDwgAc1015HandleRef ref={0};
    if(!musa_dwg_ac1015_read_handle_ref(bits,self,&ref)||
       ref.code!=expected_code||!member(map,count,ref.target))return 0;
    *target=ref.target;
    return 1;
}
static int read_links_impl(
    const uint8_t *objects,size_t object_bytes,size_t offset,
    uint64_t self,const MusaDwgAc1015HandleEntry *map,size_t map_count,
    uint64_t *reactors,size_t capacity,MusaDwgAc1018ReactorAudit *out,
    int expect_zero){
    if(!objects||!out||!reactors||!self||offset>=object_bytes||
       !validate_map(map,map_count,object_bytes,self,offset))return 0;
    const uint8_t *record=objects+offset;
    const size_t remaining=object_bytes-offset;
    size_t size_bytes=0u,data_bytes=0u;
    if(!read_ms(record,remaining,&size_bytes,&data_bytes)||
       remaining<size_bytes+2u||data_bytes>remaining-size_bytes-2u||
       data_bytes>SIZE_MAX/8u)return 0;
    uint16_t crc=0u;
    if(!musa_core_crc16_arc(0xc0c1u,record,size_bytes+data_bytes,&crc))
        return 0;
    const size_t crc_pos=size_bytes+data_bytes;
    if(crc!=((uint16_t)record[crc_pos]|
              ((uint16_t)record[crc_pos+1u]<<8u)))return 0;
    MusaDwgAc1015Bits c={0};
    if(!musa_dwg_ac1015_bits_init(&c,record+size_bytes,
                                  data_bytes,0u,data_bytes*8u))return 0;
    uint16_t typ=0u;
    uint32_t before_handles=0u,graphics=0u,mode=0u;
    uint32_t reactor_count=0u,xdict_missing=0u,eed_bytes=0u;
    MusaDwgAc1015HandleRef embedded_self={0};
    if(!musa_dwg_ac1015_bits_read_bs(&c,&typ)||
       (typ!=19u&&typ!=77u)||
       !musa_dwg_ac1015_bits_read_rl(&c,&before_handles)||
       (size_t)before_handles>data_bytes*8u||
       !musa_dwg_ac1015_read_handle_ref(&c,self,&embedded_self)||
       embedded_self.code!=0u||embedded_self.target!=self||
       !musa_dwg_ac1018_eed_skip(&c,&eed_bytes)||
       !musa_dwg_ac1015_bits_read_u(&c,1u,&graphics)||graphics!=0u||
       !musa_dwg_ac1015_bits_read_u(&c,2u,&mode)||(mode!=0u&&mode!=2u)||
       !musa_dwg_ac1015_bits_read_bl(&c,&reactor_count)||
       ((expect_zero&&reactor_count!=0u)||(!expect_zero&&reactor_count==0u))||
       reactor_count>64u||reactor_count>capacity||
       !musa_dwg_ac1015_bits_read_u(&c,1u,&xdict_missing)||
       c.bit_pos>(size_t)before_handles)return 0;
    MusaDwgAc1015Bits tail={0};
    if(!musa_dwg_ac1015_bits_init(&tail,c.data,data_bytes,
                                  (size_t)before_handles,data_bytes*8u))
        return 0;
    uint64_t found[64]={0};
    MusaDwgAc1018ReactorAudit result={0};
    result.handle=self;result.entity_mode=(uint8_t)mode;
    result.eed_raw_bytes_skipped=eed_bytes;result.reactor_count=reactor_count;
    if(mode==0u&&!check_ref(&tail,self,12u,map,map_count,
                           &result.owner_handle))return 0;
    for(uint32_t i=0u;i<reactor_count;i++){
        if(!check_ref(&tail,self,4u,map,map_count,&found[i]))return 0;
    }
    if(!xdict_missing&&!check_ref(&tail,self,3u,map,map_count,
                                 &result.xdictionary_handle))return 0;
    if(!check_ref(&tail,self,5u,map,map_count,&result.layer_handle))return 0;
    /* R2004 often uses nonzero terminal padding bits; these are CRC-backed,
     * but their semantics aren't inferred. Reject any extra full byte. */
    if(tail.bit_pos>tail.bit_end||tail.bit_end-tail.bit_pos>7u)return 0;
    result.record_crc_verified=1u;result.handle_membership_verified=1u;
    result.geometry_decoded=0u;
    memcpy(reactors,found,(size_t)reactor_count*sizeof(found[0]));
    *out=result;
    return 1;
}

/* Legacy positive-reactor API remains strict and unchanged externally. */
int musa_dwg_ac1018_read_reactor_links(
    const uint8_t *objects,size_t object_bytes,size_t offset,
    uint64_t self,const MusaDwgAc1015HandleEntry *map,size_t map_count,
    uint64_t *reactors,size_t capacity,MusaDwgAc1018ReactorAudit *out){
    return read_links_impl(objects,object_bytes,offset,self,
                           map,map_count,reactors,capacity,out,0);
}
/* Zero-reactor case still verifies H owner/layer/XDictionary references
 * and full sorted index membership. Never invent empty-reference success
 * if the on-disk BL reactor count was actually nonzero. */
int musa_dwg_ac1018_read_zero_reactor_links(
    const uint8_t *objects,size_t object_bytes,size_t offset,
    uint64_t self,const MusaDwgAc1015HandleEntry *map,size_t map_count,
    MusaDwgAc1018ReactorAudit *out){
    uint64_t unused[1]={0};
    return read_links_impl(objects,object_bytes,offset,self,
                           map,map_count,unused,1u,out,1);
}
