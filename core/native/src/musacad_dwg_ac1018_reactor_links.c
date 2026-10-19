/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1018_reactor_links.h"
#include "musacad_dwg_ac1018_reactor_links_internal.h"
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
/* Two safe, explicitly tagged layouts. The original AC1015 public
 * decoder still validates the entire caller-supplied map. Only the
 * scene-owned AC1018 map (fully CRC-checked on scene_open) may bypass
 * repeated full-index scans and use binary search for each H target. */
typedef struct LinkIndex {
    const MusaDwgAc1015HandleEntry *legacy;
    const MusaDwgAc1018HandleEntry *scene;
    size_t count;
    int prevalidated_scene;
} LinkIndex;
static uint64_t h_at(const LinkIndex *idx,size_t n){
    return idx->prevalidated_scene?
        idx->scene[n].handle:idx->legacy[n].handle;
}
static uint64_t off_at(const LinkIndex *idx,size_t n){
    return idx->prevalidated_scene?
        idx->scene[n].object_offset:idx->legacy[n].file_offset;
}
static int member(const LinkIndex *idx,uint64_t target){
    size_t lo=0u,hi=idx->count;
    if(!target)return 0;
    while(lo<hi){
        const size_t mid=lo+(hi-lo)/2u;
        if(h_at(idx,mid)<target)lo=mid+1u;else hi=mid;
    }
    return lo<idx->count&&h_at(idx,lo)==target;
}
static int validate_map(const LinkIndex *idx,
    size_t object_bytes,uint64_t self,size_t off){
    if(!idx||!idx->count||!object_bytes||!self||
       (idx->prevalidated_scene?!idx->scene:!idx->legacy))return 0;
    /* Fail-closed public input: never trust a forged AC1015 map. */
    if(!idx->prevalidated_scene){
        for(size_t i=0u;i<idx->count;i++){
            if(!h_at(idx,i)||
               (i&&h_at(idx,i-1u)>=h_at(idx,i))||
               off_at(idx,i)>=(uint64_t)object_bytes)return 0;
        }
    }
    size_t lo=0u,hi=idx->count;
    while(lo<hi){
        const size_t mid=lo+(hi-lo)/2u;
        if(h_at(idx,mid)<self)lo=mid+1u;else hi=mid;
    }
    return lo<idx->count&&h_at(idx,lo)==self&&
           off_at(idx,lo)==(uint64_t)off;
}
static int check_ref(MusaDwgAc1015Bits *bits,uint64_t self,
    uint8_t expected_code,const LinkIndex *idx,uint64_t *target){
    MusaDwgAc1015HandleRef ref={0};
    if(!musa_dwg_ac1015_read_handle_ref(bits,self,&ref)||
       ref.code!=expected_code||!member(idx,ref.target))return 0;
    *target=ref.target;
    return 1;
}
static int read_links_impl(
    const uint8_t *objects,size_t object_bytes,size_t offset,
    uint64_t self,const LinkIndex *idx,
    uint64_t *reactors,size_t capacity,MusaDwgAc1018ReactorAudit *out,
    int expect_zero){
    if(!objects||!out||!reactors||!self||offset>=object_bytes||
       !validate_map(idx,object_bytes,self,offset))return 0;
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
    if(mode==0u&&!check_ref(&tail,self,12u,idx,
                           &result.owner_handle))return 0;
    for(uint32_t i=0u;i<reactor_count;i++){
        if(!check_ref(&tail,self,4u,idx,&found[i]))return 0;
    }
    if(!xdict_missing&&!check_ref(&tail,self,3u,idx,
                                 &result.xdictionary_handle))return 0;
    if(!check_ref(&tail,self,5u,idx,&result.layer_handle))return 0;
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
    const LinkIndex idx={map,NULL,map_count,0};
    return read_links_impl(objects,object_bytes,offset,self,
                           &idx,reactors,capacity,out,0);
}
/* Zero-reactor case still verifies H owner/layer/XDictionary references
 * and full sorted index membership. Never invent empty-reference success
 * if the on-disk BL reactor count was actually nonzero. */
int musa_dwg_ac1018_read_zero_reactor_links(
    const uint8_t *objects,size_t object_bytes,size_t offset,
    uint64_t self,const MusaDwgAc1015HandleEntry *map,size_t map_count,
    MusaDwgAc1018ReactorAudit *out){
    uint64_t unused[1]={0};
    const LinkIndex idx={map,NULL,map_count,0};
    return read_links_impl(objects,object_bytes,offset,self,
                           &idx,unused,1u,out,1);
}

/* PRIVATE scene-only hook. Caller must supply the intact handle map
 * returned from musa_dwg_ac1018_handles_index and owned by the cached
 * validated scene; NOT for arbitrary maps or unverified input. */
int musa_dwg_ac1018_read_links_verified_scene_index(
    const uint8_t *objects,size_t object_bytes,size_t offset,
    uint64_t self,const MusaDwgAc1018HandleEntry *entries,size_t count,
    uint64_t *reactors,size_t capacity,MusaDwgAc1018ReactorAudit *out,
    int expect_zero){
    const LinkIndex idx={NULL,entries,count,1};
    return read_links_impl(objects,object_bytes,offset,self,
                           &idx,reactors,capacity,out,expect_zero?1:0);
}
