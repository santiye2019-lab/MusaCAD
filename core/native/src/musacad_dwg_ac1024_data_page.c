/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "musacad_dwg_ac1024_data_page.h"
#include "musacad_dwg_ac1024_section_directory.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define MAX_SOURCE_REFERENCES 65536u
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
           ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
static uint64_t get64(const uint8_t *p){
    return (uint64_t)get32(p)|((uint64_t)get32(p+4u)<<32u);
}
static void put32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8u);
    p[2]=(uint8_t)(x>>16u);p[3]=(uint8_t)(x>>24u);
}
static uint32_t adler(uint32_t seed,const uint8_t *p,size_t n){
    uint64_t lo=seed&65535u,hi=seed>>16u;
    for(size_t at=0;at<n;){
        const size_t take=n-at<5552u?n-at:5552u;
        for(size_t i=0;i<take;i++){lo+=p[at+i];hi+=lo;}
        lo%=65521u;hi%=65521u;at+=take;
    }
    return ((uint32_t)hi<<16u)|(uint32_t)lo;
}
static int overlap(const void *a,size_t na,const void *b,size_t nb){
    if(!a||!b||!na||!nb)return 0;
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y?y-x<na:x-y<nb;
}
static const MusaDwgAc1018PageLocation *locate(
    const MusaDwgAc1018PageLocation *p,size_t n,uint32_t id){
    size_t lo=0u,hi=n;
    while(lo<hi){
        const size_t mid=lo+(hi-lo)/2u;
        if(p[mid].page_id<id)lo=mid+1u;
        else hi=mid;
    }
    return lo<n&&p[lo].page_id==id?&p[lo]:NULL;
}
int musa_dwg_ac1024_data_pages_authenticated(
    const uint8_t *file,size_t file_bytes,
    const uint8_t *directory,size_t directory_bytes,
    const MusaDwgAc1018PageLocation *locations,size_t physical_count,
    MusaDwgAc1024DataPageEvidence *out){
    if(!file||!directory||!locations||!out||!physical_count||
       physical_count>MAX_SOURCE_REFERENCES||
       physical_count>SIZE_MAX/sizeof(*locations)||
       overlap(file,file_bytes,out,sizeof(*out))||
       overlap(directory,directory_bytes,out,sizeof(*out))||
       overlap(locations,physical_count*sizeof(*locations),out,sizeof(*out)))
        return 0;
    MusaDwgAc1024DirectoryPageLinkEvidence linked={0};
    if(!musa_dwg_ac1024_directory_physical_links(
           directory,directory_bytes,locations,physical_count,&linked))
        return 0;
    MusaDwgAc1024DataPageEvidence checked={0};
    const uint32_t sections=get32(directory);
    size_t at=20u;
    for(uint32_t i=0;i<sections;i++){
        const uint32_t pages=get32(directory+at+8u);
        const uint32_t section_id=get32(directory+at+24u);
        at+=96u;
        if(pages && !section_id)return 0;
        for(uint32_t j=0;j<pages;j++){
            const uint8_t *entry=directory+at+(size_t)j*16u;
            const uint32_t id=get32(entry),packed=get32(entry+4u);
            const uint64_t logical_offset=get64(entry+8u);
            const MusaDwgAc1018PageLocation *loc=
                locate(locations,physical_count,id);
            if(!loc||loc->file_offset>file_bytes||
               loc->file_span_bytes<32u||
               file_bytes-(size_t)loc->file_offset<32u||
               packed>loc->file_span_bytes-32u||
               packed>file_bytes-(size_t)loc->file_offset-32u)
                return 0;
            const size_t pos=(size_t)loc->file_offset;
            const uint32_t mask=UINT32_C(0x4164536b)^(uint32_t)pos;
            uint32_t words[8];
            for(unsigned k=0;k<8u;k++)
                words[k]=get32(file+pos+4u*k)^mask;
            if(words[0]!=UINT32_C(0x4163043b)||
               words[1]!=section_id||
               words[2]!=packed||
               words[3]<packed||words[3]-packed>63u||
               words[4]!=(uint32_t)logical_offset||
               words[5]!=(uint32_t)(logical_offset>>32u))
                return 0;
            const uint32_t payload_crc=adler(0u,file+pos+32u,packed);
            if(payload_crc!=words[7])return 0;
            uint8_t clear[32];
            for(unsigned k=0;k<8u;k++)
                put32(clear+4u*k,k==6u?0u:words[k]);
            if(adler(payload_crc,clear,sizeof clear)!=words[6])
                return 0;
            checked.verified_pages++;
            checked.verified_packed_bytes+=packed;
        }
        at+=(size_t)pages*16u;
    }
    if(at!=directory_bytes||checked.verified_pages!=
       linked.matched_page_references)
        return 0;
    *out=checked;
    return 1;
}
